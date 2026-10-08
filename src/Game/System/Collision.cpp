#include "Collision.hpp"

using namespace asmjit;

namespace IW3SR
{
	// Collision triangles index cm.verts in 16 bits. A map past 65536 vertices states, in each
	// partition's two padding bytes, the window in steps of 64 its indices count from; IzFF's linker
	// writes such a zone as version 6, so a client without this refuses it instead of colliding
	// against the wrong vertices.
	constexpr uint32_t WindowedVertices = 0x10000;
	constexpr uint32_t WindowStep = 64;

	// Every load of cm.verts in the four triangle tests, then the five that hand it to the physics
	// colliders with the partition's first triangle in ecx. Each is a six byte mov, so a call to the
	// matching thunk and a nop take its place.
	constexpr uintptr_t VertsInEdi = 0x4EFCEE;
	constexpr uintptr_t VertsInEdx[] = { 0x4F05C6, 0x4F0F96, 0x4F1406, 0x5A7DE6, 0x5A7E02, 0x5A7E27, 0x5A7E4E,
		0x5A7E6E };

	// DB_LoadXFileInternal compares the zone's version against 5 here and stores the stream position
	// right after, which together are the nine bytes the jump replaces.
	constexpr uintptr_t VersionSite = 0x477F23;
	constexpr uintptr_t VersionResume = 0x477F2C;
	constexpr uintptr_t VersionStreamPos = 0xE344EC;

	static bool Matches(uintptr_t site, std::initializer_list<uint8_t> bytes)
	{
		size_t offset = 0;
		for (const uint8_t byte : bytes)
		{
			if (Memory::Get<uint8_t>(site + offset++) != byte)
				return false;
		}
		return true;
	}

	// indices in esi, verts into edi; everything else as it was, flags included.
	ASM_FUNCTION(CollisionVertsEdi_h)
	{
		a.pushfd();
		a.push(x86::eax);
		a.push(x86::ecx);
		a.push(x86::edx);
		a.push(x86::esi);
		a.call(GCollision::Verts);
		a.add(x86::esp, 0x04);
		a.mov(x86::edi, x86::eax);
		a.pop(x86::edx);
		a.pop(x86::ecx);
		a.pop(x86::eax);
		a.popfd();
		a.ret();
	}

	// indices in ecx, verts into edx, written over the saved edx so the pops hand it back.
	ASM_FUNCTION(CollisionVertsEdx_h)
	{
		a.pushfd();
		a.push(x86::eax);
		a.push(x86::ecx);
		a.push(x86::edx);
		a.push(x86::ecx);
		a.call(GCollision::Verts);
		a.add(x86::esp, 0x04);
		a.mov(x86::dword_ptr(x86::esp), x86::eax);
		a.pop(x86::edx);
		a.pop(x86::ecx);
		a.pop(x86::eax);
		a.popfd();
		a.ret();
	}

	// Version 6 is read as 5, which is all it differs by. The displaced store goes first, so the
	// flags the je after the jump reads are the compare's.
	ASM_FUNCTION(ZoneVersion_h)
	{
		Label stock = a.newLabel();
		a.push(x86::ecx);
		a.mov(x86::ecx, imm(VersionStreamPos));
		a.mov(x86::dword_ptr(x86::ecx), x86::edi);
		a.pop(x86::ecx);
		a.cmp(x86::eax, 6);
		a.jne(stock);
		a.mov(x86::eax, 5);
		a.bind(stock);
		a.cmp(x86::eax, 5);
		a.push(imm(VersionResume));
		a.ret();
	}

	// All or nothing: a client that reads some vertices through windows and some without would
	// collide worse than one that reads none, and the version gate opens only with the rest.
	void GCollision::Initialize()
	{
		const std::initializer_list<uint8_t> edi = { 0x8B, 0x3D, 0x1C, 0x99, 0x40, 0x01 };
		const std::initializer_list<uint8_t> edx = { 0x8B, 0x15, 0x1C, 0x99, 0x40, 0x01 };
		const std::initializer_list<uint8_t> version = { 0x83, 0xF8, 0x05, 0x89, 0x3D, 0xEC, 0x44, 0xE3, 0x00 };

		if (!Matches(VertsInEdi, edi) || !Matches(VersionSite, version))
			return;
		for (const uintptr_t site : VertsInEdx)
		{
			if (!Matches(site, edx))
				return;
		}

		Memory::CALL(VertsInEdi, ASM_LOAD(CollisionVertsEdi_h));
		Memory::NOP(VertsInEdi + 5, 1);
		const uintptr_t thunk = ASM_LOAD(CollisionVertsEdx_h);
		for (const uintptr_t site : VertsInEdx)
		{
			Memory::CALL(site, thunk);
			Memory::NOP(site + 5, 1);
		}
		Memory::JMP(VersionSite, ASM_LOAD(ZoneVersion_h));
		Memory::NOP(VersionSite + 5, 4);

		CM_InitThreadData_h.Install();
	}

	// CM_LoadMap sets up every trace context once the clipmap is in, the main one first, and no trace
	// runs before it returns: the one place the table can be rebuilt with nothing reading it.
	void GCollision::InitThreadData(unsigned int threadContext)
	{
		if (threadContext == 0)
			Build();
		CM_InitThreadData_h(threadContext);
	}

	// A triangle's window, from the partition owning it.
	void GCollision::Build()
	{
		Windows.clear();
		if (!cm->partitions || !cm->triIndices || cm->vertCount <= WindowedVertices || cm->triCount <= 0)
			return;

		Windows.assign(static_cast<size_t>(cm->triCount), 0);
		for (int index = 0; index < cm->partitionCount; index++)
		{
			const CollisionPartition& partition = cm->partitions[index];
			for (int tri = partition.firstTri; tri < partition.firstTri + partition.triCount; tri++)
			{
				if (tri >= 0 && tri < cm->triCount)
					Windows[tri] = partition.window;
			}
		}
	}

	// The vertex array a triangle's indices count from.
	const float* GCollision::Verts(const uint16_t* indices)
	{
		const float* verts = cm->verts[0];
		if (Windows.empty() || !cm->triIndices)
			return verts;

		const size_t tri = static_cast<size_t>(indices - cm->triIndices) / 3;
		return tri < Windows.size() ? verts + size_t(Windows[tri]) * WindowStep * 3 : verts;
	}

	const float* GCollision::Vertex(const uint16_t* indices, int corner)
	{
		return Verts(indices) + size_t(indices[corner]) * 3;
	}
}

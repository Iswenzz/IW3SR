#include "ServerGhost.hpp"

#include "Game/System/Channel.hpp"
#include "Game/System/Transport.hpp"

namespace IW3SR
{
	namespace
	{
		constexpr uint8_t Version = 1;
		constexpr uint8_t Header[] = { 0xFF, 0xFF, 0xFF, 0xFF, 's', 'r', 'g', 'h', 'o', 's', 't', 0 };

		// The ghost is known for this long after the last of its state arrived.
		constexpr int StaleTime = 500;

		// Its model counts as drawn for this long, which covers the frame the scene was built in.
		constexpr int DrawnTime = 100;

		constexpr int FlagDelta = 1;
		constexpr int MaxSlots = 64;
		constexpr int MaxNameLength = 63;

		// Feet to head, standing.
		constexpr float Height = 70.0f;
	}

	// The state, one with each snapshot: version, the client slot the ghost takes, flags, the record's time,
	// the delta, then the name. Swallowed whenever it carries the header, so the engine never parses one.
	bool ServerGhost::Packet(const netadr_t* from, msg_t* msg)
	{
		if (!from || !msg || !msg->data || msg->cursize < static_cast<int>(sizeof(Header))
			|| std::memcmp(msg->data, Header, sizeof(Header)) != 0)
			return false;
		if (!cls || !GChannel::FromServer(*from))
			return true;

		NetReader in(msg->data, msg->cursize, sizeof(Header));
		if (in.ReadByte() != Version)
			return true;

		ServerGhostState state;
		state.Slot = in.ReadByte();
		state.HasDelta = in.ReadByte() & FlagDelta;
		state.RecordTime = in.ReadLong();
		state.Delta = in.ReadLong();

		for (int c = in.ReadByte(); c > 0 && static_cast<int>(state.Name.size()) < MaxNameLength; c = in.ReadByte())
			state.Name.push_back(static_cast<char>(c));
		if (in.Overflowed || state.Slot < 0 || state.Slot >= MaxSlots)
			return true;

		state.Time = cls->realtime;
		State = state;
		return true;
	}

	// The ghost while its state keeps arriving; null once it stops.
	const ServerGhostState* ServerGhost::Get()
	{
		if (!cls || !State.Time || cls->realtime - State.Time >= StaleTime)
			return nullptr;
		return &State;
	}

	// Every model with a pose passes here on its way into the scene, and false keeps the ghost's out: right
	// at the view it would be all the player sees. Where it stands is kept for its label.
	bool ServerGhost::SceneModel(const cpose_t* pose, int entnum)
	{
		const ServerGhostState* state = Get();
		if (!state || !pose || !cgs || entnum != state->Slot)
			return true;

		const vec3 origin = pose->origin;
		const vec3 view = cgs->refdef.vieworg;
		const vec3 nearest(origin.x, origin.y, std::clamp(view.z, origin.z, origin.z + Height));

		Visible = glm::distance(view, nearest) >= HideDistance;
		Position = origin;
		Drawn = cls->realtime;
		return Visible;
	}

	// Where the ghost stands, in a frame that draws it.
	bool ServerGhost::Origin(vec3& origin)
	{
		if (!Get() || !Visible || cls->realtime - Drawn >= DrawnTime)
			return false;

		origin = Position;
		return true;
	}
}

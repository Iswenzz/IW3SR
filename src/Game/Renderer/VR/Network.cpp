#include "Network.hpp"

#include "Game/System/Channel.hpp"
#include "Game/System/Protocol.hpp"
#include "Game/System/Transport.hpp"

namespace IW3SR
{
	namespace
	{
		constexpr uint8_t Version = 1;
		constexpr uint8_t Header[] = { 0xFF, 0xFF, 0xFF, 0xFF, 'v', 'r', 0 };

		// Sent this often; the server relays each one as it comes in.
		constexpr int SendInterval = 33;

		// A player's state is drawn for this long after the last of it arrived.
		constexpr int StaleTime = 500;

		// Positions to a sixteenth of a unit, rotations as unit quaternions scaled to a short.
		constexpr float PositionScale = 16.0f;
		constexpr float RotationScale = 32767.0f;

		int ToShort(float value)
		{
			return static_cast<int>(std::clamp(std::round(value), -32767.0f, 32767.0f));
		}

		void WritePose(NetWriter& out, const VRNetPose& pose)
		{
			for (int i = 0; i < 3; i++)
				out.WriteShort(ToShort(pose.Offset[i] * PositionScale));

			const glm::quat rotation = glm::normalize(pose.Rotation);
			out.WriteShort(ToShort(rotation.x * RotationScale));
			out.WriteShort(ToShort(rotation.y * RotationScale));
			out.WriteShort(ToShort(rotation.z * RotationScale));
			out.WriteShort(ToShort(rotation.w * RotationScale));
		}

		VRNetPose ReadPose(NetReader& in)
		{
			VRNetPose pose;
			for (int i = 0; i < 3; i++)
				pose.Offset[i] = static_cast<float>(in.ReadShort()) / PositionScale;

			const float x = static_cast<float>(in.ReadShort()) / RotationScale;
			const float y = static_cast<float>(in.ReadShort()) / RotationScale;
			const float z = static_cast<float>(in.ReadShort()) / RotationScale;
			const float w = static_cast<float>(in.ReadShort()) / RotationScale;
			const glm::quat rotation = glm::quat::wxyz(w, x, y, z);
			const float length = glm::length(rotation);
			if (length > 0.001f)
				pose.Rotation = rotation / length;
			return pose;
		}

		bool ReadState(NetReader& in, VRNetState& state)
		{
			if (in.ReadByte() != Version)
				return false;

			state.Parts = static_cast<uint8_t>(in.ReadByte());
			state.Trackers = static_cast<uint32_t>(in.ReadByte()) & ((1u << VRTrackerCount) - 1);
			for (int i = 0; i < 3; i++)
				state.HeadAngles[i] = SHORT2ANGLE(static_cast<float>(in.ReadShort() & 0xFFFF));
			state.BodyYaw = SHORT2ANGLE(static_cast<float>(in.ReadShort() & 0xFFFF));

			if (state.Parts & VRPartHead)
				state.Head = ReadPose(in);
			if (state.Parts & VRPartRight)
				state.RightHand = ReadPose(in);
			if (state.Parts & VRPartLeft)
				state.LeftHand = ReadPose(in);
			for (int i = 0; i < VRTrackerCount; i++)
			{
				if (state.Trackers & (1u << i))
					state.TrackerPoses[i] = ReadPose(in);
			}
			return !in.Overflowed;
		}
	}

	// The datagram: the header, the qport the server finds the client by, then the state. Only sent to a
	// server that says it relays it.
	void GVRNetwork::Send(const VRNetState& state)
	{
		if (!cls || !client_ui || client_ui->connectionState != CA_ACTIVE || clc.demoplaying)
			return;
		if (cls->realtime >= LastSend && cls->realtime - LastSend < SendInterval)
			return;
		if (GProtocol::SystemInfoValue("sr_vrRelay") != "1")
			return;
		LastSend = cls->realtime;

		uint8_t buffer[512] = {};
		NetWriter out(buffer, sizeof(buffer));
		out.WriteData(Header, sizeof(Header));
		out.WriteShort(clc.netchan.qport);
		out.WriteByte(Version);
		out.WriteByte(state.Parts);
		out.WriteByte(static_cast<int>(state.Trackers));
		for (int i = 0; i < 3; i++)
			out.WriteShort(ANGLE2SHORT(state.HeadAngles[i]));
		out.WriteShort(ANGLE2SHORT(state.BodyYaw));

		if (state.Parts & VRPartHead)
			WritePose(out, state.Head);
		if (state.Parts & VRPartRight)
			WritePose(out, state.RightHand);
		if (state.Parts & VRPartLeft)
			WritePose(out, state.LeftHand);
		for (int i = 0; i < VRTrackerCount; i++)
		{
			if (state.Trackers & (1u << i))
				WritePose(out, state.TrackerPoses[i]);
		}

		if (!out.Overflowed)
			NET_SendPacket(NS_CLIENT1, out.CurSize, buffer, clc.netchan.remoteAddress);
	}

	// The relay: the header, a count, then for each player the client it is about, the size and the
	// state. Swallowed whenever it carries the header, so the engine never parses one.
	bool GVRNetwork::Packet(const netadr_t* from, msg_t* msg)
	{
		if (!from || !msg || !msg->data || msg->cursize < static_cast<int>(sizeof(Header))
			|| std::memcmp(msg->data, Header, sizeof(Header)) != 0)
			return false;
		if (!cls || !GChannel::FromServer(*from))
			return true;

		NetReader in(msg->data, msg->cursize, sizeof(Header));
		const int count = in.ReadByte();
		for (int i = 0; i < count && !in.Overflowed; i++)
		{
			const int client = in.ReadByte();
			const int size = in.ReadByte();
			if (in.Overflowed || client < 0 || client >= static_cast<int>(Players.size()) || size <= 0
				|| in.ReadCount + size > in.CurSize)
				break;

			NetReader body(msg->data, in.ReadCount + size, in.ReadCount);
			VRNetState state;
			if (ReadState(body, state))
			{
				state.Time = cls->realtime;
				Players[client] = state;
			}
			in.ReadCount += size;
		}
		return true;
	}

	const VRNetState* GVRNetwork::Get(int clientNum)
	{
		if (!cls || clientNum < 0 || clientNum >= static_cast<int>(Players.size()))
			return nullptr;

		const VRNetState& state = Players[clientNum];
		return state.Time && cls->realtime - state.Time < StaleTime ? &state : nullptr;
	}
}

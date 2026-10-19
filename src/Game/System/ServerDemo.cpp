#include "ServerDemo.hpp"

#include "Game/System/Channel.hpp"
#include "Game/System/Transport.hpp"

namespace IW3SR
{
	namespace
	{
		constexpr uint8_t Version = 1;
		constexpr uint8_t Header[] = { 0xFF, 0xFF, 0xFF, 0xFF, 's', 'r', 'd', 'e', 'm', 'o', 0 };

		// The demo is shown for this long after the last of its state arrived.
		constexpr int StaleTime = 500;

		// The run time is carried at most this far from the server time of its state, so it holds still
		// while states go missing.
		constexpr int MaxCarry = 250;

		constexpr float RateScale = 100.0f;
		constexpr int MaxNameLength = 63;
	}

	// The state, one with each snapshot: version, server time, run time, playback rate, velocity, fps, the
	// movement keys and buttons, then the name. Swallowed whenever it carries the header, so the engine never
	// parses one.
	bool ServerDemo::Packet(const netadr_t* from, msg_t* msg)
	{
		if (!from || !msg || !msg->data || msg->cursize < static_cast<int>(sizeof(Header))
			|| std::memcmp(msg->data, Header, sizeof(Header)) != 0)
			return false;
		if (!cls || !GChannel::FromServer(*from))
			return true;

		NetReader in(msg->data, msg->cursize, sizeof(Header));
		if (in.ReadByte() != Version)
			return true;

		ServerDemoState state;
		state.ServerTime = in.ReadLong();
		state.RunTime = in.ReadLong();
		state.Rate = static_cast<float>(in.ReadShort()) / RateScale;
		state.Velocity = in.ReadShort();
		state.FPS = in.ReadShort();
		state.ForwardMove = static_cast<int8_t>(in.ReadByte());
		state.RightMove = static_cast<int8_t>(in.ReadByte());
		state.Buttons = in.ReadLong();

		for (int c = in.ReadByte(); c > 0 && static_cast<int>(state.Name.size()) < MaxNameLength; c = in.ReadByte())
			state.Name.push_back(static_cast<char>(c));
		if (in.Overflowed)
			return true;

		state.Time = cls->realtime;
		State = state;
		return true;
	}

	// The demo while its state keeps arriving; null once it stops, so callers show their own values again.
	const ServerDemoState* ServerDemo::Get()
	{
		if (!cls || !State.Time || cls->realtime - State.Time >= StaleTime)
			return nullptr;
		return &State;
	}

	// The run time at the frame being drawn, carried from its state's server time at the playback rate so it
	// runs smoothly between two states.
	int ServerDemo::RunTime()
	{
		const ServerDemoState* state = Get();
		if (!state)
			return 0;

		const int carry = std::clamp(cgs->time - state->ServerTime, -MaxCarry, MaxCarry);
		return std::max(0, state->RunTime + static_cast<int>(std::round(carry * state->Rate)));
	}
}

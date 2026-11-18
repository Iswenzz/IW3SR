#pragma once
// Super Monkey Ball's ball, after https://github.com/camthesaxman/smb-decomp (ball.c, world.c, stcoli.c)
#include "CoD4.hpp"

// Laid out alike in every file that includes this, whatever packing was left on before it (server.h leaves 1).
#pragma pack(push, 8)

namespace IW3SR
{
	// A ball between two ticks of the game's own 60 Hz clock, which a move draws between.
	struct MonkeyState
	{
		vec3 Position{};
		vec3 Previous{};
		vec3 Velocity{};
		vec3 Origin{};
		vec3 Speed{};
		int TiltX = 0;
		int TiltZ = 0;
		// The face last rolled on, the turn a tick gives the ball, and which way two marks on it point.
		vec3 Normal{ 0, 0, 1 };
		vec3 Spin{};
		vec3 Ahead{ 1, 0, 0 };
		vec3 Left{ 0, 1, 0 };
		float Time = 0;
		// How far the stage's lean has pitched the view, to take it off again.
		float Pitch = 0;
		// The bit the game flips when it puts the player somewhere, as last seen, and one more: nought is never.
		int Placed = 0;
		bool Grounded = false;
		bool Running = false;
	};

	// Something the ball has sunk into, a face, an edge or a corner: the way out of it and how deep.
	struct MonkeyTouch
	{
		vec3 Normal{};
		float Depth = 0;
	};

	// A face a line from the ball stopped at: how far the ball's middle is from its plane, and whether the
	// nearest point of that plane is on the face.
	struct MonkeyFace
	{
		vec3 Normal{};
		float Away = 0;
		bool Flat = false;
	};

	class Monkey
	{
	public:
		static void GroundTrace(pmove_t* pm, pml_t* pml, const MonkeyState& state);
		static void Move(pmove_t* pm, pml_t* pml, MonkeyState& state);
		static bool Pose(const MonkeyState& state, vec3& origin, vec3& angles, bool inside);

	private:
		static void Follow(const playerState_s* ps, MonkeyState& state);
		static void Tick(pmove_t* pm, MonkeyState& state);
		static void Tilt(const pmove_t* pm, MonkeyState& state);
		static void Lean(pmove_t* pm, MonkeyState& state);
		static vec3 Gravity(const pmove_t* pm, const MonkeyState& state);
		static void Slide(pmove_t* pm, MonkeyState& state);
		static int Touching(pmove_t* pm, const vec3& centre, const vec3& last, MonkeyTouch* touches);
		static bool Ray(pmove_t* pm, const vec3& from, const vec3& to, trace_t& trace);
		static bool Reached(pmove_t* pm, const vec3& centre, const vec3& point, const vec3& inward, float apart);
		static void Bounce(vec3& velocity, const vec3& normal, bool rub);
		static void Roll(MonkeyState& state);
	};
}

#pragma pack(pop)

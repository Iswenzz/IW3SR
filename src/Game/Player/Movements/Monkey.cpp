#include "Monkey.hpp"

// The converter's units for one of the game's.
#define monkey_scale 70.0f
#define monkey_tick (1000.0f / 60.0f)
#define monkey_radius (0.5f * monkey_scale)
#define monkey_gravity (0.009799992f * monkey_scale)
#define monkey_restitution 0.5f
#define monkey_friction 0.01f
#define monkey_brake 0.65f
// The stage leans 23 degrees at most, a fifth of the way there each tick, in the game's 65536ths of a turn.
#define monkey_tilt 4187
#define monkey_tilt_ease 0.2
#define monkey_angle (3.14159265358979f / 32768.0f)

// The game's traces stop this far short of a face. A point worked out from faces is looked for this far
// inside the map, and taken as found within this much of where it should be.
#define MONKEY_SKIN 0.125f
#define MONKEY_INSIDE 0.5f
#define MONKEY_NEAR 1.5f
// How many faces and touches a look keeps, and how far the ball goes between two looks, in radii.
#define MONKEY_FACES 8
#define MONKEY_TOUCHES 6
#define MONKEY_STRIDE 0.4f
#define MONKEY_STEPS 8
#define MONKEY_TICKS 8
// How far down the view looks at the ball to begin with: leaning all the way forward brings it level.
#define MONKEY_VIEW 25.0f
// The bit of a player's flags the game flips whenever it puts the player somewhere.
#define MONKEY_PLACED 0x4
#define MONKEY_MOVED 0.05f
// Two balls part this much faster than they met, so the one that hits is knocked back too, and sunk into
// one another they are eased apart at this much of a tick per unit.
#define MONKEY_BUMP 1.3f
#define MONKEY_EASE 0.1f

namespace IW3SR
{
	// A direction turned about an axis by the axis's own length, in radians.
	static vec3 Spun(const vec3& v, const vec3& spin)
	{
		const float angle = glm::length(spin);
		if (angle < FLT_EPSILON)
			return v;

		const vec3 axis = spin / angle;
		return v * cosf(angle) + glm::cross(axis, v) * sinf(angle) + axis * (glm::dot(axis, v) * (1.0f - cosf(angle)));
	}

	// The ball is never walked: every move is its own, and the ground only says what the body plays.
	void Monkey::GroundTrace(pmove_t* pm, pml_t* pml, const MonkeyState& state)
	{
		pml->walking = false;
		pml->groundPlane = false;
		pml->almostGroundPlane = false;
		pm->ps->groundEntityNum = state.Running && state.Grounded ? ENTITYNUM_WORLD : ENTITYNUM_NONE;
	}

	void Monkey::Move(pmove_t* pm, pml_t* pml, MonkeyState& state)
	{
		playerState_s* ps = pm->ps;
		Follow(ps, state);

		state.Time += static_cast<float>(pml->msec);
		for (int i = 0; i < MONKEY_TICKS && state.Time >= monkey_tick; i++)
		{
			Tick(pm, state);
			state.Time -= monkey_tick;
		}
		if (state.Time >= monkey_tick)
			state.Time = 0;

		ps->origin = glm::mix(state.Previous, state.Position, state.Time / monkey_tick);
		ps->origin[2] -= monkey_radius;
		ps->velocity = state.Velocity * (1000.0f / monkey_tick);
		Lean(pm, state);
		ps->groundEntityNum = state.Grounded ? ENTITYNUM_WORLD : ENTITYNUM_NONE;

		state.Origin = ps->origin;
		state.Speed = ps->velocity;
	}

	// Where the ball's middle is and how far it has rolled, as the last move left it; or where what rides
	// inside the ball stands, its feet on the ball's own bottom, turning with it.
	bool Monkey::Pose(const MonkeyState& state, vec3& origin, vec3& angles, bool inside)
	{
		if (!state.Running)
			return false;

		// The marks are where the last tick left them; the move drew the ball short of that.
		const float between = state.Time / monkey_tick;
		const vec3 ahead = Spun(state.Ahead, state.Spin * (between - 1.0f));
		const vec3 left = Spun(state.Left, state.Spin * (between - 1.0f));
		const vec3 up = glm::cross(ahead, left);

		origin = glm::mix(state.Previous, state.Position, between);
		if (inside)
			origin -= up * monkey_radius;
		angles = vec3(-glm::degrees(asinf(glm::clamp(ahead.z, -1.0f, 1.0f))), glm::degrees(atan2f(ahead.y, ahead.x)),
			glm::degrees(atan2f(left.z, up.z)));
		return true;
	}

	// Two balls that met over the last tick, turned apart along the line between them. Both states take it at
	// once, so neither meets the other again before its next move; the change to this one's velocity is given
	// back in units a second for its playerstate, the other's being the opposite.
	vec3 Monkey::Bump(MonkeyState& self, MonkeyState& other)
	{
		if (!self.Running || !other.Running)
			return vec3(0);

		// Where the two middles came nearest over the tick.
		const vec3 from = self.Previous - other.Previous;
		const vec3 along = (self.Position - other.Position) - from;
		const float length = glm::dot(along, along);
		const float t = length > 0.0f ? glm::clamp(-glm::dot(from, along) / length, 0.0f, 1.0f) : 1.0f;
		const vec3 apart = from + along * t;
		const float distance = glm::length(apart);
		if (distance >= 2.0f * monkey_radius)
			return vec3(0);

		const vec3 normal = distance > 0.01f ? apart / distance : vec3(1, 0, 0);
		const float closing = glm::dot(self.Velocity - other.Velocity, normal);
		float impulse = closing < 0.0f ? -closing * (1.0f + MONKEY_BUMP) * 0.5f : 0.0f;
		const float sunk = 2.0f * monkey_radius - glm::length(self.Position - other.Position);
		if (sunk > 0.0f && closing + 2.0f * impulse < sunk * MONKEY_EASE)
			impulse = (sunk * MONKEY_EASE - closing) * 0.5f;
		if (impulse <= 0.0f)
			return vec3(0);

		const vec3 push = normal * impulse;
		const vec3 speed = push * (1000.0f / monkey_tick);
		self.Velocity += push;
		self.Speed += speed;
		other.Velocity -= push;
		other.Speed -= speed;
		return speed;
	}

	// The game keeps its camera level with the world while the stage leans under the ball. Here the stage
	// cannot lean, so the view does, the other way: its up is what the leaning gravity pulls against.
	void Monkey::Lean(pmove_t* pm, MonkeyState& state)
	{
		playerState_s* ps = pm->ps;

		// Put somewhere by the game, at a spawn or a teleport, the view is the game's again: it is set looking
		// down on the ball, and the lean begins from nothing.
		if (state.Placed != (ps->eFlags & MONKEY_PLACED) + 1)
		{
			state.Placed = (ps->eFlags & MONKEY_PLACED) + 1;
			state.Pitch = 0;
			state.TiltX = state.TiltZ = 0;
			ps->delta_angles[0] += MONKEY_VIEW - ps->viewangles[0];
			ps->viewangles[0] = MONKEY_VIEW;
		}

		const float x = state.TiltX * monkey_angle;
		const float z = state.TiltZ * monkey_angle;
		const vec3 up(cosf(x) * sinf(z), sinf(x), cosf(x) * cosf(z));
		const float yaw = glm::radians(ps->viewangles[1]);

		// Down is the view's pitch, clockwise its roll.
		const float pitch = glm::degrees(asinf(glm::clamp(up.x * cosf(yaw) + up.y * sinf(yaw), -1.0f, 1.0f)));
		const float roll = glm::degrees(asinf(glm::clamp(up.x * sinf(yaw) - up.y * cosf(yaw), -1.0f, 1.0f)));

		// The mouse keeps the pitch; the lean is laid over it.
		ps->delta_angles[0] += pitch - state.Pitch;
		ps->viewangles[0] += pitch - state.Pitch;
		state.Pitch = pitch;
		ps->delta_angles[2] = roll - pm->cmd.angles[2] * (360.0f / 65536.0f);
		ps->viewangles[2] = roll;
	}

	// Whatever moved the player since the last step, a teleport, a push or a correction, moves the ball with it.
	void Monkey::Follow(const playerState_s* ps, MonkeyState& state)
	{
		if (!state.Running)
		{
			state = {};
			state.Position = ps->origin + vec3(0, 0, monkey_radius);
			state.Previous = state.Position;
			state.Velocity = ps->velocity * (monkey_tick / 1000.0f);
			// The marks start the way the view looks, so whoever stands in the ball faces ahead.
			const float yaw = glm::radians(ps->viewangles[1]);
			state.Ahead = vec3(cosf(yaw), sinf(yaw), 0);
			state.Left = vec3(-sinf(yaw), cosf(yaw), 0);
			state.Running = true;
			return;
		}
		const vec3 moved = ps->origin - state.Origin;
		if (glm::dot(moved, moved) > MONKEY_MOVED * MONKEY_MOVED)
		{
			state.Position += moved;
			state.Previous += moved;
		}
		const vec3 pushed = ps->velocity - state.Speed;
		if (glm::dot(pushed, pushed) > MONKEY_MOVED * MONKEY_MOVED)
			state.Velocity += pushed * (monkey_tick / 1000.0f);
	}

	void Monkey::Tick(pmove_t* pm, MonkeyState& state)
	{
		Tilt(pm, state);

		state.Previous = state.Position;
		state.Velocity += Gravity(pm, state);
		Slide(pm, state);
		Roll(state);
	}

	// On a face the ball turns as fast as it travels over it; off one it keeps the turn it left with.
	void Monkey::Roll(MonkeyState& state)
	{
		if (state.Grounded)
			state.Spin = glm::cross(state.Normal, state.Velocity) / monkey_radius;

		state.Ahead = Spun(state.Ahead, state.Spin);
		state.Left = Spun(state.Left, state.Spin);

		// Kept square, which many small turns would not leave them.
		state.Ahead = glm::normalize(state.Ahead);
		state.Left = glm::normalize(state.Left - state.Ahead * glm::dot(state.Ahead, state.Left));
	}

	// The stick leans the stage about the camera, and the lean is kept about the world's axes as the game
	// keeps it: x right, y up, z towards the viewer.
	void Monkey::Tilt(const pmove_t* pm, MonkeyState& state)
	{
		const float stickX = glm::clamp(pm->cmd.rightmove / 127.0f, -1.0f, 1.0f);
		const float stickY = -glm::clamp(pm->cmd.forwardmove / 127.0f, -1.0f, 1.0f);

		const float a = stickY * monkey_tilt * monkey_angle;
		const float b = -stickX * monkey_tilt * monkey_angle;
		const float c = glm::radians(pm->ps->viewangles[1] - 90.0f);

		const vec3 leaned(-sinf(b), cosf(b) * cosf(a), cosf(b) * sinf(a));
		const vec3 up(leaned.x * cosf(c) + leaned.z * sinf(c), leaned.y, -leaned.x * sinf(c) + leaned.z * cosf(c));

		const int x = static_cast<int>(atan2f(up.z, up.y) / monkey_angle);
		const int z = -static_cast<int>(atan2f(up.x, sqrtf(up.z * up.z + up.y * up.y)) / monkey_angle);

		state.TiltX = static_cast<int>(state.TiltX + (x - state.TiltX) * monkey_tilt_ease);
		state.TiltZ = static_cast<int>(state.TiltZ + (z - state.TiltZ) * monkey_tilt_ease);
	}

	// Gravity as the leaning stage sees it. A grounded ball rolling back at the camera is pulled less.
	vec3 Monkey::Gravity(const pmove_t* pm, const MonkeyState& state)
	{
		float gravity = monkey_gravity;

		const vec2 rolling(state.Velocity.x, state.Velocity.y);
		const float speed = glm::length(rolling);
		if (state.Grounded && speed > FLT_EPSILON)
		{
			const float yaw = glm::radians(pm->ps->viewangles[1]);
			const float ahead = glm::dot(rolling / speed, vec2(cosf(yaw), sinf(yaw)));
			if (ahead < 0)
				gravity *= 1.0f + ahead * monkey_brake;
		}
		const float x = state.TiltX * monkey_angle;
		const float z = state.TiltZ * monkey_angle;
		return vec3(-cosf(x) * sinf(z), -sinf(x), -cosf(x) * cosf(z)) * gravity;
	}

	// A line through the map, as thin as the game traces: whether it stops, and where and against what.
	bool Monkey::Ray(pmove_t* pm, const vec3& from, const vec3& to, trace_t& trace)
	{
		trace = {};
		PM_PlayerTrace(pm, &trace, from, vec3(0), vec3(0), to, pm->ps->clientNum, pm->tracemask);
		return !trace.startsolid && !trace.allsolid && trace.fraction < 1.0f;
	}

	// Whether the map is really there at a point worked out from the planes of two or three faces, which
	// run on past the faces' own ends. Aimed a little into the map, so the line does not slip by the edge.
	bool Monkey::Reached(pmove_t* pm, const vec3& centre, const vec3& point, const vec3& inward, float apart)
	{
		const vec3 aim = point + inward * MONKEY_INSIDE - centre;
		const float length = glm::length(aim);
		if (length < FLT_EPSILON)
			return false;

		trace_t trace;
		if (!Ray(pm, centre, centre + aim * ((length + 1.0f) / length), trace))
			return false;
		return fabsf(trace.fraction * (length + 1.0f) - apart) < MONKEY_NEAR;
	}

	// What the ball at a place has sunk into. The game asks each triangle near the ball for its face, its
	// edges and its corners; the map here answers only lines, so lines are sent out all round the ball. A face
	// one stops at touches the ball where the nearest point of its plane is on it; where it is not, the ball
	// is past the face's end, and the edge or corner it shares with the other faces found is what it touches.
	int Monkey::Touching(pmove_t* pm, const vec3& centre, const vec3& last, MonkeyTouch* touches)
	{
		static std::vector<vec3> ways;
		if (ways.empty())
		{
			for (int x = -1; x <= 1; x++)
				for (int y = -1; y <= 1; y++)
					for (int z = -1; z <= 1; z++)
						if (x || y || z)
							ways.push_back(glm::normalize(vec3(x, y, z)));
		}

		MonkeyFace faces[MONKEY_FACES];
		int known = 0;
		float nearest = FLT_MAX;
		vec3 toward(0);
		const float length = monkey_radius + 1.0f;
		const auto look = [&](const vec3& way)
		{
			trace_t trace;
			if (!Ray(pm, centre, centre + way * length, trace))
				return;
			const float facing = -glm::dot(way, trace.normal);
			if (facing <= 0.01f)
				return;
			// The game's traces stop an eighth of a unit short of a face, measured square to it.
			const float away = trace.fraction * length * facing + MONKEY_SKIN;
			if (away / facing < nearest)
			{
				nearest = away / facing;
				toward = way;
			}
			if (away >= monkey_radius)
				return;
			for (int i = 0; i < known; i++)
			{
				if (glm::dot(faces[i].Normal, trace.normal) > 0.999f && fabsf(faces[i].Away - away) < 0.5f)
					return;
			}
			if (known < MONKEY_FACES)
				faces[known++] = { trace.normal, away, false };
		};
		look(-last);
		for (const vec3& way : ways)
			look(way);

		int found = 0;
		const auto touch = [&](const vec3& normal, float depth)
		{
			if (depth > 0.0f && found < MONKEY_TOUCHES)
				touches[found++] = { normal, depth };
		};

		// Faces the ball is over.
		for (int i = 0; i < known; i++)
		{
			trace_t trace;
			const float reach = faces[i].Away + 1.0f;
			if (!Ray(pm, centre, centre - faces[i].Normal * reach, trace) || glm::dot(trace.normal, faces[i].Normal) < 0.99f)
				continue;
			faces[i].Flat = true;
			touch(faces[i].Normal, monkey_radius - (trace.fraction * reach + MONKEY_SKIN));
		}

		// Edges two faces it is past the ends of meet in: the nearest point of the line their planes share.
		bool edged[MONKEY_FACES] = {};
		for (int i = 0; i < known; i++)
		{
			for (int j = i + 1; j < known; j++)
			{
				if (faces[i].Flat || faces[j].Flat)
					continue;
				const float turn = glm::dot(faces[i].Normal, faces[j].Normal);
				if (fabsf(turn) > 0.98f)
					continue;
				const float a = (faces[i].Away - faces[j].Away * turn) / (1.0f - turn * turn);
				const float b = (faces[j].Away - faces[i].Away * turn) / (1.0f - turn * turn);
				if (a <= 0.0f || b <= 0.0f)
					continue;
				const vec3 out = faces[i].Normal * a + faces[j].Normal * b;
				const float apart = glm::length(out);
				if (apart >= monkey_radius || apart < 0.01f)
					continue;
				if (!Reached(pm, centre, centre - out, -glm::normalize(faces[i].Normal + faces[j].Normal), apart))
					continue;
				edged[i] = edged[j] = true;
				touch(out / apart, monkey_radius - apart);
			}
		}

		// Corners three such faces meet in, where no edge of theirs was found.
		for (int i = 0; i < known; i++)
		{
			for (int j = i + 1; j < known; j++)
			{
				for (int k = j + 1; k < known; k++)
				{
					if (faces[i].Flat || faces[j].Flat || faces[k].Flat || edged[i] || edged[j] || edged[k])
						continue;
					const glm::mat3 planes(glm::transpose(glm::mat3(faces[i].Normal, faces[j].Normal, faces[k].Normal)));
					if (fabsf(glm::determinant(planes)) < 0.05f)
						continue;
					const vec3 out = glm::inverse(planes) * vec3(faces[i].Away, faces[j].Away, faces[k].Away);
					const float apart = glm::length(out);
					if (apart >= monkey_radius || apart < 0.01f)
						continue;
					const vec3 inward = -glm::normalize(faces[i].Normal + faces[j].Normal + faces[k].Normal);
					if (Reached(pm, centre, centre - out, inward, apart))
						touch(out / apart, monkey_radius - apart);
				}
			}
		}

		// Nothing made out of the faces, yet a line stopped inside the ball: what it stopped at is what the
		// ball touches, looked for more closely round that line.
		if (found == 0 && nearest < monkey_radius)
		{
			float spread = 0.25f;
			for (int round = 0; round < 3; round++, spread *= 0.5f)
			{
				const vec3 side = glm::normalize(glm::cross(toward, fabsf(toward.z) < 0.9f ? vec3(0, 0, 1) : vec3(1, 0, 0)));
				const vec3 other = glm::cross(toward, side);
				const vec3 middle = toward;
				for (int i = 0; i < 6; i++)
				{
					const float angle = i * 1.0471976f;
					const vec3 way = glm::normalize(middle + (side * cosf(angle) + other * sinf(angle)) * spread);
					trace_t trace;
					if (!Ray(pm, centre, centre + way * length, trace))
						continue;
					const float facing = std::max(-glm::dot(way, trace.normal), 0.05f);
					const float apart = trace.fraction * length + MONKEY_SKIN / facing;
					if (apart < nearest)
					{
						nearest = apart;
						toward = way;
					}
				}
			}
			touch(-toward, monkey_radius - nearest);
		}
		return found;
	}

	// The game moves the ball, then pushes it out of all it sank into, each the way out of it. A ball going
	// fast is moved in several shorter steps, so that nothing thin is passed between two looks.
	void Monkey::Slide(pmove_t* pm, MonkeyState& state)
	{
		trace_t inside = {};
		PM_PlayerTrace(pm, &inside, state.Position, vec3(0), vec3(0), state.Position, pm->ps->clientNum, pm->tracemask);
		state.Grounded = false;
		if (inside.startsolid || inside.allsolid)
		{
			// Sunk whole in something that was not there a tick ago: out through the top.
			state.Position[2] += 1.0f;
			state.Velocity = vec3(0);
			return;
		}

		const float stride = MONKEY_STRIDE * monkey_radius;
		const int steps = glm::clamp(static_cast<int>(ceilf(glm::length(state.Velocity) / stride)), 1, MONKEY_STEPS);
		bool rubbed = false;
		for (int step = 0; step < steps; step++)
		{
			state.Position += state.Velocity / static_cast<float>(steps);

			MonkeyTouch touches[MONKEY_TOUCHES];
			const int count = Touching(pm, state.Position, state.Normal, touches);
			std::sort(touches, touches + count, [](const MonkeyTouch& a, const MonkeyTouch& b) { return a.Depth > b.Depth; });

			vec3 pushed(0);
			for (int i = 0; i < count; i++)
			{
				// What an earlier push already brought the ball out of is not pushed again.
				const float left = touches[i].Depth - glm::dot(pushed, touches[i].Normal);
				if (left <= 0.0f)
					continue;
				state.Position += touches[i].Normal * left;
				pushed += touches[i].Normal * left;

				Bounce(state.Velocity, touches[i].Normal, !rubbed);
				rubbed = true;
				if (!state.Grounded)
					state.Normal = touches[i].Normal;
				state.Grounded = true;
			}
		}
	}

	// A hundredth of the speed along the face is lost to it, once a tick. Speed into the face is lost outright
	// when it is less than five ticks of falling, and past that half of the rest comes back.
	void Monkey::Bounce(vec3& velocity, const vec3& normal, bool rub)
	{
		const float into = glm::dot(normal, velocity);
		if (into >= 0)
			return;

		if (rub)
			velocity -= (velocity - normal * into) * monkey_friction;

		const float least = -5.0f * monkey_gravity;
		if (into >= least)
			velocity -= normal * into;
		else
			velocity -= normal * (least + (1.0f + monkey_restitution) * (into - least));
	}
}

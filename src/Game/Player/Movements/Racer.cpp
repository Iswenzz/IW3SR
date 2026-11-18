#include "Racer.hpp"
#include "Q3.hpp"

// Mario Kart Wii's own numbers, a frame of its sixty: the 150cc Flame Runner under Funky Kong for
// the speed, the Quacker under Baby Daisy for how it is reached.
#define RACER_UNIT 17.7165f
#define RACER_FRAMES 60.0f
#define RACER_SPEED 84.010f
#define RACER_BOOST 7.0f
#define RACER_SETTLE 3.0f
#define RACER_COAST 1.212f

#define RACER_FRICTION 8.0f
#define RACER_ACCELERATE 15.0f
#define RACER_GRIP 640.0f
#define RACER_START 2.0f
#define RACER_STAND 60
#define RACER_BIG 96.0f
#define RACER_REACH 64.0f
#define RACER_AGAINST -0.5f
#define RACER_STOP 100.0f
#define RACER_ROLL 300.0f
#define RACER_SLOWS 28
#define OVERCLIP 1.001f

namespace IW3SR
{
	static const float GAINS[] = { 0.500f, 1.750f, 0.450f, 0.045f };
	static const float STEPS[] = { 0.20f, 0.90f, 0.98f };
	// The Flame Runner's share of its speed on road, weak off-road, off-road and heavy off-road. A course
	// says which a ground is in two bits of its surface flags.
	static const float GROUNDS[] = { 1.0f, 0.584f, 0.362f, 0.208f };

	void Racer::WalkMove(pmove_t* pm, pml_t* pml, bool big)
	{
		const auto ps = pm->ps;
		ps->pm_flags |= PMF_NO_SPRINT;
		View(ps, big);

		const float limit = Ground(static_cast<float>(ps->speed), pml->groundTrace.surfaceFlags);
		if (Q3::JumpCheck(pm, pml, true))
		{
			AirMove(pm, pml, big);
			return;
		}
		Q3::SetMovementDir(pm);

		pml->forward[2] = 0.0f;
		pml->right[2] = 0.0f;
		Q3::ClipVelocity(pml->forward, pml->groundTrace.normal, pml->forward, OVERCLIP);
		Q3::ClipVelocity(pml->right, pml->groundTrace.normal, pml->right, OVERCLIP);
		pml->forward = CoD4::Normalize(pml->forward);
		pml->right = CoD4::Normalize(pml->right);

		vec3 wishdir;
		for (int i = 0; i < 3; i++)
			wishdir[i] = pml->forward[i] * pm->cmd.forwardmove + pml->right[i] * pm->cmd.rightmove;

		const bool driven = glm::length(wishdir) > 0.0f;
		if (driven)
			wishdir = CoD4::Normalize(wishdir);

		const float allowed = Allowed(glm::length(ps->velocity), limit, pml->frametime, driven);

		// Friction is what turns the run toward the keys; the steering is given more than the speed
		// allowed so that it always makes the friction good, and the speed is held after.
		Q3::Friction(pm, pml, driven ? RACER_FRICTION : Coast(glm::length(ps->velocity)));
		if (driven)
			Q3::Accelerate(ps, pml, wishdir, std::max(allowed, RACER_GRIP), RACER_ACCELERATE);
		if ((pml->groundTrace.surfaceFlags & SURF_SLICK) || CoD4::InKnockback(ps))
			ps->velocity[2] -= static_cast<float>(ps->gravity) * pml->frametime;

		const float speed = std::min(glm::length(ps->velocity), allowed);
		Q3::ClipVelocity(ps->velocity, pml->groundTrace.normal, ps->velocity, OVERCLIP);
		if (speed > 0.0f && glm::length(ps->velocity) > 0.0f)
			ps->velocity = CoD4::Normalize(ps->velocity) * speed;

		if (ps->velocity[0] == 0.0f && ps->velocity[1] == 0.0f)
			return;

		Q3::StepSlideMove(pm, pml, false, true);
	}

	// CPM's air for the steering; the speed is the ground's own. It is gained by the same curve while the keys
	// run on, lost the same with none held, and held to what the ground under the jump allows, so a jump
	// neither stalls a start nor carries a racer over grass.
	void Racer::AirMove(pmove_t* pm, pml_t* pml, bool big)
	{
		const auto ps = pm->ps;
		View(ps, big);
		const float limit = Ground(static_cast<float>(ps->speed), Under(pm));
		const bool keys = pm->cmd.forwardmove || pm->cmd.rightmove;
		const bool driven = Boosted(limit) || (keys && With(pm, pml));
		const float before = glm::length(glm::vec2(ps->velocity));
		float allowed = Allowed(before, limit, pml->frametime, driven);
		if (!keys && !driven)
			allowed = std::min(allowed, std::max(0.0f, before - std::max(before, RACER_STOP) * Coast(before) * pml->frametime));

		Q3::AirMove(pm, pml, true);

		const float after = glm::length(glm::vec2(ps->velocity));
		if (after > 0.0f && (after > allowed || driven))
		{
			ps->velocity[0] *= allowed / after;
			ps->velocity[1] *= allowed / after;
		}
		// A jump from a stand has no run to add to yet: it starts the way the keys point.
		else if (after <= 0.0f && keys && driven)
		{
			const glm::vec2 wish = glm::normalize(glm::vec2(pml->forward) * static_cast<float>(pm->cmd.forwardmove)
				+ glm::vec2(pml->right) * static_cast<float>(pm->cmd.rightmove));
			ps->velocity[0] = wish.x * allowed;
			ps->velocity[1] = wish.y * allowed;
		}
	}

	// A star, a mushroom, a boost panel: any limit over the game's own speed.
	bool Racer::Boosted(float limit)
	{
		return limit > RACER_SPEED * RACER_UNIT + 1.0f;
	}

	// The limit on this ground: off-road slows whoever is not boosted.
	float Racer::Ground(float limit, int surfaceFlags)
	{
		return Boosted(limit) ? limit : limit * GROUNDS[(static_cast<unsigned>(surfaceFlags) >> RACER_SLOWS) & 3];
	}

	// The surface flags of the ground under a jump, none where there is none within a jump's reach.
	int Racer::Under(pmove_t* pm)
	{
		const vec3 from = pm->ps->origin;
		trace_t trace = {};
		PM_PlayerTrace(pm, &trace, from, vec3(0), vec3(0), from - vec3(0, 0, RACER_REACH), pm->ps->clientNum, pm->tracemask);
		return !trace.startsolid && !trace.allsolid && trace.fraction < 1.0f ? trace.surfaceFlags : 0;
	}

	// Whether the keys run on with the way already gone, rather than against it to brake.
	bool Racer::With(pmove_t* pm, pml_t* pml)
	{
		const glm::vec2 wish = glm::vec2(pml->forward) * static_cast<float>(pm->cmd.forwardmove)
			+ glm::vec2(pml->right) * static_cast<float>(pm->cmd.rightmove);
		const glm::vec2 run(pm->ps->velocity);
		if (glm::length(wish) <= 0.0f)
			return false;
		if (glm::length(run) < 1.0f)
			return true;
		return glm::dot(glm::normalize(wish), glm::normalize(run)) > RACER_AGAINST;
	}

	// The eyes of someone grown stand higher. The engine draws the view back to a stand's every frame,
	// so it is put back every frame, and falls by itself once the size is gone.
	void Racer::View(playerState_s* ps, bool big)
	{
		if (big && ps->viewHeightTarget == RACER_STAND)
			ps->viewHeightCurrent = RACER_BIG;
	}

	// What a second adds at this share of the limit: the game's curve, or a boost's flat rate.
	float Racer::Gain(float speed, float limit)
	{
		if (Boosted(limit))
			return RACER_BOOST * RACER_UNIT * RACER_FRAMES;

		const float ratio = limit > 0.0f ? speed / limit : 1.0f;
		float gain = GAINS[3];
		float from = 0.0f;

		for (int i = 0; i < 3; i++)
		{
			if (ratio < STEPS[i])
			{
				gain = GAINS[i] + (GAINS[i + 1] - GAINS[i]) / (STEPS[i] - from) * (ratio - from);
				break;
			}
			from = STEPS[i];
		}
		return gain * RACER_UNIT * RACER_FRAMES;
	}

	// The friction with no key held: a kart's long roll at speed, a walker's stop once slow, so the tap
	// of a key does not leave the racer creeping on.
	float Racer::Coast(float speed)
	{
		const float slow = glm::clamp((RACER_ROLL - speed) / (RACER_ROLL - RACER_STOP), 0.0f, 1.0f);
		return glm::mix(RACER_COAST, RACER_FRICTION, slow);
	}

	// The most this frame may end at: gained toward the limit while driven, eased down to it once
	// over. Never under what Q3's friction stops dead.
	float Racer::Allowed(float speed, float limit, float frametime, bool driven)
	{
		if (speed > limit)
			return std::max(limit, speed - RACER_SETTLE * RACER_UNIT * RACER_FRAMES * frametime);
		if (!driven)
			return speed;
		return std::min(limit, std::max(speed + Gain(speed, limit) * frametime, RACER_START));
	}
}

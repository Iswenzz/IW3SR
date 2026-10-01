#pragma once
#include "Game/Base.hpp"

#include <glm/gtc/quaternion.hpp>

namespace IW3SR
{
	// A model the engine posed, whose bones are moved afterwards. The engine works a bone out from its
	// parent only when something asks for it, so only the bones asked for here are moved: whatever hangs
	// from them is worked out later, from the moved parent, and follows.
	class VRSkeleton
	{
	public:
		VRSkeleton(DObj_s* obj, const cpose_t* pose, bool fresh);

		static unsigned int Tag(const char* name);

		bool World(const char* name, vec3& origin, mat3& axis) const;
		bool Rest(const char* name, mat3& axis) const;
		void Move(const char* name, const glm::quat& turn, const vec3& from, const vec3& to) const;
		bool Reach(const char* upper, const char* lower, const char* end, const vec3& target, const vec3& pole) const;
		void Bend(const char* name, const char* end, const vec3& target, float limit) const;
		void Turn(const char* name, const mat3& axis) const;
		void Orient(const char* name, const char* pivot, const mat3& axis, float share) const;

	private:
		DObj_s* Obj = nullptr;
		const cpose_t* Pose = nullptr;
		const XModel* Model = nullptr;

		int Bone(const char* name) const;
	};
}

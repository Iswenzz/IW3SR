#include "Skeleton.hpp"

namespace IW3SR
{
	namespace
	{
		// A limb's hinge in each of its two bones' own frames, by the model and the limb's upper bone.
		struct KnownHinge
		{
			const XModel* Model = nullptr;
			int Bone = -1;
			vec3 Upper{};
			vec3 Lower{};
		};

		// Bent less than this, as the sine of the angle, an animated limb tells too little of its hinge.
		constexpr float MinHingeBend = 0.2f;

		std::vector<KnownHinge> Hinges;

		bool Descends(int bone, int ancestor, const XModel* model)
		{
			const int roots = static_cast<uint8_t>(model->numRootBones);
			while (bone > ancestor && bone >= roots)
				bone -= static_cast<uint8_t>(model->parentList[bone - roots]);
			return bone == ancestor;
		}

		// The shortest turn taking one direction onto another.
		glm::quat Between(const vec3& from, const vec3& to)
		{
			const float dot = glm::dot(from, to);
			if (dot < -0.9999f)
			{
				vec3 axis = glm::cross(vec3(1.0f, 0.0f, 0.0f), from);
				if (glm::length2(axis) < 1e-6f)
					axis = glm::cross(vec3(0.0f, 1.0f, 0.0f), from);
				return glm::angleAxis(glm::pi<float>(), glm::normalize(axis));
			}
			const vec3 cross = glm::cross(from, to);
			return glm::normalize(glm::quat::wxyz(1.0f + dot, cross.x, cross.y, cross.z));
		}

		// The frame whose first axis runs along, and whose second is as near across as it can be.
		mat3 Frame(const vec3& along, const vec3& across)
		{
			const vec3 x = glm::normalize(along);
			const vec3 y = glm::normalize(across - glm::dot(across, x) * x);
			return mat3(x, y, glm::cross(x, y));
		}
	}

	// Fresh forgets what the engine worked out so far, which may predate this frame's pose, or a bone
	// moved above it.
	VRSkeleton::VRSkeleton(DObj_s* obj, const cpose_t* pose, bool fresh) : Obj(obj), Pose(pose)
	{
		Model = obj && obj->models ? obj->models[0] : nullptr;
		if (obj && fresh)
			std::memset(&obj->skel.partBits, 0, sizeof(obj->skel.partBits));
	}

	// The script string a bone is named by, which there is none of until a model naming it is loaded.
	unsigned int VRSkeleton::Tag(const char* name)
	{
		return SL_FindStringOfSize(name, static_cast<unsigned int>(std::strlen(name) + 1));
	}

	int VRSkeleton::Bone(const char* name) const
	{
		const unsigned int tag = Tag(name);
		uint8_t index = 254;
		if (!Obj || !Model || !tag || !DObjGetBoneIndex(Obj, tag, &index)
			|| index >= static_cast<uint8_t>(Model->numBones))
			return -1;
		return index;
	}

	bool VRSkeleton::World(const char* name, vec3& origin, mat3& axis) const
	{
		const unsigned int tag = Tag(name);
		float matrix[3][3] = {};
		if (!Obj || !Pose || !tag || !CG_DObjGetWorldTagMatrix(Pose, Obj, tag, matrix, &origin[0]))
			return false;

		axis = mat3(vec3(matrix[0][0], matrix[0][1], matrix[0][2]), vec3(matrix[1][0], matrix[1][1], matrix[1][2]),
			vec3(matrix[2][0], matrix[2][1], matrix[2][2]));
		return true;
	}

	// The bone's axes in the model's rest pose, which stands facing along x with z up.
	bool VRSkeleton::Rest(const char* name, mat3& axis) const
	{
		const int bone = Bone(name);
		if (bone < 0 || !Model->baseMat)
			return false;

		const float* q = Model->baseMat[bone].quat;
		axis = glm::mat3_cast(glm::normalize(glm::quat::wxyz(q[3], q[0], q[1], q[2])));
		return true;
	}

	// Turns and moves the bone and everything below it already worked out, so a point at from ends up at
	// to. The engine keeps its bones less the view offset.
	void VRSkeleton::Move(const char* name, const glm::quat& turn, const vec3& from, const vec3& to) const
	{
		const int root = Bone(name);
		DObjAnimMat* mats = Obj ? Obj->skel.mat : nullptr;
		if (root < 0 || !mats || !cgs)
			return;

		const vec3 view = cgs->refdef.viewOffset;
		const vec3 shift = to - turn * from + turn * view - view;
		const int bones = static_cast<uint8_t>(Model->numBones);
		for (int bone = root; bone < bones; bone++)
		{
			const uint32_t done = static_cast<uint32_t>(Obj->skel.partBits.skel[bone >> 5]) & (0x80000000u >> (bone & 31));
			if (!done || !Descends(bone, root, Model))
				continue;

			DObjAnimMat& mat = mats[bone];
			const glm::quat rotation = turn * glm::quat::wxyz(mat.quat[3], mat.quat[0], mat.quat[1], mat.quat[2]);
			const vec3 trans = turn * vec3(mat.trans[0], mat.trans[1], mat.trans[2]) + shift;
			mat.quat[0] = rotation.x;
			mat.quat[1] = rotation.y;
			mat.quat[2] = rotation.z;
			mat.quat[3] = rotation.w;
			mat.trans[0] = trans.x;
			mat.trans[1] = trans.y;
			mat.trans[2] = trans.z;
		}
	}

	// The upper bone turns about its joint and the lower about its own so the end lands on the target,
	// bent toward the pole, a direction from the upper joint. Out of reach the limb points at the target.
	// The joint between them is a hinge, which both bones keep their animated axis on: the limb rolls to
	// bend the way it can, rather than the lower bone swinging off sideways and twisting the skin.
	bool VRSkeleton::Reach(const char* upper, const char* lower, const char* end, const vec3& target,
		const vec3& pole) const
	{
		vec3 a{};
		vec3 b{};
		vec3 c{};
		mat3 axis{ 1.0f };
		mat3 upperAxis{ 1.0f };
		mat3 lowerAxis{ 1.0f };
		if (!World(end, c, axis) || !World(lower, b, lowerAxis) || !World(upper, a, upperAxis))
			return false;

		const float first = glm::distance(a, b);
		const float second = glm::distance(b, c);
		float reach = glm::distance(a, target);
		if (first < 0.01f || second < 0.01f || reach < 0.01f)
			return false;

		const vec3 direction = (target - a) / reach;
		reach = std::clamp(reach, std::abs(first - second) + 0.01f, first + second - 0.01f);

		vec3 bend = pole - glm::dot(pole, direction) * direction;
		if (glm::length2(bend) < 1e-6f)
			bend = (b - a) - glm::dot(b - a, direction) * direction;
		if (glm::length2(bend) < 1e-6f)
			return false;
		bend = glm::normalize(bend);

		const float cosine = std::clamp((first * first + reach * reach - second * second) / (2.0f * first * reach),
			-1.0f, 1.0f);
		const vec3 joint = a + first * (cosine * direction + std::sqrt(1.0f - cosine * cosine) * bend);

		vec3 upperHinge{};
		vec3 lowerHinge{};
		if (Hinge(Bone(upper), a, b, c, upperAxis, lowerAxis, upperHinge, lowerHinge))
		{
			// The axis turning the upper bone onto the lower, as the animation hinge is taken.
			const vec3 hinge = glm::normalize(glm::cross(bend, direction));
			const mat3 upperTo = Frame(joint - a, hinge)
				* glm::transpose(Frame(glm::transpose(upperAxis) * (b - a), upperHinge));
			const glm::quat shoulder = glm::quat_cast(upperTo * glm::transpose(upperAxis));
			Move(upper, shoulder, a, a);

			const mat3 lowerNow = glm::mat3_cast(shoulder) * lowerAxis;
			const mat3 lowerTo = Frame(a + direction * reach - joint, hinge)
				* glm::transpose(Frame(glm::transpose(lowerAxis) * (c - b), lowerHinge));
			Move(lower, glm::quat_cast(lowerTo * glm::transpose(lowerNow)), joint, joint);
			return true;
		}

		const glm::quat shoulder = Between(glm::normalize(b - a), glm::normalize(joint - a));
		Move(upper, shoulder, a, a);

		const vec3 moved = a + shoulder * (c - a);
		const glm::quat elbow = Between(glm::normalize(moved - joint), glm::normalize(a + direction * reach - joint));
		Move(lower, elbow, joint, joint);
		return true;
	}

	// The axis the joint between two bones bends about, in each bone's own frame: from the animation while
	// it holds the limb bent, else from the last time it did.
	bool VRSkeleton::Hinge(int bone, const vec3& a, const vec3& b, const vec3& c, const mat3& upperAxis,
		const mat3& lowerAxis, vec3& upperHinge, vec3& lowerHinge) const
	{
		auto known = std::find_if(Hinges.begin(), Hinges.end(),
			[&](const KnownHinge& hinge) { return hinge.Model == Model && hinge.Bone == bone; });

		const vec3 normal = glm::cross(b - a, c - b);
		if (bone >= 0 && glm::length(normal) > MinHingeBend * glm::distance(a, b) * glm::distance(b, c))
		{
			if (known == Hinges.end())
				known = Hinges.insert(Hinges.end(), KnownHinge{ Model, bone });

			const vec3 hinge = glm::normalize(normal);
			known->Upper = glm::transpose(upperAxis) * hinge;
			known->Lower = glm::transpose(lowerAxis) * hinge;
		}
		if (known == Hinges.end())
			return false;

		upperHinge = known->Upper;
		lowerHinge = known->Lower;
		return true;
	}

	// Turns the bone about its joint so the end points that share of the way at the target, by at most
	// limit radians.
	void VRSkeleton::Bend(const char* name, const char* end, const vec3& target, float limit, float share) const
	{
		vec3 joint{};
		vec3 tip{};
		mat3 axis{ 1.0f };
		if (!World(end, tip, axis) || !World(name, joint, axis) || glm::length2(tip - joint) < 1e-4f
			|| glm::length2(target - joint) < 1e-4f)
			return;

		const glm::quat full = glm::slerp(glm::quat::wxyz(1.0f, 0.0f, 0.0f, 0.0f),
			Between(glm::normalize(tip - joint), glm::normalize(target - joint)), share);
		const float angle = glm::angle(full);
		const glm::quat turn = angle > limit ? glm::slerp(glm::quat::wxyz(1.0f, 0.0f, 0.0f, 0.0f), full, limit / angle) : full;
		Move(name, turn, joint, joint);
	}

	// The bone's axes become these, turning about its own origin.
	void VRSkeleton::Turn(const char* name, const mat3& axis) const
	{
		Orient(name, name, axis, 1.0f);
	}

	// Turns the pivot about its joint so the named bone, hanging from it, turns that share of the way to
	// these axes.
	void VRSkeleton::Orient(const char* name, const char* pivot, const mat3& axis, float share) const
	{
		vec3 origin{};
		vec3 joint{};
		mat3 current{ 1.0f };
		mat3 frame{ 1.0f };
		if (!World(name, origin, current) || !World(pivot, joint, frame))
			return;

		const glm::quat full = glm::quat_cast(axis * glm::transpose(current));
		Move(pivot, glm::slerp(glm::quat::wxyz(1.0f, 0.0f, 0.0f, 0.0f), full, share), joint, joint);
	}
}

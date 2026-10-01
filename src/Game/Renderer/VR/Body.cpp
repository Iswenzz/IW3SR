#include "Body.hpp"
#include "Skeleton.hpp"

namespace IW3SR
{
	namespace
	{
		// The wrist sits behind the middle of a controller's grip, along the way the hand points, in game units.
		constexpr float WristBack = 3.0f;

		// How far the back bends to bring the head over the player's own, at most, in radians.
		constexpr float MaxBend = 0.7f;

		constexpr int ProneFlag = 0x8;

		// The parent of a model melded onto the body by bone names, the head among them; the weapon and the
		// knife hang from a bone of the hands instead.
		constexpr uint8_t Melded = 0xFF;

		// Left, then right, as the bones are named.
		constexpr const char* Sides[2] = { "le", "ri" };

		// One bit per bone of a DObj, highest bit first, as the engine keeps them.
		using PartBits = std::array<uint32_t, 4>;

		bool Bit(const PartBits& bits, int bone)
		{
			return bits[bone >> 5] & (0x80000000u >> (bone & 31));
		}

		void SetBit(PartBits& bits, int bone)
		{
			if (bone >= 0 && bone < 128)
				bits[bone >> 5] |= 0x80000000u >> (bone & 31);
		}

		PartBits SurfaceBits(const XSurface& surface)
		{
			PartBits bits{};
			std::memcpy(bits.data(), surface.partBits, sizeof(surface.partBits));
			return bits;
		}

		// A bone of one side, as "j_wrist_" and the left side make "j_wrist_le".
		std::string Bone(const char* name, int side)
		{
			return std::string(name) + Sides[side];
		}

		// Whether a bone of the model is the named one, or hangs from it.
		bool Under(const XModel* model, int bone, unsigned int name)
		{
			const int roots = static_cast<uint8_t>(model->numRootBones);
			while (name)
			{
				if (model->boneNames[bone] == name)
					return true;
				if (bone < roots)
					break;
				bone -= static_cast<uint8_t>(model->parentList[bone - roots]);
			}
			return false;
		}

		// Every surface of the model that moves with the head, the face, hair and helmet, around the view
		// otherwise. The head model carries the hands as well; a hand that shares a surface with the head
		// goes with it.
		void HideHead(const XModel* model, int base, PartBits& hidden)
		{
			const unsigned int names[] = { VRSkeleton::Tag("j_neck"), VRSkeleton::Tag("j_head") };
			PartBits head{};
			const int bones = static_cast<uint8_t>(model->numBones);
			for (int bone = 0; bone < bones; bone++)
			{
				for (const unsigned int name : names)
				{
					if (Under(model, bone, name))
						SetBit(head, bone);
				}
			}

			const XModelLodInfo& lod = model->lodInfo[0];
			for (const XSurface& surface : std::span<const XSurface>(model->surfs + lod.surfIndex, lod.numsurfs))
			{
				const PartBits bits = SurfaceBits(surface);
				for (int bone = 0; bone < bones; bone++)
				{
					if (Bit(bits, bone) && Bit(head, bone))
						SetBit(hidden, base + bone);
				}
			}
		}

		mat3 AnglesAxis(const vec3& angles)
		{
			vec3 forward, right, up;
			Math::AngleVectors(angles, forward, right, up);
			return mat3(forward, -right, up);
		}

		bool Has(const VRNetState& state, VRTracker tracker)
		{
			return state.Trackers & (1u << static_cast<int>(tracker));
		}
	}

	void GVRBody::Reset()
	{
		Headset = false;
		OwnShown = false;
	}

	// Called for every frame the headset draws, with what the player shares of itself when it plays.
	void GVRBody::SetOwn(const VRNetState* state, bool gun)
	{
		Headset = true;
		OwnShown = state != nullptr;
		OwnGun = gun;
		if (state)
			Own = *state;
	}

	// CG_Player draws the player the view belongs to only in third person; its own body, in first too.
	bool GVRBody::PlayerVisible()
	{
		return OwnShown || (cgs && cgs->renderingThirdPerson);
	}

	// Every player model passes here on its way into the scene, and so does the view weapon, which false
	// keeps out while the headset draws: the player's own body holds the gun there.
	bool GVRBody::SceneModel(DObj_s* obj, const cpose_t* pose, int entnum)
	{
		if (!obj || !pose || !cgs)
			return true;
		if (pose == &cgs->viewModelPose)
			return !Headset;
		if (entnum < 0 || entnum >= 64)
			return true;

		// A player's pose is the first thing in its entity.
		const auto* cent = reinterpret_cast<const centity_s*>(pose);
		const bool prone = cent->nextState.lerp.eFlags & ProneFlag;
		const float* angles = cgs->bgs.clientinfo[entnum].playerAngles;
		const vec3 aim(angles[0], angles[1], angles[2]);

		if (entnum == cgs->clientNum && pose == &cgs->predictedPlayerEntity.pose)
		{
			if (OwnShown && !cgs->renderingThirdPerson)
				Hide(obj, OwnGun);
			else if (std::exchange(Hidden, nullptr) == obj)
				std::memset(obj->hidePartBits, 0, sizeof(obj->hidePartBits));

			if (OwnShown)
				Pose(obj, pose, Own, aim, prone, !cgs->renderingThirdPerson);
			return true;
		}

		if (const VRNetState* state = GVRNetwork::Get(entnum))
			Pose(obj, pose, *state, aim, prone, false);
		return true;
	}

	// The state's poses are offsets from where the player stands. The animation is measured first, then
	// moved from the hips outward, each step on the bones the one before left in place. Lying down, only
	// the arms and the head follow.
	void GVRBody::Pose(DObj_s* obj, const cpose_t* pose, const VRNetState& state, const vec3& aim, bool prone,
		bool headless)
	{
		// The eyes are turned onto the headset and the head with them. Looking at them works out bones of
		// the head model, which would not follow the body's after that, so the skeleton starts over.
		vec3 eye{};
		mat3 eyes{ 1.0f };
		const bool sighted = VRSkeleton(obj, pose, true).World("tag_eye", eye, eyes);

		const VRSkeleton skeleton(obj, pose, true);
		const vec3 origin = pose->origin;
		const auto at = [&](const VRNetPose& part) { return origin + part.Offset; };
		const auto axis = [](const VRNetPose& part) { return glm::mat3_cast(part.Rotation); };
		const auto tracked = [&](VRTracker role) -> const VRNetPose& { return state.TrackerPoses[static_cast<int>(role)]; };

		vec3 grip{};
		vec3 rightWrist{};
		vec3 leftWrist{};
		vec3 head{};
		mat3 weapon{ 1.0f };
		mat3 rightHand{ 1.0f };
		mat3 leftHand{ 1.0f };
		mat3 headAxis{ 1.0f };
		const bool armed = skeleton.World("tag_weapon_right", grip, weapon)
			&& skeleton.World("j_wrist_ri", rightWrist, rightHand) && skeleton.World("j_wrist_le", leftWrist, leftHand);
		const bool looking = (state.Parts & VRPartHead) && skeleton.World("j_head", head, headAxis);

		const mat3 view = sighted ? eyes : AnglesAxis(aim);
		const mat3 turned = axis(state.Head) * glm::transpose(view) * headAxis;
		const vec3 sight = sighted ? turned * (glm::transpose(headAxis) * (eye - head)) : vec3(0.0f);

		// The body faces the way the player's hips do, over the waist tracker. The back takes the way the
		// chest tracker faces, bent at its lower and upper joints alike; without one, it bends to bring the
		// eyes over the player's, which lean with the view.
		const glm::quat body = glm::angleAxis(Math::DegToRad(Math::AngleDelta(state.BodyYaw, pose->angles[YAW])),
			vec3(0.0f, 0.0f, 1.0f));
		vec3 hips{};
		mat3 frame{ 1.0f };
		mat3 spine{ 1.0f };
		if (!prone && skeleton.World("j_mainroot", hips, frame))
		{
			const vec3 waist = Has(state, VRTracker::Waist) ? at(tracked(VRTracker::Waist)) : hips;
			skeleton.Move("j_mainroot", body, hips, vec3(waist.x, waist.y, hips.z));

			if (Has(state, VRTracker::Chest) && skeleton.Rest("j_spine4", spine))
			{
				const mat3 chest = axis(tracked(VRTracker::Chest)) * spine;
				skeleton.Orient("j_spine4", "j_spinelower", chest, 0.5f);
				skeleton.Orient("j_spine4", "j_spineupper", chest, 1.0f);
			}
			else if (looking)
			{
				const vec3 camera = at(state.Head);
				skeleton.Bend("j_spinelower", "j_head", vec3(camera.x - sight.x, camera.y - sight.y, head.z), MaxBend);
			}
		}

		// The legs reach for the feet trackers, bent toward the knee trackers or the way the body faces, and
		// the feet turn the way the player's do.
		const vec3 forward = AnglesAxis({ 0.0f, state.BodyYaw, 0.0f })[0];
		for (int side = 0; side < 2 && !prone; side++)
		{
			const VRTracker foot = side ? VRTracker::RightFoot : VRTracker::LeftFoot;
			const VRTracker knee = side ? VRTracker::RightKnee : VRTracker::LeftKnee;
			const std::string hip = Bone("j_hip_", side);
			const std::string ankle = Bone("j_ankle_", side);
			vec3 root{};
			mat3 rest{ 1.0f };
			if (!Has(state, foot) || !skeleton.World(hip.c_str(), root, frame))
				continue;

			const vec3 pole = Has(state, knee) ? at(tracked(knee)) - root : forward;
			if (skeleton.Reach(hip.c_str(), Bone("j_knee_", side).c_str(), ankle.c_str(), at(tracked(foot)), pole)
				&& skeleton.Rest(ankle.c_str(), rest))
				skeleton.Turn(ankle.c_str(), axis(tracked(foot)) * rest);
		}

		// The elbows hang down and out, unless trackers say where.
		const auto arm = [&](int side, const vec3& target)
		{
			const VRTracker elbow = side ? VRTracker::RightElbow : VRTracker::LeftElbow;
			const std::string shoulder = Bone("j_shoulder_", side);
			vec3 root{};
			vec3 chest{};
			if (!skeleton.World(shoulder.c_str(), root, frame))
				return false;

			vec3 pole = Has(state, elbow) ? at(tracked(elbow)) - root : vec3(0.0f, 0.0f, -1.0f);
			if (!Has(state, elbow) && skeleton.World("j_spine4", chest, frame))
			{
				const vec3 out(root.x - chest.x, root.y - chest.y, 0.0f);
				if (glm::length2(out) > 1e-4f)
					pole += glm::normalize(out) * 0.5f;
			}
			return skeleton.Reach(shoulder.c_str(), Bone("j_elbow_", side).c_str(), Bone("j_wrist_", side).c_str(),
				target, pole);
		};

		if (armed)
		{
			// The weapon hangs from the right hand, so the wrist goes where that puts the weapon on the grip,
			// turned the way the hand points. Untracked, the hand is left where the body carried it.
			vec3 gunAt = grip;
			mat3 gun = weapon;
			const mat3 toGun = glm::transpose(weapon);
			if (state.Parts & VRPartRight)
			{
				gunAt = at(state.RightHand);
				gun = axis(state.RightHand);
				if (arm(1, gunAt + gun * (toGun * (rightWrist - grip))))
					skeleton.Turn("j_wrist_ri", gun * toGun * rightHand);
			}
			else
				skeleton.World("tag_weapon_right", gunAt, gun);

			// The left hand stays on the weapon where the animation holds it, reloads included, unless its
			// controller is away from there.
			if (state.Parts & (VRPartRight | VRPartLeft))
			{
				vec3 target = gunAt + gun * (toGun * (leftWrist - grip));
				glm::quat turn = glm::quat_cast(gun * toGun * leftHand);
				if (state.Parts & VRPartLeft)
				{
					const mat3 hand = axis(state.LeftHand);
					const vec3 free = at(state.LeftHand) - hand[0] * WristBack;
					const float reach = glm::distance(free, target) / UnitsPerMeter;
					const float follow = std::clamp((reach - VRGripNear) / (VRGripFar - VRGripNear), 0.0f, 1.0f);
					target = glm::mix(target, free, follow);
					turn = glm::slerp(turn, glm::quat_cast(hand * toGun * leftHand), follow);
				}
				if (arm(0, target))
					skeleton.Turn("j_wrist_le", glm::mat3_cast(turn));
			}
		}

		if (looking)
			skeleton.Turn("j_head", turned);

		// A model all of one piece keeps its head in that piece, out of reach of hiding by surfaces, so the
		// player's own neck is folded into its chest, and the head with it, out of the view.
		vec3 chest{};
		vec3 neck{};
		if (headless && skeleton.World("j_spine4", chest, frame) && skeleton.World("j_neck", neck, frame))
			skeleton.Move("j_neck", glm::quat::wxyz(1.0f, 0.0f, 0.0f, 0.0f), neck, chest);
	}

	// The head, so the view is not inside it, and the weapon while the player draws none. The bits stay
	// until the model is made again, which clears them.
	void GVRBody::Hide(DObj_s* obj, bool gun)
	{
		PartBits hidden{};
		const int count = static_cast<uint8_t>(obj->numModels);
		const auto* parents = reinterpret_cast<const uint8_t*>(obj->models + count);
		for (int i = 0, base = 0; i < count && obj->models[i]; i++)
		{
			const XModel* model = obj->models[i];
			const int bones = static_cast<uint8_t>(model->numBones);
			if (i > 0 && parents[i] == Melded)
				HideHead(model, base, hidden);
			else if (i > 0 && !gun)
			{
				for (int bone = 0; bone < bones; bone++)
					SetBit(hidden, base + bone);
			}
			base += bones;
		}
		std::memcpy(obj->hidePartBits, hidden.data(), sizeof(obj->hidePartBits));
		Hidden = obj;
	}
}

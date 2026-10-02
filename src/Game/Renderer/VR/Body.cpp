#include "Body.hpp"
#include "Skeleton.hpp"

namespace IW3SR
{
	namespace
	{
		// The wrist sits behind the middle of a controller's grip, along the way the hand points, in game units.
		constexpr float WristBack = 3.0f;

		// How far below the view the player's own neck is kept in the air, in game units, so a jump that lifts
		// the body does not bring its inside into view. On the ground it stays where it stands, or the arms
		// could not reach the hands at its sides.
		constexpr float NeckClearance = 7.0f;

		// How the back curves to bring the head over the player's: each joint up from the hips turns its
		// share of the way, by at most its limit in radians, and the neck takes the rest.
		struct LeanJoint
		{
			const char* Bone;
			float Share;
			float Limit;
		};
		constexpr LeanJoint Lean[] = {
			{ "j_spinelower", 0.3f, 0.3f },
			{ "j_spineupper", 0.4f, 0.3f },
			{ "j_spine4", 0.5f, 0.3f },
		};
		constexpr float NeckLimit = 0.5f;

		// Without a chest tracker the chest turns this share toward both hands held out in front of it, by at
		// most MaxChestTwist degrees.
		constexpr float ChestFollowsHands = 0.4f;
		constexpr float MaxChestTwist = 45.0f;
		constexpr float MinHandsOut = 6.0f;

		// The collarbone lifts the shoulder toward a hand raised above it, or reached out past this much of
		// the arm, by this share of the way and at most the limit in radians.
		constexpr float ClavicleShare = 0.3f;
		constexpr float ClavicleLimit = 0.35f;
		constexpr float ClavicleReach = 0.8f;

		// Without a tracker an elbow points down, a little out, and away from the palm and the thumb, so it
		// follows the way the hand is turned.
		constexpr float ElbowOut = 0.3f;
		constexpr float ElbowPalm = 0.7f;
		constexpr float ElbowThumb = 0.4f;

		// Where a player's shoulders sit against its eyes, and how long its arms may be, in game units.
		constexpr float ShoulderDrop = 9.0f;
		constexpr float ShoulderWidth = 7.0f;
		constexpr float ShortestArm = 19.7f;
		constexpr float LongestArm = 33.5f;
		constexpr float UsualArm = 23.6f;

		// A reach this far out of the longest is an arm held straight: the longest includes the shoulder
		// pushed forward, which an arm hanging at the side never has.
		constexpr float BentReach = 0.7f;
		constexpr float StraightReach = 0.85f;

		// How far a straightened arm may overshoot the hand, which then slides back up the forearm.
		constexpr float MaxSlide = 3.0f;

		constexpr int ProneFlag = 0x8;
		constexpr int NoGround = 1023;

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

		// How straight a player's real arm is: its hand against the shoulder below the head, over the longest
		// reach that player was seen to make, which soon is its arm's length.
		float Extension(int entnum, int side, const vec3& shoulder, const vec3& hand)
		{
			static float longest[64][2] = {};
			float& arm = longest[entnum & 63][side];
			const float reach = glm::distance(shoulder, hand);
			arm = std::clamp(std::max(arm > 0.0f ? arm : UsualArm, reach), ShortestArm, LongestArm);
			return std::clamp(reach / arm, 0.0f, 1.0f);
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

	// The VR player a first person view follows, whose body is shown as that player sees it.
	const VRNetState* GVRBody::Followed()
	{
		if (!cgs || cgs->renderingThirdPerson)
			return nullptr;

		const playerState_s& ps = cgs->predictedPlayerState;
		return ps.clientNum != cgs->clientNum ? GVRNetwork::Get(ps.clientNum) : nullptr;
	}

	// CG_Player draws the player the view belongs to only in third person; its own body in first too, and
	// a followed VR player's.
	bool GVRBody::PlayerVisible()
	{
		return OwnShown || (cgs && cgs->renderingThirdPerson) || Followed();
	}

	// Every player model passes here on its way into the scene, and so does the view weapon, which false
	// keeps out while the headset draws: the player's own body holds the gun there.
	bool GVRBody::SceneModel(DObj_s* obj, const cpose_t* pose, int entnum)
	{
		if (!obj || !pose || !cgs)
			return true;
		if (pose == &cgs->viewModelPose)
			return !Headset && !Followed();
		if (entnum < 0 || entnum >= 64)
			return true;

		// A player's pose is the first thing in its entity.
		const auto* cent = reinterpret_cast<const centity_s*>(pose);
		const bool prone = cent->nextState.lerp.eFlags & ProneFlag;
		const bool airborne = cent->nextState.groundEntityNum == NoGround;
		const float* angles = cgs->bgs.clientinfo[entnum].playerAngles;
		const vec3 aim(angles[0], angles[1], angles[2]);

		// The player the view belongs to, or follows: seen from inside its own head.
		if (pose == &cgs->predictedPlayerEntity.pose)
		{
			const bool own = entnum == cgs->clientNum;
			const VRNetState* state = own ? (OwnShown ? &Own : nullptr) : Followed();
			const bool inside = state && !cgs->renderingThirdPerson;
			if (inside)
				Hide(obj, own ? OwnGun : true);
			else if (std::exchange(Hidden, nullptr) == obj)
				std::memset(obj->hidePartBits, 0, sizeof(obj->hidePartBits));

			if (state)
				Pose(obj, pose, entnum, *state, aim, prone, inside, airborne);
			return true;
		}

		if (const VRNetState* state = GVRNetwork::Get(entnum))
			Pose(obj, pose, entnum, *state, aim, prone, false, false);
		return true;
	}

	// The state's poses are offsets from where the player stands. The animation is measured first, then
	// moved from the hips outward, each step on the bones the one before left in place. Lying down, only
	// the arms and the head follow.
	void GVRBody::Pose(DObj_s* obj, const cpose_t* pose, int entnum, const VRNetState& state, const vec3& aim,
		bool prone, bool headless, bool airborne)
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
		const mat3 facing = AnglesAxis({ 0.0f, state.BodyYaw, 0.0f });
		const vec3 up(0.0f, 0.0f, 1.0f);
		vec3 hips{};
		mat3 frame{ 1.0f };
		mat3 rest{ 1.0f };
		if (!prone && skeleton.World("j_mainroot", hips, frame))
		{
			const vec3 waist = Has(state, VRTracker::Waist) ? at(tracked(VRTracker::Waist)) : hips;
			skeleton.Move("j_mainroot", body, hips, vec3(waist.x, waist.y, hips.z));

			// The engine bends the back, the neck and the head with the aim, which in VR is the head's, and
			// leans them into its crouch: they start upright over the hips instead, facing the body's way.
			for (const char* bone : { "j_spinelower", "j_spineupper", "j_spine4", "j_neck", "j_head" })
			{
				if (skeleton.Rest(bone, rest))
					skeleton.Turn(bone, facing * rest);
			}

			const bool chested = Has(state, VRTracker::Chest) && skeleton.Rest("j_spine4", rest);
			if (chested)
			{
				const mat3 chest = axis(tracked(VRTracker::Chest)) * rest;
				skeleton.Orient("j_spine4", "j_spinelower", chest, 0.5f);
				skeleton.Orient("j_spine4", "j_spineupper", chest, 1.0f);
			}
			else if ((state.Parts & VRPartRight) && (state.Parts & VRPartLeft))
			{
				vec3 lower{};
				vec3 upper{};
				const vec3 hands = (at(state.RightHand) + at(state.LeftHand)) * 0.5f;
				if (skeleton.World("j_spineupper", lower, frame) && glm::length(vec2(hands - lower)) > MinHandsOut)
				{
					const float toward = Math::RadToDeg(std::atan2(hands.y - lower.y, hands.x - lower.x));
					const float twist = Math::DegToRad(std::clamp(Math::AngleDelta(toward, state.BodyYaw),
						-MaxChestTwist, MaxChestTwist) * ChestFollowsHands);
					skeleton.Move("j_spineupper", glm::angleAxis(twist * 0.5f, up), lower, lower);
					if (skeleton.World("j_spine4", upper, frame))
						skeleton.Move("j_spine4", glm::angleAxis(twist * 0.5f, up), upper, upper);
				}
			}

			vec3 crown{};
			if (looking && skeleton.World("j_head", crown, frame))
			{
				const vec3 camera = at(state.Head);
				const vec3 over(camera.x - sight.x, camera.y - sight.y, crown.z);
				for (const LeanJoint& joint : Lean)
				{
					if (!chested)
						skeleton.Bend(joint.Bone, "j_head", over, joint.Limit, joint.Share);
				}
				skeleton.Bend("j_neck", "j_head", over, NeckLimit);
			}

			vec3 neck{};
			const float top = at(state.Head).z - NeckClearance;
			if (headless && airborne && (state.Parts & VRPartHead) && skeleton.World("j_neck", neck, frame) && neck.z > top
				&& skeleton.World("j_mainroot", hips, frame))
				skeleton.Move("j_mainroot", glm::quat::wxyz(1.0f, 0.0f, 0.0f, 0.0f), hips, hips - vec3(0.0f, 0.0f, neck.z - top));
		}

		// The legs reach for the feet trackers, bent toward the knee trackers or the way the body faces, and
		// the feet turn the way the player's do.
		const vec3 forward = facing[0];
		for (int side = 0; side < 2 && !prone; side++)
		{
			const VRTracker foot = side ? VRTracker::RightFoot : VRTracker::LeftFoot;
			const VRTracker knee = side ? VRTracker::RightKnee : VRTracker::LeftKnee;
			const std::string hip = Bone("j_hip_", side);
			const std::string ankle = Bone("j_ankle_", side);
			vec3 root{};
			if (!Has(state, foot) || !skeleton.World(hip.c_str(), root, frame))
				continue;

			// The knee points the way the foot does.
			const vec3 pole = Has(state, knee) ? at(tracked(knee)) - root : axis(tracked(foot))[0] + forward;
			if (skeleton.Reach(hip.c_str(), Bone("j_knee_", side).c_str(), ankle.c_str(), at(tracked(foot)), pole)
				&& skeleton.Rest(ankle.c_str(), rest))
				skeleton.Turn(ankle.c_str(), axis(tracked(foot)) * rest);
		}

		// The elbows hang down and out, unless trackers say where. The model's arm is as straight as the
		// player's, and where that reaches past the hand, the hand slides back up the forearm onto it.
		const vec3 headset = at(state.Head);
		const vec3 across = facing[1];
		const auto straightness = [&](int side, const vec3& hand)
		{
			if (!(state.Parts & VRPartHead))
				return 0.0f;
			const vec3 shoulder = headset + across * (side ? -ShoulderWidth : ShoulderWidth) - vec3(0.0f, 0.0f, ShoulderDrop);
			const float extension = Extension(entnum, side, shoulder, hand);
			return std::clamp((extension - BentReach) / (StraightReach - BentReach), 0.0f, 1.0f);
		};

		// The helper bones around the elbow and along the forearm are posed for the animation's bent arm.
		// The bulge takes half of the elbow's turn since, and the forearm half of the wrist's, or the elbow
		// keeps its bend in the skin and the forearm twists like a wrapper.
		struct ArmRest
		{
			mat3 Shoulder{ 1.0f }, Elbow{ 1.0f }, Wrist{ 1.0f }, Bulge{ 1.0f }, Twist{ 1.0f };
			bool Known = false;
		};
		ArmRest rests[2];
		for (int side = 0; side < 2; side++)
		{
			ArmRest& rest = rests[side];
			vec3 point{};
			rest.Known = skeleton.World(Bone("j_shoulder_", side).c_str(), point, rest.Shoulder)
				&& skeleton.World(Bone("j_elbow_", side).c_str(), point, rest.Elbow)
				&& skeleton.World(Bone("j_wrist_", side).c_str(), point, rest.Wrist)
				&& skeleton.World(Bone("j_elbow_bulge_", side).c_str(), point, rest.Bulge)
				&& skeleton.World(Bone("j_wristtwist_", side).c_str(), point, rest.Twist);
		}
		const auto relax = [&](int side)
		{
			const ArmRest& rest = rests[side];
			vec3 point{};
			mat3 shoulder{ 1.0f };
			mat3 elbow{ 1.0f };
			mat3 wrist{ 1.0f };
			if (!rest.Known || !skeleton.World(Bone("j_shoulder_", side).c_str(), point, shoulder)
				|| !skeleton.World(Bone("j_elbow_", side).c_str(), point, elbow)
				|| !skeleton.World(Bone("j_wrist_", side).c_str(), point, wrist))
				return;

			const glm::quat identity = glm::quat::wxyz(1.0f, 0.0f, 0.0f, 0.0f);
			const glm::quat bent = glm::quat_cast(glm::transpose(shoulder) * elbow
				* glm::transpose(glm::transpose(rest.Shoulder) * rest.Elbow));
			skeleton.Turn(Bone("j_elbow_bulge_", side).c_str(),
				shoulder * glm::mat3_cast(glm::slerp(identity, bent, 0.5f)) * glm::transpose(rest.Shoulder) * rest.Bulge);

			const glm::quat rolled = glm::quat_cast(glm::transpose(elbow) * wrist
				* glm::transpose(glm::transpose(rest.Elbow) * rest.Wrist));
			skeleton.Turn(Bone("j_wristtwist_", side).c_str(),
				elbow * glm::mat3_cast(glm::slerp(identity, rolled, 0.5f)) * glm::transpose(rest.Elbow) * rest.Twist);
		};
		const auto arm = [&](int side, const vec3& target, float straight, const vec3& palm, const vec3& thumb)
		{
			const VRTracker elbow = side ? VRTracker::RightElbow : VRTracker::LeftElbow;
			const std::string clavicle = Bone("j_clavicle_", side);
			const std::string shoulder = Bone("j_shoulder_", side);
			const std::string elbowBone = Bone("j_elbow_", side);
			const std::string wristBone = Bone("j_wrist_", side);
			vec3 root{};
			vec3 joint{};
			vec3 end{};
			if (!skeleton.World(wristBone.c_str(), end, frame) || !skeleton.World(elbowBone.c_str(), joint, frame)
				|| !skeleton.World(shoulder.c_str(), root, frame))
				return false;
			const float length = glm::distance(root, joint) + glm::distance(joint, end);

			const vec3 toHand = target - root;
			if (length > 0.01f && glm::length2(toHand) > 1e-4f)
			{
				const float raised = std::clamp(glm::dot(glm::normalize(toHand), up), 0.0f, 1.0f);
				const float outreach =
					std::clamp((glm::length(toHand) / length - ClavicleReach) / (1.0f - ClavicleReach), 0.0f, 1.0f);
				skeleton.Bend(clavicle.c_str(), shoulder.c_str(), target, ClavicleLimit,
					ClavicleShare * std::max(raised, outreach));
				if (!skeleton.World(shoulder.c_str(), root, frame))
					return false;
			}

			const vec3 outward = side ? -across : across;
			const vec3 pole = Has(state, elbow) ? at(tracked(elbow)) - root
				: -up + outward * ElbowOut - palm * ElbowPalm - thumb * ElbowThumb;

			const float reach = glm::distance(root, target);
			const float wanted = std::min(straight, 0.999f) * length;
			const vec3 aimed = wanted > reach && reach > 0.01f
				? root + (target - root) * (std::min(wanted, reach + MaxSlide) / reach)
				: target;
			if (!skeleton.Reach(shoulder.c_str(), elbowBone.c_str(), wristBone.c_str(), aimed, pole))
				return false;
			if (aimed != target && skeleton.World(wristBone.c_str(), end, frame))
				skeleton.Move(wristBone.c_str(), glm::quat::wxyz(1.0f, 0.0f, 0.0f, 0.0f), end, target);
			return true;
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
				if (arm(1, gunAt + gun * (toGun * (rightWrist - grip)), straightness(1, gunAt), gun[1], gun[2]))
				{
					skeleton.Turn("j_wrist_ri", gun * toGun * rightHand);
					relax(1);
				}
			}
			else
				skeleton.World("tag_weapon_right", gunAt, gun);

			// The left hand stays on the weapon where the animation holds it, reloads included, unless its
			// controller is away from there.
			if (state.Parts & (VRPartRight | VRPartLeft))
			{
				vec3 target = gunAt + gun * (toGun * (leftWrist - grip));
				glm::quat turn = glm::quat_cast(gun * toGun * leftHand);
				float straight = 0.0f;
				vec3 palm = -gun[1];
				vec3 thumb = gun[2];
				if (state.Parts & VRPartLeft)
				{
					const mat3 hand = axis(state.LeftHand);
					const vec3 free = at(state.LeftHand) - hand[0] * WristBack;
					const float reach = glm::distance(free, target) / UnitsPerMeter;
					const float follow = std::clamp((reach - VRGripNear) / (VRGripFar - VRGripNear), 0.0f, 1.0f);
					target = glm::mix(target, free, follow);
					turn = glm::slerp(turn, glm::quat_cast(hand * toGun * leftHand), follow);
					straight = straightness(0, at(state.LeftHand)) * follow;
					palm = glm::mix(palm, -hand[1], follow);
					thumb = glm::mix(thumb, hand[2], follow);
				}
				if (arm(0, target, straight, palm, thumb))
				{
					skeleton.Turn("j_wrist_le", glm::mat3_cast(turn));
					relax(0);
				}
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

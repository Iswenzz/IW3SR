#pragma once
#include "Network.hpp"

namespace IW3SR
{
	// Player models posed from VR state, as the engine adds them to the scene, the way VR games pose an
	// avatar: facing the way the hips do, the back upright and curving to follow the chest or bring the
	// head over the player's, the head turned the way it looks, the shoulders and the hinged arms reaching
	// for the hands with the weapon on the grip, and the legs and feet following body trackers. In VR the player sees its own body too, holding the gun, and never the view weapon; so does
	// whoever follows that player in first person.
	class GVRBody
	{
	public:
		static void Reset();
		static void SetOwn(const VRNetState* state, bool gun);
		static bool PlayerVisible();
		static bool SceneModel(DObj_s* obj, const cpose_t* pose, int entnum);

	private:
		static inline VRNetState Own;
		static inline bool Headset = false;
		static inline bool OwnShown = false;
		static inline bool OwnGun = true;
		static inline DObj_s* Hidden = nullptr;

		static void Pose(DObj_s* obj, const cpose_t* pose, int entnum, const VRNetState& state, const vec3& aim,
			bool prone, bool headless, bool airborne);
		static void Hide(DObj_s* obj, bool gun);
		static const VRNetState* Followed();
	};
}

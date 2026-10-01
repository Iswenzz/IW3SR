#pragma once
#include "VR.hpp"

#include <glm/gtc/quaternion.hpp>

namespace IW3SR
{
	constexpr uint8_t VRPartHead = 1;
	constexpr uint8_t VRPartRight = 2;
	constexpr uint8_t VRPartLeft = 4;

	// A pose as it travels: an offset from the player's origin, in game units, and a world rotation.
	struct VRNetPose
	{
		vec3 Offset{};
		glm::quat Rotation = glm::quat::wxyz(1.0f, 0.0f, 0.0f, 0.0f);
	};

	// What a VR player shares of itself: where the head looks, for spectators, and the way the body faces
	// and where the head, the hands and the trackers are, for drawing the body.
	struct VRNetState
	{
		int Time = 0;
		uint8_t Parts = 0;
		uint32_t Trackers = 0;
		vec3 HeadAngles{};
		float BodyYaw = 0.0f;
		VRNetPose Head;
		VRNetPose RightHand;
		VRNetPose LeftHand;
		VRNetPose TrackerPoses[VRTrackerCount];
	};

	// VR state shared between IW3SR clients through SR-Server: a player's own goes out as an out of band
	// "vr" datagram, and the server relays everyone else's the same way.
	class GVRNetwork
	{
	public:
		static void Send(const VRNetState& state);
		static bool Packet(const netadr_t* from, msg_t* msg);
		static const VRNetState* Get(int clientNum);

	private:
		static inline std::array<VRNetState, 64> Players = {};
		static inline int LastSend = 0;
	};
}

#pragma once
#include "Base.hpp"

namespace IW3SR::Addons
{
	enum class NodeEnum
	{
		Player,
		FPS,
		Velocity,
		Map,
		Timer,
		Health,
		Hook
	};

	struct Node
	{
		Text Element;
		NodeEnum Type;
		int Hook = 0;
		std::string HookString;
		bool HookFloat = false;
		vec2 OriginalPosition;

		SERIALIZE(Node, Element, Type, Hook, HookString, HookFloat)
	};

	class KMOV : public Module
	{
	public:
		float JumpPower = 50;
		float AnglesPower = 70;
		float FirePower = 5;

		Node NodeLT;
		Node NodeLB;
		Node NodeRT;
		Node NodeRB;

		KMOV();
		virtual ~KMOV() = default;

		void Initialize() override;
		void Menu() override;
		void MenuNode(Node& node);

		void OnSpawn(EventClientSpawn& event) override;
		void OnRender() override;

	private:
		vec2 CurrentOffset;

		int StartTime = 0;
		float LandingOrigin = 0;
		float JumpOrigin = 0;
		vec3 AnglesDelta;
		float FireDuration = 0;

		bool IsShaking = false;
		bool IsBouncing = false;

		void Compute();
		vec2 Angles();
		float Jump();
		vec2 Fire();

		void RenderNode(Node& node);

		SERIALIZE_POLY(KMOV, Module, JumpPower, AnglesPower, FirePower, NodeLT, NodeLB, NodeRT, NodeRB)
	};
}

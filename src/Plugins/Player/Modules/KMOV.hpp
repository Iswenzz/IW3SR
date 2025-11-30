#pragma once
#include "Base.hpp"

namespace IW3SR::Addons
{
	/// <summary>
	/// Node types.
	/// </summary>
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

	/// <summary>
	/// Node element.
	/// </summary>
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

	/// <summary>
	/// Kinetic movement HUD.
	/// </summary>
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

		/// <summary>
		/// Create the module.
		/// </summary>
		KMOV();
		virtual ~KMOV() = default;

		/// <summary>
		/// Initialize module.
		/// </summary>
		void Initialize() override;

		/// <summary>
		/// Menu drawing.
		/// </summary>
		void Menu() override;

		/// <summary>
		/// Menu node drawing.
		/// </summary>
		/// <param name="node">The node.</param>
		void MenuNode(Node& node);

		/// <summary>
		/// Client spawn.
		/// </summary>
		/// <param name="event">The event.</param>
		void OnSpawn(EventClientSpawn& event) override;

		/// <summary>
		/// Render frame.
		/// </summary>
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

		/// <summary>
		/// Compute values.
		/// </summary>
		void Compute();

		/// <summary>
		/// Angles animation.
		/// </summary>
		/// <returns></returns>
		vec2 Angles();

		/// <summary>
		/// Jump animation.
		/// </summary>
		/// <returns></returns>
		float Jump();

		/// <summary>
		/// Fire shaking animation.
		/// </summary>
		/// <returns></returns>
		vec2 Fire();

		/// <summary>
		/// Compute the node value.
		/// </summary>
		/// <param name="node">The node.</param>
		void RenderNode(Node& node);

		SERIALIZE_POLY(KMOV, Module, JumpPower, AnglesPower, FirePower, NodeLT, NodeLB, NodeRT, NodeRB)
	};
}

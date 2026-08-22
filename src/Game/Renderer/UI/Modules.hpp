#pragma once
#include "Game/Base.hpp"

#include "Game/Renderer/Modules/Modules.hpp"

namespace IW3SR::UC
{
	class Modules : public Frame
	{
	public:
		Modules();
		virtual ~Modules() = default;

		void Initialize() override;
		void OnRender() override;

	private:
		std::string Selected;
		std::string Filter;
		bool ShowSettings = false;

		void Reset();
		void Sidebar(float width);
		void Category(const std::string& group);
		void Entry(const Ref<Module>& entry);
		void Details();
		void Header(const std::string& title, const std::string& subtitle);
		void Settings();
		void Placeholder();

		bool Matches(const Ref<Module>& entry) const;
	};
}

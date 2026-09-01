#pragma once
#include "Game/Base.hpp"

#include "Game/Renderer/Modules/Modules.hpp"

namespace IW3SR::UC
{
	// What the panel on the right shows. Modules follows the sidebar selection, the rest are the
	// pages reached from the footer.
	enum class Page
	{
		Modules,
		Settings,
		Theme,
		About
	};

	class Main : public Frame
	{
	public:
		Main();
		virtual ~Main() = default;

		void Initialize() override;
		void OnRender() override;

	private:
		Page Current = Page::Modules;
		std::string Selected;
		std::string Filter;
		bool IsReloading = false;

		void Reset();
		void Sidebar(float width);
		void Category(const std::string& group);
		void Entry(const Ref<Module>& entry);
		void Link(Page page, const std::string& label, bool highlight = false);
		void Details();
		void Header(const std::string& title, const std::string& subtitle);
		void Settings();
		void Theme();
		void Placeholder();

		void Reload();
		void Compile();

		bool Matches(const Ref<Module>& entry) const;
	};
}

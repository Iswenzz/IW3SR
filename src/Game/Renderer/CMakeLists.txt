add_subdirectory(Backends)
add_subdirectory(Drawing)
add_subdirectory(Settings)
add_subdirectory(Modules)
add_subdirectory(UI)

target_sources(IW3SR PRIVATE
	GUI.hpp
	GUI.cpp
	Renderer.hpp
	Renderer.cpp)

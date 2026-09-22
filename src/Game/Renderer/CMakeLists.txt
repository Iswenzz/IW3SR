add_subdirectory(Drawing)
add_subdirectory(Modules)
add_subdirectory(Portal)
add_subdirectory(UI)

target_sources(IW3SR PRIVATE
	Materials.hpp
	Materials.cpp
	Renderer.hpp
	Renderer.cpp)

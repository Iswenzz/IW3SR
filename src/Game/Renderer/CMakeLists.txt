add_subdirectory(Drawing)
add_subdirectory(Modules)
add_subdirectory(Portal)
add_subdirectory(SSAO)
add_subdirectory(UI)
add_subdirectory(VR)
add_subdirectory(Water)

target_sources(IW3SR PRIVATE
	Ghost.hpp
	Ghost.cpp
	Lighting.hpp
	Lighting.cpp
	Materials.hpp
	Materials.cpp
	Renderer.hpp
	Renderer.cpp
	Shadows.hpp
	Shadows.cpp)

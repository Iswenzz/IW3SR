add_subdirectory(Drawing)
add_subdirectory(Modules)
add_subdirectory(UI)

target_sources(IW3SR PRIVATE
	Renderer.hpp
	Renderer.cpp)

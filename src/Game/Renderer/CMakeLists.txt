add_subdirectory(Drawing)
add_subdirectory(Settings)
add_subdirectory(Modules)

target_sources(IW3SR PRIVATE
	Renderer.hpp
	Renderer.cpp)

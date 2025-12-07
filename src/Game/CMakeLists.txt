add_library(IW3SR SHARED)
add_library(IW3SR.API INTERFACE)

add_subdirectory(Definitions)
add_subdirectory(Math)
add_subdirectory(Player)
add_subdirectory(Renderer)
add_subdirectory(System)

target_precompile_headers(IW3SR PRIVATE
	Base.hpp
	Plugin.hpp)

target_sources(IW3SR PRIVATE
	main.cpp
	Application.cpp
	Base.hpp
	Plugin.hpp)

# Build
target_include_directories(IW3SR PUBLIC ..)
target_link_libraries(IW3SR PUBLIC IzEngine CEF::CEF)

set_target_properties(IW3SR PROPERTIES
	OUTPUT_NAME ddraw
	LINK_FLAGS "/DEF:${CMAKE_CURRENT_SOURCE_DIR}/proxy.def")

# API
target_link_libraries(IW3SR.API INTERFACE IW3SR)
target_compile_definitions(IW3SR.API INTERFACE API=IMPORT)

# Tree
get_target_property(SOURCES IW3SR SOURCES)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCES})

# Debugger
if(MSVC)
	set_target_properties(IW3SR PROPERTIES
		VS_DEBUGGER_COMMAND $ENV{COD4}\\iw3mp.exe
		VS_DEBUGGER_WORKING_DIRECTORY $ENV{COD4})
endif()

# Install
install(TARGETS IW3SR RUNTIME DESTINATION ${COD4})

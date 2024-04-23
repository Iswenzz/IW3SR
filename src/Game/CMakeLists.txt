add_library(IW3SR SHARED)
add_library(IW3SR.API INTERFACE)

add_subdirectory(Client)
add_subdirectory(Definitions)
add_subdirectory(GUI)
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

# Tests
if(BUILD_TESTING)
	add_executable(IW3SR.Tests)
	target_link_libraries(IW3SR.Tests PRIVATE IzEngine.Tests IW3SR)
	add_test(NAME Game COMMAND IW3SR.Tests)
endif()

# Build
target_include_directories(IW3SR PUBLIC ..)
target_link_libraries(IW3SR PUBLIC IzEngine)
set_target_properties(IW3SR PROPERTIES OUTPUT_NAME IW3SR SUFFIX ".asi")

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
install(TARGETS IW3SR RUNTIME DESTINATION ${COD4}/miles)
install(DIRECTORY ${CMAKE_SOURCE_DIR}/resources DESTINATION ${COD4}/iw3sr)

install(DIRECTORY
	${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/$<$<CONFIG:Debug>:debug/>bin/
	DESTINATION ${COD4} FILES_MATCHING PATTERN "*.dll")

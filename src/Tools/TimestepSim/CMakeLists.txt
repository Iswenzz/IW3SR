# Runs the game's movement offline, so a change to the timestep can be proven against a vanilla
# client without the game. Also builds on its own, away from the game's toolchain:
#   cmake -S src/Tools/TimestepSim -B <dir> && cmake --build <dir>
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
	cmake_minimum_required(VERSION 3.25)
	project(TimestepSim LANGUAGES CXX)

	set(CMAKE_CXX_STANDARD 23)
	set(CMAKE_CXX_STANDARD_REQUIRED ON)
	set_property(GLOBAL PROPERTY USE_FOLDERS ON)
endif()

set(IW3SR_SRC ${CMAKE_CURRENT_SOURCE_DIR}/../..)

# The movement and scheduling under test are compiled in from src/ rather than copied, so what is
# measured here cannot drift from what ships.
set(SHIPPING
	${IW3SR_SRC}/Game/System/Schedule.cpp
	${IW3SR_SRC}/Game/Player/Movements/CoD4.cpp
	${IW3SR_SRC}/Game/Player/Movements/Q3.cpp
	${IW3SR_SRC}/Game/Player/Movements/CS.cpp)

add_executable(TimestepSim
	main.cpp

	Engine/Client.cpp
	Engine/Com.cpp
	Engine/Dvar.cpp
	Engine/Pmove.cpp
	Engine/Trace.cpp

	Sim/Hand.cpp
	Sim/Run.cpp
	Sim/Timestep.cpp

	${SHIPPING})

# Shim first: it answers the movement sources' include of Game/Base.hpp with the simulator's
# types instead of the whole game.
target_include_directories(TimestepSim PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/Shim
	${CMAKE_CURRENT_SOURCE_DIR}
	${IW3SR_SRC})

# glm is header only here, so the tool needs no vcpkg toolchain of its own. Inside the game's build
# the imported target is already there and carries the include directory with it.
if(TARGET glm::glm)
	target_link_libraries(TimestepSim PRIVATE glm::glm)
else()
	set(GLM_INCLUDE_DIR "${IW3SR_SRC}/../build/vcpkg_installed/x86-windows/include"
		CACHE PATH "Directory containing glm/glm.hpp")

	if(NOT EXISTS "${GLM_INCLUDE_DIR}/glm/glm.hpp")
		message(FATAL_ERROR "glm not found under ${GLM_INCLUDE_DIR}; set -DGLM_INCLUDE_DIR=<path>")
	endif()
	target_include_directories(TimestepSim PRIVATE ${GLM_INCLUDE_DIR})
endif()

if(MSVC)
	target_compile_options(TimestepSim PRIVATE /fp:precise)
else()
	target_compile_options(TimestepSim PRIVATE -Wall -ffp-contract=off)
endif()

# Tree
set_target_properties(TimestepSim PROPERTIES FOLDER Tools)
get_target_property(SOURCES TimestepSim SOURCES)
list(REMOVE_ITEM SOURCES ${SHIPPING})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCES})
source_group("Under Test" FILES ${SHIPPING})

# The simulator is the integration half of the timestep's tests, so it runs under ctest beside
# the unit tests rather than only by hand.
if(BUILD_TESTING)
	add_test(NAME TimestepSim COMMAND TimestepSim --self-test)
endif()

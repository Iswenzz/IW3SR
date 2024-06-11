cmake_minimum_required(VERSION 3.27)
project(IW3SR VERSION 0.1.0 LANGUAGES CXX)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Compilers
if(MSVC)
	set(CMAKE_VS_INCLUDE_INSTALL_TO_DEFAULT_BUILD true)
endif()

# Tests
if(BUILD_TESTING)
	add_executable(IW3SR.Tests)
	add_test(NAME IW3SR COMMAND IW3SR.Tests)
	target_link_libraries(IW3SR.Tests PRIVATE IzEngine.Tests IW3SR)
endif()

# Path
if(NOT DEFINED ENV{COD4})
	message(FATAL_ERROR "ERROR: COD4 environment variable is required to locate the game directory.")
endif()
file(TO_CMAKE_PATH $ENV{COD4} COD4)

# Application
set(APPLICATION_ID "iw3sr")
set(APPLICATION_VERSION ${CMAKE_PROJECT_VERSION})

# Build
add_subdirectory(engine)
add_subdirectory(src)

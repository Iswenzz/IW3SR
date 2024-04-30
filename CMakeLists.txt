cmake_minimum_required(VERSION 3.27)
project(IW3SR VERSION 1.0.0 LANGUAGES CXX)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Compilers
if(MSVC)
	set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
endif()

# Path
if(NOT DEFINED ENV{COD4})
	message(FATAL_ERROR "ERROR: COD4 environment variable is required to locate the game directory.")
endif()
file(TO_CMAKE_PATH $ENV{COD4} COD4)

# Application
set(APPLICATION_ID "iw3sr")

# Build
add_subdirectory(engine)
add_subdirectory(src)

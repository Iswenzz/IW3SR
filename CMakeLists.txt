cmake_minimum_required(VERSION 3.27)
project(IW3SR VERSION 1.0.0 LANGUAGES CXX)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Compilers
if(MSVC)
	set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
	set(CMAKE_VS_INCLUDE_INSTALL_TO_DEFAULT_BUILD true)
endif()

# Application
set(APPLICATION_ID "iw3sr")

# Path
if(NOT DEFINED ENV{COD4})
	message(FATAL_ERROR "ERROR: COD4 environment variable is not defined. Please set it to the root directory of your CoD4 installation.")
endif()
file(TO_CMAKE_PATH $ENV{COD4} COD4)

# Build
add_subdirectory(engine)
add_subdirectory(src)

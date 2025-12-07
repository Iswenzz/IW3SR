cmake_minimum_required(VERSION 3.27)
project(IW3SR VERSION 0.1.1 LANGUAGES CXX)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# CMake
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

# Compilers
if(MSVC)
	set(CMAKE_VS_INCLUDE_INSTALL_TO_DEFAULT_BUILD true)
	# add_compile_options(/fsanitize=address)
endif()

# Dependencies
find_package(CEF REQUIRED)

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

# Install
install(DIRECTORY ${CMAKE_SOURCE_DIR}/resources DESTINATION ${COD4}/iw3sr)
install(FILES ${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/share/cef/Resources/locales/en-US.pak
    DESTINATION ${COD4}/iw3sr/resources/cef/locales)
install(DIRECTORY ${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/share/cef/Resources/
    DESTINATION ${COD4} PATTERN "locales" EXCLUDE)
install(DIRECTORY
	${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/$<$<CONFIG:Debug>:debug/>bin/
	DESTINATION ${COD4} FILES_MATCHING PATTERN "*.dll")
install(DIRECTORY
	${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/$<$<CONFIG:Debug>:debug/>bin/
	DESTINATION ${COD4} FILES_MATCHING PATTERN "*.bin")
install(DIRECTORY
	${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/$<$<CONFIG:Debug>:debug/>bin/
	DESTINATION ${COD4} FILES_MATCHING PATTERN "*.json")

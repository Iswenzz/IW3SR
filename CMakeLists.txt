cmake_minimum_required(VERSION 4.2)
project(IW3SR VERSION 1.8.0 LANGUAGES CXX)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Compilers
if(MSVC)
	set(CMAKE_VS_INCLUDE_INSTALL_TO_DEFAULT_BUILD true)
	# add_compile_options(/fsanitize=address)
endif()

# CMake
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

# Dependencies
find_package(CURL REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(implot CONFIG REQUIRED)
find_package(libzippp CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(Opus CONFIG REQUIRED)
find_package(PolyHook_2 CONFIG REQUIRED)
find_package(SampleRate CONFIG REQUIRED)
find_package(Speex REQUIRED)

# Path
if(NOT DEFINED ENV{COD4})
	message(FATAL_ERROR "ERROR: COD4 environment variable is required to locate the game directory.")
endif()
file(TO_CMAKE_PATH $ENV{COD4} COD4)

# Application
set(APPLICATION_ID "IW3SR")
set(APPLICATION_VERSION ${CMAKE_PROJECT_VERSION})
set(SENTRY_DSN "https://549fb8644c528d116b01e65a61214004@o4512017499815936.ingest.de.sentry.io/4512017504206928")

# Build
add_subdirectory(engine)
add_subdirectory(src)

# Install
install(TARGETS CEF CrashReporter RUNTIME
	DESTINATION ${COD4}/iw3sr/Bin)

install(DIRECTORY ${CMAKE_SOURCE_DIR}/resources/
	DESTINATION ${COD4}/iw3sr/Resources)

install(FILES ${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/share/cef/Resources/locales/en-US.pak
	DESTINATION ${COD4}/iw3sr/Bin)

install(DIRECTORY ${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/share/cef/Resources/
    DESTINATION ${COD4}/iw3sr/Bin PATTERN "locales" EXCLUDE)

install(DIRECTORY
	${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/$<$<CONFIG:Debug>:debug/>bin/
	DESTINATION ${COD4}/iw3sr/Bin FILES_MATCHING PATTERN "*.dll")

install(DIRECTORY
	${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/$<$<CONFIG:Debug>:debug/>bin/
	DESTINATION ${COD4}/iw3sr/Bin FILES_MATCHING PATTERN "*.bin")

install(DIRECTORY
	${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/$<$<CONFIG:Debug>:debug/>bin/
	DESTINATION ${COD4}/iw3sr/Bin FILES_MATCHING PATTERN "*.json")

install(FILES
	${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/tools/sentry-native/crashpad_handler.exe
	${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/bin/zlib1.dll
	DESTINATION ${COD4}/iw3sr/Bin)

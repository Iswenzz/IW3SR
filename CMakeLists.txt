cmake_minimum_required(VERSION 4.2)
project(IW3SR VERSION 1.10.0 LANGUAGES CXX)
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
find_package(OpenXR CONFIG REQUIRED)
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

# Resources/*.zip, packed from the engine's resources/ and ours.
iz_resource_archives(TARGET IW3SR SOURCE ${CMAKE_SOURCE_DIR}/resources
	DESTINATION ${COD4}/iw3sr/Resources
	NAMES fonts shaders textures)

# Install
install(TARGETS CEF CrashReporter RUNTIME
	DESTINATION ${COD4}/iw3sr/Bin)

# The iw3sr zone GZones mounts with common_mp, and iw3sr.iwd, the images its material draws with, which GZones
# opens itself. resources/zone/build.ps1 rebuilds the zone from resources/zone/raw; the IWD is packed here.
set(ZONE_IMAGES ${CMAKE_SOURCE_DIR}/resources/zone/raw/images)
set(ZONE_IWD ${CMAKE_BINARY_DIR}/resources/iw3sr.iwd)
file(GLOB ZONE_IMAGE_FILES CONFIGURE_DEPENDS ${ZONE_IMAGES}/*)
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/resources/iw3sr)
add_custom_command(
	OUTPUT ${ZONE_IWD}
	COMMAND ${CMAKE_COMMAND} -DDIRS=${ZONE_IMAGES} -DTOP=images -DOUTPUT=${ZONE_IWD} -P ${IZ_PACK_RESOURCES}
	WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/resources/iw3sr
	DEPENDS ${ZONE_IMAGE_FILES} ${IZ_PACK_RESOURCES}
	COMMENT "Packing iw3sr.iwd"
	VERBATIM)
add_custom_target(IW3SR.zone DEPENDS ${ZONE_IWD} SOURCES ${ZONE_IMAGE_FILES})
set_target_properties(IW3SR.zone PROPERTIES FOLDER Resources)
add_dependencies(IW3SR IW3SR.zone)
install(FILES ${CMAKE_SOURCE_DIR}/resources/zone/iw3sr.ff ${ZONE_IWD}
	DESTINATION ${COD4}/iw3sr/Zone)


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

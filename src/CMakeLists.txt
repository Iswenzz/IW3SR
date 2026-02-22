add_subdirectory(Game)
add_subdirectory(Loader)
add_subdirectory(Plugins)

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/Private")
    add_subdirectory(Private)
endif()

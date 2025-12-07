add_subdirectory(Game)
add_subdirectory(Plugins)
add_subdirectory(Tools)

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/Private")
    add_subdirectory(Private)
endif()

add_library(Player SHARED main.cpp)

add_subdirectory(Modules)
add_subdirectory(Movements)

# Build
target_link_libraries(Player PRIVATE IW3SR.API)
target_include_directories(Player PRIVATE .)

# Tree
set_target_properties(Player PROPERTIES FOLDER Plugins)
get_target_property(SOURCES Player SOURCES)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCES})

# Install
install(TARGETS Player RUNTIME DESTINATION ${COD4}/iw3sr/plugins)

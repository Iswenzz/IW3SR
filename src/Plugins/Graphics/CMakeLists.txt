add_library(Graphics SHARED main.cpp)
add_subdirectory(Settings)

# Build
target_link_libraries(Graphics PRIVATE IW3SR.API)
target_include_directories(Graphics PRIVATE .)

# Tree
set_target_properties(Graphics PROPERTIES FOLDER Plugins)
get_target_property(SOURCES Graphics SOURCES)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCES})

# Install
install(TARGETS Graphics RUNTIME DESTINATION ${COD4}/iw3sr/plugins)

add_executable(Updater main.cpp)

# Tree
get_target_property(SOURCES Updater SOURCES)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCES})

# Install
install(TARGETS Updater RUNTIME DESTINATION ${COD4}/iw3sr/Bin)

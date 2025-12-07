add_executable(CEF WIN32 main.cpp)

# Build
target_link_libraries(CEF PRIVATE CEF::CEF)

# Tree
set_target_properties(CEF PROPERTIES FOLDER Tools)
get_target_property(SOURCES CEF SOURCES)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCES})

# Install
install(TARGETS CEF RUNTIME DESTINATION ${COD4})

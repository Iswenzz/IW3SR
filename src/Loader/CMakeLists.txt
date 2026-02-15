add_library(Loader SHARED main.cpp)

set_target_properties(Loader PROPERTIES
	OUTPUT_NAME ddraw
	LINK_FLAGS "/DEF:${CMAKE_CURRENT_SOURCE_DIR}/proxy.def")

# Tree
get_target_property(SOURCES Loader SOURCES)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCES})

# Install
install(TARGETS Loader RUNTIME DESTINATION ${COD4})

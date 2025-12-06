find_path(CEF_INCLUDE_DIR cef_api_hash.h)
get_filename_component(CEF_ROOT_DIR "${CEF_INCLUDE_DIR}/.." ABSOLUTE)

set(CEF_LIBS
	${CEF_ROOT_DIR}/$<$<CONFIG:Debug>:debug/>lib/libcef.lib
	${CEF_ROOT_DIR}/$<$<CONFIG:Debug>:debug/>lib/libcef_dll_wrapper.lib)

set(CEF_INCLUDE_DIRS
	${CEF_ROOT_DIR}
	${CEF_INCLUDE_DIR})

message(${CEF_LIBS})
message(${CEF_INCLUDE_DIRS})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(CEF DEFAULT_MSG CEF_LIBS CEF_INCLUDE_DIRS)
mark_as_advanced(CEF CEF_LIBS CEF_INCLUDE_DIRS)

if(NOT TARGET CEF::CEF)
	add_library(CEF::CEF INTERFACE IMPORTED)
	set_property(TARGET CEF::CEF PROPERTY INTERFACE_LINK_LIBRARIES "${CEF_LIBS}")
	set_property(TARGET CEF::CEF PROPERTY INTERFACE_INCLUDE_DIRECTORIES "${CEF_INCLUDE_DIRS}")
endif()

add_library(fltk STATIC IMPORTED GLOBAL)
if(WIN32)
    set_property(TARGET fltk PROPERTY IMPORTED_LOCATION_DEBUG ${WORKSPACE_ROOT}/extern/mswin/fltk/debug/fltkd.lib)
    set_property(TARGET fltk PROPERTY IMPORTED_LOCATION_RELEASE ${WORKSPACE_ROOT}/extern/mswin/fltk/release/fltk.lib)
elseif(APPLE)
	set_property(TARGET fltk PROPERTY IMPORTED_LOCATION_DEBUG ${WORKSPACE_ROOT}/extern/macos/fltk/debug/libfltk.a)
    set_property(TARGET fltk PROPERTY IMPORTED_LOCATION_RELEASE ${WORKSPACE_ROOT}/extern/mcaos/fltk/release/libfltk.a)
endif()
#target_include_directories(fltk INTERFACE inc/)
set_property(TARGET fltk APPEND PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${WORKSPACE_ROOT}/extern/common/fltk/inc/)

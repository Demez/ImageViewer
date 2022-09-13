message( "Current Project: ImGui" )

set(
	IMGUI_FILES
	imconfig.h
	imgui.cpp
	imgui.h
	imgui_demo.cpp
	imgui_draw.cpp
	imgui_impl_sdlrenderer.cpp
	imgui_impl_sdlrenderer.h
	imgui_impl_win32.cpp
	imgui_impl_win32.h
	imgui_internal.h
	imgui_tables.cpp
	imgui_widgets.cpp
	imstb_rectpack.h
	imstb_textedit.h
	imstb_truetype.h
)

include( "../core.cmake" )
include( "../sdl2.cmake" )

add_library(
	ImGui SHARED
	${IMGUI_FILES} )
	
target_link_libraries( ImGui SDL2 )

set_target_properties(
	ImGui PROPERTIES
	LINKER_LANGUAGE CXX
	OUTPUT_NAME imgui
)

# set output directories for all builds (Debug, Release, etc.)
foreach( OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES} )
    string( TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG )

    set_target_properties(
    	ImGui PROPERTIES
    	RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${IMGVIEW_BUILD}/${OUTPUTCONFIG}
    	LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${IMGVIEW_BUILD}/${OUTPUTCONFIG}
    )
endforeach( OUTPUTCONFIG CMAKE_CONFIGURATION_TYPES )


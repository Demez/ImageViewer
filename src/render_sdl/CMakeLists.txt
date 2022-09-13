cmake_minimum_required( VERSION 3.18 FATAL_ERROR )

set( SRC_FILES
  render_sdl.cpp
)

include( ../../core.cmake )

add_compile_definitions( "RENDER_DLL" )

include_directories(
	".."
	"../../thirdparty/SDL2/include"
)

link_directories( "../../thirdparty/SDL2/lib/x64" )

add_library( RenderSDL SHARED ${SRC_FILES} )

target_link_libraries(
	RenderSDL
	SDL2
	ImGui
)

# set output directories for all builds (Debug, Release, etc.)
foreach( OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES} )
    string( TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG )

    set_target_properties(
    	RenderSDL PROPERTIES
    	RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${IMGVIEW_BUILD}/${OUTPUTCONFIG}
    	LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${IMGVIEW_BUILD}/${OUTPUTCONFIG}
    )
endforeach( OUTPUTCONFIG CMAKE_CONFIGURATION_TYPES )


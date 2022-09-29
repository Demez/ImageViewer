message( "Current Project: ImGui" )

set(
	IMGUI_FILES
	imconfig.h
	imgui.cpp
	imgui.h
	imgui_demo.cpp
	imgui_draw.cpp
	imgui_internal.h
	imgui_tables.cpp
	imgui_widgets.cpp
	
	imstb_rectpack.h
	imstb_textedit.h
	imstb_truetype.h
	
	misc/freetype/imgui_freetype.cpp
	misc/freetype/imgui_freetype.h
)

if( MSVC )
	set( IMGUI_PLAT_FILES
		imgui_impl_win32.cpp
		imgui_impl_win32.h
	)
else()
	set( IMGUI_PLAT_FILES
	)
endif()

include( ../core.cmake )

add_library(
	ImGui SHARED
	${IMGUI_FILES}
	${IMGUI_PLAT_FILES}
)

if ( MSVC )
	# windows freetype library
	# https://github.com/ubawurinna/freetype-windows-binaries/blob/master/release%20static/vs2015-2022/win64/freetype.lib
	
	set( FREETYPE_PATH "../thirdparty/freetype-windows-binaries" )
	# set( FREETYPE_PATH "../thirdparty/freetype" )
	set( FREETYPE_INC_DIR "${FREETYPE_PATH}/include" )
	set( FREETYPE_LIB_DIR "${FREETYPE_PATH}/release static/vs2015-2022/win64/" )
else()
	message( FATAL_ERROR "NO FREETYPE LIBRARY SETUP IN CMAKE ON THIS PLATFORM!!!" )
endif()

set_target_properties(
	ImGui PROPERTIES
	LINKER_LANGUAGE CXX
	OUTPUT_NAME imgui
)

target_link_directories(
	ImGui PRIVATE
	"${FREETYPE_LIB_DIR}"
)

target_link_libraries(
	ImGui PRIVATE
	freetype
)

target_include_directories(
	ImGui PRIVATE
	"${FREETYPE_INC_DIR}"
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



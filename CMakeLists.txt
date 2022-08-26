cmake_minimum_required( VERSION 3.18 FATAL_ERROR )

project( ImageViewer )

add_subdirectory( imgui )

set( SRC_FILES
  src/main.cpp
  src/folder_info.cpp
  src/folder_info.h
  src/util.cpp
  src/util.h

  # SDL2 Renderer abstraction
  src/render_sdl/render_sdl.cpp
  src/render_sdl/render_sdl.h
  
  # UI
  src/ui/imagelist.cpp
  src/ui/imagelist.h
  src/ui/imageview.cpp
  src/ui/imageview.h

  src/formats/imageloader.cpp
  src/formats/imageloader.h
  
  # Image Formats
  # src/formats/format_win32_wic.cpp
  src/formats/format_png.cpp
  src/formats/format_jpeg.cpp
)

if( MSVC )
	set( PLAT_FILES
	)
else()
	set( PLAT_FILES
	)
endif()

include( core.cmake )

include_directories(
	"thirdparty/libspng/spng"
	"thirdparty/libjpeg-turbo"
)

link_directories(
	"thirdparty/libjpeg-turbo/build"
)

add_executable( ImageViewer ${SRC_FILES} ${PLAT_FILES} )

target_link_libraries(
	ImageViewer
	SDL2
	ImGui
	turbojpeg-static
)

if( MSVC )
	target_link_libraries( ImageViewer Shlwapi )
endif()

if ( ${CMAKE_BUILD_TYPE} NOT STREQUAL '' )
	add_custom_command( TARGET ImageViewer POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_if_different ${SPNG_LIB_DIR}/spng.dll ${IMGVIEW_BUILD}/spng.dll
	)
endif()

# set output directories for all builds (Debug, Release, etc.)
foreach( OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES} )
    string( TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG )

    set_target_properties(
    	ImageViewer PROPERTIES
    	RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${IMGVIEW_BUILD}/${OUTPUTCONFIG}
    	LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${IMGVIEW_BUILD}/${OUTPUTCONFIG}
    )

	if( MSVC )
		# uhhh
		if ( ${OUTPUTCONFIG} STREQUAL DEBUG )
			add_custom_command( TARGET ImageViewer POST_BUILD
				COMMAND ${CMAKE_COMMAND} -E copy_if_different ${SPNG_LIB_DIR}/Debug/spng.dll ${IMGVIEW_BUILD}/${OUTPUTCONFIG}/spng.dll
			)
		else()
			add_custom_command( TARGET ImageViewer POST_BUILD
				COMMAND ${CMAKE_COMMAND} -E copy_if_different ${SPNG_LIB_DIR}/Release/spng.dll ${IMGVIEW_BUILD}/${OUTPUTCONFIG}/spng.dll
			)
		endif()

	endif()
endforeach( OUTPUTCONFIG CMAKE_CONFIGURATION_TYPES )


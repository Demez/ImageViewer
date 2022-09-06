cmake_minimum_required( VERSION 3.18 FATAL_ERROR )

project( ImageViewer )

add_subdirectory( imgui )

set( SRC_FILES
  src/main.cpp
  src/folder_info.cpp
  src/folder_info.h
  src/util.cpp
  src/util.h
  src/args.cpp
  src/args.h
  src/platform.h
  src/render.h

  # SDL2 Renderer abstraction
  src/render_sdl/render_sdl.cpp
  
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
  src/formats/format_jpegxl.cpp
)

if( MSVC )
	set( PLAT_FILES
		src/win/drag_drop.cpp
		src/win/drag_drop.h
		src/win/platform_win32.cpp
	)
else()
	set( PLAT_FILES
	)
endif()

include( core.cmake )

set( JXL_DIR "${CMAKE_CURRENT_LIST_DIR}/thirdparty/libjxl" )

include_directories(
	"thirdparty/libspng/spng"
	"thirdparty/libjpeg-turbo"
	"${JXL_DIR}/lib/include"
	"${JXL_DIR}/build/lib/include"
)

link_directories(
	"thirdparty/libjpeg-turbo/build"
	"${JXL_DIR}/build/lib"
)

add_executable( ImageViewer ${SRC_FILES} ${PLAT_FILES} )

target_link_libraries(
	ImageViewer
	SDL2
	spng
	ImGui
	turbojpeg-static

	# why don't these work????????
	# jxl_dec-static
	# jxl_threads-static

	jxl_dec
	jxl_threads
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
			add_custom_command( TARGET ImageViewer POST_BUILD
				COMMAND ${CMAKE_COMMAND} -E copy_if_different ${JXL_DIR}/build/Debug/jxl_threads.dll ${IMGVIEW_BUILD}/${OUTPUTCONFIG}/jxl_threads.dll
			)
			add_custom_command( TARGET ImageViewer POST_BUILD
				COMMAND ${CMAKE_COMMAND} -E copy_if_different ${JXL_DIR}/build/Debug/jxl_dec.dll ${IMGVIEW_BUILD}/${OUTPUTCONFIG}/jxl_dec.dll
			)
		else()
			add_custom_command( TARGET ImageViewer POST_BUILD
				COMMAND ${CMAKE_COMMAND} -E copy_if_different ${SPNG_LIB_DIR}/Release/spng.dll ${IMGVIEW_BUILD}/${OUTPUTCONFIG}/spng.dll
			)
			add_custom_command( TARGET ImageViewer POST_BUILD
				COMMAND ${CMAKE_COMMAND} -E copy_if_different ${JXL_DIR}/build/Release/jxl_threads.dll ${IMGVIEW_BUILD}/${OUTPUTCONFIG}/jxl_threads.dll
			)
			add_custom_command( TARGET ImageViewer POST_BUILD
				COMMAND ${CMAKE_COMMAND} -E copy_if_different ${JXL_DIR}/build/Release/jxl_dec.dll ${IMGVIEW_BUILD}/${OUTPUTCONFIG}/jxl_dec.dll
			)
		endif()

	endif()
endforeach( OUTPUTCONFIG CMAKE_CONFIGURATION_TYPES )


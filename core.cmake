# grab the framework global vars
# set( CMAKE_POSITION_INDEPENDENT_CODE ON )

set( CMAKE_CXX_STANDARD 20 )  # could do 23

set( IMGVIEW_BUILD     "${CMAKE_CURRENT_LIST_DIR}/imgview/" )
set( IMGVIEW_ROOT       "${CMAKE_CURRENT_LIST_DIR}" )
set( IMGVIEW_THIRDPARTY "${CMAKE_CURRENT_LIST_DIR}/thirdparty/" )

set( CMAKE_RUNTIME_OUTPUT_DIRECTORY         ${IMGVIEW_BUILD} )
set( CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG   ${IMGVIEW_BUILD} )
set( CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${IMGVIEW_BUILD} )

set( CMAKE_LIBRARY_OUTPUT_DIRECTORY         ${IMGVIEW_BUILD} )
set( CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG   ${IMGVIEW_BUILD} )
set( CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE ${IMGVIEW_BUILD} )
	
set( CMAKE_ARCHIVE_OUTPUT_DIRECTORY         ${IMGVIEW_ROOT}/obj )
set( CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG   ${IMGVIEW_ROOT}/obj/Debug )
set( CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${IMGVIEW_ROOT}/obj/Release )
set( CMAKE_SHARED_LIBRARY_PREFIX "" )

set_property( GLOBAL PROPERTY PREFIX "" )

set( IMGUI_DIR "${CMAKE_CURRENT_LIST_DIR}/imgui" )

include_directories(
	"${IMGUI_DIR}"
	"${CMAKE_CURRENT_LIST_DIR}/src"
)

add_compile_definitions( "nuts=once" )

# Compiler/Platform specifc options
if( MSVC )

	add_compile_definitions(
		NOMINMAX
		_CRT_SECURE_NO_WARNINGS
		_ALLOW_RUNTIME_LIBRARY_MISMATCH _ALLOW_ITERATOR_DEBUG_LEVEL_MISMATCH _ALLOW_MSC_VER_MISMATCH
		_AMD64_ __x86_64 __amd64
		UNICODE _UNICODE
	)
	
	# Remove default warning level from CMAKE_CXX_FLAGS
	string ( REGEX REPLACE "/W[0-4]" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}" )

	if( CMAKE_BUILD_TYPE STREQUAL Debug )
		add_compile_options( "/Od" )  # no optimizations
		string ( REGEX REPLACE "/Zi" "/ZI" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}" )  # Use Edit and Continue on Debug
	elseif( CMAKE_BUILD_TYPE STREQUAL Release )
		add_compile_options( "/O2" )  # level 2 optimizations (max in msvc)
	endif()
	
	# Use these runtime libraries for both so it doesn't crash smh my head
	# actually this is useless right now because of core.dll, god dammit
	set_property( GLOBAL PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreadedDLL" )

	set( MSVC_VERSION 1939 )
	set( CMAKE_MFC_FLAG 0 )

	add_compile_options(
		"/W3"               # Warning Level 3
		"/MP"               # multi-threaded compilation
		"/permissive-"      # make MSVC very compliant
		"/Zc:__cplusplus"   # set the __cplusplus macro to be the correct version
		"/fp:fast"          # fast floating point model
		"/GF"               # string pooling: allows compiler to create a single copy of identical strings in the program image and in memory during execution
		
		"/arch:AVX2"
		
		# disable stupid warnings
		"/wd4244"  # conversion from 'X' to 'X', possible loss of data
		"/wd4267"  # conversion from 'X' to 'X', possible loss of data (yes there's 2 idk why)
		"/wd4305"  # truncation from 'double' to 'float'
	)
	
	add_link_options(
		"/INCREMENTAL"
	)
	
else()  # linux

	set( CMAKE_CXX_COMPILER g++ )

	link_libraries( pthread m )
	
	if ( CMAKE_BUILD_TYPE STREQUAL Debug )
		add_compile_options( -g )
	else()
		add_compile_options( -O2 )
	endif()
	
	# From Thermite:
	# Added -Wno-unused-parameter because this warning as an error is %*$*ing dumb
	# Added -Wno-missing-field-initializers because designated initializers initialize to 0 anyway, so screw you
	# Added -Wno-deprecated-enum-enum-conversion because ImGui
	add_compile_options(
		-pedantic
		-Wno-unused-parameter
		-Wno-missing-field-initializers
		-Wno-deprecated-enum-enum-conversion
	)
	
endif()


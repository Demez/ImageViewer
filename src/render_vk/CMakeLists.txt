message( "Current Project: RenderVK" )

include( ../../core.cmake )

set(
	RENDER_VK_FILES
	
	render_vk.cpp
	render_vk.h
	instance.cpp
	descriptor_set.cpp
	swapchain.cpp
	present.cpp
	render_pass.cpp
	texture.cpp
	shader_image.cpp
	
	# imgui files
	${IMGUI_DIR}/imgui_impl_vulkan.cpp
	${IMGUI_DIR}/imgui_impl_vulkan.h
	
	# shared files from main program
	../platform.h
	../log.cpp
	../log.h
	../util.cpp
	../util.h
)

add_compile_definitions( "RENDER_DLL" )

include( ../../vulkan.cmake )

add_library( RenderVK SHARED ${RENDER_VK_FILES} )

add_dependencies( RenderVK "ImGui" )

set_target_properties(
	RenderVK PROPERTIES
	OUTPUT_NAME render_vk
)

target_link_libraries(
	RenderVK
	PRIVATE
	ImGui
	${VK_LIB}
)

if ( VULKAN_DEBUG )
	target_compile_definitions( RenderVK PRIVATE
		-DVK_DEBUG=1
	)
else()
	target_compile_definitions( RenderVK PRIVATE
		-DVK_DEBUG=0
	)
endif()

# set output directories for all builds (Debug, Release, etc.)
foreach( OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES} )
    string( TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG )

    set_target_properties(
    	RenderVK PROPERTIES
    	RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${IMGVIEW_BUILD}/${OUTPUTCONFIG}
    	LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${IMGVIEW_BUILD}/${OUTPUTCONFIG}
    )
endforeach( OUTPUTCONFIG CMAKE_CONFIGURATION_TYPES )


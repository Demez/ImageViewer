#include "../render.h"
#include "platform.h"
#include "util.h"
#include "log.h"

#include "render_vk.h"

#include "imgui_impl_win32.h"
#include "imgui_impl_vulkan.h"

#include <unordered_map>


int                  gWidth  = 1280;
int                  gHeight = 720;

float                gClearR = 0.f;
float                gClearG = 0.f;
float                gClearB = 0.f;

static VkCommandPool gSingleTime;
static VkCommandPool gPrimary;


char const* VKString( VkResult sResult )
{
	switch ( sResult )
	{
		default:
			return "Unknown";
		case VK_SUCCESS:
			return "VK_SUCCESS";
		case VK_ERROR_OUT_OF_HOST_MEMORY:
			return "VK_ERROR_OUT_OF_HOST_MEMORY";
		case VK_ERROR_OUT_OF_DEVICE_MEMORY:
			return "VK_ERROR_OUT_OF_DEVICE_MEMORY";
		case VK_ERROR_INITIALIZATION_FAILED:
			return "VK_ERROR_INITIALIZATION_FAILED";
		case VK_ERROR_DEVICE_LOST:
			return "VK_ERROR_DEVICE_LOST";
		case VK_ERROR_MEMORY_MAP_FAILED:
			return "VK_ERROR_MEMORY_MAP_FAILED";
		case VK_ERROR_LAYER_NOT_PRESENT:
			return "VK_ERROR_LAYER_NOT_PRESENT";
		case VK_ERROR_EXTENSION_NOT_PRESENT:
			return "VK_ERROR_EXTENSION_NOT_PRESENT";
		case VK_ERROR_FEATURE_NOT_PRESENT:
			return "VK_ERROR_FEATURE_NOT_PRESENT";
		case VK_ERROR_INCOMPATIBLE_DRIVER:
			return "VK_ERROR_INCOMPATIBLE_DRIVER";
		case VK_ERROR_TOO_MANY_OBJECTS:
			return "VK_ERROR_TOO_MANY_OBJECTS";
		case VK_ERROR_FORMAT_NOT_SUPPORTED:
			return "VK_ERROR_FORMAT_NOT_SUPPORTED";
		case VK_ERROR_FRAGMENTED_POOL:
			return "VK_ERROR_FRAGMENTED_POOL";
		case VK_ERROR_UNKNOWN:
			return "VK_ERROR_UNKNOWN";
		case VK_ERROR_OUT_OF_POOL_MEMORY:
			return "VK_ERROR_OUT_OF_POOL_MEMORY";
		case VK_ERROR_INVALID_EXTERNAL_HANDLE:
			return "VK_ERROR_INVALID_EXTERNAL_HANDLE";
		case VK_ERROR_FRAGMENTATION:
			return "VK_ERROR_FRAGMENTATION";
		case VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS:
			return "VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS";
		case VK_ERROR_SURFACE_LOST_KHR:
			return "VK_ERROR_SURFACE_LOST_KHR";
		case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:
			return "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR";
		case VK_SUBOPTIMAL_KHR:
			return "VK_SUBOPTIMAL_KHR";
		case VK_ERROR_OUT_OF_DATE_KHR:
			return "VK_ERROR_OUT_OF_DATE_KHR";
		case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR:
			return "VK_ERROR_INCOMPATIBLE_DISPLAY_KHR";
		case VK_ERROR_VALIDATION_FAILED_EXT:
			return "VK_ERROR_VALIDATION_FAILED_EXT";
		case VK_ERROR_INVALID_SHADER_NV:
			return "VK_ERROR_INVALID_SHADER_NV";
		case VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT:
			return "VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT";
		case VK_ERROR_NOT_PERMITTED_EXT:
			return "VK_ERROR_NOT_PERMITTED_EXT";
		case VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT:
			return "VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT";
		case VK_THREAD_IDLE_KHR:
			return "VK_THREAD_IDLE_KHR";
		case VK_THREAD_DONE_KHR:
			return "VK_THREAD_DONE_KHR";
		case VK_OPERATION_DEFERRED_KHR:
			return "VK_OPERATION_DEFERRED_KHR";
		case VK_OPERATION_NOT_DEFERRED_KHR:
			return "VK_OPERATION_NOT_DEFERRED_KHR";
		case VK_PIPELINE_COMPILE_REQUIRED_EXT:
			return "VK_PIPELINE_COMPILE_REQUIRED_EXT";
	}
}


void VK_CheckResult( VkResult sResult, char const* spMsg )
{
	if ( sResult == VK_SUCCESS )
		return;

	char pBuf[ 1024 ];
	snprintf( pBuf, sizeof( pBuf ), "Vulkan Error %s: %s", spMsg, VKString( sResult ) );

	// SDL_ShowSimpleMessageBox( SDL_MESSAGEBOX_ERROR, "Vulkan Error", pBuf, nullptr );
	LogFatal( pBuf );
}


void VK_CheckResult( VkResult sResult )
{
	if ( sResult == VK_SUCCESS )
		return;

	char pBuf[ 1024 ];
	snprintf( pBuf, sizeof( pBuf ), "Vulkan Error: %s", VKString( sResult ) );

	// SDL_ShowSimpleMessageBox( SDL_MESSAGEBOX_ERROR, "Vulkan Error", pBuf, nullptr );
	LogFatal( pBuf );
}


// Copies memory to the GPU.
void VK_memcpy( VkDeviceMemory sBufferMemory, VkDeviceSize sSize, const void* spData )
{
	void* pData;
	VK_CheckResult( vkMapMemory( VK_GetDevice(), sBufferMemory, 0, sSize, 0, &pData ), "Vulkan: Failed to map memory" );
	memcpy( pData, spData, (size_t)sSize );
	vkUnmapMemory( VK_GetDevice(), sBufferMemory );
}


VkSampleCountFlagBits VK_GetMSAASamples()
{
	return VK_SAMPLE_COUNT_1_BIT;
}


void VK_CreateCommandPool( VkCommandPool& sCmdPool, VkCommandPoolCreateFlags sFlags )
{
	QueueFamilyIndices      q = VK_FindQueueFamilies( VK_GetPhysicalDevice() );

	VkCommandPoolCreateInfo aCommandPoolInfo{ VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
	aCommandPoolInfo.pNext            = nullptr;
	aCommandPoolInfo.flags            = sFlags;
	aCommandPoolInfo.queueFamilyIndex = q.aGraphicsFamily;

	VK_CheckResult( vkCreateCommandPool( VK_GetDevice(), &aCommandPoolInfo, nullptr, &sCmdPool ), "Failed to create command pool!" );
}


void VK_DestroyCommandPool( VkCommandPool& srPool )
{
	vkDestroyCommandPool( VK_GetDevice(), srPool, nullptr );
}


void VK_ResetCommandPool( VkCommandPool& srPool, VkCommandPoolResetFlags sFlags )
{
	VK_CheckResult( vkResetCommandPool( VK_GetDevice(), srPool, sFlags ), "Failed to reset command pool!" );
}


VkCommandPool& VK_GetSingleTimeCommandPool()
{
	return gSingleTime;
}


VkCommandPool& VK_GetPrimaryCommandPool()
{
	return gPrimary;
}


void VK_CreateBuffer( VkBuffer& srBuffer, VkDeviceMemory& srBufferMem, u32 sBufferSize, VkBufferUsageFlags sUsage, int sMemBits )
{
	// create a vertex buffer
	VkBufferCreateInfo aBufferInfo{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
	aBufferInfo.size        = sBufferSize;
	aBufferInfo.usage       = sUsage;
	aBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VK_CheckResult( vkCreateBuffer( VK_GetDevice(), &aBufferInfo, nullptr, &srBuffer ), "Failed to create buffer" );

	// allocate memory for the vertex buffer
	VkMemoryRequirements aMemReqs;
	vkGetBufferMemoryRequirements( VK_GetDevice(), srBuffer, &aMemReqs );

	VkMemoryAllocateInfo aMemAllocInfo{ VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
	aMemAllocInfo.allocationSize  = aMemReqs.size;
	aMemAllocInfo.memoryTypeIndex = VK_GetMemoryType( aMemReqs.memoryTypeBits, sMemBits );

	VK_CheckResult( vkAllocateMemory( VK_GetDevice(), &aMemAllocInfo, nullptr, &srBufferMem ), "Failed to allocate buffer memory" );

	// bind the vertex buffer to the device memory
	VK_CheckResult( vkBindBufferMemory( VK_GetDevice(), srBuffer, srBufferMem, 0 ), "Failed to bind buffer" );
}


void VK_DestroyBuffer( VkBuffer& srBuffer, VkDeviceMemory& srBufferMem )
{
	if ( srBuffer )
		vkDestroyBuffer( VK_GetDevice(), srBuffer, nullptr );

	if ( srBufferMem )
		vkFreeMemory( VK_GetDevice(), srBufferMem, nullptr );
}


bool VK_InitImGui()
{
	ImGui_ImplVulkan_InitInfo init_info{};
	init_info.Instance        = VK_GetInstance();
	init_info.PhysicalDevice  = VK_GetPhysicalDevice();
	init_info.Device          = VK_GetDevice();
	init_info.Queue           = VK_GetGraphicsQueue();
	init_info.DescriptorPool  = VK_GetDescPool();
	init_info.MinImageCount   = VK_GetSwapImageCount();
	init_info.ImageCount      = VK_GetSwapImageCount();
	init_info.MSAASamples     = VK_GetMSAASamples();
	init_info.CheckVkResultFn = VK_CheckResult;

	if ( !ImGui_ImplVulkan_Init( &init_info, VK_GetRenderPass() ) )
		return false;

	VK_SingleCommand( []( VkCommandBuffer c ) { ImGui_ImplVulkan_CreateFontsTexture( c ); } );

	ImGui_ImplVulkan_DestroyFontUploadObjects();

	return true;
}


// ----------------------------------------------------------------------------------
// Render Abstraction


bool Render_Init( void* spWindow )
{
	if ( !VK_CreateInstance() )
	{
		printf( "Render: Failed to create Vulkan Instance\n" );
		return false;
	}

	VK_CreateSurface( spWindow );
	VK_SetupPhysicalDevice();
	VK_CreateDevice();
	VK_CreateDescSets();

	VK_CreateCommandPool( VK_GetSingleTimeCommandPool() );
	VK_CreateCommandPool( VK_GetPrimaryCommandPool() );

	VK_CreateSwapchain();
	VK_CreateFences();
	VK_CreateSemaphores();

	VK_AllocateCommands();
	
	if ( !VK_InitImGui() )
	{
		fputs( "Render: Failed to init ImGui for Vulkan\n", stderr );
		Render_Shutdown();
		return false;
	}

	// Load up image shader and create buffer for image mesh
	VK_CreateImageLayout();
	VK_CreateImageShader();

	printf( "Render: Loaded Vulkan Renderer\n" );

	return true;
}


void Render_Shutdown()
{
	VK_DestroyInstance();
	VK_DestroySurface();
	VK_DestroyDescSets();
	VK_DestroySwapchain();
	VK_FreeCommands();
	VK_DestroyCommandPool( VK_GetSingleTimeCommandPool() );
	VK_DestroyCommandPool( VK_GetPrimaryCommandPool() );
}


void VK_Reset()
{
	VK_DestroySwapchain();

	VK_DestroyRenderTargets();
	VK_DestroyRenderPasses();

	// ----------------------
	// recreate resources

	VK_CreateSwapchain();
	
	// recreate main renderpass and backbuffer
	VK_GetRenderPass();
	VK_GetBackBuffer();
}


void Render_NewFrame()
{
	ImGui_ImplVulkan_NewFrame();
	VK_ClearDrawQueue();
}


void Render_Reset()
{
	VK_Reset();
}


void Render_Present()
{
	VK_RecordCommands();
	VK_Present();
}


void Render_SetResolution( int sWidth, int sHeight )
{
	gWidth  = sWidth;
	gHeight = sHeight;
}


void Render_SetClearColor( int r, int g, int b )
{
	gClearR = r / 255.f;
	gClearG = g / 255.f;
	gClearB = b / 255.f;
}


void Render_GetClearColor( int& r, int& g, int& b )
{
	r = gClearR * 255.f;
	g = gClearG * 255.f;
	b = gClearB * 255.f;
}


bool Render_LoadImage( ImageInfo* spInfo, std::vector< char >& srData )
{
	return VK_CreateTexture( spInfo, srData );
}


void Render_FreeImage( ImageInfo* spInfo )
{;
	VK_DestroyTexture( spInfo );
}


void Render_DrawImage( ImageInfo* spInfo, const ImageDrawInfo& srDrawInfo )
{
	VK_AddImageDrawInfo( spInfo, srDrawInfo );
}


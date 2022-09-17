#include "../render.h"
#include "platform.h"
#include "util.h"
#include "log.h"

#include "render_vk.h"

#include "imgui_impl_win32.h"
#include "imgui_impl_vulkan.h"

#include <unordered_map>


int gWidth  = 120;
int gHeight = 720;

float gClearR = 0.f;
float gClearG = 0.f;
float gClearB = 0.f;


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
		case VK_ERROR_SURFACE_LOST_KHR:
			return "VK_ERROR_SURFACE_LOST_KHR";
		case VK_ERROR_FRAGMENTED_POOL:
			return "VK_ERROR_FRAGMENTED_POOL";
		case VK_ERROR_UNKNOWN:
			return "VK_ERROR_UNKNOWN";
		case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:
			return "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR";
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


VkSampleCountFlagBits VK_GetMSAASamples()
{
	return VK_SAMPLE_COUNT_1_BIT;
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

	// maybe don't use resolve on imgui renderpass???? idk
	if ( !ImGui_ImplVulkan_Init( &init_info, VK_GetVkRenderPass() ) )
	{
		return false;
	}

	VK_SingleCommand( []( VkCommandBuffer c ) { ImGui_ImplVulkan_CreateFontsTexture( c ); } );

	ImGui_ImplVulkan_DestroyFontUploadObjects();

	return true;
}


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
	VK_CreateDrawThreads();
	VK_AllocateCommands();
	
	if ( !VK_InitImGui() )
	{
		fputs( "Render: Failed to init ImGui for Vulkan\n", stderr );
		Render_Shutdown();
		return false;
	}

	return true;
}


void Render_Shutdown()
{
	VK_DestroyInstance();
	VK_DestroySurface();
	VK_DestroyDescSets();
	VK_DestroyCommandPool( VK_GetSingleTimeCommandPool() );
	VK_DestroyCommandPool( VK_GetPrimaryCommandPool() );
	VK_DestroySwapchain();
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
#if 0
	auto find = gImageTextures.find( spInfo );
	
	// Image is already loaded
	if ( find != gImageTextures.end() )
		return true;

	int                 pitch  = spInfo->aWidth;
	SDL_PixelFormatEnum sdlFmt = SDL_PIXELFORMAT_ABGR8888;

	if ( spInfo->aFormat == FMT_RGB8 )
	{
		sdlFmt = SDL_PIXELFORMAT_RGB24;
		pitch *= 3;
	}
	else if ( spInfo->aFormat == FMT_BGR8 )
	{
		sdlFmt = SDL_PIXELFORMAT_BGR24;
		pitch *= 3;
	}
	else
	{
		pitch *= 4;
	}

	SDL_Surface* surf = SDL_CreateRGBSurfaceWithFormatFrom(
		srData.data(), 
		spInfo->aWidth,
		spInfo->aHeight,
		spInfo->aBitDepth,
		pitch,
		sdlFmt
	);

	if ( surf == nullptr )
		return false;

	SDL_Texture* tex = SDL_CreateTextureFromSurface( gRenderer, surf );
	SDL_FreeSurface( surf );

	if ( tex == nullptr )
	{
		printf( "[Render] Failed to create texture from surface: %s\n", SDL_GetError() );
		return false;
	}

	// gImageTextures[ spInfo ] = surf;
	gImageTextures[ spInfo ] = tex;

	return true;
#endif
	return false;
}


void Render_FreeImage( ImageInfo* spInfo )
{
#if 0
	auto find = gImageTextures.find( spInfo );

	// Image is not loaded
	if ( find == gImageTextures.end() )
		return;

	SDL_DestroyTexture( find->second );

	gImageTextures.erase( spInfo );
#endif
}


void Render_DrawImage( ImageInfo* spInfo, const ImageDrawInfo& srDrawInfo )
{
#if 0
	auto find = gImageTextures.find( spInfo );

	// Image is already loaded
	if ( find == gImageTextures.end() )
		return;

	// SDL_Surface* surf = find->second;
	SDL_Texture* tex = find->second;

	SDL_Rect dstRect{
		.x = (int)round(srDrawInfo.aX),
		.y = (int)round(srDrawInfo.aY),
		.w = srDrawInfo.aWidth,
		.h = srDrawInfo.aHeight
	};

	SDL_RenderCopy( gRenderer, tex, nullptr, &dstRect );
#endif
}


#pragma nuts

#ifdef _WIN32
  #define VK_USE_PLATFORM_WIN32_KHR 1
#endif

#include <vulkan/vulkan.hpp>


struct ImageInfo;
struct ImageDrawInfo;
enum ImageFilter : unsigned char;


#define VK_COLOR_RESOLVE 1


extern float gClearR;
extern float gClearG;
extern float gClearB;

extern int   gWidth;
extern int   gHeight;


#if _DEBUG
extern PFN_vkDebugMarkerSetObjectTagEXT  pfnDebugMarkerSetObjectTag;
extern PFN_vkDebugMarkerSetObjectNameEXT pfnDebugMarkerSetObjectName;
extern PFN_vkCmdDebugMarkerBeginEXT      pfnCmdDebugMarkerBegin;
extern PFN_vkCmdDebugMarkerEndEXT        pfnCmdDebugMarkerEnd;
extern PFN_vkCmdDebugMarkerInsertEXT     pfnCmdDebugMarkerInsert;
#endif



struct QueueFamilyIndices
{
	int  aPresentFamily  = -1;
	int  aGraphicsFamily = -1;

	// Function that returns true if there is a valid queue family available.
	bool Complete() { return ( aPresentFamily > -1 ) && ( aGraphicsFamily > -1 ); }
};


struct SwapChainSupportInfo
{
	VkSurfaceCapabilitiesKHR          aCapabilities;
	std::vector< VkSurfaceFormatKHR > aFormats;
	std::vector< VkPresentModeKHR >   aPresentModes;
};


struct TextureVK
{
	size_t         aIndex;

	u16            aWidth;
	u16            aHeight;

	VkImage        aImage;
	VkImageView    aImageView;
	VkDeviceMemory aMemory;
	ImageFilter    aFilter;
	bool           aRenderTarget = false;
};


struct RenderTarget
{
	std::vector< TextureVK* >    aImages;
	std::vector< VkFramebuffer > aFrameBuffers;
};


class vec2
{
  public:
	vec2( float x = 0.f, float y = 0.f ) :
		x( x ), y( y )
	{
	}

	float           x{}, y{};

	constexpr float operator[]( int i )
	{
		// index into memory where vars are stored, and clamp to not read garbage
		return *( &x + std::clamp( i, 0, 1 ) );
	}

	constexpr void operator=( const vec2& other )
	{
		// Guard self assignment
		if ( this == &other )
			return;

		std::memcpy( &x, &other.x, sizeof( vec2 ) );
	}

	constexpr bool operator==( const vec2& other )
	{
		// Guard self assignment
		if ( this == &other )
			return true;

		return !( std::memcmp( &x, &other.x, sizeof( vec2 ) ) );
	}
};


class vec3
{
  public:
	vec3( float x = 0.f, float y = 0.f, float z = 0.f ) :
		x( x ), y( y ), z( z )
	{
	}

	float           x{}, y{}, z{};

	constexpr float operator[]( int i )
	{
		// index into memory where vars are stored, and clamp to not read garbage
		return *( &x + std::clamp( i, 0, 2 ) );
	}

	constexpr void operator=( const vec3& other )
	{
		// Guard self assignment
		if ( this == &other )
			return;

		std::memcpy( &x, &other.x, sizeof( vec3 ) );
	}

	constexpr bool operator==( const vec3& other )
	{
		// Guard self assignment
		if ( this == &other )
			return true;

		return !( std::memcmp( &x, &other.x, sizeof( vec3 ) ) );
	}
};


// meh
struct ImgVert_t
{
	vec2 aPos;
	vec2 aUV;
};


struct ImgPush_t
{
	vec2 aScale;
	vec2 aTranslate;
	int aTexIndex;
};

// --------------------------------------------------------------------------------------
// General

char const*                           VKString( VkResult sResult );

void                                  VK_CheckResult( VkResult sResult, char const* spMsg );
void                                  VK_CheckResult( VkResult sResult );

void                                  VK_memcpy( VkDeviceMemory sBufferMemory, VkDeviceSize sSize, const void* spData );

void                                  VK_Reset();

VkSampleCountFlagBits                 VK_GetMSAASamples();

VkCommandBuffer                       VK_GetCommandBuffer();
u32                                   VK_GetCommandIndex();

// --------------------------------------------------------------------------------------
// Vulkan Instance

bool                                  VK_CreateInstance();
void                                  VK_DestroyInstance();

void                                  VK_CreateSurface( void* spWindow );
void                                  VK_DestroySurface();

void                                  VK_SetupPhysicalDevice();
void                                  VK_CreateDevice();

VkInstance                            VK_GetInstance();
VkSurfaceKHR                          VK_GetSurface();

VkDevice                              VK_GetDevice();
VkPhysicalDevice                      VK_GetPhysicalDevice();

VkQueue                               VK_GetGraphicsQueue();
VkQueue                               VK_GetPresentQueue();

bool                                  VK_CheckValidationLayerSupport();
VkSampleCountFlagBits                 VK_FindMaxMSAASamples();

uint32_t                              VK_GetMemoryType( uint32_t sTypeFilter, VkMemoryPropertyFlags sProperties );
QueueFamilyIndices                    VK_FindQueueFamilies( VkPhysicalDevice sDevice );
SwapChainSupportInfo                  VK_CheckSwapChainSupport( VkPhysicalDevice sDevice );

// --------------------------------------------------------------------------------------
// Swapchain

void                                  VK_CreateSwapchain();
void                                  VK_DestroySwapchain();

u32                                   VK_GetSwapImageCount();
const std::vector< VkImage >&         VK_GetSwapImages();
const std::vector< VkImageView >&     VK_GetSwapImageViews();
const VkExtent2D&                     VK_GetSwapExtent();
VkSurfaceFormatKHR                    VK_GetSwapSurfaceFormat();
VkFormat                              VK_GetSwapFormat();
VkColorSpaceKHR                       VK_GetSwapColorSpace();
VkSwapchainKHR                        VK_GetSwapchain();

// --------------------------------------------------------------------------------------
// Descriptor Pool

void                                  VK_CreateDescSets();
void                                  VK_DestroyDescSets();

VkDescriptorPool                      VK_GetDescPool();
VkDescriptorSetLayout                 VK_GetDescImageSet();
VkDescriptorSetLayout                 VK_GetDescBufferSet();

// --------------------------------------------------------------------------------------
// Command Pool

void                                  VK_CreateCommandPool( VkCommandPool& srPool, VkCommandPoolCreateFlags sFlags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT );
void                                  VK_DestroyCommandPool( VkCommandPool& srPool );
void                                  VK_ResetCommandPool( VkCommandPool& srPool, VkCommandPoolResetFlags sFlags = 0 );

VkCommandPool&                        VK_GetSingleTimeCommandPool();
VkCommandPool&                        VK_GetPrimaryCommandPool();

// --------------------------------------------------------------------------------------
// Render Pass

void                                  VK_DestroyRenderPasses();
VkRenderPass                          VK_GetRenderPass();

// --------------------------------------------------------------------------------------
// Present

void                                  VK_CreateFences();
void                                  VK_CreateSemaphores();
void                                  VK_AllocateCommands();
void                                  VK_FreeCommands();
void                                  VK_SingleCommand( std::function< void( VkCommandBuffer ) > sFunc );
void                                  VK_RecordCommands();
void                                  VK_Present();

// --------------------------------------------------------------------------------------
// Shader System

// bool                                  VK_CreateShaders();
// void                                  VK_DestroyShaders();
// 
// void                                  VK_AddShader();
// void                                  VK_GetShaderModule();
// void                                  VK_DrawShader();

// --------------------------------------------------------------------------------------
// Image Shader
// shouldn't really be here, but for this type of program? it's probably fine
void                                  VK_ClearDrawQueue();
void                                  VK_CreateImageShader();
void                                  VK_BindImageShader();
void                                  VK_DrawImageShader();
void                                  VK_AddImageDrawInfo( ImageInfo* spInfo, const ImageDrawInfo& srDrawInfo );

// --------------------------------------------------------------------------------------
// Buffers

void                                  VK_CreateBuffer( VkBuffer& srBuffer, VkDeviceMemory& srBufferMem, u32 sBufferSize, VkBufferUsageFlags sUsage, int sMemBits );
void                                  VK_DestroyBuffer( VkBuffer& srBuffer, VkDeviceMemory& srBufferMem );

// --------------------------------------------------------------------------------------
// Textures and Render Targets

VkSampler                             VK_GetSampler( ImageFilter filter );

TextureVK*                            VK_CreateTexture( ImageInfo* spImageInfo, const std::vector< char >& sData );
void                                  VK_DestroyTexture( ImageInfo* spImageInfo );
void                                  VK_DestroyTexture( TextureVK* spTexture );
TextureVK*                            VK_GetTexture( ImageInfo* spImageInfo );
RenderTarget*                         VK_CreateRenderTarget( const std::vector< TextureVK* >& srImages, u16 sWidth, u16 sHeight, const std::vector< VkImageView >& srSwapImages = {} );
void                                  VK_DestroyRenderTarget( RenderTarget* spTarget );
void                                  VK_DestroyRenderTargets();
void                                  VK_RebuildRenderTargets();

RenderTarget*                         VK_GetBackBuffer();

void                                  VK_CreateImageLayout();
VkDescriptorSetLayout                 VK_GetImageLayout();

const std::vector< VkDescriptorSet >& VK_GetImageSets();
VkDescriptorSet                       VK_GetImageSet( size_t sIndex );
void                                  VK_UpdateImageSets();


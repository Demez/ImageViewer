#pragma nuts

#ifdef _WIN32
  #define VK_USE_PLATFORM_WIN32_KHR 1
#endif

#include <vulkan/vulkan.hpp>


struct ImageInfo;


extern float gClearR;
extern float gClearG;
extern float gClearB;

extern int   gWidth;
extern int   gHeight;


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
	u16            aWidth;
	u16            aHeight;

	VkImage        aImage;
	VkImageView    aImageView;
	VkDeviceMemory aMemory;
	VkSampler      aSampler;
};


struct RenderTarget
{
	std::vector< TextureVK* >    aImages;
	std::vector< VkFramebuffer > aFrameBuffers;
};


struct RenderPass
{
	VkRenderPass       aRenderPass;
	VkFormat           aFormat;
	VkExtent2D         aExtent;
	VkImageViewType    aViewType;
	VkImageAspectFlags aAspectFlags;
	VkImageView        aView;

	RenderPass( const std::vector< VkAttachmentDescription >& srAttachments,
	            const std::vector< VkSubpassDescription >&    srSubpasses,
	            const std::vector< VkSubpassDependency >&     srDependencies );
	~RenderPass();

	const constexpr VkRenderPass GetRenderPass() { return aRenderPass; }
};


// --------------------------------------------------------------------------------------
// General

char const*                VKString( VkResult sResult );

void                       VK_CheckResult( VkResult sResult, char const* spMsg );
void                       VK_CheckResult( VkResult sResult );

VkSampleCountFlagBits      VK_GetMSAASamples();

VkCommandBuffer            VK_GetCommandBuffer();


// --------------------------------------------------------------------------------------
// Vulkan Instance

bool                       VK_CreateInstance();
void                       VK_DestroyInstance();

void                       VK_CreateSurface( void* spWindow );
void                       VK_DestroySurface();

void                       VK_SetupPhysicalDevice();
void                       VK_CreateDevice();

VkInstance                 VK_GetInstance();
VkSurfaceKHR               VK_GetSurface();

VkDevice                   VK_GetDevice();
VkPhysicalDevice           VK_GetPhysicalDevice();

VkQueue                    VK_GetGraphicsQueue();
VkQueue                    VK_GetPresentQueue();

bool                       VK_CheckValidationLayerSupport();
VkSampleCountFlagBits      VK_FindMaxMSAASamples();

uint32_t                   VK_GetMemoryType( uint32_t sTypeFilter, VkMemoryPropertyFlags sProperties );
QueueFamilyIndices         VK_FindQueueFamilies( VkPhysicalDevice sDevice );
SwapChainSupportInfo       VK_CheckSwapChainSupport( VkPhysicalDevice sDevice );


// --------------------------------------------------------------------------------------
// Swapchain

void                       VK_CreateSwapchain();
void                       VK_DestroySwapchain();
void                       VK_RebuildSwapchain();

u32                        VK_GetSwapImageCount();
std::vector< VkImage >     VK_GetSwapImages();
std::vector< VkImageView > VK_GetSwapImageViews();
VkExtent2D                 VK_GetSwapExtent();
VkSurfaceFormatKHR         VK_GetSwapSurfaceFormat();
VkFormat                   VK_GetSwapFormat();
VkColorSpaceKHR            VK_GetSwapColorSpace();
VkSwapchainKHR             VK_GetSwapchain();


// --------------------------------------------------------------------------------------
// Descriptor Pool

void                       VK_CreateDescSets();
void                       VK_DestroyDescSets();

VkDescriptorPool           VK_GetDescPool();
VkDescriptorSetLayout      VK_GetDescImageSet();
VkDescriptorSetLayout      VK_GetDescBufferSet();


// --------------------------------------------------------------------------------------
// Command Pool

void                       VK_CreateCommandPool( VkCommandPool& srPool, VkCommandPoolCreateFlags sFlags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT );
void                       VK_DestroyCommandPool( VkCommandPool& srPool );
void                       VK_ResetCommandPool( VkCommandPool& srPool, VkCommandPoolResetFlags sFlags = 0 );

VkCommandPool&             VK_GetSingleTimeCommandPool();
VkCommandPool&             VK_GetPrimaryCommandPool();

void                       VK_SingleCommand( std::function< void( VkCommandBuffer ) > sFunc );


// --------------------------------------------------------------------------------------
// Render Pass

void                       VK_DestroyRenderPasses();
RenderPass*                VK_GetRenderPass();
VkRenderPass               VK_GetVkRenderPass();


// --------------------------------------------------------------------------------------
// Present

void                       VK_CreateDrawThreads();
void                       VK_AllocateCommands();
void                       VK_RecordCommands();
void                       VK_Present();


// --------------------------------------------------------------------------------------
// Textures and Render Targets

VkSampler&                 VK_GetSampler();

TextureVK*                 VK_CreateTexture( ImageInfo* spImageInfo, std::vector< char > sData );
RenderTarget*              VK_CreateRenderTarget( const std::vector< TextureVK* >& srImages, u16 sWidth, u16 sHeight, const std::vector< VkImageView >& srSwapImages = {} );
void                       VK_DestroyRenderTarget( RenderTarget* spTarget );
void                       VK_DestroyRenderTargets();
void                       VK_RebuildRenderTargets();

RenderTarget*              VK_GetBackBuffer();


#include "platform.h"
#include "util.h"
#include "log.h"
#include "args.h"

#include "render_vk.h"

#include <set>


#ifdef NDEBUG
constexpr bool        gEnableValidationLayers = false;
constexpr char const* gpValidationLayers[]    = { 0 };
#else
constexpr bool        gEnableValidationLayers = true;
constexpr char const* gpValidationLayers[]    = { "VK_LAYER_KHRONOS_validation" };
#endif


constexpr char const* gpExtensions[] = {
#ifdef _WIN32
	VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
#endif
	VK_KHR_SURFACE_EXTENSION_NAME,
#if _DEBUG
	VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
#endif
};


constexpr char const* gpDeviceExtensions[] = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME,
	"VK_EXT_descriptor_indexing"
};


constexpr char const* gpOptionalExtensions[] = {
	VK_EXT_FILTER_CUBIC_EXTENSION_NAME,
};


constexpr char const* gpOptionalDeviceExtensions[] = {
	VK_EXT_FILTER_CUBIC_EXTENSION_NAME,
};


#if _DEBUG
PFN_vkDebugMarkerSetObjectTagEXT  pfnDebugMarkerSetObjectTag;
PFN_vkDebugMarkerSetObjectNameEXT pfnDebugMarkerSetObjectName;
PFN_vkCmdDebugMarkerBeginEXT      pfnCmdDebugMarkerBegin;
PFN_vkCmdDebugMarkerEndEXT        pfnCmdDebugMarkerEnd;
PFN_vkCmdDebugMarkerInsertEXT     pfnCmdDebugMarkerInsert;
#endif


static VkInstance                        gInstance;
static VkDebugUtilsMessengerEXT          gLayers;
static VkSurfaceKHR                      gSurface;
static VkSampleCountFlagBits             gSampleCount;
static VkPhysicalDevice                  gPhysicalDevice;
static VkDevice                          gDevice;
static VkQueue                           gGraphicsQueue;
static VkQueue                           gPresentQueue;

static VkSurfaceCapabilitiesKHR          gSwapCapabilities;
static std::vector< VkSurfaceFormatKHR > gSwapFormats;
static std::vector< VkPresentModeKHR >   gSwapPresentModes;


VKAPI_ATTR VkBool32 VKAPI_CALL VK_DebugCallback( VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType,
                                              const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData )
{
	if ( !gEnableValidationLayers )
		return VK_FALSE;

	if ( messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT )
	{
		std::string formatted;
		vstring( formatted, "\n%s\n\n", pCallbackData->pMessage );
		printf( formatted.c_str() );
	}

	return VK_FALSE;
}


bool VK_CheckValidationLayerSupport()
{
	bool         layerFound;
	unsigned int layerCount;
	vkEnumerateInstanceLayerProperties( &layerCount, NULL );

	std::vector< VkLayerProperties > availableLayers( layerCount );
	vkEnumerateInstanceLayerProperties( &layerCount, availableLayers.data() );

	for ( auto layerName : gpValidationLayers )
	{
		layerFound = false;

		for ( const auto& layerProperties : availableLayers )
		{
			if ( strcmp( layerName, layerProperties.layerName ) == 0 )
			{
				layerFound = true;
				break;
			}
		}

		if ( !layerFound )
		{
			return false;
		}
	}

	return true;
}

constexpr VkDebugUtilsMessengerCreateInfoEXT gLayerInfo = {
	.sType           = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
	.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
	.messageType     = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
	.pfnUserCallback = VK_DebugCallback,
};

VkResult VK_CreateValidationLayers()
{
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr( gInstance, "vkCreateDebugUtilsMessengerEXT" );
	if ( func != NULL )
		return func( gInstance, &gLayerInfo, nullptr, &gLayers );
	else
		return VK_ERROR_EXTENSION_NOT_PRESENT;
}


void VK_DestroyValidationLayers()
{
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr( gInstance, "vkDestroyDebugUtilsMessengerEXT" );
	if ( func != NULL )
		func( gInstance, gLayers, nullptr );
}


bool VK_CheckDeviceExtensionSupport( VkPhysicalDevice sDevice )
{
	unsigned int extensionCount;
	vkEnumerateDeviceExtensionProperties( sDevice, NULL, &extensionCount, NULL );

	std::vector< VkExtensionProperties > availableExtensions( extensionCount );
	vkEnumerateDeviceExtensionProperties( sDevice, NULL, &extensionCount, availableExtensions.data() );

	std::set< std::string > requiredExtensions( gpDeviceExtensions, gpDeviceExtensions + ARR_SIZE( gpDeviceExtensions ) );

	if ( ARGS_HAS( "-list-exts" ) )
	{
		printf( "Device Extensions: %zd\n", availableExtensions.size() );

		for ( const auto& extension : availableExtensions )
		{
			auto ret = requiredExtensions.find( extension.extensionName );

			if ( ret != requiredExtensions.end() )
			{
				printf( " [LOADED] %s \n", extension.extensionName );
				requiredExtensions.erase( extension.extensionName );
			}
			else
			{
				printf( "          %s\n", extension.extensionName );
			}
		}

		printf( "\n" );
	}
	else
	{
		for ( const auto& extension : availableExtensions )
		{
			requiredExtensions.erase( extension.extensionName );
		}
	}

	return requiredExtensions.empty();
}


bool VK_CreateInstance()
{
	if ( gEnableValidationLayers && !VK_CheckValidationLayerSupport() )
		LogFatal( "Validation layers requested, but not available!" );

	VkApplicationInfo appInfo{ VK_STRUCTURE_TYPE_APPLICATION_INFO };
	appInfo.pApplicationName   = "ProtoViewer";
	appInfo.applicationVersion = VK_MAKE_VERSION( 1, 0, 0 );
	appInfo.pEngineName        = "ProtoViewer";
	appInfo.engineVersion      = VK_MAKE_VERSION( 1, 0, 0 );
	appInfo.apiVersion         = VK_HEADER_VERSION_COMPLETE;

	VkInstanceCreateInfo createInfo{ VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
	createInfo.pApplicationInfo = &appInfo;

	if ( gEnableValidationLayers )
	{
		createInfo.enabledLayerCount   = (unsigned int)ARR_SIZE( gpValidationLayers );
		createInfo.ppEnabledLayerNames = gpValidationLayers;
		createInfo.pNext               = (VkDebugUtilsMessengerCreateInfoEXT*)&gLayerInfo;
	}
	else
	{
		createInfo.enabledLayerCount = 0;
		createInfo.pNext             = NULL;
	}

	createInfo.enabledExtensionCount   = (uint32_t)ARR_SIZE( gpExtensions );
	createInfo.ppEnabledExtensionNames = gpExtensions;

	VK_CheckResult( vkCreateInstance( &createInfo, NULL, &gInstance ), "Failed to create instance!" );

	unsigned int extensionCount = 0;
	vkEnumerateInstanceExtensionProperties( NULL, &extensionCount, NULL );

	std::vector< VkExtensionProperties > extProps( extensionCount );
	vkEnumerateInstanceExtensionProperties( NULL, &extensionCount, extProps.data() );

	if ( ARGS_HAS( "-list-exts" ) )
	{
		printf( "%d Vulkan extensions available:\n", extensionCount );
	
		for ( const auto& ext : extProps )
			printf( "    %s\n", ext.extensionName );
	
		printf( "\n" );
	}

	if ( gEnableValidationLayers && VK_CreateValidationLayers() != VK_SUCCESS )
		LogFatal( "Failed to create validation layers!" );
	
	return true;
}


void VK_DestroyInstance()
{
	VK_DestroyValidationLayers();
	vkDestroyDevice( gDevice, NULL );
	vkDestroySurfaceKHR( gInstance, gSurface, NULL );
	vkDestroyInstance( gInstance, NULL );
}


uint32_t VK_GetMemoryType( uint32_t sTypeFilter, VkMemoryPropertyFlags sProperties )
{
	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties( gPhysicalDevice, &memProperties );

	for ( uint32_t i = 0; i < memProperties.memoryTypeCount; ++i )
	{
		if ( ( sTypeFilter & ( 1 << i ) ) && ( memProperties.memoryTypes[ i ].propertyFlags & sProperties ) == sProperties )
		{
			return i;
		}
	}

	return INT32_MAX;
}


// TODO: rethink this
void VK_FindQueueFamilies( VkPhysicalDevice sDevice, u32* spGraphics, u32* spPresent )
{
	uint32_t           queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties( sDevice, &queueFamilyCount, nullptr );

	std::vector< VkQueueFamilyProperties > queueFamilies( queueFamilyCount );
	vkGetPhysicalDeviceQueueFamilyProperties( sDevice, &queueFamilyCount, queueFamilies.data() );  // Logic to find queue family indices to populate struct with
	
	u32 i = 0;
	for ( const auto& queueFamily : queueFamilies )
	{
		if ( queueFamily.queueFlags & (VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT) )
		{
			VkBool32 presentSupport = false;
			VK_CheckResult( vkGetPhysicalDeviceSurfaceSupportKHR( sDevice, i, VK_GetSurface(), &presentSupport ),
			                "Failed to Get Physical Device Surface Support" );

			if ( presentSupport && spPresent )
				*spPresent = i;

			*spGraphics = i;
		}

		// if ( indices.Complete() )
		// 	break;
		return;

		i++;
	}
}


bool VK_ValidQueueFamilies( u32& srPresent, u32& srGraphics )
{
	return true;
}


VkSurfaceCapabilitiesKHR VK_GetSwapCapabilities()
{
	return gSwapCapabilities;
}


VkSurfaceFormatKHR VK_ChooseSwapSurfaceFormat()
{
	for ( const auto& availableFormat : gSwapFormats )
	{
		if ( availableFormat.format == gColorFormat && availableFormat.colorSpace == gColorSpace )
		{
			return availableFormat;
		}
	}
	return gSwapFormats[ 0 ];
}


VkPresentModeKHR VK_ChooseSwapPresentMode()
{
	// if ( !GetOption( "VSync" ) )
	return VK_PRESENT_MODE_IMMEDIATE_KHR;

	for ( const auto& availablePresentMode : gSwapPresentModes )
	{
		if ( availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR )
			return availablePresentMode;
	}
	return VK_PRESENT_MODE_FIFO_KHR;
}


VkExtent2D VK_ChooseSwapExtent()
{
	// TODO: probably use gWidth and gHeight instead, not sure how this would behave on linux lol
	// if ( srCapabilities.currentExtent.width != UINT32_MAX )
	// 	return srCapabilities.currentExtent;

	VkExtent2D size{
		std::max( gSwapCapabilities.minImageExtent.width, std::min( gSwapCapabilities.maxImageExtent.width, (u32)gWidth ) ),
		std::max( gSwapCapabilities.minImageExtent.height, std::min( gSwapCapabilities.maxImageExtent.height, (u32)gHeight ) ),
	};

	return size;
}


void VK_UpdateSwapchainInfo()
{
	if ( !gPhysicalDevice )
	{
		printf( "VK_UpdateSwapchainInfo(): No Physical Device?\n" );
		return;
	}

	VK_CheckResult( vkGetPhysicalDeviceSurfaceCapabilitiesKHR( gPhysicalDevice, VK_GetSurface(), &gSwapCapabilities ),
	                "Failed to Get Physical Device Surface Capabilities" );
}


bool VK_SetupSwapchainInfo( VkPhysicalDevice sDevice )
{
	VkSurfaceKHR surf = VK_GetSurface();
	VK_CheckResult( vkGetPhysicalDeviceSurfaceCapabilitiesKHR( sDevice, surf, &gSwapCapabilities ),
	                "Failed to Get Physical Device Surface Capabilities" );

	uint32_t formatCount;
	VK_CheckResult( vkGetPhysicalDeviceSurfaceFormatsKHR( sDevice, surf, &formatCount, NULL ),
	                "Failed to Get Physical Device Surface Formats" );

	if ( formatCount != 0 )
	{
		gSwapFormats.resize( formatCount );
		VK_CheckResult( vkGetPhysicalDeviceSurfaceFormatsKHR( sDevice, surf, &formatCount, gSwapFormats.data() ),
		                "Failed to Get Physical Device Surface Formats" );
	}

	uint32_t presentModeCount;
	VK_CheckResult( vkGetPhysicalDeviceSurfacePresentModesKHR( sDevice, surf, &presentModeCount, NULL ),
	                "Failed to Get Physical Device Surface Present Modes" );

	if ( presentModeCount != 0 )
	{
		gSwapPresentModes.resize( presentModeCount );
		VK_CheckResult( vkGetPhysicalDeviceSurfacePresentModesKHR( sDevice, surf, &presentModeCount, gSwapPresentModes.data() ),
		                "Failed to Get Physical Device Surface Present Modes" );
	}

	return !gSwapFormats.empty() && !gSwapPresentModes.empty();
}


bool VK_SuitableCard( VkPhysicalDevice sDevice )
{
	bool extensionsSupported = VK_CheckDeviceExtensionSupport( sDevice );
	bool swapChainAdequate   = false;

	if ( extensionsSupported )
		swapChainAdequate = VK_SetupSwapchainInfo( sDevice );

	u32 graphics, present;
	VK_FindQueueFamilies( sDevice, &graphics, &present );

	return VK_ValidQueueFamilies( present, graphics ) && extensionsSupported && swapChainAdequate;
}


void VK_CreateSurface( void* spWindow )
{
#ifdef _WIN32
	VkWin32SurfaceCreateInfoKHR surfCreateInfo{ VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR };
	surfCreateInfo.hwnd      = (HWND)spWindow;
	surfCreateInfo.hinstance = GetModuleHandle( NULL );
	surfCreateInfo.flags     = 0;
	surfCreateInfo.pNext     = NULL;

	VK_CheckResult( vkCreateWin32SurfaceKHR( VK_GetInstance(), &surfCreateInfo, nullptr, &gSurface ), "Failed to create Surface" );
#else
  #error "create vulkan surface"
#endif
}


void VK_SetupPhysicalDevice()
{
	gPhysicalDevice      = VK_NULL_HANDLE;
	uint32_t deviceCount = 0;

	vkEnumeratePhysicalDevices( gInstance, &deviceCount, NULL );
	if ( deviceCount == 0 )
	{
		throw std::runtime_error( "Failed to find GPUs with Vulkan support!" );
	}
	std::vector< VkPhysicalDevice > devices( deviceCount );
	vkEnumeratePhysicalDevices( gInstance, &deviceCount, devices.data() );

	for ( const auto& device : devices )
	{
		if ( VK_SuitableCard( device ) )
		{
			gPhysicalDevice = device;
			gSampleCount = VK_FindMaxMSAASamples();
			// SetOption( "MSAA Samples", aSampleCount );
			break;
		}
	}

	if ( gPhysicalDevice == VK_NULL_HANDLE )
		LogFatal( "Failed to find a suitable GPU!" );
}


void VK_CreateDevice()
{
	float queuePriority = 1.0f;
	u32   graphics, present;
	VK_FindQueueFamilies( gPhysicalDevice, &graphics, &present );

	std::set< u32 > uniqueQueueFamilies = { graphics, present };
	std::vector< VkDeviceQueueCreateInfo > queueCreateInfos;

	for ( uint32_t queueFamily : uniqueQueueFamilies )
	{
		VkDeviceQueueCreateInfo queueCreateInfo = {
			.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
			.queueFamilyIndex = queueFamily,
			.queueCount       = 1,
			.pQueuePriorities = &queuePriority,
		};

		queueCreateInfos.push_back( queueCreateInfo );
	}

	VkPhysicalDeviceDescriptorIndexingFeaturesEXT indexing{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES_EXT };
	indexing.pNext                                    = nullptr;
	indexing.descriptorBindingPartiallyBound          = VK_TRUE;
	indexing.runtimeDescriptorArray                   = VK_TRUE;
	indexing.descriptorBindingVariableDescriptorCount = VK_TRUE;

	VkPhysicalDeviceFeatures deviceFeatures{};
	deviceFeatures.samplerAnisotropy = VK_TRUE;

	VkDeviceCreateInfo createInfo    = {
		   .sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
		   .pNext                   = &indexing,
		   .flags                   = 0,
		   .queueCreateInfoCount    = (u32)queueCreateInfos.size(),
		   .pQueueCreateInfos       = queueCreateInfos.data(),
		   .enabledLayerCount       = (u32)ARR_SIZE( gpValidationLayers ),
		   .ppEnabledLayerNames     = gpValidationLayers,
		   .enabledExtensionCount   = (u32)ARR_SIZE( gpDeviceExtensions ),
		   .ppEnabledExtensionNames = gpDeviceExtensions,
		   .pEnabledFeatures        = &deviceFeatures,
	};

	VK_CheckResult( vkCreateDevice( gPhysicalDevice, &createInfo, NULL, &gDevice ), "Failed to create logical device!" );

	vkGetDeviceQueue( gDevice, graphics, 0, &gGraphicsQueue );
	vkGetDeviceQueue( gDevice, present, 0, &gPresentQueue );

#if _DEBUG
	pfnDebugMarkerSetObjectTag  = (PFN_vkDebugMarkerSetObjectTagEXT)vkGetInstanceProcAddr( VK_GetInstance(), "vkDebugMarkerSetObjectTagEXT" );
	pfnDebugMarkerSetObjectName = (PFN_vkDebugMarkerSetObjectNameEXT)vkGetInstanceProcAddr( VK_GetInstance(), "vkDebugMarkerSetObjectNameEXT" );
	pfnCmdDebugMarkerBegin      = (PFN_vkCmdDebugMarkerBeginEXT)vkGetInstanceProcAddr( VK_GetInstance(), "vkCmdDebugMarkerBeginEXT" );
	pfnCmdDebugMarkerEnd        = (PFN_vkCmdDebugMarkerEndEXT)vkGetInstanceProcAddr( VK_GetInstance(), "vkCmdDebugMarkerEndEXT" );
	pfnCmdDebugMarkerInsert     = (PFN_vkCmdDebugMarkerInsertEXT)vkGetInstanceProcAddr( VK_GetInstance(), "vkCmdDebugMarkerInsertEXT" );
#endif
}


VkInstance VK_GetInstance()
{
	return gInstance;
}


VkSurfaceKHR VK_GetSurface()
{
	return gSurface;
}


VkDevice VK_GetDevice()
{
	return gDevice;
}


VkPhysicalDevice VK_GetPhysicalDevice()
{
	return gPhysicalDevice;
}


VkQueue VK_GetGraphicsQueue()
{
	return gGraphicsQueue;
}


VkQueue VK_GetPresentQueue()
{
	return gPresentQueue;
}


VkSampleCountFlagBits VK_FindMaxMSAASamples()
{
	VkPhysicalDeviceProperties physicalDeviceProperties;
	vkGetPhysicalDeviceProperties( gPhysicalDevice, &physicalDeviceProperties );

	// VkSampleCountFlags counts = physicalDeviceProperties.limits.framebufferColorSampleCounts & physicalDeviceProperties.limits.framebufferDepthSampleCounts;
	// if ( counts & VK_SAMPLE_COUNT_64_BIT ) return VK_SAMPLE_COUNT_64_BIT;
	// if ( counts & VK_SAMPLE_COUNT_32_BIT ) return VK_SAMPLE_COUNT_32_BIT;
	// if ( counts & VK_SAMPLE_COUNT_16_BIT ) return VK_SAMPLE_COUNT_16_BIT;
	// if ( counts & VK_SAMPLE_COUNT_8_BIT ) return VK_SAMPLE_COUNT_8_BIT;
	// if ( counts & VK_SAMPLE_COUNT_4_BIT ) return VK_SAMPLE_COUNT_4_BIT;
	// if ( counts & VK_SAMPLE_COUNT_2_BIT ) return VK_SAMPLE_COUNT_2_BIT;

	return VK_SAMPLE_COUNT_1_BIT;
}


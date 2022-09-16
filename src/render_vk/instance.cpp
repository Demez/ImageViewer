#include "platform.h"
#include "util.h"
#include "log.h"

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
	VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
};


constexpr char const*           gpDeviceExtensions[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME, "VK_EXT_descriptor_indexing" };


static VkInstance               gInstance;
static VkDebugUtilsMessengerEXT gLayers;
static VkSurfaceKHR             gSurface;
static VkSampleCountFlagBits    gSampleCount;
static VkPhysicalDevice         gPhysicalDevice;
static VkDevice                 gDevice;
static VkQueue                  gGraphicsQueue;
static VkQueue                  gPresentQueue;


VKAPI_ATTR VkBool32 VKAPI_CALL VK_DebugCallback( VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType,
                                              const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData )
{
	if ( !gEnableValidationLayers )
		return VK_FALSE;

	if ( messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT )
	{
		std::string formatted;
		vstring( formatted, "%s\n\n", pCallbackData->pMessage );
		printf( formatted.c_str() );
	}

	return VK_FALSE;
}


constexpr VkDebugUtilsMessengerCreateInfoEXT gLayerInfo = {
	.sType           = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
	.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
	.messageType     = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
	.pfnUserCallback = VK_DebugCallback,
};


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


VkResult VK_CreateValidationLayers()
{
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr( gInstance, "vkCreateDebugUtilsMessengerEXT" );
	if ( func != NULL )
		return func( gInstance, &gLayerInfo, nullptr, &gLayers );
	else
		return VK_ERROR_EXTENSION_NOT_PRESENT;
}


bool VK_CheckDeviceExtensionSupport( VkPhysicalDevice sDevice )
{
	unsigned int extensionCount;
	vkEnumerateDeviceExtensionProperties( sDevice, NULL, &extensionCount, NULL );

	std::vector< VkExtensionProperties > availableExtensions( extensionCount );
	vkEnumerateDeviceExtensionProperties( sDevice, NULL, &extensionCount, availableExtensions.data() );

	std::set< std::string > requiredExtensions( gpDeviceExtensions, gpDeviceExtensions + ARR_SIZE( gpDeviceExtensions ) );

	for ( const auto& extension : availableExtensions )
	{
		requiredExtensions.erase( extension.extensionName );
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

	printf( "%d Vulkan extensions available:\n", extensionCount );

	for ( const auto& ext : extProps )
		printf( "\t%s\n", ext.extensionName );

	printf( "\n" );

	if ( gEnableValidationLayers && VK_CreateValidationLayers() != VK_SUCCESS )
		LogFatal( "Failed to create validation layers!" );
	
	return true;
}


void VK_DestroyInstance()
{
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


QueueFamilyIndices VK_FindQueueFamilies( VkPhysicalDevice sDevice )
{
	QueueFamilyIndices indices;
	uint32_t           queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties( sDevice, &queueFamilyCount, nullptr );

	std::vector< VkQueueFamilyProperties > queueFamilies( queueFamilyCount );
	vkGetPhysicalDeviceQueueFamilyProperties( sDevice, &queueFamilyCount, queueFamilies.data() );  // Logic to find queue family indices to populate struct with
	
	int i = 0;
	for ( const auto& queueFamily : queueFamilies )
	{
		if ( queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT )
		{
			VkBool32 presentSupport = false;
			vkGetPhysicalDeviceSurfaceSupportKHR( sDevice, i, VK_GetSurface(), &presentSupport );
			if ( presentSupport )
				indices.aPresentFamily = i;

			indices.aGraphicsFamily = i;
		}

		if ( indices.Complete() )
			break;

		i++;
	}

	return indices;
}


SwapChainSupportInfo VK_CheckSwapChainSupport( VkPhysicalDevice sDevice )
{
	auto                 surf = VK_GetSurface();
	SwapChainSupportInfo details;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR( sDevice, surf, &details.aCapabilities );

	uint32_t formatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR( sDevice, surf, &formatCount, NULL );

	if ( formatCount != 0 )
	{
		details.aFormats.resize( formatCount );
		vkGetPhysicalDeviceSurfaceFormatsKHR( sDevice, surf, &formatCount, details.aFormats.data() );
	}

	uint32_t presentModeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR( sDevice, surf, &presentModeCount, NULL );

	if ( presentModeCount != 0 )
	{
		details.aPresentModes.resize( presentModeCount );
		vkGetPhysicalDeviceSurfacePresentModesKHR( sDevice, surf, &presentModeCount, details.aPresentModes.data() );
	}

	return details;
}


bool VK_SuitableCard( VkPhysicalDevice sDevice )
{
	QueueFamilyIndices indices             = VK_FindQueueFamilies( sDevice );
	bool               extensionsSupported = VK_CheckDeviceExtensionSupport( sDevice );
	bool               swapChainAdequate   = false;

	if ( extensionsSupported )
	{
		SwapChainSupportInfo swapChainSupport = VK_CheckSwapChainSupport( sDevice );
		swapChainAdequate                     = !swapChainSupport.aFormats.empty() && !swapChainSupport.aPresentModes.empty();
	}

	return indices.Complete() && extensionsSupported && swapChainAdequate;
}


void VK_CreateSurface( void* spWindow )
{
#ifdef _WIN32
	VkWin32SurfaceCreateInfoKHR surfCreateInfo{ VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR };
	surfCreateInfo.hwnd      = (HWND)spWindow;
	surfCreateInfo.hinstance = GetModuleHandle( NULL );
	surfCreateInfo.flags     = 0;
	surfCreateInfo.pNext     = NULL;

	VK_CheckResult( vkCreateWin32SurfaceKHR( VK_GetInstance(), &surfCreateInfo, nullptr, &gSurface ), "Failed to create Surface\n" );
#else
  #error "create vulkan surface"
#endif
}


void VK_DestroySurface()
{
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
	QueueFamilyIndices                     indices = VK_FindQueueFamilies( gPhysicalDevice );

	std::vector< VkDeviceQueueCreateInfo > queueCreateInfos;
	std::set< uint32_t >                   uniqueQueueFamilies = { (uint32_t)indices.aGraphicsFamily, (uint32_t)indices.aPresentFamily };

	float                                  queuePriority       = 1.0f;
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

	VkPhysicalDeviceFeatures deviceFeatures{};
	deviceFeatures.samplerAnisotropy = VK_TRUE;

	VkPhysicalDeviceDescriptorIndexingFeaturesEXT indexing{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES_EXT };
	indexing.pNext                                    = nullptr;
	indexing.descriptorBindingPartiallyBound          = VK_TRUE;
	indexing.runtimeDescriptorArray                   = VK_TRUE;
	indexing.descriptorBindingVariableDescriptorCount = VK_TRUE;

	VkDeviceCreateInfo createInfo                     = {
							.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
							.pNext                   = &indexing,
							.flags                   = 0,
							.queueCreateInfoCount    = (uint32_t)queueCreateInfos.size(),
							.pQueueCreateInfos       = queueCreateInfos.data(),
							.enabledLayerCount       = (uint32_t)ARR_SIZE( gpValidationLayers ),
							.ppEnabledLayerNames     = gpValidationLayers,
							.enabledExtensionCount   = (uint32_t)ARR_SIZE( gpDeviceExtensions ),
							.ppEnabledExtensionNames = gpDeviceExtensions,
							.pEnabledFeatures        = &deviceFeatures,
	};

	VK_CheckResult( vkCreateDevice( gPhysicalDevice, &createInfo, NULL, &gDevice ), "Failed to create logical device!" );

	vkGetDeviceQueue( gDevice, indices.aGraphicsFamily, 0, &gGraphicsQueue );
	vkGetDeviceQueue( gDevice, indices.aPresentFamily, 0, &gPresentQueue );
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

	VkSampleCountFlags counts = physicalDeviceProperties.limits.framebufferColorSampleCounts & physicalDeviceProperties.limits.framebufferDepthSampleCounts;
	if ( counts & VK_SAMPLE_COUNT_64_BIT ) return VK_SAMPLE_COUNT_64_BIT;
	if ( counts & VK_SAMPLE_COUNT_32_BIT ) return VK_SAMPLE_COUNT_32_BIT;
	if ( counts & VK_SAMPLE_COUNT_16_BIT ) return VK_SAMPLE_COUNT_16_BIT;
	if ( counts & VK_SAMPLE_COUNT_8_BIT ) return VK_SAMPLE_COUNT_8_BIT;
	if ( counts & VK_SAMPLE_COUNT_4_BIT ) return VK_SAMPLE_COUNT_4_BIT;
	if ( counts & VK_SAMPLE_COUNT_2_BIT ) return VK_SAMPLE_COUNT_2_BIT;

	return VK_SAMPLE_COUNT_1_BIT;
}


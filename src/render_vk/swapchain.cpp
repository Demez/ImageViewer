#include "platform.h"
#include "util.h"
#include "log.h"

#include "render_vk.h"

#include <algorithm>


static VkSurfaceFormatKHR         gSurfaceFormat;
static VkPresentModeKHR           gPresentMode;
static VkExtent2D                 gExtent;
static VkSwapchainKHR             gSwapChain;
static std::vector< VkImage >     gImages;
static std::vector< VkImageView > gImageViews;


// constexpr VkFormat                gColorFormat = VK_FORMAT_B8G8R8A8_SRGB;
// constexpr VkColorSpaceKHR         gColorSpace  = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;

constexpr VkFormat                gColorFormat = VK_FORMAT_B8G8R8A8_UNORM;
constexpr VkColorSpaceKHR         gColorSpace  = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;


VkSurfaceFormatKHR ChooseSwapSurfaceFormat( const std::vector< VkSurfaceFormatKHR >& srAvailableFormats )
{
	for ( const auto& availableFormat : srAvailableFormats )
	{
		if ( availableFormat.format == gColorFormat && availableFormat.colorSpace == gColorSpace )
		{
			return availableFormat;
		}
	}
	return srAvailableFormats[ 0 ];
}


VkPresentModeKHR ChooseSwapPresentMode( const std::vector< VkPresentModeKHR >& srAvailablePresentModes )
{
	// if ( !GetOption( "VSync" ) )
	 	return VK_PRESENT_MODE_IMMEDIATE_KHR;

	for ( const auto& availablePresentMode : srAvailablePresentModes )
	{
		if ( availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR )
			return availablePresentMode;
	}
	return VK_PRESENT_MODE_FIFO_KHR;
}


VkExtent2D ChooseSwapExtent( const VkSurfaceCapabilitiesKHR& srCapabilities )
{
	// TODO: probably use gWidth and gHeight instead, not sure how this would behave on linux lol
	if ( srCapabilities.currentExtent.width != UINT32_MAX )
		return srCapabilities.currentExtent;

	VkExtent2D size{
		std::max( srCapabilities.minImageExtent.width, std::min( srCapabilities.maxImageExtent.width, (u32)gWidth ) ),
		std::max( srCapabilities.minImageExtent.height, std::min( srCapabilities.maxImageExtent.height, (u32)gHeight ) ),
	};

	return size;
}


void VK_CreateSwapchain()
{
	auto swapChainSupport = VK_CheckSwapChainSupport( VK_GetPhysicalDevice() );

	gSurfaceFormat        = ChooseSwapSurfaceFormat( swapChainSupport.aFormats );
	gPresentMode          = ChooseSwapPresentMode( swapChainSupport.aPresentModes );
	gExtent               = ChooseSwapExtent( swapChainSupport.aCapabilities );

	uint32_t imageCount   = swapChainSupport.aCapabilities.minImageCount;

	if ( swapChainSupport.aCapabilities.maxImageCount > 0 && imageCount > swapChainSupport.aCapabilities.maxImageCount )
		imageCount = swapChainSupport.aCapabilities.maxImageCount;

	VkSwapchainCreateInfoKHR createInfo = {
		.sType            = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
		.surface          = VK_GetSurface(),
		.minImageCount    = imageCount,
		.imageFormat      = gSurfaceFormat.format,
		.imageColorSpace  = gSurfaceFormat.colorSpace,
		.imageExtent      = gExtent,
		.imageArrayLayers = 1,
		.imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
	};

	QueueFamilyIndices indices              = VK_FindQueueFamilies( VK_GetPhysicalDevice() );
	uint32_t           queueFamilyIndices[] = { (uint32_t)indices.aGraphicsFamily, (uint32_t)indices.aPresentFamily };

	if ( indices.aGraphicsFamily != indices.aPresentFamily )
	{
		createInfo.imageSharingMode      = VK_SHARING_MODE_CONCURRENT;
		createInfo.queueFamilyIndexCount = 2;
		createInfo.pQueueFamilyIndices   = queueFamilyIndices;
	}
	else
	{
		createInfo.imageSharingMode      = VK_SHARING_MODE_EXCLUSIVE;
		createInfo.queueFamilyIndexCount = 0;     // Optional
		createInfo.pQueueFamilyIndices   = NULL;  // Optional
	}

	createInfo.preTransform   = swapChainSupport.aCapabilities.currentTransform;
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	createInfo.presentMode    = gPresentMode;
	createInfo.clipped        = VK_TRUE;
	createInfo.oldSwapchain   = VK_NULL_HANDLE;

	VK_CheckResult( vkCreateSwapchainKHR( VK_GetDevice(), &createInfo, NULL, &gSwapChain ), "Failed to create swap chain!" );

	vkGetSwapchainImagesKHR( VK_GetDevice(), gSwapChain, &imageCount, NULL );
	gImages.resize( imageCount );
	vkGetSwapchainImagesKHR( VK_GetDevice(), gSwapChain, &imageCount, gImages.data() );

	gImageViews.resize( gImages.size() );
	for ( int i = 0; i < gImages.size(); ++i )
	{
		VkImageViewCreateInfo aImageViewInfo           = {};
		aImageViewInfo.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		aImageViewInfo.pNext                           = nullptr;
		aImageViewInfo.flags                           = 0;
		aImageViewInfo.image                           = gImages[ i ];
		aImageViewInfo.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
		aImageViewInfo.format                          = gColorFormat;
		aImageViewInfo.components.r                    = VK_COMPONENT_SWIZZLE_IDENTITY;
		aImageViewInfo.components.g                    = VK_COMPONENT_SWIZZLE_IDENTITY;
		aImageViewInfo.components.b                    = VK_COMPONENT_SWIZZLE_IDENTITY;
		aImageViewInfo.components.a                    = VK_COMPONENT_SWIZZLE_IDENTITY;
		aImageViewInfo.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
		aImageViewInfo.subresourceRange.baseMipLevel   = 0;
		aImageViewInfo.subresourceRange.levelCount     = 1;
		aImageViewInfo.subresourceRange.baseArrayLayer = 0;
		aImageViewInfo.subresourceRange.layerCount     = 1;

		VK_CheckResult( vkCreateImageView( VK_GetDevice(), &aImageViewInfo, nullptr, &gImageViews[ i ] ), "Failed to create image view!" );
	}
}


void VK_DestroySwapchain()
{
	for ( auto& imgView : gImageViews )
		vkDestroyImageView( VK_GetDevice(), imgView, nullptr );
	
	vkDestroySwapchainKHR( VK_GetDevice(), gSwapChain, NULL );

	gImageViews.clear();
	gImages.clear();  // destroyed with vkDestroySwapchainKHR

	gSwapChain = nullptr;
}


u32 VK_GetSwapImageCount()
{
	return (u32)gImages.size();
}


const std::vector< VkImage >& VK_GetSwapImages()
{
	return gImages;
}


const std::vector< VkImageView >& VK_GetSwapImageViews()
{
	return gImageViews;
}


const VkExtent2D& VK_GetSwapExtent()
{
	return gExtent;
}


VkSurfaceFormatKHR VK_GetSwapSurfaceFormat()
{
	return gSurfaceFormat;
}


VkFormat VK_GetSwapFormat()
{
	return gSurfaceFormat.format;
}


VkColorSpaceKHR VK_GetSwapColorSpace()
{
	return gSurfaceFormat.colorSpace;
}


VkSwapchainKHR VK_GetSwapchain()
{
	return gSwapChain;
}


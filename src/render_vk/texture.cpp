#include "../render.h"
#include "platform.h"
#include "util.h"
#include "log.h"
#include "../formats/imageloader.h"

#include "render_vk.h"


VkSampler                                    gSamplers[ ImageFilter_VulkanCount ]{};

std::vector< TextureVK* >                    gTextures;
static std::vector< RenderTarget* >          gRenderTargets;
static RenderTarget*                         gpBackBuffer = nullptr;

std::unordered_map< ImageInfo*, TextureVK* > gImageMap;


void VK_SetImageLayout( VkImage sImage, VkImageLayout sOldLayout, VkImageLayout sNewLayout, VkImageSubresourceRange& sSubresourceRange )
{
	VkImageMemoryBarrier barrier{};
	barrier.sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout           = sOldLayout;
	barrier.newLayout           = sNewLayout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image               = sImage;
	barrier.subresourceRange    = sSubresourceRange;

	VkPipelineStageFlags sourceStage;
	VkPipelineStageFlags destinationStage;

#if 1
	switch ( sOldLayout )
	{
		case VK_IMAGE_LAYOUT_UNDEFINED:
			barrier.srcAccessMask = 0;
			sourceStage           = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			break;

		case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			sourceStage           = VK_PIPELINE_STAGE_TRANSFER_BIT;
			break;

		default:
			LogFatal( "Unsupported old layout transition!\n" );
			break;
	}

	switch ( sNewLayout )
	{
		case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			destinationStage      = VK_PIPELINE_STAGE_TRANSFER_BIT;
			break;

		case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
			destinationStage      = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
			break;

		case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
			barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
			destinationStage      = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
			break;

		default:
			LogFatal( "Unsupported new layout transition!\n" );
			break;
	}

#else

	if ( sOldLayout == VK_IMAGE_LAYOUT_UNDEFINED && sNewLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL )
	{
		barrier.srcAccessMask = VK_ACCESS_NONE_KHR;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		sourceStage           = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage      = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if ( sOldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && sNewLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL )
	{
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		sourceStage           = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destinationStage      = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else if ( sOldLayout == VK_IMAGE_LAYOUT_UNDEFINED && sNewLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL )
	{
		barrier.srcAccessMask = VK_ACCESS_NONE_KHR;
		barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

		sourceStage           = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage      = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	}
	else
		LogFatal( "Unsupported layout transition!\n" );
#endif

	/* Submit to the GPU.  */
	VK_SingleCommand( [ & ]( VkCommandBuffer c )
	                  { vkCmdPipelineBarrier( c, sourceStage, destinationStage, 0, 0, NULL, 0, NULL, 1, &barrier ); } );
}


void VK_SetImageLayout( VkImage sImage, VkImageLayout sOldLayout, VkImageLayout sNewLayout, u32 sMipLevels )
{
	VkImageSubresourceRange subresourceRange{};
	subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
	subresourceRange.baseMipLevel   = 0;
	subresourceRange.levelCount     = sMipLevels;
	subresourceRange.baseArrayLayer = 0;
	subresourceRange.layerCount     = 1;

	VK_SetImageLayout( sImage, sOldLayout, sNewLayout, subresourceRange );
}


VkFormat VK_GetFormat( PixelFormat pixFmt )
{
	switch ( pixFmt )
	{
		default: return VK_FORMAT_UNDEFINED;
		case FMT_RGB8: return VK_FORMAT_UNDEFINED;  // GPU's don't support 24 bit formats
		case FMT_BGR8: return VK_FORMAT_UNDEFINED;
		case FMT_RGBA8: return VK_FORMAT_R8G8B8A8_UNORM;
		case FMT_BGRA8: return VK_FORMAT_B8G8R8A8_UNORM;
	}
}


// NOTE: may need to change this for texture filtering later, oh boy
VkSampler VK_GetSampler( ImageFilter filter )
{
	if ( filter >= ImageFilter_VulkanCount || filter < 0 )
	{
		filter = ImageFilter_Nearest;
	}

	if ( gSamplers[filter] )
		return gSamplers[filter];

	VkFilter vkFilter = VK_FILTER_NEAREST;

	if ( filter == ImageFilter_Linear )
		vkFilter = VK_FILTER_LINEAR;

	else if ( filter == ImageFilter_Cubic )
		vkFilter = VK_FILTER_CUBIC_IMG;

	VkSamplerCreateInfo samplerInfo{};
	samplerInfo.sType        = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter    = vkFilter;
	samplerInfo.minFilter    = vkFilter;
	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

	VkPhysicalDeviceProperties properties{};
	vkGetPhysicalDeviceProperties( VK_GetPhysicalDevice(), &properties );

	samplerInfo.anisotropyEnable        = VK_TRUE;
	samplerInfo.maxAnisotropy           = properties.limits.maxSamplerAnisotropy;
	samplerInfo.borderColor             = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerInfo.unnormalizedCoordinates = VK_FALSE; 
	samplerInfo.compareEnable           = VK_FALSE;
	samplerInfo.compareOp               = VK_COMPARE_OP_ALWAYS;
	samplerInfo.mipmapMode              = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerInfo.mipLodBias              = 0.0f;
	samplerInfo.minLod                  = 0.0f;
	samplerInfo.maxLod                  = 1000.0f;

	VK_CheckResult( vkCreateSampler( VK_GetDevice(), &samplerInfo, NULL, &gSamplers[filter] ), "Failed to create sampler!" );

	return gSamplers[filter];
}


TextureVK* VK_NewTexture()
{
	TextureVK* tex = gTextures.emplace_back( new TextureVK );
	tex->aIndex    = gTextures.size() - 1;
	tex->aFilter   = ImageFilter_Nearest;
	return tex;
}


TextureVK* VK_CreateTexture( const ivec2& srSize, VkFormat sFormat )  // , VkImageUsageFlagBits sUsage 
{
	TextureVK* tex = gTextures.emplace_back( new TextureVK );
	tex->aIndex    = gTextures.size() - 1;
	tex->aSize     = srSize;
	tex->aFilter   = ImageFilter_Nearest;

	VkImageCreateInfo createInfo{ VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
	createInfo.imageType     = VK_IMAGE_TYPE_2D;
	createInfo.extent.width  = srSize.x;
	createInfo.extent.height = srSize.y;
	createInfo.extent.depth  = 1;
	createInfo.mipLevels     = 1;
	createInfo.arrayLayers   = 1;
	createInfo.format        = sFormat;
	createInfo.tiling        = VK_IMAGE_TILING_OPTIMAL;
	createInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	createInfo.usage         = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
	createInfo.samples       = VK_SAMPLE_COUNT_1_BIT;
	createInfo.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;

	VK_CheckResult( vkCreateImage( VK_GetDevice(), &createInfo, NULL, &tex->aImage ), "Failed to create image" );

	// Allocate and Bind Image Memory
	{
		VkMemoryRequirements memRequirements;
		vkGetImageMemoryRequirements( VK_GetDevice(), tex->aImage, &memRequirements );

		VkMemoryAllocateInfo allocInfo{ VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
		allocInfo.allocationSize  = memRequirements.size;
		allocInfo.memoryTypeIndex = VK_GetMemoryType( memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );

		VK_CheckResult( vkAllocateMemory( VK_GetDevice(), &allocInfo, NULL, &tex->aMemory ), "Failed to allocate image memory" );

		VK_CheckResult( vkBindImageMemory( VK_GetDevice(), tex->aImage, tex->aMemory, 0 ), "Failed to bind image memory" );
	}

	VK_SetImageLayout( tex->aImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 1 );

	// Create Image View
	VkImageViewCreateInfo viewInfo{};
	viewInfo.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image                           = tex->aImage;
	viewInfo.viewType                        = VK_IMAGE_VIEW_TYPE_2D;  // TODO: CHANGE THIS FOR ANIMATED IMAGE SUPPORT !!!!!!
	viewInfo.format                          = sFormat;
	viewInfo.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
	viewInfo.subresourceRange.baseMipLevel   = 0;
	viewInfo.subresourceRange.levelCount     = 1;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount     = 1;

	VK_CheckResult( vkCreateImageView( VK_GetDevice(), &viewInfo, nullptr, &tex->aImageView ), "Failed to create Image View" );

	VK_UpdateImageSets();

	return tex;
}


TextureVK* VK_CreateTexture( ImageInfo* spImageInfo, const std::vector< char >& sData )
{
	auto startTime = std::chrono::high_resolution_clock::now();

	auto find = gImageMap.find( spImageInfo );

	// Image is already loaded
	if ( find != gImageMap.end() )
		return find->second;

	VkFormat vkFormat  = VK_GetFormat( spImageInfo->aFormat );
	if ( vkFormat == VK_FORMAT_UNDEFINED )
	{
		printf( "VK_CreateTexture(): Unsupported image format!\n" );
		return nullptr;
	}

	VkBuffer          stagingBuffer;
	VkDeviceMemory    stagingMemory;

	TextureVK*        tex = gTextures.emplace_back( new TextureVK );
	tex->aIndex           = gTextures.size() - 1;
	tex->aSize.x          = spImageInfo->aWidth;
	tex->aSize.y          = spImageInfo->aHeight;
	tex->aFilter          = ImageFilter_Nearest;

	VK_CreateBuffer( stagingBuffer, stagingMemory, sData.size(), VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT );

	VK_memcpy( stagingMemory, sData.size(), sData.data() );

	VkImageCreateInfo createInfo{ VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
	createInfo.imageType     = VK_IMAGE_TYPE_2D;
	createInfo.extent.width  = spImageInfo->aWidth;
	createInfo.extent.height = spImageInfo->aHeight;
	createInfo.extent.depth  = 1;
	createInfo.mipLevels     = 1;
	createInfo.arrayLayers   = 1;
	createInfo.format        = vkFormat;
	createInfo.tiling        = VK_IMAGE_TILING_OPTIMAL;
	createInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	createInfo.usage         = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
	createInfo.samples       = VK_SAMPLE_COUNT_1_BIT;
	createInfo.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;

	VK_CheckResult( vkCreateImage( VK_GetDevice(), &createInfo, NULL, &tex->aImage ), "Failed to create image" );

	// Allocate and Bind Image Memory
	{
		VkMemoryRequirements memRequirements;
		vkGetImageMemoryRequirements( VK_GetDevice(), tex->aImage, &memRequirements );

		VkMemoryAllocateInfo allocInfo{ VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
		allocInfo.allocationSize  = memRequirements.size;
		allocInfo.memoryTypeIndex = VK_GetMemoryType( memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );

		VK_CheckResult( vkAllocateMemory( VK_GetDevice(), &allocInfo, NULL, &tex->aMemory ), "Failed to allocate image memory" );

		VK_CheckResult( vkBindImageMemory( VK_GetDevice(), tex->aImage, tex->aMemory, 0 ), "Failed to bind image memory" );
	}

	VK_SetImageLayout( tex->aImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1 );

	// Copy Buffer to Image
	VkBufferImageCopy region{};
	region.bufferOffset                    = 0;
	region.bufferRowLength                 = 0;
	region.bufferImageHeight               = 0;

	region.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.mipLevel       = 0;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount     = 1;

	region.imageOffset                     = { 0, 0, 0 };
	region.imageExtent                     = { (u32)spImageInfo->aWidth, (u32)spImageInfo->aHeight, 1 };

	VK_SingleCommand( [ & ]( VkCommandBuffer c )
	                  { vkCmdCopyBufferToImage( c, stagingBuffer, tex->aImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region ); } );

	// Destroy Buffer
	VK_DestroyBuffer( stagingBuffer, stagingMemory );

	// Create Image View
	VkImageViewCreateInfo viewInfo{};
	viewInfo.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image                           = tex->aImage;
	viewInfo.viewType                        = VK_IMAGE_VIEW_TYPE_2D;  // TODO: CHANGE THIS FOR ANIMATED IMAGE SUPPORT !!!!!!
	viewInfo.format                          = vkFormat;
	viewInfo.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
	viewInfo.subresourceRange.baseMipLevel   = 0;
	viewInfo.subresourceRange.levelCount     = 1;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount     = 1;

	VK_CheckResult( vkCreateImageView( VK_GetDevice(), &viewInfo, nullptr, &tex->aImageView ), "Failed to create Image View" );

#if _DEBUG
	// VkDebugMarkerObjectNameInfoEXT nameInfo{ VK_STRUCTURE_TYPE_DEBUG_MARKER_OBJECT_NAME_INFO_EXT };
	// nameInfo.object      = (u64)tex->aImage;
	// nameInfo.objectType  = VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT;
	// nameInfo.pObjectName = "DEMEZ IMAGE";
	// pfnDebugMarkerSetObjectName( VK_GetDevice(), &nameInfo );
#endif

	gImageMap[ spImageInfo ] = tex;

	VK_UpdateImageSets();

	auto  currentTime = std::chrono::high_resolution_clock::now();
	float time        = std::chrono::duration< float, std::chrono::seconds::period >( currentTime - startTime ).count();
	printf( "VK_CreateTexture(): Time taken to create texture: %.6f\n", time );

	return tex;
}


void VK_DestroyTexture( TextureVK* srTexture )
{
	VK_WaitForPresentQueue();

	vkDestroyImageView( VK_GetDevice(), srTexture->aImageView, nullptr );
	vkDestroyImage( VK_GetDevice(), srTexture->aImage, nullptr );

	// TODO: probably change this so i don't call delete on the texture, and maybe i can reuse this image memory somehow
	vkFreeMemory( VK_GetDevice(), srTexture->aMemory, nullptr );

	vec_remove( gTextures, srTexture );
	delete srTexture;
}


void VK_DestroyTexture( ImageInfo* spImageInfo )
{
	auto find = gImageMap.find( spImageInfo );

	// Image is not loaded
	if ( find == gImageMap.end() )
		return;

	VK_DestroyTexture( find->second );

	gImageMap.erase( spImageInfo );
}


void VK_DestroyAllTextures()
{
	for ( auto& tex : gTextures )
	{
		if ( !tex )
			continue;

		vkDestroyImageView( VK_GetDevice(), tex->aImageView, nullptr );
		vkDestroyImage( VK_GetDevice(), tex->aImage, nullptr );

		// TODO: probably change this so i don't call delete on the texture, and maybe i can reuse this image memory somehow
		vkFreeMemory( VK_GetDevice(), tex->aMemory, nullptr );
		delete tex;
	}

	for ( auto& sampler : gSamplers )
	{
		if ( sampler )
			vkDestroySampler( VK_GetDevice(), sampler, nullptr );
	}

	gTextures.clear();
	gImageMap.clear();
	memset( gSamplers, 0, ImageFilter_VulkanCount );
}


TextureVK* VK_GetTexture( ImageInfo* spImageInfo )
{
	auto find = gImageMap.find( spImageInfo );

	// Image is loaded
	if ( find != gImageMap.end() )
		return find->second;

	return nullptr;
}


void VK_CreateRenderTargetInt( RenderTarget* target, const std::vector< TextureVK* >& srImages, u16 sWidth, u16 sHeight, const std::vector< VkImageView >& srSwapImages )
{
	target->aImages.resize( srImages.size() );
	target->aFrameBuffers.resize( srSwapImages.size() );

	for ( size_t i = 0; i < srImages.size(); ++i )
		target->aImages[ i ] = srImages[ i ];

	for ( u32 i = 0; i < VK_GetSwapImageCount(); ++i )
	{
		std::vector< VkImageView > attachments;

		if ( srSwapImages.size() )
			attachments.push_back( srSwapImages[ i ] );
		// attachments.push_back( VK_GetSwapImageViews()[ i ] );

		// for ( auto image : srImages )
		// 	attachments.push_back( image->aImageView );
		attachments.push_back( srImages[1]->aImageView );

		// MSAA: push swap image if you want msaa in the future
		 // if ( srSwapImages.size() )
		 //	attachments.push_back( srSwapImages[ i ] );
			// attachments.push_back( VK_GetSwapImageViews()[ i ] );

		VkFramebufferCreateInfo framebufferInfo{ VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
		framebufferInfo.renderPass      = VK_GetRenderPass();
		framebufferInfo.attachmentCount = attachments.size();
		framebufferInfo.pAttachments    = attachments.data();
		framebufferInfo.width           = sWidth;
		framebufferInfo.height          = sHeight;
		framebufferInfo.layers          = 1;

		VK_CheckResult( vkCreateFramebuffer( VK_GetDevice(), &framebufferInfo, nullptr, &target->aFrameBuffers[ i ] ), "Failed to create framebuffer" );
	}
}


RenderTarget* VK_CreateRenderTarget( const std::vector< TextureVK* >& srImages, u16 sWidth, u16 sHeight, const std::vector< VkImageView >& srSwapImages )
{
	auto target = gRenderTargets.emplace_back( new RenderTarget );
	VK_CreateRenderTargetInt( target, srImages, sWidth, sHeight, srSwapImages );
	return target;
}


void VK_DestroyRenderTarget( RenderTarget* spTarget )
{
	if ( !spTarget )
		return;

	for ( auto frameBuffer : spTarget->aFrameBuffers )
		vkDestroyFramebuffer( VK_GetDevice(), frameBuffer, nullptr );

	for ( auto& texture : spTarget->aImages )
		VK_DestroyTexture( texture );

	vec_remove( gRenderTargets, spTarget );
	delete spTarget;
}


void VK_DestroyRenderTargets()
{
	for ( auto& target : gRenderTargets )
	{
		VK_DestroyRenderTarget( target );
	}

	gRenderTargets.clear();
	gpBackBuffer = nullptr;
}


void VK_RebuildRenderTargets()
{
	for ( auto& target : gRenderTargets )
	{
		VK_DestroyRenderTarget( target );
		// VK_CreateRenderTargetInt( target, target.aImages, target.aImages[ 0 ]->aWidth, target.aImages[ 0 ]->aHeight, {} );
	}
}


RenderTarget* CreateBackBuffer()
{
	/*
     *    Our backbuffer contains 3 render operations: Color, Depth, and Resolve,
     *    so we'll make those now.
     */
	TextureVK* colorTex = VK_NewTexture();
	colorTex->aRenderTarget = true;

	VkImageCreateInfo color;
	color.sType                 = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	color.pNext                 = nullptr;
	color.flags                 = 0;
	color.imageType             = VK_IMAGE_TYPE_2D;
	color.format                = VK_GetSwapFormat();
	color.extent.width          = VK_GetSwapExtent().width;
	color.extent.height         = VK_GetSwapExtent().height;
	color.extent.depth          = 1;
	color.mipLevels             = 1;
	color.arrayLayers           = 1;
	color.samples               = VK_GetMSAASamples();
	color.tiling                = VK_IMAGE_TILING_OPTIMAL;
	// DEMEZ TEST
	// color.usage                 = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	// color.usage                 = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	color.usage                 = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	// color.usage                 = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT;
	color.sharingMode           = VK_SHARING_MODE_EXCLUSIVE;
	color.queueFamilyIndexCount = 0;
	color.pQueueFamilyIndices   = nullptr;
	color.initialLayout         = VK_IMAGE_LAYOUT_UNDEFINED;

	VK_CheckResult( vkCreateImage( VK_GetDevice(), &color, nullptr, &colorTex->aImage ), "Failed to create color image!" );

	VkMemoryRequirements memReqs;
	vkGetImageMemoryRequirements( VK_GetDevice(), colorTex->aImage, &memReqs );

	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType                = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.pNext                = nullptr;
	allocInfo.allocationSize       = memReqs.size;
	allocInfo.memoryTypeIndex      = VK_GetMemoryType( memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );

	VK_CheckResult( vkAllocateMemory( VK_GetDevice(), &allocInfo, nullptr, &colorTex->aMemory ), "Failed to allocate color image memory!" );
	VK_CheckResult( vkBindImageMemory( VK_GetDevice(), colorTex->aImage, colorTex->aMemory, 0 ), "Failed to bind color image memory" );

	VkImageViewCreateInfo colorView;
	colorView.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	colorView.pNext                           = nullptr;
	colorView.flags                           = 0;
	colorView.image                           = colorTex->aImage;
	colorView.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
	colorView.format                          = color.format;
	colorView.components                      = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
	colorView.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
	colorView.subresourceRange.baseMipLevel   = 0;
	colorView.subresourceRange.levelCount     = 1;
	colorView.subresourceRange.baseArrayLayer = 0;
	colorView.subresourceRange.layerCount     = 1;

	VK_CheckResult( vkCreateImageView( VK_GetDevice(), &colorView, nullptr, &colorTex->aImageView ), "Failed to create color image view!" );

	// ------------------------------------------------------
	// Create Depth Texture

	TextureVK* depthTex     = VK_NewTexture();
	depthTex->aRenderTarget = true;

	VkImageCreateInfo depth;
	depth.sType                 = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	depth.pNext                 = nullptr;
	depth.flags                 = 0;
	depth.imageType             = VK_IMAGE_TYPE_2D;
	depth.format                = VK_FORMAT_D32_SFLOAT;
	depth.extent.width          = VK_GetSwapExtent().width;
	depth.extent.height         = VK_GetSwapExtent().height;
	depth.extent.depth          = 1;
	depth.mipLevels             = 1;
	depth.arrayLayers           = 1;
	depth.samples               = VK_GetMSAASamples();
	depth.tiling                = VK_IMAGE_TILING_OPTIMAL;
	depth.usage                 = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	depth.sharingMode           = VK_SHARING_MODE_EXCLUSIVE;
	depth.queueFamilyIndexCount = 0;
	depth.pQueueFamilyIndices   = nullptr;
	depth.initialLayout         = VK_IMAGE_LAYOUT_UNDEFINED;

	VK_CheckResult( vkCreateImage( VK_GetDevice(), &depth, nullptr, &depthTex->aImage ), "Failed to create depth image" );

	vkGetImageMemoryRequirements( VK_GetDevice(), depthTex->aImage, &memReqs );

	allocInfo.allocationSize  = memReqs.size;
	allocInfo.memoryTypeIndex = VK_GetMemoryType( memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );

	VK_CheckResult( vkAllocateMemory( VK_GetDevice(), &allocInfo, nullptr, &depthTex->aMemory ), "Failed to allocate depth image memory" );
	VK_CheckResult( vkBindImageMemory( VK_GetDevice(), depthTex->aImage, depthTex->aMemory, 0 ), "Failed to bind depth image memory" );

	VkImageViewCreateInfo depthView;
	depthView.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	depthView.pNext                           = nullptr;
	depthView.flags                           = 0;
	depthView.image                           = depthTex->aImage;
	depthView.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
	depthView.format                          = depth.format;
	depthView.components                      = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
	depthView.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_DEPTH_BIT;
	depthView.subresourceRange.baseMipLevel   = 0;
	depthView.subresourceRange.levelCount     = 1;
	depthView.subresourceRange.baseArrayLayer = 0;
	depthView.subresourceRange.layerCount     = 1;

	VK_CheckResult( vkCreateImageView( VK_GetDevice(), &depthView, nullptr, &depthTex->aImageView ), "Failed to create depth image view" );

	RenderTarget* rt = VK_CreateRenderTarget( { colorTex, depthTex }, VK_GetSwapExtent().width, VK_GetSwapExtent().height, VK_GetSwapImageViews() );

	return rt;
}

/*
 *    Returns the backbuffer.^
 *    The returned backbuffer contains framebuffers which
 *    are to be drawn to during command buffer recording.
 *    Previously, these were wrongly assumed to be the
 *    same as the swapchain images, but it turns out that
 *    this doesn't matter, it just needs something to draw to.
 * 
 *    @return RenderTarget *    The backbuffer.
 */
RenderTarget* VK_GetBackBuffer()
{
	if ( !gpBackBuffer )
	{
		gpBackBuffer = CreateBackBuffer();
	}
	return gpBackBuffer;
}

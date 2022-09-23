#include "../render.h"
#include "platform.h"
#include "util.h"
#include "log.h"
#include "../formats/imageloader.h"

#include "render_vk.h"


static u32 gFilterShader[] = {
  #include "shaders/filter.comp.u32"
};


// Shader
static VkShaderModule        gShaderModule   = nullptr;

static VkPipeline            gPipeline       = nullptr;
static VkPipelineLayout      gPipelineLayout = nullptr;

static VkDescriptorSetLayout gLayouts[ 2 ]{};

// Mesh
static VkBuffer              gVertexBuffer    = nullptr;
static VkDeviceMemory        gVertexBufferMem = nullptr;

// static VkBuffer              gIndexBuffer     = nullptr;
// static VkDeviceMemory        gIndexBufferMem  = nullptr;

// dumb
std::unordered_map< TextureVK*, TextureVK* > gOutTextures;

struct ComputerShaderTask_t
{
	ImageInfo*    apInfo;
	ImageDrawInfo aDrawInfo;
	TextureVK*    apTexture;
	TextureVK*    apStorage;
};


struct FilterPush_t
{
	vec2        aBicubicScale;
	vec2        aSourceSize;
	vec2        aDestSize;
	int         aTexIndex;
	ImageFilter aFilterType;
};


static std::vector< ComputerShaderTask_t > gTaskQueue;


void VK_AddFilterTask( ImageInfo* spInfo, const ImageDrawInfo& srDrawInfo )
{
	TextureVK* tex = VK_GetTexture( spInfo );

	if ( !tex )
	{
		printf( "Render: Texture for image is nullptr!\n" );
		return;
	}

	TextureVK* tex2 = nullptr;
	auto find = gOutTextures.find( tex );
	if ( find == gOutTextures.end() )
	{
		// lol, this sucks
		if ( gOutTextures.size() )
		{
			for ( auto& [texture, storageTex] : gOutTextures )
			{
				VK_RemoveImageStorage( storageTex );
				VK_DestroyTexture( storageTex );
			}

			gOutTextures.clear();
		}

		tex2                = VK_CreateTexture( { spInfo->aWidth, spInfo->aHeight }, VK_FORMAT_B8G8R8A8_UNORM );
		// tex2                = VK_CreateTexture( { srDrawInfo.aWidth, srDrawInfo.aHeight }, VK_FORMAT_B8G8R8A8_UNORM );
		gOutTextures[ tex ] = tex2;
	}
	else
	{
		tex2 = find->second;

		// BLECH, eating vram, need to limit to window size? but then i would need to recreate the image during a resize? oh boy
		// if ( srDrawInfo.aWidth != tex2->aSize.x || srDrawInfo.aHeight != tex2->aSize.y )
		// {
		// 	VK_DestroyTexture( tex2 );
		// 	tex2 = VK_CreateTexture( { srDrawInfo.aWidth, srDrawInfo.aHeight }, VK_FORMAT_B8G8R8A8_UNORM );
		// }
	}

	VK_AddImageStorage( tex2 );
	VK_UpdateImageStorage();  // lazy hack

	gTaskQueue.push_back( { spInfo, srDrawInfo, tex, tex2 } );
}


void VK_CreateFilterShader()
{
	gLayouts[ 0 ] = VK_GetImageLayout();
	gLayouts[ 1 ] = VK_GetImageStorageLayout();
	
	// VkDescriptorImageInfo imageInfo{};
	// imageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
	// imageInfo.imageView   = imageView;

	VkPipelineLayoutCreateInfo pipelineInfo{
		.sType          = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.setLayoutCount = ARR_SIZE( gLayouts ),
		.pSetLayouts    = gLayouts
	};

	VkPushConstantRange pushConstantRange{
		.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
		.offset     = 0,
		.size       = sizeof( FilterPush_t )
	};

	pipelineInfo.pushConstantRangeCount    = 1;
	pipelineInfo.pPushConstantRanges    = &pushConstantRange;

	VK_CheckResult( vkCreatePipelineLayout( VK_GetDevice(), &pipelineInfo, NULL, &gPipelineLayout ), "Failed to create pipeline layout" );

	gShaderModule = VK_CreateShaderModule( gFilterShader, sizeof( gFilterShader ) );

	VkComputePipelineCreateInfo pipelineCreateInfo{ VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO };
	pipelineCreateInfo.stage.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	pipelineCreateInfo.stage.stage  = VK_SHADER_STAGE_COMPUTE_BIT;
	pipelineCreateInfo.stage.module = gShaderModule;
	pipelineCreateInfo.stage.pName  = "main";
	pipelineCreateInfo.layout       = gPipelineLayout;

	VK_CheckResult( vkCreateComputePipelines( VK_GetDevice(), nullptr, 1, &pipelineCreateInfo, nullptr, &gPipeline ), "Failed to create compute pipeline" );

	// apparently you can destroy the shader module after creating the pipeline? uhhhh
}


void VK_DestroyFilterShader()
{
	if ( gShaderModule )
		vkDestroyShaderModule( VK_GetDevice(), gShaderModule, nullptr );

	gShaderModule = nullptr;

	if ( gVertexBuffer )
		VK_DestroyBuffer( gVertexBuffer, gVertexBufferMem );

	gVertexBuffer    = nullptr;
	gVertexBufferMem = nullptr;

	if ( gPipelineLayout )
		vkDestroyPipelineLayout( VK_GetDevice(), gPipelineLayout, nullptr );

	if ( gPipeline )
		vkDestroyPipeline( VK_GetDevice(), gPipeline, nullptr );

	gPipeline       = nullptr;
	gPipelineLayout = nullptr;
}


void VK_BindFilterShader()
{
	vkCmdBindPipeline( VK_GetCommandBuffer(), VK_PIPELINE_BIND_POINT_COMPUTE, gPipeline );

	VkDescriptorSet sets[] = {
		VK_GetImageSets()[ VK_GetCommandIndex() ],
		VK_GetImageStorage()[ VK_GetCommandIndex() ],
	};

	vkCmdBindDescriptorSets( VK_GetCommandBuffer(), VK_PIPELINE_BIND_POINT_COMPUTE, gPipelineLayout, 0, ARR_SIZE( sets ), sets, 0, nullptr );
}


void VK_RunFilterShader()
{
	if ( gTaskQueue.empty() )
		return;

	VK_BindFilterShader();

	for ( const auto& computeTask : gTaskQueue )
	{
		VkImageMemoryBarrier computeMemoryBarrier = {};
		computeMemoryBarrier.sType                = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		computeMemoryBarrier.oldLayout            = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		computeMemoryBarrier.newLayout            = VK_IMAGE_LAYOUT_GENERAL;
		computeMemoryBarrier.image                = computeTask.apStorage->aImage;
		computeMemoryBarrier.subresourceRange     = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
		computeMemoryBarrier.srcAccessMask        = 0;
		computeMemoryBarrier.dstAccessMask        = VK_ACCESS_SHADER_WRITE_BIT;

		vkCmdPipelineBarrier(
		  VK_GetCommandBuffer(),
		  VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
		  VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
		  0,
		  0, nullptr,
		  0, nullptr,
		  1, &computeMemoryBarrier );

		// update push constant
		FilterPush_t push{};
		push.aFilterType   = computeTask.aDrawInfo.aFilter;
		push.aTexIndex     = computeTask.apTexture->aIndex;
		push.aSourceSize.x = computeTask.apInfo->aWidth;
		push.aSourceSize.y = computeTask.apInfo->aHeight;
		push.aDestSize.x   = computeTask.aDrawInfo.aWidth;
		push.aDestSize.y   = computeTask.aDrawInfo.aWidth;

		vkCmdPushConstants( VK_GetCommandBuffer(), gPipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof( FilterPush_t ), &push );

		// vkCmdDispatch( VK_GetCommandBuffer(), 1, 1, 1 );
		// vkCmdDispatch( VK_GetCommandBuffer(), computeTask.apInfo->aWidth / 32, computeTask.apInfo->aHeight / 32, 1 );
		vkCmdDispatch( VK_GetCommandBuffer(), computeTask.apInfo->aWidth, computeTask.apInfo->aHeight, 1 );
		// vkCmdDispatch( VK_GetCommandBuffer(), computeTask.aDrawInfo.aWidth, computeTask.aDrawInfo.aHeight, 1 );

		
        VkImageMemoryBarrier screenQuadMemoryBarrier = {};
		screenQuadMemoryBarrier.sType                = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		screenQuadMemoryBarrier.oldLayout            = VK_IMAGE_LAYOUT_GENERAL;
		screenQuadMemoryBarrier.newLayout            = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		screenQuadMemoryBarrier.image                = computeTask.apStorage->aImage;
		screenQuadMemoryBarrier.subresourceRange     = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
		screenQuadMemoryBarrier.srcAccessMask        = VK_ACCESS_SHADER_WRITE_BIT;
		screenQuadMemoryBarrier.dstAccessMask        = VK_ACCESS_SHADER_READ_BIT;

		vkCmdPipelineBarrier(
		  VK_GetCommandBuffer(),
		  VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
		  VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
		  0,
		  0, nullptr,
		  0, nullptr,
		  1, &screenQuadMemoryBarrier
		);
	}
}


void VK_PostImageFilter()
{
	// uhhh
	for ( const auto& computeTask : gTaskQueue )
	{
		// VK_SetImageLayout( computeTask.apTexture->aImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 1 );

		// VkImageMemoryBarrier screenQuadMemoryBarrier = {};
		// screenQuadMemoryBarrier.sType                = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		// screenQuadMemoryBarrier.oldLayout            = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		// screenQuadMemoryBarrier.newLayout            = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		// screenQuadMemoryBarrier.image                = computeTask.apTexture->aImage;
		// screenQuadMemoryBarrier.subresourceRange     = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
		// screenQuadMemoryBarrier.srcAccessMask        = VK_ACCESS_SHADER_WRITE_BIT;
		// screenQuadMemoryBarrier.dstAccessMask        = VK_ACCESS_SHADER_READ_BIT;
		// 
		// vkCmdPipelineBarrier(
		//   VK_GetCommandBuffer(),
		//   VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
		//   VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
		//   0,
		//   0, nullptr,
		//   0, nullptr,
		//   1, &screenQuadMemoryBarrier
		// );
	}

	gTaskQueue.clear();
}


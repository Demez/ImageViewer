#include "../render.h"
#include "platform.h"
#include "util.h"
#include "log.h"
#include "../formats/imageloader.h"

#include "render_vk.h"


static u32 gImageShaderFrag[] = {
  #include "shaders/image.frag.u32"
};

static u32 gImageShaderVert[] = {
  #include "shaders/image.vert.u32"
};


// meh
struct ImgVert_t
{
	vec2 aPos;
	vec2 aUV;
};


struct ImgPush_t
{
	vec2        aScale;
	vec2        aTranslate;

	vec2        aViewPort;
	vec2        aTextureSize;
	vec2        aDrawSize;

	int         aTexIndex;
	ImageFilter aFilterType;
	float       aRotation;
};


// Shader
static VkShaderModule        gShaderModules[ 2 ]{};

static VkPipeline            gPipeline       = nullptr;
static VkPipelineLayout      gPipelineLayout = nullptr;

static VkDescriptorSetLayout gLayouts[ 2 ]{};


// Mesh
static VkBuffer              gVertexBuffer    = nullptr;
static VkDeviceMemory        gVertexBufferMem = nullptr;

// static VkBuffer              gIndexBuffer     = nullptr;
// static VkDeviceMemory        gIndexBufferMem  = nullptr;

std::vector< ImgVert_t >     gVertices;


struct ImageShaderDraw_t
{
	ImageInfo*     apInfo;
	ImageDrawInfo  aDrawInfo;
	TextureVK*     apTexture;
};


static std::vector< ImageShaderDraw_t > gDrawQueue;


void VK_AddImageDrawInfo( ImageInfo* spInfo, const ImageDrawInfo& srDrawInfo )
{
	TextureVK* tex = VK_GetTexture( spInfo );

	if ( !tex )
	{
		printf( "Render: Texture for image is nullptr!\n" );
		return;
	}

	// blech
	if ( tex->aFilter != srDrawInfo.aFilter )
	{
		tex->aFilter = srDrawInfo.aFilter;
		
		if ( tex->aFilter == ImageFilter_Nearest || tex->aFilter == ImageFilter_Linear )
		{
			// VK_UpdateImageSets();
			// VK_AddFilterTask( spInfo, srDrawInfo );
		}
	}

	gDrawQueue.push_back( { spInfo, srDrawInfo, tex } );
}


VkShaderModule VK_CreateShaderModule( u32* spByteCode, u32 sSize )
{
	VkShaderModuleCreateInfo createInfo{ VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };

	createInfo.codeSize = sSize;
	createInfo.pCode    = spByteCode;

	VkShaderModule shaderModule;
	VK_CheckResult( vkCreateShaderModule( VK_GetDevice(), &createInfo, NULL, &shaderModule ), "Failed to create shader module!" );

	return shaderModule;
}


void VK_DestroyShaderModule( VkShaderModule shaderModule )
{
	if ( shaderModule )
		vkDestroyShaderModule( VK_GetDevice(), shaderModule, nullptr );
}


void VK_ClearDrawQueue()
{
	gDrawQueue.clear();
}


void VK_BindImageShader()
{
	VkViewport viewport{};
	viewport.x = 0;
	viewport.y = 0;
	viewport.width  = VK_GetSwapExtent().width;
	viewport.height = VK_GetSwapExtent().height;
	viewport.minDepth = 0.f;
	viewport.maxDepth = 1.f;

	vkCmdSetViewport( VK_GetCommandBuffer(), 0, 1, &viewport );

	VkRect2D scissor{
		.offset = { 0, 0 },
		.extent = VK_GetSwapExtent(),
	};

	vkCmdSetScissor( VK_GetCommandBuffer(), 0, 1, &scissor );

	// bind pipeline and descriptor sets
	vkCmdBindPipeline( VK_GetCommandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, gPipeline );

	// bind vertex buffer
	VkBuffer     aBuffers[] = { gVertexBuffer };
	VkDeviceSize aOffsets[] = { 0 };
	vkCmdBindVertexBuffers( VK_GetCommandBuffer(), 0, 1, aBuffers, aOffsets );

	VkDescriptorSet sets[] = {
		VK_GetImageSets()[ VK_GetCommandIndex() ],
		VK_GetImageStorage()[ VK_GetCommandIndex() ],
	};

	vkCmdBindDescriptorSets( VK_GetCommandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, gPipelineLayout, 0, 1, sets, 0, nullptr );
}


void VK_DrawImageShader()
{
	for ( const auto& shaderDraw : gDrawQueue )
	{
		auto&     drawInfo = shaderDraw.aDrawInfo;

		// update push constant
		ImgPush_t push{};
		push.aViewPort.x = VK_GetSwapExtent().width;
		push.aViewPort.y      = VK_GetSwapExtent().height;

		push.aScale.x = 2.0f / VK_GetSwapExtent().width;
		push.aScale.y = 2.0f / VK_GetSwapExtent().height;
		// push.aTranslate.x = ( drawInfo.aX - ( VK_GetSwapExtent().width / 2.f ) ) * push.aScale.x;
		// push.aTranslate.y = ( drawInfo.aY - ( VK_GetSwapExtent().height / 2.f ) ) * push.aScale.y;
		push.aTranslate.x = drawInfo.aX;
		push.aTranslate.y = drawInfo.aY;

		push.aScale.x *= drawInfo.aWidth;
		push.aScale.y *= drawInfo.aHeight;
		
		// useless? just use the textureSize() function?
		push.aDrawSize.x    = shaderDraw.aDrawInfo.aWidth;
		push.aDrawSize.y    = shaderDraw.aDrawInfo.aHeight;
		push.aTextureSize.x = shaderDraw.apInfo->aWidth;
		push.aTextureSize.y = shaderDraw.apInfo->aHeight;

		push.aFilterType = shaderDraw.aDrawInfo.aFilter;

		// printf( "SCALE:  %.6f x %.6f   TRANSLATE: %.6f x %.6f   OFFSET: %.6f x %.6f\n",
		// 	push.aScale.x, push.aScale.y,
		// 	push.aTranslate.x, push.aTranslate.y,
		// 	drawInfo.aX, drawInfo.aY
		// );

		push.aTexIndex = shaderDraw.apTexture->aIndex;
		push.aRotation = shaderDraw.aDrawInfo.aRotation;

		// maybe check if the compute shader has an output for this? idk

		vkCmdPushConstants( VK_GetCommandBuffer(), gPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof( ImgPush_t ), &push );

		vkCmdDraw( VK_GetCommandBuffer(), gVertices.size(), 1, 0, 0 );
	}
}


void VK_CreateImageMesh()
{
	// gVertices.emplace_back( vec2{ -1.0f, -1.0f }, vec2{ 0.0f, 0.0f } );  // Bottom Left
	// gVertices.emplace_back( vec2{ 1.0f, -1.0f }, vec2{ 1.0f, 0.0f } );   // Bottom Right
	// gVertices.emplace_back( vec2{ 1.0f, 1.0f }, vec2{ 1.0f, 1.0f } );    // Top Right
	// 
	// gVertices.emplace_back( vec2{ -1.0f, -1.0f }, vec2{ 0.0f, 0.0f } );  // Bottom Left
	// gVertices.emplace_back( vec2{ -1.0f, 1.0f }, vec2{ 0.0f, 1.0f } );   // Top Left
	// gVertices.emplace_back( vec2{ 1.0f, 1.0f }, vec2{ 1.0f, 1.0f } );    // Top Right

	gVertices.emplace_back( vec2{ 0.0f, 0.0f }, vec2{ 0.0f, 0.0f } );  // Bottom Left
	gVertices.emplace_back( vec2{ 1.0f, 0.0f }, vec2{ 1.0f, 0.0f } );   // Bottom Right
	gVertices.emplace_back( vec2{ 1.0f, 1.0f }, vec2{ 1.0f, 1.0f } );    // Top Right

	gVertices.emplace_back( vec2{ 0.0f, 0.0f }, vec2{ 0.0f, 0.0f } );  // Bottom Left
	gVertices.emplace_back( vec2{ 0.0f, 1.0f }, vec2{ 0.0f, 1.0f } );   // Top Left
	gVertices.emplace_back( vec2{ 1.0f, 1.0f }, vec2{ 1.0f, 1.0f } );    // Top Right

	// create a vertex buffer for the mesh
	VkDeviceSize   bufferSize = sizeof( ImgVert_t ) * gVertices.size();

	VkBuffer       stagingBuffer;
	VkDeviceMemory stagingBufferMemory;

	VK_CreateBuffer(
	  stagingBuffer,
	  stagingBufferMemory,
	  bufferSize,
	  VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
	  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT );

	VK_memcpy( stagingBufferMemory, bufferSize, gVertices.data() );

	VK_CreateBuffer(
	  gVertexBuffer,
	  gVertexBufferMem,
	  bufferSize,
	  VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
	  VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );

	// copy buffer from cpu to the gpu
	VkBufferCopy copyRegion{
		.srcOffset = 0,
		.dstOffset = 0,
		.size      = bufferSize,
	};

	VK_SingleCommand( [ & ]( VkCommandBuffer c )
	                  { vkCmdCopyBuffer( c, stagingBuffer, gVertexBuffer, 1, &copyRegion ); } );

	// free staging buffer
	VK_DestroyBuffer( stagingBuffer, stagingBufferMemory );
}


void VK_CreateImagePipeline()
{
	gLayouts[ 0 ] = VK_GetImageLayout();
	gLayouts[ 1 ] = VK_GetImageStorageLayout();

	VkPipelineLayoutCreateInfo pipelineCreateInfo{
		.sType          = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.setLayoutCount = ARR_SIZE( gLayouts ),
		.pSetLayouts    = gLayouts
	};

	VkPushConstantRange pushConstantRange{
		.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
		.offset     = 0,
		.size       = sizeof( ImgPush_t )
	};

	pipelineCreateInfo.pushConstantRangeCount = 1;
	pipelineCreateInfo.pPushConstantRanges    = &pushConstantRange;

	VK_CheckResult( vkCreatePipelineLayout( VK_GetDevice(), &pipelineCreateInfo, NULL, &gPipelineLayout ), "Failed to create pipeline layout" );

	VkPipelineShaderStageCreateInfo shaderStages[ 2 ]{
		{ .sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		  .stage  = VK_SHADER_STAGE_VERTEX_BIT,
		  .module = gShaderModules[ 0 ],
		  .pName  = "main" },

		{ .sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		  .stage  = VK_SHADER_STAGE_FRAGMENT_BIT,
		  .module = gShaderModules[ 1 ],
		  .pName  = "main" }
	};

	VkVertexInputBindingDescription bindingDescriptions[] = 
	{
		{ .binding   = 0,
		  .stride    = sizeof( ImgVert_t ),
		  .inputRate = VK_VERTEX_INPUT_RATE_VERTEX },
	};

	VkVertexInputAttributeDescription attributeDescriptions[] = 
	{
		{ .location = 0,
		  .binding  = 0,
		  .format   = VK_FORMAT_R32G32_SFLOAT,
		  .offset   = offsetof( ImgVert_t, aPos ) },

		{ .location = 1,
		  .binding  = 0,
		  .format   = VK_FORMAT_R32G32_SFLOAT,
		  .offset   = offsetof( ImgVert_t, aUV ) },
	};

	VkPipelineVertexInputStateCreateInfo vertexInputInfo{
		.sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
		.vertexBindingDescriptionCount   = ARR_SIZE( bindingDescriptions ),
		.pVertexBindingDescriptions      = bindingDescriptions,
		.vertexAttributeDescriptionCount = ARR_SIZE( attributeDescriptions ),
		.pVertexAttributeDescriptions    = attributeDescriptions
	};

	VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo{
		.sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
		.topology               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
		.primitiveRestartEnable = VK_FALSE
	};

	const auto& swapExtent = VK_GetSwapExtent();

	VkViewport viewport{
		.x        = 0.0f,
		.y        = 0,
		.width    = (float)swapExtent.width,
		.height   = (float)swapExtent.height,
		.minDepth = 0.0f,
		.maxDepth = 1.0f
	};

	VkRect2D scissor{
		.offset = { 0, 0 },
		.extent = swapExtent,
	};

	VkPipelineViewportStateCreateInfo viewportInfo{
		.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
		.viewportCount = 1,
		.pViewports    = &viewport,
		.scissorCount  = 1,
		.pScissors     = &scissor
	};

	// create rasterizer
	VkPipelineRasterizationStateCreateInfo rasterizer{};  //	Turns  primitives into fragments, aka, pixels for the framebuffer
	rasterizer.sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable        = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode             = VK_POLYGON_MODE_FILL;  //	Fill with fragments, can optionally use VK_POLYGON_MODE_LINE for a wireframe
	rasterizer.lineWidth               = 1.0f;
	// rasterizer.cullMode 			= ( sFlags & NO_CULLING ) ? VK_CULL_MODE_NONE : VK_CULL_MODE_BACK_BIT;	//	FIX FOR MAKING 2D SPRITES WORK!!! WOOOOO!!!!
	// rasterizer.cullMode 			= VK_CULL_MODE_BACK_BIT;
	rasterizer.cullMode                = VK_CULL_MODE_NONE;
	rasterizer.frontFace               = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizer.depthBiasEnable         = VK_FALSE;
	rasterizer.depthBiasConstantFactor = 0.0f;  // Optional
	rasterizer.depthBiasClamp          = 0.0f;  // Optional
	rasterizer.depthBiasSlopeFactor    = 0.0f;  // Optional

	VkPipelineMultisampleStateCreateInfo multisampling{};  //	Performs anti-aliasing
	multisampling.sType                 = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable   = VK_FALSE;
	multisampling.rasterizationSamples  = VK_GetMSAASamples();
	multisampling.minSampleShading      = 1.0f;      // Optional
	multisampling.pSampleMask           = NULL;      // Optional
	multisampling.alphaToCoverageEnable = VK_FALSE;  // Optional
	multisampling.alphaToOneEnable      = VK_FALSE;  // Optional

	VkPipelineColorBlendAttachmentState colorBlendAttachment{};
	colorBlendAttachment.colorWriteMask      = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable         = VK_TRUE;
	colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	colorBlendAttachment.colorBlendOp        = VK_BLEND_OP_ADD;
	colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendAttachment.alphaBlendOp        = VK_BLEND_OP_ADD;

	VkPipelineDepthStencilStateCreateInfo depthStencil{};
	depthStencil.sType                 = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	//depthStencil.depthTestEnable 		= ( sFlags & NO_DEPTH ) ? VK_FALSE : VK_TRUE;
	//depthStencil.depthWriteEnable		= ( sFlags & NO_DEPTH ) ? VK_FALSE : VK_TRUE;
	depthStencil.depthTestEnable       = VK_TRUE;
	depthStencil.depthWriteEnable      = VK_TRUE;
	depthStencil.depthCompareOp        = VK_COMPARE_OP_LESS;
	depthStencil.depthBoundsTestEnable = VK_FALSE;
	depthStencil.minDepthBounds        = 0.0f;  // Optional
	depthStencil.maxDepthBounds        = 1.0f;  // Optional
	depthStencil.stencilTestEnable     = VK_FALSE;
	depthStencil.front                 = {};  // Optional
	depthStencil.back                  = {};  // Optional

	VkPipelineColorBlendStateCreateInfo colorBlending{};
	colorBlending.sType               = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.logicOpEnable       = VK_FALSE;
	colorBlending.logicOp             = VK_LOGIC_OP_COPY;  // Optional
	colorBlending.attachmentCount     = 1;
	colorBlending.pAttachments        = &colorBlendAttachment;
	colorBlending.blendConstants[ 0 ] = 0.0f;  // Optional
	colorBlending.blendConstants[ 1 ] = 0.0f;  // Optional
	colorBlending.blendConstants[ 2 ] = 0.0f;  // Optional
	colorBlending.blendConstants[ 3 ] = 0.0f;  // Optional

	VkDynamicState                   dynamicStates[]{ VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
	
	VkPipelineDynamicStateCreateInfo dynamicState{};
	dynamicState.sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.dynamicStateCount = ARR_SIZE( dynamicStates );
	dynamicState.pDynamicStates    = dynamicStates;

	VkGraphicsPipelineCreateInfo pipelineInfo{};  //	Combine all the objects above into one parameter for graphics pipeline creation
	pipelineInfo.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount          = ARR_SIZE( shaderStages );
	pipelineInfo.pStages             = shaderStages;
	pipelineInfo.pVertexInputState   = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssemblyInfo;
	pipelineInfo.pViewportState      = &viewportInfo;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState   = &multisampling;
	pipelineInfo.pDepthStencilState  = &depthStencil;
	pipelineInfo.pColorBlendState    = &colorBlending;
	pipelineInfo.pDynamicState       = &dynamicState;
	pipelineInfo.layout              = gPipelineLayout;
	pipelineInfo.renderPass          = VK_GetRenderPass();
	pipelineInfo.subpass             = 0;
	pipelineInfo.basePipelineHandle  = VK_NULL_HANDLE;  // Optional, very important for later when making new pipelines. It is less expensive to reference an existing similar pipeline
	pipelineInfo.basePipelineIndex   = -1;              // Optional

	VK_CheckResult( vkCreateGraphicsPipelines( VK_GetDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, NULL, &gPipeline ), "Failed to create graphics pipeline!" );
}


void VK_CreateImageShader()
{
	gShaderModules[ 0 ] = VK_CreateShaderModule( gImageShaderVert, sizeof( gImageShaderVert ) );
	gShaderModules[ 1 ] = VK_CreateShaderModule( gImageShaderFrag, sizeof( gImageShaderFrag ) );

	VK_CreateImagePipeline();
	VK_CreateImageMesh();
}


void VK_DestroyImageShader()
{
	if ( gShaderModules[ 0 ] )
		vkDestroyShaderModule( VK_GetDevice(), gShaderModules[ 0 ], nullptr );

	if ( gShaderModules[ 1 ] )
		vkDestroyShaderModule( VK_GetDevice(), gShaderModules[ 1 ], nullptr );

	gShaderModules[ 0 ] = nullptr;
	gShaderModules[ 1 ] = nullptr;

	if ( gVertexBuffer )
		VK_DestroyBuffer( gVertexBuffer, gVertexBufferMem );

	gVertexBuffer = nullptr;
	gVertexBufferMem = nullptr;

	if ( gPipelineLayout )
		vkDestroyPipelineLayout( VK_GetDevice(), gPipelineLayout, nullptr );

	if ( gPipeline )
		vkDestroyPipeline( VK_GetDevice(), gPipeline, nullptr );

	gPipeline = nullptr;
	gPipelineLayout = nullptr;
}


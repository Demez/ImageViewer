#include "platform.h"
#include "util.h"
#include "log.h"

#include "render_vk.h"


static std::vector< RenderPass* > gRenderPasses;


VkFormat VK_GetColorFormat()
{
	return VK_GetSwapFormat();
}


VkFormat VK_GetDepthFormat()
{
	return VK_FORMAT_D32_SFLOAT;
}


RenderPass::RenderPass( const std::vector< VkAttachmentDescription >& srAttachments,
                        const std::vector< VkSubpassDescription >&    srSubpasses,
                        const std::vector< VkSubpassDependency >&     srDependencies )
{
	VkRenderPassCreateInfo renderPassInfo{ VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
	renderPassInfo.attachmentCount = static_cast< uint32_t >( srAttachments.size() );
	renderPassInfo.pAttachments    = srAttachments.data();
	renderPassInfo.subpassCount    = static_cast< uint32_t >( srSubpasses.size() );
	renderPassInfo.pSubpasses      = srSubpasses.data();
	renderPassInfo.dependencyCount = static_cast< uint32_t >( srDependencies.size() );
	renderPassInfo.pDependencies   = srDependencies.data();

	VK_CheckResult( vkCreateRenderPass( VK_GetDevice(), &renderPassInfo, nullptr, &aRenderPass ), "Failed to create render pass!" );
}


RenderPass::~RenderPass()
{
	vkDestroyRenderPass( VK_GetDevice(), aRenderPass, nullptr );
}


std::vector< RenderPass* >& GetRenderPasses()
{
	if ( gRenderPasses.size() )
		return gRenderPasses;

	/*
     *    Create the default color and depth render pass.
     *    This is the standard draw pass, and later, other
     *    passes may do deferred for example.
     */
	VkAttachmentDescription colorAttachment = {};
	colorAttachment.format                  = VK_GetSwapFormat();
	colorAttachment.samples                 = VK_GetMSAASamples();
	colorAttachment.loadOp                  = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp                 = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp           = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp          = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout           = VK_IMAGE_LAYOUT_UNDEFINED;

#pragma message( "GetRenderPasses(): colorAttachment finalLayout is different than graphics1" )
	// was color attachment in graphics 1
	colorAttachment.finalLayout                         = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	// colorAttachment.finalLayout             = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	colorAttachment.samples                             = VK_GetMSAASamples();

	VkAttachmentDescription depthAttachment             = {};
	depthAttachment.format                              = VK_GetDepthFormat();
	depthAttachment.samples                             = VK_GetMSAASamples();
	depthAttachment.loadOp                              = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.storeOp                             = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.stencilLoadOp                       = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthAttachment.stencilStoreOp                      = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.initialLayout                       = VK_IMAGE_LAYOUT_UNDEFINED;
	depthAttachment.finalLayout                         = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	depthAttachment.samples                             = VK_GetMSAASamples();

	VkAttachmentDescription colorAttachmentResolve      = {};
	colorAttachmentResolve.format                       = VK_GetSwapFormat();
	colorAttachmentResolve.samples                      = VK_SAMPLE_COUNT_1_BIT;
	colorAttachmentResolve.loadOp                       = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachmentResolve.storeOp                      = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachmentResolve.stencilLoadOp                = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachmentResolve.stencilStoreOp               = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachmentResolve.initialLayout                = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachmentResolve.finalLayout                  = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference colorAttachmentRef            = {};
	colorAttachmentRef.attachment                       = 0;
	colorAttachmentRef.layout                           = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depthAttachmentRef            = {};
	depthAttachmentRef.attachment                       = 1;
	depthAttachmentRef.layout                           = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference colorAttachmentResolveRef     = {};
	colorAttachmentResolveRef.attachment                = 2;
	colorAttachmentResolveRef.layout                    = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass                        = {};
	subpass.pipelineBindPoint                           = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount                        = 1;
	subpass.pColorAttachments                           = &colorAttachmentRef;
	subpass.pDepthStencilAttachment                     = &depthAttachmentRef;
	subpass.pResolveAttachments                         = &colorAttachmentResolveRef;

	VkSubpassDependency dependency                      = {};
	dependency.srcSubpass                               = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass                               = 0;
	dependency.srcStageMask                             = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	// dependency.srcStageMask                = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dependency.srcAccessMask                            = 0;
	dependency.dstStageMask                             = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	// dependency.dstStageMask                = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dependency.dstAccessMask                            = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	// dependency.dstAccessMask               = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

	std::vector< VkAttachmentDescription > attachments  = { colorAttachment, depthAttachment, colorAttachmentResolve };
	std::vector< VkSubpassDescription >    subpasses    = { subpass };
	std::vector< VkSubpassDependency >     dependencies = { dependency };

	/*
     *    TODO:
     *
     *    Either make this somehow not destructable or find a better way
     *    to do this.
     */
	auto                                   pRenderPass  = new RenderPass( attachments, subpasses, dependencies );

	gRenderPasses.push_back( pRenderPass );

	return gRenderPasses;
}


#if 0
VkRenderPass GetRenderPass( RenderPassStage sStage )
{
	for ( auto& renderPass : GetRenderPasses() )
	{
		if ( renderPass->GetStage() == sStage )
			return renderPass->GetRenderPass();
	}

	return nullptr;
}
#endif


void VK_DestroyRenderPasses()
{
	for ( auto& renderPass : gRenderPasses )
	{
		delete renderPass;
	}

	gRenderPasses.clear();
}


RenderPass* VK_GetRenderPass()
{
	for ( auto& renderPass : GetRenderPasses() )
	{
		return renderPass;
	}

	return nullptr;
}


VkRenderPass VK_GetVkRenderPass()
{
	for ( auto& renderPass : GetRenderPasses() )
	{
		return renderPass->aRenderPass;
	}

	return nullptr;
}



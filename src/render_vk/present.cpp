#include "../render.h"
#include "platform.h"
#include "util.h"
#include "log.h"

#include "render_vk.h"

#include "imgui_impl_win32.h"
#include "imgui_impl_vulkan.h"

#include <thread>

#include "imgui.h"
#include "imgui_impl_vulkan.h"


constexpr u32                  MAX_FRAMES_IN_FLIGHT = 2;

// Primary Command Buffers
std::vector< VkCommandBuffer > gCommandBuffers;

std::vector< VkSemaphore >     gImageAvailableSemaphores;
std::vector< VkSemaphore >     gRenderFinishedSemaphores;
std::vector< VkFence >         gFences;
std::vector< VkFence >         gInFlightFences;

u32                            gFrameIndex = 0;


VkCommandBuffer VK_GetCommandBuffer()
{
	return gCommandBuffers[ gFrameIndex ];
}


void CreateFences()
{
	gFences.resize( MAX_FRAMES_IN_FLIGHT );
	gInFlightFences.resize( VK_GetSwapImageCount() );

	VkFenceCreateInfo info = {};
	info.sType             = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	info.flags             = VK_FENCE_CREATE_SIGNALED_BIT;

	for ( u32 i = 0; i < MAX_FRAMES_IN_FLIGHT; i++ )
	{
		VK_CheckResult( vkCreateFence( VK_GetDevice(), &info, nullptr, &gFences[ i ] ), "Failed to create fence!" );
	}
}

void CreateSemaphores()
{
	gImageAvailableSemaphores.resize( MAX_FRAMES_IN_FLIGHT );
	gRenderFinishedSemaphores.resize( MAX_FRAMES_IN_FLIGHT );

	VkSemaphoreCreateInfo info = {};
	info.sType                 = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	for ( u32 i = 0; i < MAX_FRAMES_IN_FLIGHT; i++ )
	{
		VK_CheckResult( vkCreateSemaphore( VK_GetDevice(), &info, nullptr, &gImageAvailableSemaphores[ i ] ), "Failed to create semaphore!" );
		VK_CheckResult( vkCreateSemaphore( VK_GetDevice(), &info, nullptr, &gRenderFinishedSemaphores[ i ] ), "Failed to create semaphore!" );
	}
}


void VK_CreateDrawThreads()
{
	CreateFences();
	CreateSemaphores();
}


void VK_AllocateCommands()
{
	gCommandBuffers.resize( VK_GetSwapImageCount() );

	/*
    *    Allocate primary command buffers
    */
	VkCommandBufferAllocateInfo primAlloc{};
	primAlloc.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	primAlloc.pNext              = nullptr;
	primAlloc.commandPool        = VK_GetPrimaryCommandPool();
	primAlloc.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	primAlloc.commandBufferCount = gCommandBuffers.size();

	VK_CheckResult( vkAllocateCommandBuffers( VK_GetDevice(), &primAlloc, gCommandBuffers.data() ), "Failed to allocate primary command buffers" );
}


void VK_RecordCommands()
{
	/*
     *    For each framebuffer, allocate a primary
     *    command buffer, and record the commands.
     */
	for ( u32 i = 0; i < gCommandBuffers.size(); ++i )
	{
		VkCommandBufferBeginInfo begin{};
		begin.sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		begin.pNext            = nullptr;
		begin.flags            = 0;
		begin.pInheritanceInfo = nullptr;

		VK_CheckResult( vkBeginCommandBuffer( gCommandBuffers[ i ], &begin ), "Failed to begin recording command buffer!" );

		VkRenderPassBeginInfo renderPassBeginInfo{};
		renderPassBeginInfo.sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassBeginInfo.pNext             = nullptr;
		renderPassBeginInfo.renderPass        = VK_GetVkRenderPass();
		renderPassBeginInfo.framebuffer       = VK_GetBackBuffer()->aFrameBuffers[ i ];
		renderPassBeginInfo.renderArea.offset = { 0, 0 };
		renderPassBeginInfo.renderArea.extent = VK_GetSwapExtent();

		VkClearValue clearValues[ 2 ];
		clearValues[ 0 ].color              = { gClearR, gClearG, gClearB, 1.0f };
		clearValues[ 1 ].depthStencil       = { 1.0f, 0 };

		renderPassBeginInfo.clearValueCount = ARR_SIZE( clearValues );
		renderPassBeginInfo.pClearValues    = clearValues;

		vkCmdBeginRenderPass( gCommandBuffers[ i ], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE );

		/*
         *    Render UI.
         */
		ImGui::Render();
		/*
         *    Run ImGui commands.
         */
		ImGui_ImplVulkan_RenderDrawData( ImGui::GetDrawData(), gCommandBuffers[ i ] );

		vkCmdEndRenderPass( gCommandBuffers[ i ] );

		VK_CheckResult( vkEndCommandBuffer( gCommandBuffers[ i ] ), "Failed to end recording command buffer!" );
	}
}


void VK_Present()
{
	vkWaitForFences( VK_GetDevice(), 1, &gFences[ gFrameIndex ], VK_TRUE, UINT64_MAX );

	u32      imageIndex;

	VkResult res = vkAcquireNextImageKHR( VK_GetDevice(), VK_GetSwapchain(), UINT64_MAX, gImageAvailableSemaphores[ gFrameIndex ], VK_NULL_HANDLE, &imageIndex );

	if ( res == VK_ERROR_OUT_OF_DATE_KHR )
	{
		// Recreate all resources.
		printf( "VK_Reset - vkAcquireNextImageKHR\n" );
		VK_Reset();
	}

	else if ( res != VK_SUCCESS && res != VK_SUBOPTIMAL_KHR )
	{
		// Classic typo must remain.
		VK_CheckResult( res, "Failed ot acquire swapchain image!" );
	}

	if ( gInFlightFences[ imageIndex ] != VK_NULL_HANDLE )
	{
		vkWaitForFences( VK_GetDevice(), 1, &gInFlightFences[ imageIndex ], VK_TRUE, UINT64_MAX );
	}

	gInFlightFences[ imageIndex ] = gFences[ gFrameIndex ];

	VkSubmitInfo submitInfo{};
	submitInfo.sType                        = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	VkSemaphore          waitSemaphores[]   = { gImageAvailableSemaphores[ gFrameIndex ] };
	VkSemaphore          signalSemaphores[] = { gRenderFinishedSemaphores[ gFrameIndex ] };
	VkPipelineStageFlags waitStages[]       = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

	submitInfo.waitSemaphoreCount           = ARR_SIZE( waitSemaphores );
	submitInfo.pWaitSemaphores              = waitSemaphores;
	submitInfo.pWaitDstStageMask            = waitStages;
	submitInfo.commandBufferCount           = 1;
	submitInfo.pCommandBuffers              = &gCommandBuffers[ imageIndex ];
	submitInfo.signalSemaphoreCount         = ARR_SIZE( signalSemaphores );
	submitInfo.pSignalSemaphores            = signalSemaphores;

	vkResetFences( VK_GetDevice(), 1, &gFences[ gFrameIndex ] );

	VK_CheckResult( vkQueueSubmit( VK_GetGraphicsQueue(), 1, &submitInfo, gFences[ gFrameIndex ] ), "Failed to submit draw command buffer!" );

	VkSwapchainKHR   swapChains[] = { VK_GetSwapchain() };

	VkPresentInfoKHR presentInfo{};
	presentInfo.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.pNext              = nullptr;
	presentInfo.waitSemaphoreCount = ARR_SIZE( signalSemaphores );
	presentInfo.pWaitSemaphores    = signalSemaphores;
	presentInfo.swapchainCount     = ARR_SIZE( swapChains );
	presentInfo.pSwapchains        = swapChains;
	presentInfo.pImageIndices      = &imageIndex;
	presentInfo.pResults           = nullptr;

	res                            = vkQueuePresentKHR( VK_GetGraphicsQueue(), &presentInfo );

	if ( res == VK_ERROR_OUT_OF_DATE_KHR || res == VK_SUBOPTIMAL_KHR )
	{
		printf( "VK_Reset - vkQueuePresentKHR\n" );
		vkDeviceWaitIdle( VK_GetDevice() );
		VK_Reset();
	}
	else if ( res != VK_SUCCESS && res != VK_SUBOPTIMAL_KHR )
	{
		VK_CheckResult( res, "Failed to present swapchain image!" );
	}

	vkQueueWaitIdle( VK_GetPresentQueue() );

	gFrameIndex = ( gFrameIndex + 1 ) % MAX_FRAMES_IN_FLIGHT;

	vkResetCommandPool( VK_GetDevice(), VK_GetPrimaryCommandPool(), 0 );
}

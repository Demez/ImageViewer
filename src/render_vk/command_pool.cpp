#include "platform.h"
#include "util.h"
#include "log.h"

#include "render_vk.h"


static VkCommandPool gSingleTime;
static VkCommandPool gPrimary;


void VK_CreateCommandPool( VkCommandPool& sCmdPool, VkCommandPoolCreateFlags sFlags )
{
	QueueFamilyIndices      q                = VK_FindQueueFamilies( VK_GetPhysicalDevice() );

	VkCommandPoolCreateInfo aCommandPoolInfo = {};
	aCommandPoolInfo.sType                   = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	aCommandPoolInfo.pNext                   = nullptr;
	aCommandPoolInfo.flags                   = sFlags;
	aCommandPoolInfo.queueFamilyIndex        = q.aGraphicsFamily;

	VK_CheckResult( vkCreateCommandPool( VK_GetDevice(), &aCommandPoolInfo, nullptr, &sCmdPool ), "Failed to create command pool!" );
}


void VK_DestroyCommandPool( VkCommandPool& srPool )
{
	vkDestroyCommandPool( VK_GetDevice(), srPool, nullptr );
}


void VK_ResetCommandPool( VkCommandPool& srPool, VkCommandPoolResetFlags sFlags )
{
	VK_CheckResult( vkResetCommandPool( VK_GetDevice(), srPool, sFlags ), "Failed to reset command pool!" );
}


VkCommandPool& VK_GetSingleTimeCommandPool()
{
	return gSingleTime;
}


VkCommandPool& VK_GetPrimaryCommandPool()
{
	return gPrimary;
}


void VK_SingleCommand( std::function< void( VkCommandBuffer ) > sFunc )
{
	VkCommandBufferAllocateInfo aCommandBufferAllocateInfo = {};
	aCommandBufferAllocateInfo.sType                       = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	aCommandBufferAllocateInfo.pNext                       = nullptr;
	aCommandBufferAllocateInfo.commandPool                 = VK_GetSingleTimeCommandPool();
	aCommandBufferAllocateInfo.level                       = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	aCommandBufferAllocateInfo.commandBufferCount          = 1;

	VkCommandBuffer aCommandBuffer;
	VK_CheckResult( vkAllocateCommandBuffers( VK_GetDevice(), &aCommandBufferAllocateInfo, &aCommandBuffer ), "Failed to allocate command buffer!" );

	VkCommandBufferBeginInfo aCommandBufferBeginInfo = {};
	aCommandBufferBeginInfo.sType                    = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	aCommandBufferBeginInfo.pNext                    = nullptr;
	aCommandBufferBeginInfo.flags                    = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	VK_CheckResult( vkBeginCommandBuffer( aCommandBuffer, &aCommandBufferBeginInfo ), "Failed to begin command buffer!" );

	sFunc( aCommandBuffer );

	VK_CheckResult( vkEndCommandBuffer( aCommandBuffer ), "Failed to end command buffer!" );

	VkSubmitInfo submitInfo{};
	submitInfo.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers    = &aCommandBuffer;

	vkQueueSubmit( VK_GetGraphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE );
	vkQueueWaitIdle( VK_GetGraphicsQueue() );

	vkFreeCommandBuffers( VK_GetDevice(), VK_GetSingleTimeCommandPool(), 1, &aCommandBuffer );
}


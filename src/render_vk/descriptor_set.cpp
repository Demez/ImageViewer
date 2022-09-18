#include "platform.h"
#include "util.h"
#include "log.h"

#include "render_vk.h"


static uint32_t                       gDescriptorCount = 128;
static uint32_t                       gDescriptorPoolSize = 128;
static VkDescriptorPool               gDescriptorPool;
static std::vector< VkDescriptorSet > gDescriptorSets;
static VkDescriptorSetLayout          gImageLayout;
static VkDescriptorSetLayout          gBufferLayout;


void VK_CreateDescriptorPool()
{
	VkDescriptorPoolSize aPoolSizes[] = {
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, gDescriptorPoolSize },
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, gDescriptorPoolSize }
	};

	VkDescriptorPoolCreateInfo aDescriptorPoolInfo = {};
	aDescriptorPoolInfo.sType                      = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	aDescriptorPoolInfo.poolSizeCount              = ARR_SIZE( aPoolSizes );
	aDescriptorPoolInfo.pPoolSizes                 = aPoolSizes;
	aDescriptorPoolInfo.maxSets                    = gDescriptorCount;

	VK_CheckResult( vkCreateDescriptorPool( VK_GetDevice(), &aDescriptorPoolInfo, nullptr, &gDescriptorPool ), "Failed to create descriptor pool!" );
}


// TODO: Rethink this
void VK_CreateDescriptorSetLayouts()
{
	VkDescriptorSetLayoutBinding aLayoutBindings[] = {
		{ 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, nullptr },
		{ 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT, nullptr }
	};

	VkDescriptorSetLayoutCreateInfo bufferLayout = {};
	bufferLayout.sType                           = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	bufferLayout.pNext                           = nullptr;
	bufferLayout.flags                           = 0;
	bufferLayout.bindingCount                    = 1;
	bufferLayout.pBindings                       = &aLayoutBindings[ 0 ];

	VK_CheckResult( vkCreateDescriptorSetLayout( VK_GetDevice(), &bufferLayout, nullptr, &gBufferLayout ), "Failed to create descriptor set layout!" );

	VkDescriptorSetLayoutCreateInfo imageLayout = {};
	imageLayout.sType                           = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	imageLayout.pNext                           = nullptr;
	imageLayout.flags                           = 0;
	imageLayout.bindingCount                    = 1;
	imageLayout.pBindings                       = &aLayoutBindings[ 1 ];

	VK_CheckResult( vkCreateDescriptorSetLayout( VK_GetDevice(), &imageLayout, nullptr, &gImageLayout ), "Failed to create descriptor set layout!" );
}


void VK_CreateDescSets()
{
	VK_CreateDescriptorPool();
	VK_CreateDescriptorSetLayouts();
}


void VK_DestroyDescSets()
{
	vkDestroyDescriptorSetLayout( VK_GetDevice(), gBufferLayout, nullptr );
	vkDestroyDescriptorSetLayout( VK_GetDevice(), gImageLayout, nullptr );
	vkDestroyDescriptorPool( VK_GetDevice(), gDescriptorPool, nullptr );
}


VkDescriptorPool VK_GetDescPool()
{
	return gDescriptorPool;
}


VkDescriptorSetLayout VK_GetDescImageSet()
{
	return gImageLayout;
}


VkDescriptorSetLayout VK_GetDescBufferSet()
{
	return gBufferLayout;
}


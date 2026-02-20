#include "descriptor_manager.h"
#include "log.h"
#include "scene.h"

namespace afre
{
	DescriptorManager::DescriptorManager
	(
		CleanupStack& cleanupStack, 
		const VkDevice& device, 
		const VkPhysicalDevice& physicalDevice, 
		const DescriptorManagerCreateInfo& descriptorManagerCreateInfo, 
		bool& success
	)
	{
		success = false;

		const uint32_t bindingCount = static_cast<uint32_t>(descriptorManagerCreateInfo.m_bindings.size());

		// Descriptor's layout set creation
		VkDescriptorSetLayoutCreateInfo descriptorSetLayoutInfo{};
		descriptorSetLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		descriptorSetLayoutInfo.bindingCount = bindingCount;

		std::vector<VkDescriptorSetLayoutBinding> descriptorSetLayoutBindings(bindingCount);

		for (uint32_t i = 0; i < bindingCount; i++)
		{
			VkDescriptorSetLayoutBinding descriptorSetLayoutBinding{};
			descriptorSetLayoutBinding.binding = i;
			descriptorSetLayoutBinding.descriptorCount = static_cast<uint32_t>(descriptorManagerCreateInfo.m_bindings[i].m_bufferSizes.size());
			descriptorSetLayoutBinding.descriptorType = descriptorManagerCreateInfo.m_bindings[i].m_descriptorType;
			descriptorSetLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

			descriptorSetLayoutBindings[i] = descriptorSetLayoutBinding;
		}

		descriptorSetLayoutInfo.pBindings = descriptorSetLayoutBindings.data();

		const VkResult descriptorSetLayoutResult = vkCreateDescriptorSetLayout(device, &descriptorSetLayoutInfo, nullptr, &m_descriptorSetLayout);

		cleanupStack.PushCleanup([=]()
			{
				vkDestroyDescriptorSetLayout(device, m_descriptorSetLayout, nullptr);
			});

		if (descriptorSetLayoutResult == VK_SUCCESS)
		{
			AFRE_INFO("Descriptor's set layout successfully created!");
		}
		else
		{
			AFRE_CRIT("Descriptor's set layout failed to create!");
		}

		// Buffer creation
		for (uint32_t b = 0; b < bindingCount; b++)
		{
			for (uint16_t bs = 0; bs < static_cast<uint16_t>(descriptorManagerCreateInfo.m_bindings[b].m_bufferSizes.size()); bs++)
			{
				DescriptorBuffer descriptorBuffer{};

				VkBufferCreateInfo bufferInfo{};
				bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
				bufferInfo.sharingMode = VkSharingMode::VK_SHARING_MODE_EXCLUSIVE;
				bufferInfo.usage = GetBufferUsage(descriptorManagerCreateInfo.m_bindings[b].m_descriptorType);
				bufferInfo.size = descriptorManagerCreateInfo.m_bindings[b].m_bufferSizes[bs];

				const VkResult bufferResult = vkCreateBuffer(device, &bufferInfo, nullptr, &descriptorBuffer.m_buffer);

				cleanupStack.PushCleanup([=]()
					{
						vkDestroyBuffer(device, descriptorBuffer.m_buffer, nullptr);
					});

				if (bufferResult == VK_SUCCESS)
				{
					AFRE_INFO(fmt::format("A buffer was created for (binding: {}, buffer: {})!", b, bs));
				}
				else
				{
					AFRE_CRIT(fmt::format("A buffer has failed to create for (binding: {}, buffer: {})!", b, bs));
				}

				VkMemoryRequirements bufferRequirements{};
				vkGetBufferMemoryRequirements(device, descriptorBuffer.m_buffer, &bufferRequirements);

				VkMemoryAllocateInfo memoryAllocateInfo{};
				memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
				memoryAllocateInfo.allocationSize = bufferRequirements.size;

				VkPhysicalDeviceMemoryProperties memoryProperties{};
				vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);

				const VkMemoryPropertyFlags requiredMemoryProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
				for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++)
				{
					if ((memoryProperties.memoryTypes[i].propertyFlags & requiredMemoryProperties) == requiredMemoryProperties)
					{
						memoryAllocateInfo.memoryTypeIndex = i;
					}
				}

				const VkResult allocateMemoryResult = vkAllocateMemory(device, &memoryAllocateInfo, nullptr, &descriptorBuffer.m_bufferMemory);

				cleanupStack.PushCleanup([=]()
					{
						vkFreeMemory(device, descriptorBuffer.m_bufferMemory, nullptr);
					});

				if (allocateMemoryResult == VK_SUCCESS)
				{
					AFRE_INFO(fmt::format("Successfully allocated memory for (binding: {}, buffer: {})!", b, bs));
				}
				else
				{
					AFRE_CRIT(fmt::format("Failed to allocate memory for (binding: {}, buffer: {})!", b, bs));
				}

				if (vkBindBufferMemory(device, descriptorBuffer.m_buffer, descriptorBuffer.m_bufferMemory, 0) == VK_SUCCESS)
				{
					AFRE_INFO(fmt::format("Successfully bound buffer memory for (binding: {}, buffer: {})!", b, bs));
				}
				else
				{
					AFRE_CRIT(fmt::format("Failed to bind buffer memory for (binding: {}, buffer: {})!", b, bs));
				}

				if (vkMapMemory(device, descriptorBuffer.m_bufferMemory, 0, descriptorManagerCreateInfo.m_bindings[b].m_bufferSizes[bs], 0, &descriptorBuffer.m_mappedBuffer) == VK_SUCCESS)
				{
					AFRE_INFO(fmt::format("Mapped buffer memory for (binding: {}, buffer: {})!", b, bs));
				}
				else
				{
					AFRE_CRIT(fmt::format("Failed to map buffer memory for (binding: {}, buffer: {})!", b, bs));
				}

				m_buffers.push_back(descriptorBuffer);
			}
		}

		// Descriptor pool creation
		VkDescriptorPoolCreateInfo descriptorPoolInfo{};
		descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		descriptorPoolInfo.maxSets = 1;
		descriptorPoolInfo.poolSizeCount = bindingCount;

		std::vector<VkDescriptorPoolSize> descriptorPoolSizes(bindingCount);

		for (uint32_t b = 0; b < bindingCount; b++)
		{
			VkDescriptorPoolSize descriptorPoolSize{};
			descriptorPoolSize.descriptorCount = static_cast<uint32_t>(descriptorManagerCreateInfo.m_bindings[b].m_bufferSizes.size());
			descriptorPoolSize.type = descriptorManagerCreateInfo.m_bindings[b].m_descriptorType;

			descriptorPoolSizes[b] = descriptorPoolSize;
		}

		descriptorPoolInfo.pPoolSizes = descriptorPoolSizes.data();

		const VkResult descriptorPoolResult = vkCreateDescriptorPool(device, &descriptorPoolInfo, nullptr, &m_descriptorPool);

		cleanupStack.PushCleanup([=]()
			{
				vkDestroyDescriptorPool(device, m_descriptorPool, nullptr);
			});

		if (descriptorPoolResult == VK_SUCCESS)
		{
			AFRE_INFO("Created descriptor pool!");
		}
		else
		{
			AFRE_CRIT("Failed to create the descriptor pool!");
		}

		// Descriptor sets allocation
		VkDescriptorSetAllocateInfo descriptorSetAllocateInfo{};
		descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		descriptorSetAllocateInfo.pSetLayouts = &m_descriptorSetLayout;
		descriptorSetAllocateInfo.descriptorSetCount = 1;
		descriptorSetAllocateInfo.descriptorPool = m_descriptorPool;

		if (vkAllocateDescriptorSets(device, &descriptorSetAllocateInfo, &m_descriptorSet) == VK_SUCCESS)
		{
			AFRE_INFO("Allocated descriptor sets!");
		}
		else
		{
			AFRE_CRIT("Failed to allocate descriptor sets!");
		}

		// Updating descriptor sets
		uint32_t bufferCount = 0;
		for (uint32_t b = 0; b < bindingCount; b++)
		{
			const uint16_t bufferSizeCount = static_cast<uint16_t>(descriptorManagerCreateInfo.m_bindings[b].m_bufferSizes.size());

			std::vector<VkDescriptorBufferInfo> descriptorBufferInfos(bufferSizeCount);

			for (uint16_t bs = 0; bs < bufferSizeCount; bs++)
			{
				VkDescriptorBufferInfo descriptorBufferInfo{};
				descriptorBufferInfo.buffer = m_buffers[bufferCount].m_buffer;
				descriptorBufferInfo.offset = 0;
				descriptorBufferInfo.range = descriptorManagerCreateInfo.m_bindings[b].m_bufferSizes[bs];

				descriptorBufferInfos[bs] = descriptorBufferInfo;

				bufferCount++;
			}

			VkWriteDescriptorSet descriptorWrite{};
			descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrite.dstSet = m_descriptorSet;
			descriptorWrite.dstBinding = b;
			descriptorWrite.dstArrayElement = 0;
			descriptorWrite.descriptorCount = bufferSizeCount;
			descriptorWrite.descriptorType = descriptorManagerCreateInfo.m_bindings[b].m_descriptorType;
			descriptorWrite.pBufferInfo = descriptorBufferInfos.data();

			vkUpdateDescriptorSets(device, 1, &descriptorWrite, 0, {});
		}

		// Pipeline layout creation
		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = 1;
		pipelineLayoutInfo.pSetLayouts = &m_descriptorSetLayout;

		const VkResult pipelineLayoutResult = vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &m_pipelineLayout);

		cleanupStack.PushCleanup([=]()
			{
				vkDestroyPipelineLayout(device, m_pipelineLayout, nullptr);
			});

		if (pipelineLayoutResult == VK_SUCCESS)
		{
			AFRE_INFO("Created the pipeline layout!");
		}
		else
		{
			AFRE_CRIT("Failed to create the pipeline layout!");
		}

		success = true;
	}

	void DescriptorManager::RegisterVoxelDataBufferUpdater(uint16_t bufferIndex)
	{
		m_bufferUpdaters.push_back([=]()
		{
			const auto& voxelDataView = g_scene.m_registry.view<VoxelData>();

			VoxelData* voxelData = &g_scene.m_registry.get<VoxelData>(voxelDataView.front());
			if (voxelData->SetVoxelData())
			{
				memcpy(m_buffers[bufferIndex].m_mappedBuffer, voxelData, sizeof(VoxelData));
			}
		});
	}

	VkBufferUsageFlagBits DescriptorManager::GetBufferUsage(VkDescriptorType descriptorType)
	{
		switch (descriptorType)
		{
		case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
			return VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
		case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
			return VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
		default:
			break;
		}

		return (VkBufferUsageFlagBits) 0;
	}
}
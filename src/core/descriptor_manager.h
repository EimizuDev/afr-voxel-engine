#pragma once

#include <vulkan/vulkan_core.h>
#include "cleanup_stack.h"
#include "buffer_data_types.h"

namespace afre
{
	struct DescriptorBindingInfo
	{
		VkDescriptorType m_descriptorType{};
		std::vector<VkDeviceSize> m_bufferSizes{};
	};

	struct DescriptorManagerCreateInfo
	{
		std::vector<DescriptorBindingInfo> m_bindings{};
	};

	class DescriptorBuffer
	{
	public:
		VkBuffer m_buffer{};
		VkDeviceMemory m_bufferMemory{};
		void* m_mappedBuffer{};
	};

	template<typename T>
	struct BufferData
	{
		T m_data;
		bool m_shouldCopy = false;

		BufferData();
	};

	class DescriptorManager
	{
	public:
		DescriptorManager() = default;
		DescriptorManager
		(
			CleanupStack& cleanupStack,
			const VkDevice& device,
			const VkPhysicalDevice& physicalDevice,
			const DescriptorManagerCreateInfo& descriptorManagerCreateInfo,
			bool& success
		);

		VkDescriptorSet m_descriptorSet{};
		VkPipelineLayout m_pipelineLayout{};

		std::vector<DescriptorBuffer> m_buffers{};

		std::vector<std::function<void()>> m_bufferUpdaters;

		template<typename T>
		void RegisterBufferUpdater(uint16_t bufferIndex)
		{
			m_bufferUpdaters.push_back([=]()
			{
				const BufferData<T> bufferData{};
				if (bufferData.m_shouldCopy)
				{
					memcpy(m_buffers[bufferIndex].m_mappedBuffer, &bufferData.m_data, sizeof(T));
				}
			});
		}

		// Exclusive buffer updater registers
		void RegisterVoxelDataBufferUpdater(uint16_t bufferIndex);

	private:
		VkBufferUsageFlagBits GetBufferUsage(VkDescriptorType descriptorType);

		VkDescriptorSetLayout m_descriptorSetLayout;
		VkDescriptorPool m_descriptorPool{};
	};
}

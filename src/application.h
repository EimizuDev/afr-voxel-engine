#pragma once

#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "core/descriptor_manager.h"
#include <VkBootstrap.h>

namespace afre
{
	#define ENGINE_VERSION VK_MAKE_VERSION(1, 0, 0)

	#define VERSION(major, minor, patch) VK_MAKE_VERSION(major, minor, patch)

	template<typename T>
	struct Result
	{
		bool m_success = false;
		T m_returnVal{};

		Result() = default;
		Result(T returnVal) : m_returnVal(returnVal), m_success(true) {}
	};

	class Application final
	{
	public:
		Application(char* appTitle, uint16_t windowWidth, uint16_t windowHeight, uint32_t appVersion);
		~Application();
	private:
		Result<vkb::Instance> InitInstance(char* appTitle, uint32_t appVersion);

		bool InitWindow(char* appTitle, uint16_t windowWidth, uint16_t windowHeight);

		Result<vkb::PhysicalDevice> InitPhysicalDevice(const vkb::Instance& instance);

		void AddPhysicalDeviceFeatures(vkb::DeviceBuilder& deviceBuilder);
		Result<vkb::Device> InitDevice(const vkb::PhysicalDevice& physicalDevice);
		
		bool GetQueue(const vkb::Device& device);

		bool InitSwapchain(const vkb::Device& device);
		bool GetImageViews(vkb::Swapchain& swapchain);

		bool SetupDescriptorManager(const VkPhysicalDevice& physicalDevice);

		bool InitPipeline();

		bool CreateCommandPool();
		bool AllocateCommandBuffer();

		bool CreateFence();

		void SetupCallbacks();

		void Run();

		void Draw();

		CleanupStack m_cleanupStack;

		GLFWwindow* m_window = nullptr;

		uint16_t m_windowWidth;
		uint16_t m_windowHeight;

		const uint32_t kMajorVulkanV = 1;
		const uint32_t kMinorVulkanV = 4;

		VkInstance m_instance;

		#ifdef AFRE_DEBUG
			VkDebugUtilsMessengerEXT m_debugMessenger;
		#endif

		VkSurfaceKHR m_surface;

		VkDevice m_device;

		VkQueue m_queue;

		VkSwapchainKHR m_swapchain;
		std::vector<VkImageView> m_imageViews;

		DescriptorManager m_descriptorManager;

		VkPipeline m_pipeline;

		VkCommandPool m_commandPool;
		VkCommandBuffer m_commandBuffer;

		VkFence m_fence;
	};
}
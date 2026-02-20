#include "application.h"
#include "log.h"
#include <fstream>
#include "core/events.h"
#include "scene.h"

namespace afre
{
	Application::Application(char* appTitle, uint16_t windowWidth, uint16_t windowHeight, uint32_t appVersion)
	{
		const Result<vkb::Instance> instanceResult = InitInstance(appTitle, appVersion);
		if (!instanceResult.m_success) return;
		const vkb::Instance instance = instanceResult.m_returnVal;

		if (!InitWindow(appTitle, windowWidth, windowHeight)) return;

		const Result<vkb::PhysicalDevice> physicalDeviceResult = InitPhysicalDevice(instance);
		if (!physicalDeviceResult.m_success) return;
		const vkb::PhysicalDevice physicalDevice = physicalDeviceResult.m_returnVal;

		const Result<vkb::Device> deviceResult = InitDevice(physicalDevice);
		if (!deviceResult.m_success) return;
		const vkb::Device device = deviceResult.m_returnVal;

		if (!GetQueue(device)) return;

		if (!InitSwapchain(device)) return;

		if (!SetupDescriptorManager(physicalDevice.physical_device)) return;

		if (!InitPipeline()) return;

		if (!CreateCommandPool()) return;

		if (!AllocateCommandBuffer()) return;

		if (!CreateFence()) return;

		SetupCallbacks();

		glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

		Run();
	}

	Application::~Application()
	{
		vkWaitForFences(m_device, 1, &m_fence, true, 1000000000);

		m_cleanupStack.StartCleanup();
	}

	Result<vkb::Instance> Application::InitInstance(char* appTitle, uint32_t appVersion)
	{
		std::vector<VkValidationFeatureEnableEXT> validationFeaturesList = { VK_VALIDATION_FEATURE_ENABLE_DEBUG_PRINTF_EXT };

		vkb::InstanceBuilder instBuilder;
		const vkb::Result<vkb::Instance> instanceResult =
			instBuilder
		#ifdef AFRE_DEBUG
			.enable_layer("VK_LAYER_KHRONOS_validation")
			.add_debug_messenger_severity(VkDebugUtilsMessageSeverityFlagBitsEXT::VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
			.use_default_debug_messenger()
			.add_validation_feature_enable(*validationFeaturesList.data())
		#endif
			.require_api_version(kMajorVulkanV, kMinorVulkanV)
			.set_app_name(appTitle)
			.set_app_version(appVersion)
			.set_engine_name("AFR Engine")
			.set_engine_version(ENGINE_VERSION)
			.build();

		#ifdef AFRE_DEBUG
			AFRE_WARN("Ignore the loader_get_json errors if you get them. They're harmless.");
		#endif

		vkb::Instance instance{};
		if (instanceResult.has_value())
		{
			AFRE_INFO("Successfully created a vulkan instance!");

			instance = instanceResult.value();

			m_instance = instance.instance;
		#ifdef AFRE_DEBUG
			m_debugMessenger = instance.debug_messenger;
		#endif

			m_cleanupStack.PushCleanup({ [=]()
				{
					// WARNING: Make sure this order is right!
				#ifdef AFRE_DEBUG
					vkb::destroy_debug_utils_messenger(m_instance, m_debugMessenger);
				#endif

					vkDestroyInstance(m_instance, nullptr);
				} });
		}
		else
		{
			AFRE_CRIT("Failed to create a vulkan instance!");
			return Result<vkb::Instance>();
		}

		return Result<vkb::Instance>(instance);
	}

	bool Application::InitWindow(char* appTitle, uint16_t windowWidth, uint16_t windowHeight)
	{
		if (glfwInit())
		{
			AFRE_INFO("GLFW has been initialized.");

			m_cleanupStack.PushCleanup(glfwTerminate);
		}
		else
		{
			AFRE_CRIT("GLFW has failed to initialize!");
			return false;
		}

		// Window creation
		m_windowWidth = windowWidth;
		m_windowHeight = windowHeight;

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

		if (m_window = glfwCreateWindow(m_windowWidth, m_windowHeight, appTitle, nullptr, nullptr))
		{
			AFRE_INFO("GLFW window has been initialized.");
		}
		else
		{
			AFRE_CRIT("GLFW window creation failed!");
			return false;
		}

		// Window surface creation
		const VkResult surfaceResult = glfwCreateWindowSurface(m_instance, m_window, nullptr, &m_surface);

		m_cleanupStack.PushCleanup([=]()
			{
				vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
			});

		if (surfaceResult == VK_SUCCESS)
		{
			AFRE_INFO("GLFW window surface for vulkan was successfully created!");
		}
		else
		{
			AFRE_CRIT("GLFW window surface for vulkan failed to create!");
			return false;
		}

		return true;
	}

	Result<vkb::PhysicalDevice> Application::InitPhysicalDevice(const vkb::Instance& instance)
	{
		const vkb::Result<vkb::PhysicalDevice> physicalDeviceResult = {
			vkb::PhysicalDeviceSelector{ instance, m_surface }
			.set_minimum_version(kMajorVulkanV, kMinorVulkanV)
			.add_required_extension(VK_KHR_SHADER_NON_SEMANTIC_INFO_EXTENSION_NAME)
			.select() };

		vkb::PhysicalDevice physicalDevice{};
		if (physicalDeviceResult.has_value())
		{
			AFRE_INFO("Found and selected a compatible physical device!");

			physicalDevice = physicalDeviceResult.value();
		}
		else
		{
			AFRE_CRIT("Failed to find a compatible physical device!");
			return Result<vkb::PhysicalDevice>();
		}

		return Result<vkb::PhysicalDevice>(physicalDevice);
	}

	void Application::AddPhysicalDeviceFeatures(vkb::DeviceBuilder& deviceBuilder)
	{
		VkPhysicalDeviceVulkan13Features physicalDeviceVulkan13Features{};
		physicalDeviceVulkan13Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
		physicalDeviceVulkan13Features.dynamicRendering = true;
		deviceBuilder = deviceBuilder.add_pNext(&physicalDeviceVulkan13Features);

		VkPhysicalDeviceVulkan12Features physicalDeviceVulkan12Features{};
		physicalDeviceVulkan12Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
		physicalDeviceVulkan12Features.uniformBufferStandardLayout = true;
		deviceBuilder = deviceBuilder.add_pNext(&physicalDeviceVulkan12Features);

		VkPhysicalDevice16BitStorageFeatures physicalDevice16BitStorageFeatures{};
		physicalDevice16BitStorageFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_16BIT_STORAGE_FEATURES;
		physicalDevice16BitStorageFeatures.uniformAndStorageBuffer16BitAccess = true;
		physicalDevice16BitStorageFeatures.storageBuffer16BitAccess = true;
		deviceBuilder = deviceBuilder.add_pNext(&physicalDevice16BitStorageFeatures);

		VkPhysicalDeviceFeatures physicalDeviceFeatures{};
		physicalDeviceFeatures.shaderInt16 = true;
		VkPhysicalDeviceFeatures2 physicalDeviceFeatures2{};
		physicalDeviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
		physicalDeviceFeatures2.features = physicalDeviceFeatures;
		deviceBuilder = deviceBuilder.add_pNext(&physicalDeviceFeatures2);
	}

	Result<vkb::Device> Application::InitDevice(const vkb::PhysicalDevice& physicalDevice)
	{
		vkb::DeviceBuilder deviceBuilder{ physicalDevice };

		AddPhysicalDeviceFeatures(deviceBuilder);

		const vkb::Result<vkb::Device> deviceResult = deviceBuilder.build();

		vkb::Device device{};
		if (deviceResult.has_value())
		{
			AFRE_INFO("A device has been created!");

			device = deviceResult.value();
			m_device = device.device;

			m_cleanupStack.PushCleanup([=]()
				{
					vkDestroyDevice(m_device, nullptr);
				});
		}
		else
		{
			AFRE_CRIT("Failed to create a device.");
			return Result<vkb::Device>();
		}

		return Result<vkb::Device>(device);
	}

	bool Application::GetQueue(const vkb::Device& device)
	{
		const vkb::Result<VkQueue> queueResult = device.get_queue(vkb::QueueType::graphics);

		if (queueResult.has_value())
		{
			AFRE_INFO("Successfully received a queue!");

			m_queue = queueResult.value();
		}
		else
		{
			AFRE_CRIT("Failed to get a queue!");
			return false;
		}

		return true;
	}

	bool Application::InitSwapchain(const vkb::Device& device)
	{
		const vkb::Result<vkb::Swapchain> swapchainResult{
			vkb::SwapchainBuilder{device}
			.set_desired_min_image_count(1)
			.set_image_usage_flags(VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)
			.set_desired_extent(m_windowWidth, m_windowHeight)
			.set_composite_alpha_flags(VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR)
			.set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
			.build()
		};

		if (swapchainResult.has_value())
		{
			AFRE_INFO("A swapchain was created! Getting image views...");

			vkb::Swapchain swapchain = swapchainResult.value();
			m_swapchain = swapchain.swapchain;

			m_cleanupStack.PushCleanup([=]()
				{
					vkDestroySwapchainKHR(m_device, m_swapchain, nullptr);
				});

			if (!GetImageViews(swapchain)) return false;
		}
		else
		{
			AFRE_CRIT("Failed to create a swapchain!");
			return false;
		}

		return true;
	}

	bool Application::GetImageViews(vkb::Swapchain& swapchain)
	{
		const vkb::Result<std::vector<VkImageView>> imageViewsResult = swapchain.get_image_views();

		if (imageViewsResult.has_value())
		{
			AFRE_INFO("Received the image views!");

			m_imageViews = imageViewsResult.value();

			m_cleanupStack.PushCleanup([=]()
				{
					for (uint16_t i = 0; i < m_imageViews.size(); i++)
					{
						vkDestroyImageView(m_device, m_imageViews[i], nullptr);
					}
				});
		}
		else
		{
			AFRE_CRIT("Failed to get the image views!", false);
			return false;
		}

		return true;
	}

	bool Application::SetupDescriptorManager(const VkPhysicalDevice& physicalDevice)
	{
		bool success = false;

		DescriptorBindingInfo uniformBinding{};
		uniformBinding.m_descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		uniformBinding.m_bufferSizes = { sizeof(CameraData) };

		DescriptorBindingInfo storageBinding{};
		storageBinding.m_descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		storageBinding.m_bufferSizes = { sizeof(VoxelData) };

		DescriptorManagerCreateInfo descriptorManagerCreateInfo{};
		descriptorManagerCreateInfo.m_bindings = { uniformBinding, storageBinding };

		m_descriptorManager = DescriptorManager{ m_cleanupStack, m_device, physicalDevice, descriptorManagerCreateInfo, success };

		// The order of the calls bufferIndex arg must match the m_bufferSizes added order.
		m_descriptorManager.RegisterBufferUpdater<CameraData>(0);
		m_descriptorManager.RegisterVoxelDataBufferUpdater(1);

		return success;
	}

	bool Application::InitPipeline()
	{
		VkGraphicsPipelineCreateInfo pipelineInfo{};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;

		std::ifstream shader{ fmt::format("{}/assets/build/shaders/slang.spv", std::getenv("AFR_ENGINE_PATH")), std::ios::binary | std::ios::ate };

		if (!shader.is_open())
		{
			AFRE_CRIT("Failed to open a shader file!");
			return false;
		}

		std::vector<char> shaderBuffer(shader.tellg());
		shader.seekg(std::ios::beg);
		shader.read(shaderBuffer.data(), static_cast<std::streamsize>(shaderBuffer.size()));

		shader.close();

		VkShaderModuleCreateInfo shaderModuleInfo{};
		shaderModuleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		shaderModuleInfo.codeSize = shaderBuffer.size();
		shaderModuleInfo.pCode = reinterpret_cast<const uint32_t*>(shaderBuffer.data());

		VkShaderModule shaderModule;
		const VkResult shaderResult = vkCreateShaderModule(m_device, &shaderModuleInfo, nullptr, &shaderModule);

		if (shaderResult == VK_SUCCESS)
		{
			AFRE_INFO("Shader module was created!");
		}
		else
		{
			AFRE_CRIT("Shader module failed to create!");
			return false;
		}

		VkPipelineShaderStageCreateInfo vertexShaderInfo{};
		vertexShaderInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vertexShaderInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vertexShaderInfo.pName = "VertMain";
		vertexShaderInfo.module = shaderModule;

		VkPipelineShaderStageCreateInfo fragShaderInfo{};
		fragShaderInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		fragShaderInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		fragShaderInfo.pName = "FragMain";
		fragShaderInfo.module = shaderModule;

		const VkPipelineShaderStageCreateInfo stagesInfo[] = { vertexShaderInfo, fragShaderInfo };
		pipelineInfo.stageCount = 2;
		pipelineInfo.pStages = stagesInfo;

		VkPipelineRasterizationStateCreateInfo rasterizationInfo{};
		rasterizationInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizationInfo.rasterizerDiscardEnable = false;
		rasterizationInfo.lineWidth = 1.f;
		rasterizationInfo.polygonMode = VK_POLYGON_MODE_FILL;
		rasterizationInfo.cullMode = VK_CULL_MODE_NONE;
		rasterizationInfo.depthBiasEnable = false;
		rasterizationInfo.depthClampEnable = false;
		pipelineInfo.pRasterizationState = &rasterizationInfo;

		VkPipelineViewportStateCreateInfo viewportInfo{};
		viewportInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportInfo.viewportCount = 1;

		VkViewport viewport{};
		viewport.width = m_windowWidth;
		viewport.height = m_windowHeight;
		viewportInfo.pViewports = &viewport;
		pipelineInfo.pViewportState = &viewportInfo;

		VkPipelineInputAssemblyStateCreateInfo pipelineInputAssemblyInfo{};
		pipelineInputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		pipelineInputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		pipelineInputAssemblyInfo.primitiveRestartEnable = false;
		pipelineInfo.pInputAssemblyState = &pipelineInputAssemblyInfo;

		pipelineInfo.layout = m_descriptorManager.m_pipelineLayout;

		const VkResult pipelineResult = vkCreateGraphicsPipelines(m_device, nullptr, 1, &pipelineInfo, nullptr, &m_pipeline);

		m_cleanupStack.PushCleanup([=]()
			{
				vkDestroyPipeline(m_device, m_pipeline, nullptr);
			});

		if (pipelineResult == VK_SUCCESS)
		{
			AFRE_INFO("Pipeline was created!");
		}
		else
		{
			AFRE_CRIT("Pipeline failed to create!");
			return false;
		}

		vkDestroyShaderModule(m_device, shaderModule, nullptr);

		return true;
	}

	bool Application::CreateCommandPool()
	{
		VkCommandPoolCreateInfo cmdPoolInfo{};
		cmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

		const VkResult cmdPoolResult = vkCreateCommandPool(m_device, &cmdPoolInfo, nullptr, &m_commandPool);

		m_cleanupStack.PushCleanup([=]()
			{
				vkDestroyCommandPool(m_device, m_commandPool, nullptr);
			});

		if (cmdPoolResult == VK_SUCCESS)
		{
			AFRE_INFO("A command pool was successfully created!");
		}
		else
		{
			AFRE_CRIT("Failed to create a command pool!");
			return false;
		}

		return true;
	}

	bool Application::AllocateCommandBuffer()
	{
		VkCommandBufferAllocateInfo cmdBufferInfo{};
		cmdBufferInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		cmdBufferInfo.commandPool = m_commandPool;
		cmdBufferInfo.commandBufferCount = 1;
		cmdBufferInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

		const VkResult cmdBufferResult = vkAllocateCommandBuffers(m_device, &cmdBufferInfo, &m_commandBuffer);

		if (cmdBufferResult == VK_SUCCESS)
		{
			AFRE_INFO("A command buffer was allocated!");
		}
		else
		{
			AFRE_CRIT("Failed to allocate a command buffer!");
			return false;
		}

		return true;
	}

	bool Application::CreateFence()
	{
		VkFenceCreateInfo fenceInfo{};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		const VkResult fenceResult = vkCreateFence(m_device, &fenceInfo, nullptr, &m_fence);

		m_cleanupStack.PushCleanup([=]()
			{
				vkDestroyFence(m_device, m_fence, nullptr);
			});

		if (fenceResult == VK_SUCCESS)
		{
			AFRE_INFO("Created a fence!");
		}
		else
		{
			AFRE_CRIT("Failed to create a fence!");
			return false;
		}

		return true;
	}

	void Application::SetupCallbacks()
	{
		glfwSetKeyCallback(m_window, KeyCallback);
		glfwSetCursorPosCallback(m_window, CursorPositionCallback);
	}

	void Application::Run()
	{
		while (!glfwWindowShouldClose(m_window))
		{
			glfwPollEvents();
			g_scene.Update();
			Draw();
		}
	}

	void Application::Draw()
	{
		vkWaitForFences(m_device, 1, &m_fence, true, 1000000000);
		vkResetFences(m_device, 1, &m_fence);

		VkCommandBufferBeginInfo cmdBufferBeginInfo{};
		cmdBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		cmdBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		for (uint16_t i = 0; i < static_cast<uint16_t>(m_descriptorManager.m_bufferUpdaters.size()); i++)
		{
			m_descriptorManager.m_bufferUpdaters[i]();
		}

		vkBeginCommandBuffer(m_commandBuffer, &cmdBufferBeginInfo);

		vkCmdBindPipeline(m_commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);

		vkCmdBindDescriptorSets(m_commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_descriptorManager.m_pipelineLayout, 0, 1, &m_descriptorManager.m_descriptorSet, 0, nullptr);

		VkRenderingInfo renderingInfo{};
		renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
		renderingInfo.colorAttachmentCount = 1;
		renderingInfo.layerCount = 1;
		renderingInfo.renderArea.extent = { m_windowWidth , m_windowHeight };

		VkRenderingAttachmentInfo renderAttachmentInfo{};
		renderAttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
		renderAttachmentInfo.clearValue = VkClearValue{ VkClearColorValue{0.f, 0.f, 0.f, 1.f} };
		renderAttachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		renderAttachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		renderAttachmentInfo.imageView = m_imageViews[0];
		renderAttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;

		renderingInfo.pColorAttachments = &renderAttachmentInfo;

		vkCmdBeginRendering(m_commandBuffer, &renderingInfo);

		vkCmdDraw(m_commandBuffer, 6, 1, 0, 0);

		vkCmdEndRendering(m_commandBuffer);

		vkEndCommandBuffer(m_commandBuffer);

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &m_commandBuffer;

		vkQueueSubmit(m_queue, 1, &submitInfo, m_fence);

		VkPresentInfoKHR presentInfo{};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.pSwapchains = &m_swapchain;
		presentInfo.swapchainCount = 1;

		const uint32_t imageIndex = 0;

		presentInfo.pImageIndices = &imageIndex;

		vkQueuePresentKHR(m_queue, &presentInfo);
	}
}
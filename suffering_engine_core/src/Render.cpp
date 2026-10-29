#include "Render.h"

constexpr uint32_t maxFramesInFlight{ 2 };
uint32_t imageIndex{ 0 };
uint32_t frameIndex{ 0 };
VkInstance instance{ VK_NULL_HANDLE };
VkDevice device{ VK_NULL_HANDLE };
VkQueue queue{ VK_NULL_HANDLE };
VkSurfaceKHR surface{ VK_NULL_HANDLE };
bool updateSwapchain{ false };
VkSwapchainKHR swapchain{ VK_NULL_HANDLE };
VkCommandPool commandPool{ VK_NULL_HANDLE };
VkPipeline pipeline{ VK_NULL_HANDLE };
VkPipelineLayout pipelineLayout{ VK_NULL_HANDLE };
VkImage depthImage;
VmaAllocator allocator{ VK_NULL_HANDLE };
VmaAllocation depthImageAllocation;
VkImageView depthImageView;
std::vector<VkImage> swapchainImages;
std::vector<VkImageView> swapchainImageViews;
std::array<VkCommandBuffer, maxFramesInFlight> commandBuffers;
std::array<VkFence, maxFramesInFlight> fences;
std::array<VkSemaphore, maxFramesInFlight> imageAcquiredSemaphores;
std::vector<VkSemaphore> renderCompleteSemaphores;
VmaAllocation vBufferAllocation{ VK_NULL_HANDLE };
VkBuffer vBuffer{ VK_NULL_HANDLE };

struct ShaderData {
	glm::mat4 projection;
	glm::mat4 view;
	glm::mat4 model[3];
	glm::vec4 lightPos{ 0.0f, -10.0f, 10.0f, 0.0f };
	uint32_t selected{ 1 };
} shaderData{};
struct ShaderDataBuffer {
	VmaAllocation allocation{ VK_NULL_HANDLE };
	VmaAllocationInfo allocationInfo{};
	VkBuffer buffer{ VK_NULL_HANDLE };
	VkDeviceAddress deviceAddress{};
};
std::array<ShaderDataBuffer, maxFramesInFlight> shaderDataBuffers;
struct Texture {
	VmaAllocation allocation{ VK_NULL_HANDLE };
	VkImage image{ VK_NULL_HANDLE };
	VkImageView view{ VK_NULL_HANDLE };
	VkSampler sampler{ VK_NULL_HANDLE };
};
std::array<Texture, 3> textures{};
VkDescriptorPool descriptorPool{ VK_NULL_HANDLE };
VkDescriptorSetLayout descriptorSetLayoutTex{ VK_NULL_HANDLE };
VkDescriptorSet descriptorSetTex{ VK_NULL_HANDLE };
Slang::ComPtr<slang::IGlobalSession> slangGlobalSession;
glm::vec3 camPos{ 0.0f, 0.0f, -6.0f };
glm::vec3 objectRotations[3]{};
glm::ivec2 windowSize{};
struct Vertex {
	glm::vec3 pos;
	glm::vec3 normal;
	glm::vec2 uv;
};


static inline void chk(VkResult result) {
	if (result != VK_SUCCESS) {
		std::cerr << "Vulkan call returned an error (" << result << ")\n";
		exit(result);
	}
}

static inline void chkSwapchain(VkResult result) {
	if (result < VK_SUCCESS) {
		if (result == VK_ERROR_OUT_OF_DATE_KHR) {
			updateSwapchain = true;
			return;
		}
		std::cerr << "Vulkan call returned an error (" << result << ")\n";
		exit(result);
	}
}

static inline void chk(bool result) {
	if (!result) {
		std::cerr << "Call returned an error\n";
		exit(result);
	}
}

int Render::SFR_Render(int argc, char* argv[])
{
	// App info Part
	VkApplicationInfo appInfo{ .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO, .pApplicationName = "Suffering engine render", .apiVersion = VK_API_VERSION_1_3 };
	uint32_t instanceExtensionsCount{ 0 };
	char const* const* instanceExtensions{ SDL_Vulkan_GetInstanceExtensions(&instanceExtensionsCount) };

	// VKInstence part
	VkInstanceCreateInfo instanceCI{
	.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
	.pApplicationInfo = &appInfo,
	.enabledExtensionCount = instanceExtensionsCount,
	.ppEnabledExtensionNames = instanceExtensions,
	};
	chk(vkCreateInstance(&instanceCI, nullptr, &instance));
	volkLoadInstance(instance);

	// VKPhysicalDevice part
	uint32_t deviceCount{ 0 };
	chk(vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr));
	std::vector<VkPhysicalDevice> devices(deviceCount);
	chk(vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data()));
	VkPhysicalDeviceProperties2 deviceProperties{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2 };
	uint32_t deviceIndex{ 0 };
	if (argc > 1) {
		deviceIndex = std::stoi(argv[1]);
		assert(deviceIndex < deviceCount);
	}
	vkGetPhysicalDeviceProperties2(devices[deviceIndex], &deviceProperties);
	std::cout << "Selected device: " << deviceProperties.properties.deviceName << "\n";

	// Queue family part
	uint32_t queueFamilyCount{ 0 };
	vkGetPhysicalDeviceQueueFamilyProperties(devices[deviceIndex], &queueFamilyCount, nullptr);
	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(devices[deviceIndex], &queueFamilyCount, queueFamilies.data());
	uint32_t queueFamily{ 0 };
	for (size_t i = 0; i < queueFamilies.size(); i++) {
		if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
			queueFamily = i;
			break;
		}
	}
	chk(SDL_Vulkan_GetPresentationSupport(instance, devices[deviceIndex], queueFamily));

	// Referencing queue
	const float qfpriorities{ 1.0f };
	VkDeviceQueueCreateInfo queueCI{
		.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
		.queueFamilyIndex = queueFamily,
		.queueCount = 1,
		.pQueuePriorities = &qfpriorities
	};

	// Logical Device part
	const std::vector<const char*> deviceExtensions{ VK_KHR_SWAPCHAIN_EXTENSION_NAME };

	VkPhysicalDeviceVulkan12Features enabledVk12Features{
	.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
	.descriptorIndexing = true,
	.shaderSampledImageArrayNonUniformIndexing = true,
	.descriptorBindingVariableDescriptorCount = true,
	.runtimeDescriptorArray = true,
	.bufferDeviceAddress = true
	};
	VkPhysicalDeviceVulkan13Features enabledVk13Features{
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
		.pNext = &enabledVk12Features,
		.synchronization2 = true,
		.dynamicRendering = true,
	};
	VkPhysicalDeviceFeatures enabledVk10Features{
		.samplerAnisotropy = VK_TRUE
	};
	VkDeviceCreateInfo deviceCI{
	.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
	.pNext = &enabledVk13Features,
	.queueCreateInfoCount = 1,
	.pQueueCreateInfos = &queueCI,
	.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size()),
	.ppEnabledExtensionNames = deviceExtensions.data(),
	.pEnabledFeatures = &enabledVk10Features
	};
	chk(vkCreateDevice(devices[deviceIndex], &deviceCI, nullptr, &device));
	vkGetDeviceQueue(device, queueFamily, 0, &queue); // queue creation

	// VMA part
	VmaVulkanFunctions vkFunctions{
	.vkGetInstanceProcAddr = vkGetInstanceProcAddr,
	.vkGetDeviceProcAddr = vkGetDeviceProcAddr,
	.vkCreateImage = vkCreateImage
	};
	VmaAllocatorCreateInfo allocatorCI{
		.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT,
		.physicalDevice = devices[deviceIndex],
		.device = device,
		.pVulkanFunctions = &vkFunctions,
		.instance = instance
	};
	chk(vmaCreateAllocator(&allocatorCI, &allocator));

	// Windows and surface part
	SDL_Window* window = SDL_CreateWindow("Suffering engine render", 1280u, 720u, SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
	chk(SDL_Vulkan_CreateSurface(window, instance, nullptr, &surface));
	chk(SDL_GetWindowSize(window, &windowSize.x, &windowSize.y));
	VkSurfaceCapabilitiesKHR surfaceCaps{};
	chk(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(devices[deviceIndex], surface, &surfaceCaps));

	// Swapchain part
	VkExtent2D swapchainExtent{ surfaceCaps.currentExtent };
	if (surfaceCaps.currentExtent.width == 0xFFFFFFFF) {
		swapchainExtent = { .width = static_cast<uint32_t>(windowSize.x), .height = static_cast<uint32_t>(windowSize.y) };
	}
	const VkFormat imageFormat{ VK_FORMAT_B8G8R8A8_SRGB };
	VkSwapchainCreateInfoKHR swapchainCI{
		.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
		.surface = surface,
		.minImageCount = surfaceCaps.minImageCount,
		.imageFormat = imageFormat,
		.imageColorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR,
		.imageExtent{.width = swapchainExtent.width, .height = swapchainExtent.height },
		.imageArrayLayers = 1,
		.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
		.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
		.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
		.presentMode = VK_PRESENT_MODE_FIFO_KHR
	};
	chk(vkCreateSwapchainKHR(device, &swapchainCI, nullptr, &swapchain));
	uint32_t imageCount{ 0 };
	chk(vkGetSwapchainImagesKHR(device, swapchain, &imageCount, nullptr));
	swapchainImages.resize(imageCount);
	chk(vkGetSwapchainImagesKHR(device, swapchain, &imageCount, swapchainImages.data()));
	swapchainImageViews.resize(imageCount);
	for (auto i = 0; i < imageCount; i++) {
		VkImageViewCreateInfo viewCI{ .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO, .image = swapchainImages[i], .viewType = VK_IMAGE_VIEW_TYPE_2D, .format = imageFormat, .subresourceRange{.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .levelCount = 1, .layerCount = 1 } };
		chk(vkCreateImageView(device, &viewCI, nullptr, &swapchainImageViews[i]));
	}

	// Depth attachment part

	return 0;
}

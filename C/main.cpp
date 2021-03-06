#define  GLFW_INCLUDE_VULKAN // This happens to load the vulkan headers for us.
#include <GLFW/glfw3.h>      // GLFW, for visual confirmation of correctness

//useful things
#include <cstring>
#include <set>
#include <iostream>
#include <fstream>
#include <chrono>
#include <thread>

// To handle errors in C++. 
#include <iostream>
#include <stdexcept>
#include <functional>
#include <cstdlib>

// Size of window/framebuffer, length of each test etc...

#define WIDTH 1920
#define HEIGHT 1080

const std::vector<const char*> validationLayers = {
	"VK_LAYER_LUNARG_standard_validation" // Does very basic checks on shaders etc.
	// "VK_LAYER_LUNARG_api_dump" No more dumping!
};

const std::vector<const char*> deviceExtensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME // Ability to output to a display(s buffer)
};

	//Callback register helper function
	VkResult CreateDebugUtilsMessengerEXT(
			VkInstance instance, 
			const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, 
			const VkAllocationCallbacks* pAllocator, 
			VkDebugUtilsMessengerEXT* pCallback){
	    auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
	    if (func != nullptr) {
	        return func(instance, pCreateInfo, pAllocator, pCallback);
	    } else {
	        return VK_ERROR_EXTENSION_NOT_PRESENT;
	    }
	}
	
	//Callback destruction helper function
	void DestroyDebugUtilsMessengerEXT(
			VkInstance instance, 
			VkDebugUtilsMessengerEXT callback, 
			const VkAllocationCallbacks* pAllocator){
		auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
		if (func != nullptr) {
			func(instance, callback, pAllocator);
		}
	}

	class ShaderTester {
	public: 

		// Open a window, set up the graphics card, render something, then close down.
		void run(std::string shader){
			initWindow();
			initVulkan(shader);
			mainLoop(shader);
			cleanup();
		}

	private:
		
		// The callback routine that any testbed should use.
		static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
				VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
				VkDebugUtilsMessageTypeFlagsEXT messageType,
				const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
				void* pUserData){
			std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

			return VK_FALSE;
		}

		// Loads shaders
		static std::vector<char> readFile(const std::string& filename) {
			std::ifstream file(filename, std::ios::ate | std::ios::binary);

			if (!file.is_open()) {
				throw std::runtime_error("failed to open file!");
			}

			size_t fileSize = (size_t) file.tellg();
			std::vector<char> buffer(fileSize);

			file.seekg(0);
			file.read(buffer.data(), fileSize);
			
			file.close();

			return buffer;

		}

		// All the class members
		// The window & surface
		VkSurfaceKHR surface;
		GLFWwindow* window;

		// Vulkan instance things
		VkInstance instance;
		VkDebugUtilsMessengerEXT callback;
		VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
		VkDevice lDevice;

		// Vulkan queue
		VkQueue graphicsQueue;
		VkQueue presentQueue;

		// Vulkan swapqueue
		VkSwapchainKHR swapChain;
		std::vector<VkImage> swapChainImages;
		std::vector<VkImageView> swapChainImageViews;
		VkFormat swapChainImageFormat;
		VkExtent2D swapChainExtent;

		// Vulkan Rendering
		VkRenderPass renderPass;
		VkPipelineLayout pipelineLayout;
		VkPipeline graphicsPipeline;
		
		// Drawing
		std::vector<VkFramebuffer> swapChainFramebuffers;
		VkCommandPool commandPool;
		std::vector<VkCommandBuffer> commandBuffers;
		
		//Synchronisation
		VkSemaphore imageAvailableSemaphore;
		VkSemaphore renderFinishedSemaphore;

		// Open a window, using the vulkan API for rendering, which is WIDTHxHEIGHT 
		// in size (and fixed size).
		void initWindow(){
			glfwInit();
			glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
			glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
			window = glfwCreateWindow(WIDTH, HEIGHT, "Fragment Shader", nullptr, nullptr);	
		}
			

			// Ensure validation layers exist.
		    bool checkValidationLayerSupport(){
				// Find supported validation layers, store in availableLayers				
				uint32_t layerCount;
				vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

			    std::vector<VkLayerProperties> availableLayers(layerCount);
			    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());
				
				// For each layer we want, check if it is supported.
				for (const char* layerName : validationLayers) {
					bool layerFound = false;

		    			for (const auto& layerProperties : availableLayers) {
				        	if (strcmp(layerName, layerProperties.layerName) == 0) {
								layerFound = true;
								break;
							}
						}

			    	if (!layerFound) {
						return false;
					}
				}

				return true;
			
			}

			// All the information the API needs to create an instance
			void createInstance(){
				if(!checkValidationLayerSupport()){
					throw std::runtime_error("requested validation layers not available.");
				}

				//Information about the program
				VkApplicationInfo appInfo = {};
					appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
					appInfo.pApplicationName = "Fragment Shader";
					appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
					appInfo.pEngineName = "No Engine";
					appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
					appInfo.apiVersion = VK_API_VERSION_1_0;

				//Information about the extensions we require
				VkInstanceCreateInfo createInfo = {};
					createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
					createInfo.pApplicationInfo = &appInfo;
					
					//GLFW automagically creates a struct containing the required extensions
					//My project needs not concern itself with extensions, for now at least.
					uint32_t glfwExtensionCount = 0;
					const char** glfwExtensions;

					glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

					// add the debug utilities extension.
					std::vector<const char*> extensions(glfwExtensions, glfwExtensions + 
							glfwExtensionCount);
					
					extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
					
					// Add the exensions
					createInfo.enabledExtensionCount = 
						static_cast<uint32_t>(extensions.size());
					createInfo.ppEnabledExtensionNames = extensions.data();

					// Add validation layers.
					createInfo.enabledLayerCount = 
						static_cast<uint32_t>(validationLayers.size());

					createInfo.ppEnabledLayerNames = validationLayers.data();

				// Create the instance (information, allocation, location)
				VkResult result = vkCreateInstance(&createInfo, nullptr, &instance);
				if(result != VK_SUCCESS) {
					throw std::runtime_error("failed to create instance!");
				}
			}

			void setupDebugCallback(){
				VkDebugUtilsMessengerCreateInfoEXT createInfo = {};
					createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
					// All warnings, errors etc, but not normal information
					createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT 
						| VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT 
						| VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
					createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
			   			| VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT;
						//| VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
					// Print the information
					createInfo.pfnUserCallback = debugCallback;
					// TODO: Pass a pointer to the relevant shader.
					createInfo.pUserData = nullptr;
				
				//Attempt to add the callback.	
				if(CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &callback) != VK_SUCCESS){
					throw std::runtime_error("failed to set up debug callback!");
				}
			}
				
				//Helps finding queue families on a device
				struct QueueFamilyIndices{
					std::optional<uint32_t> graphicsFamily;
					std::optional<uint32_t> presentFamily;

					bool isComplete(){
							return graphicsFamily.has_value() && presentFamily.has_value();
					}
				};

				// Find the Queuefamilies with graphics capabilities
				QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device){
					QueueFamilyIndices indices;
					
					//Get the queuefamilies
					uint32_t queueFamilyCount = 0;
					vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

					std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
					vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

					// Check each queuefamily to see if there is one suitable
					int i = 0;
					for(const auto & queueFamily : queueFamilies){
						VkBool32 presentSupport = false;
						vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

						if(queueFamily.queueCount > 0 && presentSupport){
							indices.presentFamily = i;
						}

						if(queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT){
							indices.graphicsFamily = i;
						}


						if(indices.isComplete()){
							break;
						}

						i++;
					}

					return indices;
				}

				bool checkDeviceExtensionSupport(VkPhysicalDevice device){
					//Get available extensions
					uint32_t extensionCount;
					vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
					std::vector<VkExtensionProperties> extensions(extensionCount);
					vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, extensions.data());
					
					// The required extensions in a set, so we can remove those we have on this device
					std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());
	
					for(const auto& extension : extensions){
						requiredExtensions.erase(extension.extensionName);
					}

					return requiredExtensions.empty();

				}

				//Check if a GPU is suitable
				bool isDeviceSuitable(VkPhysicalDevice device){
					QueueFamilyIndices indices = findQueueFamilies(device);

					if(!indices.isComplete()) return false;

					if(!checkDeviceExtensionSupport(device)) return false;
					
					bool swapChainAdequate = false;
					SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
					swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
					if(!swapChainAdequate) return false;

					return true;
				}			
	
				//Give a device a score. 
				int getDeviceScore(VkPhysicalDevice device){
					int score = 1;
					
					VkPhysicalDeviceProperties deviceProperties;
					vkGetPhysicalDeviceProperties(device, &deviceProperties);
					
					VkPhysicalDeviceFeatures deviceFeatures;
					vkGetPhysicalDeviceFeatures(device, &deviceFeatures);
										
					if(isDeviceSuitable(device)){
						return score;
					} else {
						return 0;
					}
				}
			
			// Choose a GPU!
			void selectPhysicalDevice(){
				// Get the devices
				uint32_t deviceCount = 0;
				uint32_t deviceScore = 0;
				
				vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

				std::vector<VkPhysicalDevice> devices(deviceCount);
				vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());	

				if(devices.size() == 0) {
					throw std::runtime_error("No GPUs found!");
				}
				
				// Select a device that is most suitable
				for(size_t i = 0; i < devices.size(); i++){
					VkPhysicalDevice device = devices[i];
					if(device != VK_NULL_HANDLE && getDeviceScore(device) > deviceScore){
						physicalDevice = device;
						deviceScore = getDeviceScore(device);
					}
				}				

				// Ensure a device is selected.
				if(physicalDevice == VK_NULL_HANDLE){
					throw std::runtime_error("No suitable GPUs found!");
				}
			}
			
			void createLogicalDevice(){
				// Multiple queues need to be created
				QueueFamilyIndices indices = findQueueFamilies(physicalDevice);

				std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
				std::set<uint32_t> uniqueQueueFamilies = {indices.graphicsFamily.value(), indices.presentFamily.value()};

				float queuePriority = 1.0f;
				for (uint32_t queueFamily : uniqueQueueFamilies) {
					VkDeviceQueueCreateInfo queueCreateInfo = {};
					queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
					queueCreateInfo.queueFamilyIndex = queueFamily;
					queueCreateInfo.queueCount = 1;
					queueCreateInfo.pQueuePriorities = &queuePriority;
					queueCreateInfos.push_back(queueCreateInfo);
				}

				// Not interested in any features at the moment
				VkPhysicalDeviceFeatures deviceFeatures = {};
				
				// Create a logical device
				VkDeviceCreateInfo createInfo = {};
				createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
				createInfo.pQueueCreateInfos = queueCreateInfos.data();
				createInfo.queueCreateInfoCount = 
					static_cast<uint32_t>(queueCreateInfos.size());
				createInfo.pEnabledFeatures = &deviceFeatures;
				//info on extensions and validation layers
				createInfo.enabledExtensionCount =
				   	static_cast<uint32_t>(deviceExtensions.size());
				createInfo.ppEnabledExtensionNames = deviceExtensions.data();

				createInfo.enabledLayerCount = 
					static_cast<uint32_t>(validationLayers.size());
				createInfo.ppEnabledLayerNames = validationLayers.data();
				
				if(vkCreateDevice(physicalDevice, &createInfo, nullptr, &lDevice) != VK_SUCCESS){
					throw std::runtime_error("Creating logical GPU failed!");
				}
			
				//Get the graphics queue	
				vkGetDeviceQueue(lDevice, indices.graphicsFamily.value(), 0, &graphicsQueue);

				//Get the present queue
				vkGetDeviceQueue(lDevice, indices.graphicsFamily.value(), 0, &presentQueue);	
			}
			
			struct SwapChainSupportDetails {
				VkSurfaceCapabilitiesKHR capabilities;
				std::vector<VkSurfaceFormatKHR> formats;
				std::vector<VkPresentModeKHR> presentModes;
			};

			SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device) {
				SwapChainSupportDetails details;
				
				// Get surface capabilities, nicely provided in a struct
				vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

				// Get the formats, not so nicely provided.
				uint32_t formatCount;
				vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

				if(formatCount != 0){
					details.formats.resize(formatCount);
					vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
				}

				// Get present modes
				uint32_t presentModeCount;
				vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

				if(presentModeCount != 0){
					details.presentModes.resize(presentModeCount);
					vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
				}

				return details;
			}

			VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats){
				// Select 24 bit sRGB, if we can choose anything
				if (availableFormats.size() == 1 && availableFormats[0].format == VK_FORMAT_UNDEFINED){
					return {VK_FORMAT_B8G8R8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};
				}

				// Select 24 bit sRGB, if we can choose it from a list
				for(const auto& availableFormat: availableFormats){
					if (availableFormat.format == VK_FORMAT_B8G8R8_UNORM && 
							availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR){
						return availableFormat;						
					}
				}

				// Select something if we can't have 24 bit sRGB
				return availableFormats[0];
			}

			VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR> availablePresentModes) {
				// Choose a present mode. We want to post as many frames as possible.
				// FIFO is guaranteed, but not fastest
				VkPresentModeKHR bestMode = VK_PRESENT_MODE_FIFO_KHR;

				for (const auto& availablePresentMode : availablePresentModes) {
					if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) { 
						return availablePresentMode;
					} else if (availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR) {
						bestMode = availablePresentMode;
					}
				}

					    return bestMode;
			}

			VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
				if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
					// This is almost always a sensible value
					return capabilities.currentExtent;
				} else {
					// We have an incredibly large extent. We want to clamp it to the size of the window.
					VkExtent2D actualExtent = {WIDTH, HEIGHT};

					actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
					actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));
					return actualExtent;
				}
			}

			void createSwapChain(){
				SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice);

				VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
				swapChainImageFormat = surfaceFormat.format;

				VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
				VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);
			swapChainExtent = extent;

				uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
			   
				if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
					imageCount = swapChainSupport.capabilities.maxImageCount;
				}

				//Make the swapchain
				VkSwapchainCreateInfoKHR createInfo = {};
				createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
				createInfo.surface = surface;
				createInfo.minImageCount = imageCount;
				createInfo.imageFormat = surfaceFormat.format;
				createInfo.imageColorSpace = surfaceFormat.colorSpace;
				createInfo.imageExtent = extent;
				createInfo.imageArrayLayers = 1; // this value is for fancy 3d stuff, we only need one view
				createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT; // We want to draw straight to the screen, no post-processing.

				//Handle communication with the swapchain
				QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
				uint32_t queueFamilyIndices[] = {indices.graphicsFamily.value(), indices.presentFamily.value()};

				if(indices.graphicsFamily != indices.presentFamily){
					createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT; //Simple, but inefficient, sharing across multiple queue families.
					createInfo.queueFamilyIndexCount = 2;
					createInfo.pQueueFamilyIndices = queueFamilyIndices;
				} else {
					createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE; //Simple, efficient, exclusive to one queue family
					createInfo.queueFamilyIndexCount = 0;
					createInfo.pQueueFamilyIndices = nullptr;
				}

				createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
				createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
				createInfo.presentMode = presentMode;
				createInfo.clipped = VK_FALSE; // Render it even if we can't see it, to prevent clipping affecting performance tests 
				createInfo.oldSwapchain = VK_NULL_HANDLE;

				if (vkCreateSwapchainKHR(lDevice, &createInfo, nullptr, &swapChain) != VK_SUCCESS) {
					throw std::runtime_error("failed to create swap chain!");
				}
				
				// Get the swap chain images
				vkGetSwapchainImagesKHR(lDevice, swapChain, &imageCount, nullptr);
				swapChainImages.resize(imageCount);
				vkGetSwapchainImagesKHR(lDevice, swapChain, &imageCount, swapChainImages.data());

			}

			void createSurface() {
				if(glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS){
					throw std::runtime_error("failed to create window surface!");
				}
			}

			void createImageViews(){
				swapChainImageViews.resize(swapChainImages.size());
				for(size_t i = 0; i < swapChainImages.size(); i++){
					VkImageViewCreateInfo createInfo = {};
					createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
					createInfo.image = swapChainImages[i];
					
					createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
					createInfo.format = swapChainImageFormat;
					
					// No fancy colour things
					createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
					createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
					createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
					createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

					// No fancy display stuff
					createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
					createInfo.subresourceRange.baseMipLevel = 0;
					createInfo.subresourceRange.levelCount = 1;
					createInfo.subresourceRange.baseArrayLayer = 0;
					createInfo.subresourceRange.layerCount = 1;

					if (vkCreateImageView(lDevice, &createInfo, nullptr, &swapChainImageViews[i]) != VK_SUCCESS) {
				    	throw std::runtime_error("failed to create image views!");
					}	
				}
			}
			
			VkShaderModule createShaderModule(const std::vector<char>& code){
				VkShaderModuleCreateInfo createInfo = {};
				createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
				createInfo.codeSize = code.size();
				createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

				VkShaderModule shaderModule;
				if(vkCreateShaderModule(lDevice, &createInfo, nullptr, &shaderModule) != VK_SUCCESS){
					throw std::runtime_error("failed to create shader module");
				}

				return shaderModule;
			}
			
			void createRenderPass(){
				// The attachment (output location)
				VkAttachmentDescription colourAttachment = {};
				colourAttachment.format = swapChainImageFormat;
				colourAttachment.samples = VK_SAMPLE_COUNT_1_BIT; // No multisampling
				colourAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
				colourAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
				colourAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE; 	// No stencilling
				colourAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;  // No stencilling
				colourAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
				colourAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
				
				// Subpass dependencies let us pipeline each frame.
				VkSubpassDependency dependency = {};
				dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
				dependency.dstSubpass = 0;
				//Wait for...
				dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
				dependency.srcAccessMask = 0;
				//So we can...
				dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
				dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

				// one subpass that does the rendering
				VkAttachmentReference colourAttachmentRef = {};
				colourAttachmentRef.attachment = 0;
				colourAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

				// How to use the rendering pass in the pipeline.
				VkSubpassDescription subpass = {};
				subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
				subpass.colorAttachmentCount = 1;
				subpass.pColorAttachments = & colourAttachmentRef;

				// Build the render pass
				VkRenderPassCreateInfo renderPassInfo = {};
				renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
				renderPassInfo.attachmentCount = 1;
				renderPassInfo.pAttachments = &colourAttachment;
				renderPassInfo.subpassCount = 1;
				renderPassInfo.pSubpasses = &subpass;
				renderPassInfo.dependencyCount = 1;
				renderPassInfo.pDependencies = &dependency;

				if (vkCreateRenderPass(lDevice, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
					    throw std::runtime_error("failed to create render pass!");
				}
			}

			void createGraphicsPipeline(std::string shader){
				//Vertex shader
				auto vertShaderCode = readFile("./vert.spv");
				VkShaderModule vertShader = createShaderModule(vertShaderCode);
				
				VkPipelineShaderStageCreateInfo vertShaderInfo = {};
				vertShaderInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
				vertShaderInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
				vertShaderInfo.module = vertShader;
				vertShaderInfo.pName = "main";
				
				//Fragment shader
				auto fragShaderCode = readFile(shader);
				VkShaderModule fragShader = createShaderModule(fragShaderCode);
				
				VkPipelineShaderStageCreateInfo fragShaderInfo = {};
				fragShaderInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
				fragShaderInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
				fragShaderInfo.module = fragShader;
				fragShaderInfo.pName = "main";
				
				VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderInfo, fragShaderInfo};

				// Input no vertices
				VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
				vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
				vertexInputInfo.vertexBindingDescriptionCount = 0;
				vertexInputInfo.pVertexBindingDescriptions = nullptr;
				vertexInputInfo.vertexAttributeDescriptionCount = 0;
				vertexInputInfo.pVertexAttributeDescriptions = nullptr; 
				
				VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
				inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
				inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
				inputAssembly.primitiveRestartEnable = VK_FALSE;

				// Create a viewport
				VkViewport viewport = {};
				viewport.x = 0.0f;
				viewport.y = 0.0f;
				viewport.width = (float) swapChainExtent.width;
				viewport.height = (float) swapChainExtent.height;
				viewport.minDepth = 0.0f;
				viewport.maxDepth = 1.0f;

				VkRect2D scissor = {};
				scissor.offset = {0, 0};
				scissor.extent = swapChainExtent;

				VkPipelineViewportStateCreateInfo viewportState = {};
				viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
				viewportState.viewportCount = 1;
				viewportState.pViewports = &viewport;
				viewportState.scissorCount = 1;
				viewportState.pScissors = &scissor;

				//Rasteriser
				
				VkPipelineRasterizationStateCreateInfo rasterizer = {};
				rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
				rasterizer.depthClampEnable = VK_FALSE;
				rasterizer.rasterizerDiscardEnable = VK_FALSE;
				rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
				rasterizer.lineWidth = 1.0f;
				
				rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
				rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;

				rasterizer.depthBiasEnable = VK_FALSE;
				
				// No multisampling.
				VkPipelineMultisampleStateCreateInfo multisampling = {};
				multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
				multisampling.sampleShadingEnable = VK_FALSE;
				multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
				
				// No colour blending
				VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
				colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT 
													| VK_COLOR_COMPONENT_G_BIT 
													| VK_COLOR_COMPONENT_B_BIT 
													| VK_COLOR_COMPONENT_A_BIT;
				colorBlendAttachment.blendEnable = VK_FALSE;

				VkPipelineColorBlendStateCreateInfo colorBlending = {};
				colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
				colorBlending.logicOpEnable = VK_FALSE;
				colorBlending.logicOp = VK_LOGIC_OP_COPY;
				colorBlending.attachmentCount = 1;
				colorBlending.pAttachments = &colorBlendAttachment;

				// Build the pipeline
				VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
				pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;


				if (vkCreatePipelineLayout(lDevice, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
					throw std::runtime_error("failed to create pipeline layout!");
				}
				
				VkGraphicsPipelineCreateInfo pipelineInfo = {};
				pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
				pipelineInfo.stageCount = 2;
				pipelineInfo.pStages = shaderStages;
				pipelineInfo.pVertexInputState = &vertexInputInfo;
				pipelineInfo.pInputAssemblyState = &inputAssembly;
				pipelineInfo.pViewportState = &viewportState;
				pipelineInfo.pRasterizationState = &rasterizer;
				pipelineInfo.pMultisampleState = &multisampling;
				pipelineInfo.pColorBlendState = &colorBlending;

				pipelineInfo.layout = pipelineLayout;
				pipelineInfo.renderPass = renderPass;
				pipelineInfo.subpass = 0;

				if (vkCreateGraphicsPipelines(lDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS) {
					throw std::runtime_error("failed to create graphics pipeline!");
				}
				
				// Clean up the shaders.
				vkDestroyShaderModule(lDevice, vertShader, nullptr);
				vkDestroyShaderModule(lDevice, fragShader, nullptr);
			}
			
			void createFrameBuffers(){
				swapChainFramebuffers.resize(swapChainImageViews.size());

				for(size_t i = 0; i < swapChainImageViews.size(); i++){
					VkImageView attachments[] = {
						swapChainImageViews[i]
					};

					VkFramebufferCreateInfo frameBufferInfo = {};
					frameBufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
					frameBufferInfo.renderPass = renderPass;
					frameBufferInfo.attachmentCount = 1;
					frameBufferInfo.pAttachments = attachments;
					frameBufferInfo.width = swapChainExtent.width;
					frameBufferInfo.height = swapChainExtent.height;
					frameBufferInfo.layers = 1;

					if (vkCreateFramebuffer(lDevice, &frameBufferInfo, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS) {
						throw std::runtime_error("failed to create framebuffer!");
					}
				}
			}

			void createCommandPool(){
				QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice);

				VkCommandPoolCreateInfo poolInfo = {};
				poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
				poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

				if (vkCreateCommandPool(lDevice, &poolInfo, nullptr, &commandPool) != VK_SUCCESS) {
					throw std::runtime_error("failed to create command pool!");
				}
			}


			void createCommandBuffers() {
				//Create command buffers
				commandBuffers.resize(swapChainFramebuffers.size());

				VkCommandBufferAllocateInfo allocInfo = {};
				allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
				allocInfo.commandPool = commandPool;
				allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
				allocInfo.commandBufferCount = (uint32_t) commandBuffers.size();


				if (vkAllocateCommandBuffers(lDevice, &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
					throw std::runtime_error("failed to allocate command buffers!");
				}

				//Begin recording the buffers
				
				for (size_t i = 0; i < commandBuffers.size(); i++) {
					VkCommandBufferBeginInfo beginInfo = {};
					beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
					beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT; //Allows beginning rendering the next frame.
					
					if (vkBeginCommandBuffer(commandBuffers[i], &beginInfo) != VK_SUCCESS) {
						throw std::runtime_error("failed to begin recording command buffer!");
					}
					
					//Commands for the command buffer
					VkRenderPassBeginInfo renderPassInfo = {};
					renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
					renderPassInfo.renderPass = renderPass;
					renderPassInfo.framebuffer = swapChainFramebuffers[i];

					renderPassInfo.renderArea.offset = {0, 0};
					renderPassInfo.renderArea.extent = swapChainExtent;

					VkClearValue clearColor = {0.0f, 0.0f, 0.0f, 1.0f}; //When clearing values use black.
					renderPassInfo.clearValueCount = 1;
					renderPassInfo.pClearValues = &clearColor;

					// Add render pass to command buffer
					vkCmdBeginRenderPass(commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
					// Bind render pass to the pipeline
					vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
					// Draw things.
					vkCmdDraw(commandBuffers[i], 6, 1, 0, 0);
					// Finish the rendering.
					vkCmdEndRenderPass(commandBuffers[i]);

					if (vkEndCommandBuffer(commandBuffers[i]) != VK_SUCCESS) {
						throw std::runtime_error("failed to record command buffer!");
					}
				}
			}

			void createSemaphores(){
				VkSemaphoreCreateInfo semaphoreInfo = {};
				semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

				if (vkCreateSemaphore(lDevice, &semaphoreInfo, nullptr, &imageAvailableSemaphore) != VK_SUCCESS ||
					vkCreateSemaphore(lDevice, &semaphoreInfo, nullptr, &renderFinishedSemaphore) != VK_SUCCESS){
					throw std::runtime_error("failed to create semaphores!");
				}
			}

		void initVulkan(std::string shader){
			createInstance();
			setupDebugCallback();
			createSurface();

			selectPhysicalDevice();
			createLogicalDevice();
			
			createSwapChain();
			createImageViews();
			
			createRenderPass();
			createGraphicsPipeline(shader);
			
			createFrameBuffers();
			createCommandPool();
			createCommandBuffers();

			createSemaphores();
		}

			void drawFrame(){
				uint32_t imageIndex;
				//Take an image from the swapchain, once available
				vkAcquireNextImageKHR(lDevice, swapChain, std::numeric_limits<uint64_t>::max(),
						imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);

				//Render an image, once needed
				VkSubmitInfo submitInfo = {};
				submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

				VkSemaphore waitSemaphores[] = {imageAvailableSemaphore};
				VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
				submitInfo.waitSemaphoreCount = 1;
				submitInfo.pWaitSemaphores = waitSemaphores;
				submitInfo.pWaitDstStageMask = waitStages;

				submitInfo.commandBufferCount = 1;
				submitInfo.pCommandBuffers = &commandBuffers[imageIndex];
				
				VkSemaphore signalSemaphores[] = {renderFinishedSemaphore};
				submitInfo.signalSemaphoreCount = 1;
				submitInfo.pSignalSemaphores = signalSemaphores;
				
				if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS) {
					    throw std::runtime_error("failed to submit draw command buffer!");
				}
				
				//Put an image onto the swapchain
				
				VkPresentInfoKHR presentInfo = {};
				presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
				
				presentInfo.waitSemaphoreCount = 1;
				presentInfo.pWaitSemaphores = signalSemaphores;
				
				VkSwapchainKHR swapChains[] = {swapChain};
				presentInfo.swapchainCount = 1;
				presentInfo.pSwapchains = swapChains;
				presentInfo.pImageIndices = &imageIndex;
				
			    vkQueuePresentKHR(presentQueue, &presentInfo);
				vkQueueWaitIdle(presentQueue);
			}

		void mainLoop(std::string shader){
			clock_t time = clock();
			std::ofstream dataFile;
			dataFile.open(shader.append(".data"));
			while (!glfwWindowShouldClose(window)) {
				glfwPollEvents();
				drawFrame();
				float frameTime_us = float(clock() - time) * 1000000.0 / CLOCKS_PER_SEC;
				dataFile << std::to_string(frameTime_us) << "\n";
				time = clock();
			}
			
			dataFile.close();
			vkDeviceWaitIdle(lDevice);
		}
		
		void cleanup(){
			//Synchronisation
			vkDestroySemaphore(lDevice, renderFinishedSemaphore, nullptr);
			vkDestroySemaphore(lDevice, imageAvailableSemaphore, nullptr);

			//Drawing
			vkDestroyCommandPool(lDevice, commandPool, nullptr);
			for (auto framebuffer : swapChainFramebuffers) {
				vkDestroyFramebuffer(lDevice, framebuffer, nullptr);
			}

			//Destroy pipeline
			vkDestroyPipeline(lDevice, graphicsPipeline, nullptr);
			vkDestroyPipelineLayout(lDevice, pipelineLayout, nullptr);
			vkDestroyRenderPass(lDevice, renderPass, nullptr);
			
			//Destroy swapchain
			for(auto imageView : swapChainImageViews){
				vkDestroyImageView(lDevice, imageView, nullptr);
			}
			vkDestroySwapchainKHR(lDevice, swapChain, nullptr);
			
			//Destroy the vulkan instance
			vkDestroyDevice(lDevice, nullptr);
			
			DestroyDebugUtilsMessengerEXT(instance, callback, nullptr);
			
			vkDestroySurfaceKHR(instance, surface, nullptr);
			vkDestroyInstance(instance, nullptr);

			//Close the window
		    glfwDestroyWindow(window);
			glfwTerminate();	
		}
	};

	//Create and run all the tests
	int main(int argc, char *argv[]){	
    	
		ShaderTester testbed;

		try {
			testbed.run(argv[1]);
		} catch (const std::exception& e) {
			std::cerr << e.what() << std::endl;
			return EXIT_FAILURE;
		}

		return EXIT_SUCCESS;
	}

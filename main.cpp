#define  GLFW_INCLUDE_VULKAN // This happens to load the vulkan headers for us.
#include <GLFW/glfw3.h>      // GLFW, for visual confirmation of correctness

#include <cstring>
#include <set>

// To handle errors in C++. 
#include <iostream>
#include <stdexcept>
#include <functional>
#include <cstdlib>

// Size of window/framebuffer, length of each test etc...

#define WIDTH 1920
#define HEIGHT 1080

const std::vector<const char*> validationLayers = {
	"VK_LAYER_LUNARG_core_validation" // Does very basic checks on shaders etc.
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
		void run(){
			initWindow();
			initVulkan();
			mainLoop();
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

		// All the class members
		VkSurfaceKHR surface;
		GLFWwindow* window;
		VkInstance instance;
		VkDebugUtilsMessengerEXT callback;
		VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
		VkDevice lDevice;
		VkQueue graphicsQueue;
		VkQueue presentQueue;
		VkSwapchainKHR swapChain;

		
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
					createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
			   		//	| VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT 
					//	| VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
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
	
				//Give a device a score. TODO: make sure it's deterministic
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
				for(int i = 0; i < devices.size(); i++){
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
				vkGetDeviceQueue(lDevice, indices.graphicsFamily.value(), 0, &graphicsQueue);	
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
					if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {   // Figure out best framerate TODO
						bestMode = availablePresentMode;
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
				VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
				VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

				uint32_t imageCount = swapChainSupport.capabilities.minImageCount; // Draw as many frames as possible, to test performance

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
			}

			void createSurface() {
				if(glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS){
					throw std::runtime_error("failed to create window surface!");
				}
			}

		void initVulkan(){
			createInstance();
			setupDebugCallback();
			createSurface();
			selectPhysicalDevice();
			createLogicalDevice();
			createSwapChain();
		}

		void mainLoop(){
			//TODO: Change this to handle data collection etc...
			
			while (!glfwWindowShouldClose(window)) {
				glfwPollEvents();
			}
		}
		
		void cleanup(){
			vkDestroySwapchainKHR;
			vkDestroyDevice(lDevice, nullptr);
			DestroyDebugUtilsMessengerEXT(instance, callback, nullptr);
			
			vkDestroySurfaceKHR(instance, surface, nullptr);
			vkDestroyInstance(instance, nullptr);

		    glfwDestroyWindow(window);
			glfwTerminate();	
		}
	};

	//Create and run all the tests
	int main(){
    	ShaderTester testbed;

		try {
			testbed.run();
		} catch (const std::exception& e) {
			std::cerr << e.what() << std::endl;
			return EXIT_FAILURE;
		}

		return EXIT_SUCCESS;
	}

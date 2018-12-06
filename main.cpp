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
		}

		void mainLoop(){
			//TODO: Change this to handle data collection etc...
			
			while (!glfwWindowShouldClose(window)) {
				glfwPollEvents();
			}
		}
		
		void cleanup(){
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

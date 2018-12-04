#define  GLFW_INCLUDE_VULKAN // This happens to load the vulkan headers for us.
#include <GLFW/glfw3.h>      // GLFW, for visual confirmation of correctness

#include <cstring>

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
		
		static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
				VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
				VkDebugUtilsMessageTypeFlagsEXT messageType,
				const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
				void* pUserData){
			std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

			return VK_FALSE;
		}


		GLFWwindow* window;
		VkInstance instance;
		VkDebugUtilsMessengerEXT callback;

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

		void initVulkan(){
			createInstance();
			setupDebugCallback();	
		}

		void mainLoop(){
			//TODO: Change this to handle data collection etc...
			
			while (!glfwWindowShouldClose(window)) {
				glfwPollEvents();
			}
		}
		
		void cleanup(){
			DestroyDebugUtilsMessengerEXT(instance, callback, nullptr);

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

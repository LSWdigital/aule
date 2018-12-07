#Headers
VULKAN_SDK_PATH = ./VulkanSDK/1.1.70.0/x86_64/

# Compiler and linker flags
CFLAGS = -std=c++17 -I$(VULKAN_SDK_PATH)/include
LDFLAGS = -L$(VULKAN_SDK_PATH)/lib `pkg-config --static --libs glfw3` -lvulkan

VulkanTest: main.cpp
	    g++ $(CFLAGS) -o VulkanTest main.cpp $(LDFLAGS) -g


.PHONY: test clean

test: VulkanTest
	    ./VulkanTest

clean:
	    rm -f VulkanTest

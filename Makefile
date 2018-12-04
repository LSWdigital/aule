#Headers
VULKAN_SDK_PATH = /usr/

# Compiler and linker flags
CFLAGS = -std=c++17 -I$(VULKAN_SDK_PATH)/include
LDFLAGS = -L$(VULKAN_SDK_PATH)/lib `pkg-config --static --libs glfw3` -lvulkan

VulkanTest: main.cpp
	    g++ $(CFLAGS) -o VulkanTest main.cpp $(LDFLAGS)


.PHONY: test clean

test: VulkanTest
	    ./VulkanTest

clean:
	    rm -f VulkanTest

#Headers
VULKAN_SDK_PATH = /usr

# Compiler and linker flags
CFLAGS = -std=c++17 -I$(VULKAN_SDK_PATH)/include
LDFLAGS = -L$(VULKAN_SDK_PATH)/lib `pkg-config --static --libs glfw3` -lvulkan

Aule: main.cpp
	    g++ $(CFLAGS) -o Aule main.cpp $(LDFLAGS)
	    glslc ../shader.frag -o frag.spv --target-env=vulkan1.0
	    glslc ../quad.vert -o vert.spv --target-env=vulkan1.0

.PHONY: test clean

test:
	    ./Aule frag.spv

clean:
	    rm -f Aule
	    rm -f *.spv

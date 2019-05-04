

#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 fragColor;

layout(location = 0) out vec4 outColor;

void main() {
	vec3 colour;
	if(fragColor.x > 0.5){
		colour = vec3(1.0, 0.0, 0.0);
	} else {
		colour = fragColor;
	}
	outColor = vec4(colour, 1.0);
	
}



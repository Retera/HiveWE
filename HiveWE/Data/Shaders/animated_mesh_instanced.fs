#version 450 core

layout (binding = 0) uniform sampler2D image;
layout (location = 3) uniform float alpha_test;

in vec4 vertexColor;

in vec2 UV;

out vec4 outColor;

void main() {
	outColor = texture(image, UV) * vertexColor;

	if (outColor.a < alpha_test) {
		discard;
	}
}
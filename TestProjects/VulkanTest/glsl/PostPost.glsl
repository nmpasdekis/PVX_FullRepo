#version 440

layout(binding = 0) uniform sampler2D Color;
layout(binding = 1) uniform sampler2D Bloom;

uniform ivec2 ScreenSize;

in vec4 gl_FragCoord;

out vec4 outColor;

void main() {
	vec2 UV = gl_FragCoord.xy / ScreenSize;
	vec3 clr = texture(Color, UV).xyz + texture(Bloom, UV).xyz;
	outColor = vec4(pow(clr / (clr + vec3(1.0)), vec3(1.0/2.2)), 1.0);
}
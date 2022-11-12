#version 460
layout(binding = 0) uniform sampler2D Atlas;

in vec2 uv;
in vec4 TextColor;
out vec4 Color;

void main() {
	Color = vec4(TextColor.xyz, TextColor.w * texture(Atlas, uv).r);
	//Color = TextColor;
}
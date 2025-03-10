#version 440

layout(binding = 0) uniform sampler2D Src;

uniform ivec2 ScreenSize;

in vec4 gl_FragCoord;

out vec4 outColor;
// 	0.20236		0.179044	0.124009	0.067234	0.028532
void main() {
	float pixelSize = 1.0 / textureSize(Src, 0).x;
	vec2 UV = gl_FragCoord.xy / ScreenSize;
	
	vec3 sum =
		texture(Src, UV + vec2(pixelSize * 0, 0)).xyz * 0.20236 +

		texture(Src, UV + vec2(pixelSize * 1, 0)).xyz * 0.179044 +
		texture(Src, UV - vec2(pixelSize * 1, 0)).xyz * 0.179044 +

		texture(Src, UV + vec2(pixelSize * 2, 0)).xyz * 0.124009 +
		texture(Src, UV - vec2(pixelSize * 2, 0)).xyz * 0.124009 +

		texture(Src, UV + vec2(pixelSize * 3, 0)).xyz * 0.067234 +
		texture(Src, UV - vec2(pixelSize * 3, 0)).xyz * 0.067234 +

		texture(Src, UV + vec2(pixelSize * 4, 0)).xyz * 0.028532 +
		texture(Src, UV - vec2(pixelSize * 4, 0)).xyz * 0.028532;


	outColor = vec4(sum, 1.0);
}
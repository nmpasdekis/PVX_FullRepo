#version 460
layout(binding = 0) uniform sampler3D Atlas;

in vec3 uvw;

out vec4 Color;
out vec3 Position;
out vec3 Normal;
out vec2 PBR;

void main() {
	vec4 clr = texture(Atlas, vec3(uvw.xy, 1 - pow(uvw.z, 0.2)));
	Color = vec4(clr.xyz, clr.w * (1.0 - uvw.z) * 0.2);
	Position = vec3(0);
	Normal = vec3(0);
	PBR = vec2(1, 0);
}
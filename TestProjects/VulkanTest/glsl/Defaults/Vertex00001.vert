#version 440

layout(location = 1) in vec4 Position;
layout(location = 2) in vec3 Normal;


layout(location = 0) out Vert_t{
	vec4 Position;
	vec3 Normal;
	vec2 UV;
	vec4 Tangent;
} outVert;

struct MorphItem_t {
	vec4 Position;
};

layout(std140, binding = 0) uniform Camera{
	mat4 View;
	mat4 Projection;
	vec3 CameraPosition;
};

layout(std430, binding = 3) readonly buffer Transform {
	mat4 Model[];
};

layout(std430, binding = 4) readonly buffer MorphControl {
	float MorphWeight[];
};

layout(std430, binding = 5) readonly buffer MorphData {
	MorphItem_t[] MorphItem;
};

layout(std140, binding = 6) uniform MorphBones{
	int MorphCount;
	int BoneCount;
};

void main(){
	gl_Position = Projection * View * (outVert.Position = Model[gl_InstanceIndex] * Position);
	outVert.Normal = mat3(Model[gl_InstanceIndex]) * Normal;
}
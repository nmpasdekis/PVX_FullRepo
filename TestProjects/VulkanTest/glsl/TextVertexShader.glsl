#version 460

struct Data {
	vec4 textColor;
	mat4 Transform;
};

layout(std430, binding = 0) readonly buffer Text{
	Data DrawCall[];
};

layout(location = 0) in vec2 pos;
layout(location = 1) in vec2 UV;
layout(location = 2) in vec2 charUV;
in int gl_InstanceID;

out vec4 TextColor;
out vec2 uv;

void main() {
	Data dt = DrawCall[gl_DrawID];
	uv = UV + charUV;
	TextColor = dt.textColor;
	gl_Position = dt.Transform * vec4(pos + vec2(gl_InstanceID, 0), 0, 1);
}
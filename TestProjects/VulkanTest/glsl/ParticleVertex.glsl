#version 460

layout(std140, binding = 0) uniform Camera{
	mat4 View;
	mat4 Projection;
	vec3 CameraPosition;
};

layout(location = 0) in vec4 pos;
layout(location = 1) in vec2 UV;

layout(location = 2) in vec3 Position;
layout(location = 3) in float Rotation;
layout(location = 4) in vec2 Life;
layout(location = 5) in vec2 Scale;

out vec3 uvw;

void main() {
	float cr = cos(Rotation);
	float sr = sin(Rotation);
	mat4 model = mat4(
		View[0][0], View[1][0], View[2][0], 0,
		View[0][1], View[1][1], View[2][1], 0,
		View[0][2], View[1][2], View[2][2], 0,
		Position.x, Position.y, Position.z, 1
	);
	mat4 rot = mat4(
		cr * Scale.x, sr * Scale.y, 0, 0,
		-sr * Scale.x, cr * Scale.y,0, 0,
		0, 0, 1.0, 0,
		0, 0, 0, 1.0
	);
	gl_Position = Projection * View * model * rot * pos;
	uvw = vec3(UV, 1.0 - Life.x / Life.y);
}
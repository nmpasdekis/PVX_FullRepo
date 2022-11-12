(() => ({
	shaders: [
		{
			type: "VERTEX_SHADER",
			code: `
attribute vec2 Position;

varying vec2 oTex;

void main(){
	gl_Position = vec4(Position, 0, 1);
	oTex = Position;
}`.trim()
		},
		{
			type: "FRAGMENT_SHADER",
			code: `
precision mediump float;
uniform mat4 view;
uniform mat4 projection;
uniform sampler2D sampler0;

varying vec2 oTex;

const float M_PI = 3.1415926535897932384626433832795;

void main(){
	vec3 tmp = normalize(vec3(-oTex.x / projection[0][0], -oTex.y / projection[1][1], 1)*mat3(view));
	gl_FragColor = texture2D(sampler0, vec2(- 0.5 * atan(tmp.x, tmp.z)/M_PI, acos(tmp.y)/M_PI));
}`.trim()
		},
	],
	primitive: "TRIANGLES",
	attributes: [
		{ name: "Position", type: "FLOAT", size: 2 }
	],
	vertices: [
		-1.0, -1.0,
		1.0, -1.0,
		1.0, 1.0,
		-1.0, 1.0
	],
	indices: [
		0, 1, 2,
		0, 2, 3
	],
	textures: [
		{ sampler: "sampler0", file: "/content/images/engine room 1.jpg" }
	]
}))()
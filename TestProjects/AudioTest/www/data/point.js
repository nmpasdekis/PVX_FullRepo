(() => ({
	shaders: [
		{
			type: "VERTEX_SHADER",
			code: `
uniform mat4 view;
uniform mat4 projection;
uniform vec3 Position;
attribute vec3 Pos;

void main(){
	gl_PointSize = 5.0;
	gl_Position = projection * view * vec4(Position + Pos, 1);
}`.trim()
		},
		{
			type: "FRAGMENT_SHADER",
			code: `
precision mediump float;
uniform vec4 Color;
void main(){
	gl_FragColor = Color;
}`.trim()
		},
	],
	primitive: "POINTS",
	attributes: [
		{ name: "Pos", type: "FLOAT", size: 3 }
	],
	vertices: [ 0, 0, 0 ],
	indices: [0]
}))()
(() => ({
	...(MakeSphereUV(32, 64)),
	shaders: [
		{ 
			type: "VERTEX_SHADER", 
			code: `
uniform mat4 view;
uniform mat4 projection;

attribute vec3 Position;
attribute vec2 UV;

varying vec2 oTex;

void main(){
gl_Position = projection * view * vec4(Position, 1);
oTex = UV;
}`.trim()
		},
		{ 
			type: "FRAGMENT_SHADER", 
			code: `
precision mediump float;
uniform sampler2D sampler0;

varying vec2 oTex;

void main(){
gl_FragColor = texture2D(sampler0, oTex);
}`.trim()   
		},
	],
	textures:[
		{ sampler: "sampler0", file: "/content/images/engine room 1.jpg" }
	]
}))()
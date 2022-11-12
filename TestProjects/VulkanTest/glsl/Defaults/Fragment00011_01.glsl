#version 440

layout(location = 0) out vec4 Color;
layout(location = 1) out vec3 Position;
layout(location = 2) out vec3 Normal;
layout(location = 3) out vec2 PBR;

in Vert_t{
	vec4 Position;
	vec3 Normal;
	vec2 UV;
	vec4 Tangent;
} inVert;


layout(std140, binding = 0) uniform Camera{
	mat4 View;
	mat4 Projection;
	vec3 CameraPosition;
};

layout(std140, binding = 2) uniform Material{
	vec4 MatColor;
	vec4 MatPBR;
};

mat3 pvx_tangentRotation;

vec3 textureTangent(sampler2D normTex, vec2 UV){
	return pvx_tangentRotation * normalize(texture(normTex, UV).xyz);
}

uniform sampler2D Color_Tex;




void Fragment() {
	Normal = normalize(inVert.Normal);
	Color = texture(Color_Tex, inVert.UV) * MatColor;
	PBR = MatPBR.xy;
}


void main(){
	{
		vec3 norm = normalize(inVert.Normal);
		vec3 bin = normalize(cross(inVert.Tangent.xyz, norm) * inVert.Tangent.w);
		vec3 tang = cross(norm, bin);
		pvx_tangentRotation = mat3(tang, bin, norm);
	}
	Position = inVert.Position.xyz;
	Fragment();
}
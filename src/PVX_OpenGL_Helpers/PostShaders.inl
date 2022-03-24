namespace {

#define FUNCTIONS R"GLSL(const float PI = 3.14159265359;

vec3 fresnelSchlick(float cosTheta, vec3 F0) {
	//return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
	return F0 + (1.0 - F0) * pow(max(1.0 - cosTheta, 0.0), 5.0);
}

float DistributionGGX(vec3 N, vec3 H, float roughness) {
	float a2 = roughness*roughness;
	a2 *= a2;
	float NdotH = max(dot(N, H), 0.0);
	float NdotH2 = NdotH * NdotH;

	float denom = (NdotH2 * (a2 - 1.0) + 1.0);
	denom = PI * denom * denom;

	return a2 / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness) {
	float r = (roughness + 1.0);
	float k = (r*r) / 8.0;

	float num = NdotV;
	float denom = NdotV * (1.0 - k) + k;

	return num / denom;
}
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
	float NdotV = max(dot(N, V), 0.0);
	float NdotL = max(dot(N, L), 0.0);
	float ggx2 = GeometrySchlickGGX(NdotV, roughness);
	float ggx1 = GeometrySchlickGGX(NdotL, roughness);

	return ggx1 * ggx2;
})GLSL"

	const char* POSTPROCESSLIGHTS_GLSL = R"GLSL(#version 440

layout(binding = 0) uniform sampler2D PositionTex;
layout(binding = 1) uniform sampler2D ColorTex;
layout(binding = 2) uniform sampler2D NormalTex;
layout(binding = 3) uniform sampler2D MaterialTex;

layout(std140, binding = 0) uniform Camera{
	mat4 View;
	mat4 Projection;
	vec4 CameraPosition;
};

layout(std140, binding = 1) uniform Lights{
	float Atten3;
	float Atten2;
	float Atten1;
	int LightCount;
	struct {
		vec4 Position;
		vec4 Color;
	} Light[16];
};

in vec2 UV;

out vec4 Color;

)GLSL"


FUNCTIONS


R"GLSL(

void main() {
	vec4 Attenuation = vec4(Atten3, Atten2, Atten1, 1.0);

	ivec2 txc = ivec2(gl_FragCoord.xy);
	vec3 n0 = texelFetch(NormalTex, txc, 0).xyz;
	vec3 albedo = texelFetch(ColorTex, txc, 0).xyz;
	vec4 Material = texelFetch(MaterialTex, txc, 0);

	//vec3 n0 = texture(NormalTex, UV).xyz;
	//vec3 albedo = texture(ColorTex, UV).xyz;
	//vec4 Material = texture(MaterialTex, UV);

	const float metallic = Material.x;
	const float roughness = Material.y;
	const float ao = 1.0;

	if (dot(n0, n0)>0.01) {
		vec3 N = normalize(n0);
		//vec3 WorldPos = texture(PositionTex, UV).xyz;
		vec3 WorldPos = texelFetch(PositionTex, txc, 0).xyz;

		vec3 V = normalize(CameraPosition.xyz - WorldPos);

		vec3 F0 = mix(vec3(0.04), albedo, metallic);

		// reflectance equation
		vec3 Lo = vec3(0.0);
		for (int i = 0; i < LightCount; i++) {
			// calculate per-light radiance
			vec3 L = -Light[i].Position.xyz;
			float attenuation = 1.0;
			if (Light[i].Position.w!=0) {
				L = Light[i].Position.xyz - WorldPos;
				float distance = length(Light[i].Position.xyz - WorldPos);
				attenuation = 1.0 / dot(Attenuation, vec4(distance * distance * distance, distance * distance, distance, 1.0));
			}
			L = normalize(L);
			vec3 H = normalize(V + L);
			vec3 radiance = Light[i].Color.xyz * attenuation;

			// cook-torrance brdf
			float NDF = DistributionGGX(N, H, 1-roughness);
			float G = GeometrySmith(N, V, L, 1-roughness);
			vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);

			vec3 kS = F;
			vec3 kD = vec3(1.0) - kS;
			kD *= 1.0 - metallic;

			vec3 numerator = NDF * G * F;
			float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0);
			vec3 specular = numerator / max(denominator, 0.001);

			// add to outgoing radiance Lo
			float NdotL = max(dot(N, L), 0.0);
			Lo += (kD * albedo / PI + specular) * radiance * NdotL;
		}

		vec3 ambient = vec3(0.03) * albedo * ao;
		albedo = ambient + Lo;

		albedo = albedo / (albedo + vec3(1.0));
		albedo = pow(albedo, vec3(1.0/2.2));
	}
	Color = vec4(albedo, 1.0);
})GLSL";

	const char* POSTPROCESSLIGHTS_PREPOST_GLSL = R"GLSL(#version 440

layout(binding = 0) uniform sampler2D PositionTex;
layout(binding = 1) uniform sampler2D ColorTex;
layout(binding = 2) uniform sampler2D NormalTex;
layout(binding = 3) uniform sampler2D MaterialTex;

layout(std140, binding = 0) uniform Camera{
	mat4 View;
	mat4 Projection;
	vec4 CameraPosition;
};

layout(std140, binding = 1) uniform Lights{
	float Atten3;
	float Atten2;
	float Atten1;
	int LightCount;
	struct {
		vec4 Position;
		vec4 Color;
	} Light[16];
};

in vec2 UV;

out vec4 Color;
out vec4 Bloom;

)GLSL"


FUNCTIONS


R"GLSL(

void main() {
	vec4 Attenuation = vec4(Atten3, Atten2, Atten1, 1.0);

	ivec2 txc = ivec2(gl_FragCoord.xy);
	vec3 n0 = texelFetch(NormalTex, txc, 0).xyz;
	vec3 albedo = texelFetch(ColorTex, txc, 0).xyz;
	vec4 Material = texelFetch(MaterialTex, txc, 0);

	const float metallic = Material.x;
	const float roughness = Material.y;
	const float ao = 1.0;

	if (dot(n0, n0)>0.01) {
		vec3 N = normalize(n0);
		vec3 WorldPos = texelFetch(PositionTex, txc, 0).xyz;

		vec3 V = normalize(CameraPosition.xyz - WorldPos);

		vec3 F0 = mix(vec3(0.04), albedo, metallic);

		// reflectance equation
		vec3 Lo = vec3(0.0);
		for (int i = 0; i < LightCount; i++) {
			// calculate per-light radiance
			vec3 L = -Light[i].Position.xyz;
			float attenuation = 1.0;
			if (Light[i].Position.w!=0) {
				L = Light[i].Position.xyz - WorldPos;
				float distance = length(Light[i].Position.xyz - WorldPos);
				attenuation = 1.0 / dot(Attenuation, vec4(distance * distance * distance, distance * distance, distance, 1.0));
			}
			L = normalize(L);
			vec3 H = normalize(V + L);
			vec3 radiance = Light[i].Color.xyz * attenuation;

			// cook-torrance brdf
			float NDF = DistributionGGX(N, H, 1 - roughness);
			float G = GeometrySmith(N, V, L, 1 - roughness);
			vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);

			vec3 kS = F;
			vec3 kD = vec3(1.0) - kS;
			kD *= 1.0 - metallic;

			vec3 numerator = NDF * G * F;
			float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0);
			vec3 specular = numerator / max(denominator, 0.001);

			// add to outgoing radiance Lo
			float NdotL = max(dot(N, L), 0.0);
			Lo += (kD * albedo / PI + specular) * radiance * NdotL;
		}

		vec3 ambient = vec3(0.03) * albedo * ao;
		albedo = ambient + Lo;
	}

	//float bloom = dot(albedo, vec3(0.2126, 0.7152, 0.0722));
	float bloom = dot(albedo, vec3(0.3, 0.59, 0.11)*0.3);
	
	if (bloom>1.0) {
		Bloom = vec4(albedo, 1.0);
		Color = vec4(albedo, 1.0);
	} else {
		Bloom = vec4(0);
		Color = vec4(albedo, 1.0);
	}
})GLSL";

const char* POSTPOST_GLSL = R"GLSL(#version 440

layout(binding = 0) uniform sampler2D Color;
layout(binding = 1) uniform sampler2D Bloom;

in vec2 UV;

out vec4 outColor;

void main() {
	vec3 clr = texture(Color, UV).xyz + texture(Bloom, UV).xyz;
	clr = clr / (clr + vec3(1.0));
	//clr = vec3(1.0) - exp(-clr * 10.1));
	outColor = vec4(pow(clr, vec3(1.0/2.2)), 1.0);
})GLSL";


const char* GAUSSIANBLUR_H_GLSL = R"GLSL(#version 440

layout(binding = 0) uniform sampler2D Src;

out vec4 outColor;

ivec2 scrSize;

vec3 Fetch(vec2 Offset){
	return texelFetch(Src, ivec2(clamp(gl_FragCoord.xy+Offset, vec2(0), scrSize)),0).xyz;
}

// 	0.20236		0.179044	0.124009	0.067234	0.028532
void main() {
	scrSize = textureSize(Src, 0).xy;
	
	vec3 sum =
		Fetch(vec2(-4, 0)) * 0.028532 +
		Fetch(vec2(-3, 0)) * 0.067234 +
		Fetch(vec2(-2, 0)) * 0.124009 +
		Fetch(vec2(-1, 0)) * 0.179044 +
		Fetch(vec2( 0, 0)) * 0.20236 +
		Fetch(vec2( 1, 0)) * 0.179044 +
		Fetch(vec2( 2, 0)) * 0.124009 +
		Fetch(vec2( 3, 0)) * 0.067234 +
		Fetch(vec2( 4, 0)) * 0.028532;

	outColor = vec4(sum, 1.0);
})GLSL";

const char* GAUSSIANBLUR_V_GLSL = R"GLSL(#version 440

layout(binding = 0) uniform sampler2D Src;

out vec4 outColor;

ivec2 scrSize;

vec3 Fetch(vec2 Offset){
	return texelFetch(Src, ivec2(clamp(gl_FragCoord.xy+Offset, vec2(0), scrSize)),0).xyz;
}

// 	0.20236		0.179044	0.124009	0.067234	0.028532
void main() {
	scrSize = textureSize(Src, 0).xy;
	
	vec3 sum =
		Fetch(vec2(0,-4)) * 0.028532 +
		Fetch(vec2(0,-3)) * 0.067234 +
		Fetch(vec2(0,-2)) * 0.124009 +
		Fetch(vec2(0,-1)) * 0.179044 +
		Fetch(vec2(0, 0)) * 0.20236 +
		Fetch(vec2(0, 1)) * 0.179044 +
		Fetch(vec2(0, 2)) * 0.124009 +
		Fetch(vec2(0, 3)) * 0.067234 +
		Fetch(vec2(0, 4)) * 0.028532;

	outColor = vec4(sum, 1.0);
})GLSL";
}
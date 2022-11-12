#version 440

layout(binding = 0) uniform sampler2D PositionTex;
layout(binding = 1) uniform sampler2D ColorTex;
layout(binding = 2) uniform sampler2D NormalTex;
layout(binding = 3) uniform sampler2D MetallicRoughness;

uniform ivec2 ScreenSize;

layout(std140, binding = 0) uniform Camera{
	mat4 View;
	mat4 Projection;
	vec4 CameraPosition;
};

layout(std140, binding = 1) uniform Lights{
	int LightCount;
	vec4 Attenuation;
	struct {
		vec4 Position;
		vec4 Color;
	} Light[128];
};

//in vec2 UV;
in vec4 gl_FragCoord;

out vec4 Color;

const float PI = 3.14159265359;

vec3 fresnelSchlick(float cosTheta, vec3 F0) {
	return F0 + (1.0 - F0) * pow(max(1.0 - cosTheta, 0.0), 5.0);
}

float DistributionGGX(vec3 N, vec3 H, float roughness) {
	float a = roughness*roughness;
	float a2 = a*a;
	float NdotH = max(dot(N, H), 0.0);
	float NdotH2 = NdotH*NdotH;

	float num = a2;
	float denom = (NdotH2 * (a2 - 1.0) + 1.0);
	denom = PI * denom * denom;

	return num / denom;
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
}

void main() {
	vec2 UV = gl_FragCoord.xy / ScreenSize;

	vec3 n0 = texture(NormalTex, UV).xyz;
	vec3 albedo = texture(ColorTex, UV).xyz;

	vec4 mr = texture(MetallicRoughness, UV);
	const float metallic = mr.x;
	const float roughness = mr.y;
	const float ao = 1.0;

	if (dot(n0, n0)!=0) {
		vec3 N = normalize(n0);
		vec3 WorldPos = texture(PositionTex, UV).xyz;



		vec3 V = normalize(CameraPosition.xyz - WorldPos);

		vec3 F0 = vec3(0.04);
		F0 = mix(F0, albedo, metallic);

		// reflectance equation
		vec3 Lo = vec3(0.0);
		for (int i = 0; i < LightCount; i++) {
			// calculate per-light radiance
			vec3 L = -Light[i].Position.xyz;
			float attenuation = 1.0;
			if (Light[i].Position.w!=0) {
				L = Light[i].Position.xyz - WorldPos;
				float distance = length(Light[i].Position.xyz - WorldPos);
				attenuation = 1.0 / dot(Attenuation, vec4(distance * distance * distance, distance * distance, distance, 1));
			}
			L = normalize(L);
			vec3 H = normalize(V + L);
			vec3 radiance = Light[i].Color.xyz * attenuation;

			// cook-torrance brdf
			float NDF = DistributionGGX(N, H, roughness);
			float G = GeometrySmith(N, V, L, roughness);
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
}
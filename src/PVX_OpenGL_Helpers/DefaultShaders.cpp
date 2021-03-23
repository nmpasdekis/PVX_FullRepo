#include <PVX.inl>
#include <string>
#include <sstream>
#include "Include/PVX_OpenGL_Helpers.h"
#include <PVX_OpenGL.h>

#define HasPos		(Format&int(ItemUsage::ItemUsage_Position))
#define HasNorm		(Format&int(ItemUsage::ItemUsage_Normal))
#define HasUV		(Format&int(ItemUsage::ItemUsage_UV))
#define HasTan		(Format&int(ItemUsage::ItemUsage_Tangent))
#define HasWght		(Format&int(ItemUsage::ItemUsage_Weight))

#define HasMPos		(Format&int(ItemUsage::ItemUsage_MorphPosition))
#define HasMNorm	(Format&int(ItemUsage::ItemUsage_MorphNormal))
#define HasMUV		(Format&int(ItemUsage::ItemUsage_MorphUV))
#define HasMTan		(Format&int(ItemUsage::ItemUsage_MorphTangent))

#define HasMorph	(HasMPos || HasMNorm || HasMUV || HasMTan)

#define UseMPos		(HasMPos)
#define UseMNorm	(HasNorm && HasMNorm)
#define UseMUV		(HasUV && HasMUV)
#define UseMTan		(HasTan && HasMTan)

#define HasNotPos    ((Format&int(ItemUsage::ItemUsage_Position))==0)
#define HasNotNorm   ((Format&int(ItemUsage::ItemUsage_Normal))==0)
#define HasNotUV     ((Format&int(ItemUsage::ItemUsage_UV))==0)
#define HasNotTan    ((Format&int(ItemUsage::ItemUsage_Tangent))==0)
#define HasNotWght   ((Format&int(ItemUsage::ItemUsage_Weight))==0)

#define HasNotMPos   ((Format&int(ItemUsage::ItemUsage_MorphPosition))==0)
#define HasNotMNorm  ((Format&int(ItemUsage::ItemUsage_MorphNormal))==0)
#define HasNotMUV    ((Format&int(ItemUsage::ItemUsage_MorphUV))==0)
#define HasNotMTan   ((Format&int(ItemUsage::ItemUsage_MorphTangent))==0)


namespace {
	using PVX::Object3D::ItemUsage;
	void GetInputVert_VertexShader(std::stringstream& ret, unsigned int Format) {
		ret << "layout(location = 0) in vec4 Position;\n";
		char Index[2] = { '1', 0 };
		if (HasNorm) { ret << "layout(location = 1) in vec3 Normal;\n"; Index[0]++; }
		if (HasUV) { ret << "layout(location = " << Index << ") in vec2 UV;\n"; Index[0]++; }
		if (HasTan) { ret << "layout(location = " << Index << ") in vec4 Tangent;\n"; Index[0]++; }
		if (HasWght) {
			ret << "layout(location = " << Index << ") in vec4 BoneWeight;\n"; Index[0]++;
			ret << "layout(location = " << Index << ") in ivec4 BoneIndex;\n";
		}
	}

	void GetOutVertex_VertexShader(std::stringstream& ret, unsigned int Format) {
		ret << R"(

out Vert_t{
	vec4 Position;
)";
		if (HasNorm) ret << "\tvec3 Normal;\n";
		if (HasUV) ret << "\tvec2 UV;\n";
		if (HasTan) ret << "\tvec4 Tangent;\n";
		ret << R"(} outVert;
)";
	}

	void GetMorphType_VertexShader(std::stringstream& ret, unsigned int Format) {
		ret << R"(
struct MorphItem_t {
	vec4 Position;)";
		if (HasMNorm) ret << R"(
	vec4 Normal;)";
		if (HasMUV) ret << R"(
	vec4 UV;)";
		if (HasMTan) ret << R"(
	vec4 Tangent;)";
		ret << R"(
};
)";
	}


	void OnlyTransform(std::stringstream& ret, unsigned int Format) {
		const char* PosInput = [&] { const char* ret[]{ "MorphedPosition", "Position" }; return ret[!UseMPos]; }();
		const char* NormInput = [&] { const char* ret[]{ "MorphedNormal", "Normal" }; return ret[!UseMNorm]; }();
		const char* TanInput = [&] { const char* ret[]{ "MorphedTangent", "Tangent" }; return ret[!UseMTan]; }();

		ret << "\tgl_Position = Projection * View * (outVert.Position = Model[gl_InstanceID] * "<< PosInput <<");\n";
		if (HasNorm && HasNotTan) {
			ret << "\toutVert.Normal = mat3(Model[gl_InstanceID]) * "<< NormInput <<";\n";
		} else if (HasNorm && HasTan) {
			ret << R"shader(	mat3 m3 = mat3(Model[gl_InstanceID]);
	outVert.Normal = m3 * Normal;
	outVert.Tangent = vec4(m3 * Tangent.xyz, Tangent.w); 
)shader";
		}
	}

	void Skin(std::stringstream& ret, unsigned int Format) {
		const char* PosInput = [&] { const char* ret[]{ "MorphedPosition", "Position" }; return ret[!UseMPos]; }();
		const char* NormInput = [&] { const char* ret[]{ "MorphedNormal", "Normal" }; return ret[!UseMNorm]; }();
		const char* TanInput = [&] { const char* ret[]{ "MorphedTangent", "Tangent" }; return ret[!UseMTan]; }();


		ret << R"sdr(	vec3 b_pos = vec3(0);
)sdr";
		if (HasNorm) ret << "\tvec3 b_norm = vec3(0);\n";
		if (HasTan) ret << "\tvec3 b_tang = vec3(0);\n";

		ret <<	R"sdr(
	int Offset = BoneCount * gl_InstanceID;
	//const int Offset = 0;
	for (int i = 0; i < 4 && BoneWeight[i] > 0; i++) {
		int bIndex = Offset + BoneIndex[i];
		float bWeight = BoneWeight[i];)sdr";
		if (HasNotNorm) {
			ret << R"sdr(
		b_pos += ((Model[bIndex] * )sdr" << PosInput << R"sdr() * bWeight).xyz;
	})sdr";
		} else {
			ret << R"sdr(
		mat4 bone = Model[bIndex];
		b_pos += ((bone * )sdr" << PosInput<< R"sdr() * bWeight).xyz;)sdr";
			if (HasNotTan)
				ret << R"sdr(
		b_norm += (mat3(bone) * )sdr" << NormInput << R"sdr() * bWeight;)sdr";
			else
				ret << R"sdr(
		mat3 bone3 = mat3(bone);
		b_norm += (bone3 * )sdr" << NormInput << R"sdr() * bWeight;
		b_tang += (bone3 * )sdr" << TanInput << R"sdr(.xyz) * bWeight;)sdr";

			ret << R"sdr(
	}
	gl_Position = Projection * View * (outVert.Position = vec4(b_pos, 1));
)sdr";
			if (HasNorm) ret << "\toutVert.Normal = normalize(b_norm);\n";
			if (HasTan) ret << "\toutVert.Tangent = vec4(normalize(b_tang), " << TanInput << ".w);\n";
		}
	}
}

void Morph(std::stringstream& ret, unsigned int Format) {
	if (UseMPos) ret << "\tvec3 mPosition = Position.xyz;\n";
	if (UseMNorm) ret << "\tvec3 mNormal = Normal;\n";
	if (UseMUV) ret << "\tvec2 MorphedUV = UV;\n";
	if (UseMTan) ret << "\tvec3 mTangent = Tangent.xyz;\n";

	ret << R"sdr(
	int Item_Offset = gl_VertexID * MorphCount;
	int Control_Offset = gl_InstanceID * MorphCount;
	for (int i = 0; i < MorphCount; i++, Item_Offset++, Control_Offset++) {
		float w = MorphWeight[Control_Offset];
		MorphItem_t m = MorphItem[Item_Offset];
)sdr";

	if (UseMPos) ret << "\t\tmPosition += m.Position.xyz * w;\n";
	if (UseMNorm) ret << "\t\tmNormal += m.Normal.xyz * w;\n";
	if (UseMUV) ret << "\t\tMorphedUV += m.UV.xy * w;\n";
	if (UseMTan) ret << "\t\tmTangent += m.Tangent.xyz * w;\n";
	ret << R"sdr(	}
)sdr";
	if (UseMPos) ret << "\tvec4 MorphedPosition = vec4(mPosition, 1);\n";
	if (UseMNorm) ret << "\tvec3 MorphedNormal = normalize(mNormal);\n";
	if (UseMTan) ret << "\tvec4 MorphedTangent = vec4(normalize(mTangent), Tangent.w);\n";
}


std::string PVX::OpenGL::Helpers::Renderer::GetDefaultVertexShader(unsigned int Format) {
	std::stringstream ret;
	ret << R"(#version 440

)";

	GetInputVert_VertexShader(ret, Format);
	GetOutVertex_VertexShader(ret, Format);
	GetMorphType_VertexShader(ret, Format);


	ret << R"shader(
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

uniform int MorphCount;
uniform int BoneCount;

void main(){
)shader";

	if (HasMorph)
		Morph(ret, Format);

	if (HasNotWght)
		OnlyTransform(ret, Format);
	else
		Skin(ret, Format);


	if (HasUV) ret<<"\toutVert.UV = "<< [&] { const char* ret[]{ "MorphedUV", "UV" }; return ret[!UseMUV]; }() <<";\n";
	ret<<"}";

	auto str = ret.str();
	return str;
}

namespace {
	void GetInVertex_FragmentShader(std::stringstream& ret, unsigned int Format) {
		ret << R"(

in Vert_t{
	vec4 Position;
)";
		if (HasNorm) ret << "\tvec3 Normal;\n";
		if (HasUV) ret << "\tvec2 UV;\n";
		if (HasTan) ret << "\tvec4 Tangent;\n";
		ret << R"(} inVert;
)";
	}
}

#define HasTexColor (Frag&1)
#define HasTexMat (Frag&2)
#define HasTexBump (Frag&4)

#define UserBump (HasTexBump && HasNorm && HasTan && HasUV)

std::string PVX::OpenGL::Helpers::Renderer::GetDefaultFragmentShader(unsigned int Format, unsigned int Frag) { // )shdr"  R"shdr(
	std::stringstream ret;

	ret << R"shdr(#version 440

out vec3 Position;
out vec4 Color;
out vec3 Normal;
out vec2 PBR;)shdr";
	
	GetInVertex_FragmentShader(ret, Format);
	
	ret << R"shdr(

layout(std140, binding = 0) uniform Camera{
	mat4 View;
	mat4 Projection;
	vec3 CameraPosition;
};

layout(std140, binding = 2) uniform Material{
	vec4 MatColor;
	vec4 MatPBR;
};)shdr";

	if(HasTexColor) ret << R"shdr(

uniform sampler2D Color_Tex;)shdr";

	if (HasTexMat) ret << R"shdr(
uniform sampler2D PBR_Tex;

)shdr"; 
	if (HasTexBump) ret << R"shdr(
uniform sampler2D Bump_Tex;

)shdr";

	auto hTexBumb = HasTexBump;
	auto hNormals = HasNorm;
	auto hTan = HasTan;
	auto hUV = HasUV;

	auto test = UserBump;

	if(UserBump) ret << R"shdr(
vec3 TransformNormal(){
	vec3 norm = normalize(inVert.Normal);
	vec3 bin = normalize(cross(inVert.Tangent.xyz, norm) * inVert.Tangent.w);
	vec3 tang = cross(norm, bin);
	return mat3(tang, bin, norm) * texture(Bump_Tex, inVert.UV).xyz;
}

)shdr";


	ret << R"shdr(
void main(){
	Position = inVert.Position.xyz;)shdr";

	if (UserBump) {

		ret << R"shdr(
	Normal = TransformNormal();)shdr";
		
	} else {
		if (HasNorm) ret << R"shdr(
	Normal = normalize(inVert.Normal);)shdr";
		else ret << R"shdr(
	Normal = vec3(0);)shdr";
	}

	if (HasTexColor && HasUV) ret << R"shdr(
	Color = texture(Color_Tex, inVert.UV) * MatColor;)shdr";
	else ret << R"shdr(
	Color = MatColor;)shdr";

	if (HasTexMat && HasUV) ret << R"shdr(
	PBR = MatPBR.xy * texture(PBR_Tex, inVert.UV).xy;)shdr";
	else ret << R"shdr(
	PBR = MatPBR.xy;)shdr";

	ret << R"shdr(
})shdr";
	auto str = ret.str();
	return str;
}
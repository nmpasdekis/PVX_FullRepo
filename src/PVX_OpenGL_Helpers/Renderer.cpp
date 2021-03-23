#include "Include/PVX_OpenGL_Helpers.h"
#include <PVX_File.h>
#include <PVX_Encode.h>
#include <PVX_Image.h>
#include <PVX.inl>

namespace PVX::OpenGL::Helpers {


	const PVX::OpenGL::Shader& RefPostProcess_VertexShader();
	void DeRefPostProcess_VertexShader();
	const PVX::OpenGL::Geometry& RefPostProcess_Geometry();
	void DeRefPostProcess_Geometry();
	const PVX::OpenGL::Sampler& RefPostProcess_Sampler();
	void DeRefPostProcess_Sampler();
	void DeRef_ALL();

	Renderer::Renderer(int Width, int Height, PVX::OpenGL::Context& gl, PVX::OpenGL::FrameBufferObject* Target) : gl{ gl },
		gBufferFBO(gl, Width, Height),
		GeneralSampler{ PVX::OpenGL::TextureFilter::LINEAR, PVX::OpenGL::TextureWrap::REPEAT },
		PostProcesses{ gl }
	{
		gBuffer.Position = gBufferFBO.AddColorAttachmentRGB32F();
		gBuffer.Albedo = gBufferFBO.AddColorAttachmentRGBA8UB();
		gBuffer.Normal = gBufferFBO.AddColorAttachmentRGB16F();
		gBuffer.Material = gBufferFBO.AddColorAttachment(PVX::OpenGL::InternalFormat::RG16F, PVX::OpenGL::TextureFormat::RG, PVX::OpenGL::TextureType::HALF_FLOAT);
		gBuffer.Depth = gBufferFBO.AddDepthAttachment();
		gBufferFBO.Build();
		gBufferFBO.Name("GBuffer");

		gBuffer.Position.Name("gPosition");
		gBuffer.Albedo.Name("gAlbedo");
		gBuffer.Normal.Name("gNormal");
		gBuffer.Material.Name("gMaterial");
		gBuffer.Depth.Name("DepthBuffer");

		LightBuffer.Update(sizeof(Lights), &Lights);

		//PostProcesses.MakeSimple(Width, Height, gPosition, gAlbedo, gNormal, gMaterial, Target);
		//PostProcesses.MakeBloom(Width, Height, gBuffer, Target);
		PostProcesses.MakeSimple(Width, Height, gBuffer, Target);
	}

	void Renderer::SetCameraBuffer(PVX::OpenGL::Buffer& CamBuffer) {
		CameraBuffer = CamBuffer;
	}

	Renderer::~Renderer() {
		//DeRef_ALL();
	}

	void Renderer::Render(std::function<void()> RenderClb) {
		BindBuffer(0, CameraBuffer);

		gBufferFBO.Bind();
		RenderClb();
		//GBuffer.Unbind();

		//gl.Viewport();

		LightBuffer.Update(sizeof(Lights), &Lights);

		//PVX::OpenGL::FrameBufferObject::Unbind();

		BindBuffer(0, CameraBuffer);
		BindBuffer(1, LightBuffer);

		PostProcesses.Process();

		//for (auto& p : PostProcesses) p.Process();
	}

	PVX::OpenGL::Texture2D Renderer::LoadTexture2D(const std::wstring& Filename) {
		if (!Textures.count(Filename)) {
			auto img = PVX::ImageData::LoadLinear(Filename.c_str());
			auto& ret = Textures[Filename] = PVX::OpenGL::Texture2D::MakeTexture16F(img.Width, img.Height, img.Channels, img.Data.data());
			ret.Name(PVX::Encode::ToString(Filename).c_str());
			return ret;
		}
		return Textures.at(Filename);
	}
	PVX::OpenGL::Texture2D Renderer::LoadTexture2D(const std::string& Filename) {
		return LoadTexture2D(PVX::Encode::ToString(Filename));
	}

	PVX::OpenGL::Texture2D Renderer::LoadNormalTexture2D(const std::string& Filename, bool flipY) {
		return LoadNormalTexture2D(PVX::Encode::ToString(Filename), flipY);
	}

	PVX::OpenGL::Texture2D Renderer::LoadNormalTexture2D(const std::wstring& Filename, bool flipY) {
		auto NormalName = L"Normal_" + Filename;
		if (!Textures.count(NormalName)) {
			auto img = PVX::ImageData::LoadRaw(Filename.c_str()).Bias(false, flipY, false);;

			auto & ret = Textures[NormalName] = PVX::OpenGL::Texture2D::MakeTexture16F(img.Width, img.Height, img.Channels, img.Data.data());

			ret.Name(PVX::Encode::ToString(NormalName).c_str());
			return ret;
		}
		return Textures.at(NormalName);
	}

	PVX::OpenGL::Program Renderer::GetDefaultProgram(unsigned int VertexFormat, unsigned int Fragment) {
		auto Id = (VertexFormat << 10) | Fragment;
		if (DefaultShaderPrograms.count(Id)) return DefaultShaderPrograms.at(Id);
		if (DefaultVertexShaders.find(VertexFormat)==DefaultVertexShaders.end()) {
			DefaultVertexShaders[VertexFormat] = { PVX::OpenGL::Shader::ShaderType::VertexShader, GetDefaultVertexShader(VertexFormat) };
		}

		auto fId = ((VertexFormat & (
			uint32_t(PVX::Object3D::ItemUsage::ItemUsage_Normal) |
			uint32_t(PVX::Object3D::ItemUsage::ItemUsage_UV) |
			uint32_t(PVX::Object3D::ItemUsage::ItemUsage_Tangent)

			)) << 10) | Fragment;
		if (DefaultFragmentShaders.find(fId) == DefaultFragmentShaders.end()) {
			DefaultFragmentShaders[fId] = { PVX::OpenGL::Shader::ShaderType::FragmentShader, GetDefaultFragmentShader(VertexFormat, Fragment) };
		}
		return DefaultShaderPrograms[Id] = {
			DefaultVertexShaders.at(VertexFormat),
			DefaultFragmentShaders.at(fId)
		};
	}

	InstanceData& Renderer::GetInstance(int InstanceId) {
		auto& [objectId, Index] = Instances.at(InstanceId);
		auto& obj = Objects.at(objectId);
		return *obj->Instances.at(Index);
	}

	int Renderer::CreateInstance(int oId, bool Active) {
		int Id = NextInstanceId++;
		auto& obj = Objects[oId];
		Instances[Id] = { oId, obj->InstanceCount++ };

		obj->Instances.push_back(std::make_unique<InstanceData>(*obj, obj->InitialTransform, Active));
		obj->Instances.back()->MorphControls.resize(obj->MorphCount);

		for (auto& p: obj->Parts) {
			if (p.MorphCount)
				p.MorphControlBuffer.Update(obj->InstanceCount * p.MorphCount * sizeof(float));
			
			p.TransformBuffer.Update(obj->InstanceCount * p.UseMatrices.size() * sizeof(PVX::Matrix4x4));
		}
		return Id;
	}

	//ProgramPlus::ProgramPlus(const PVX::OpenGL::Program& p, const PVX::OpenGL::Sampler& DefSampler) : Prog{ p } {
	//	glUseProgram(Prog.Get());
	//	Prog.SetUniformBlockIndex("Camera", 0);
	//	Prog.SetUniformBlockIndex("Material", 2);
	//	Prog.SetShaderStrorageIndex("Transform", 3);
	//	Prog.SetShaderStrorageIndex("MorphControl", 4);
	//	Prog.SetShaderStrorageIndex("MorphData", 5);
	//	MorphCountIndex = Prog.UniformLocation("MorphCount");
	//	BoneCountIndex = Prog.UniformLocation("BoneCount");

	//	Prog.SetTextureIndexAndSampler("Color_Tex", 0, DefSampler);
	//	Prog.SetTextureIndexAndSampler("PBR_Tex", 1, DefSampler);
	//	Prog.SetTextureIndexAndSampler("Bump_Tex", 2, DefSampler);
	//	glUseProgram(0);
	//}
	//void ProgramPlus::SetBoneCount(int n) {  if(BoneCountIndex!=-1) glUniform1i(BoneCountIndex, n); }
	//void ProgramPlus::SetMorphCount(int n) { if (MorphCountIndex!=-1) glUniform1i(MorphCountIndex, n); }
}
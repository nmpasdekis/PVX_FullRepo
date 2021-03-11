#include "Include/PVX_OpenGL_Helpers.h"
#include <PVX_File.h>
#include <PVX_Encode.h>
#include <PVX_Image.h>
#include <PVX.inl>

namespace PVX::OpenGL::Helpers {
	Renderer::Renderer(int Width, int Height, PVX::OpenGL::Context& gl) : gl{ gl },
		FrameGeometry{ PVX::OpenGL::MakeSquareWithUV() },
		FullFrameBuffer(Width, Height),
		GeneralSampler{ PVX::OpenGL::TextureFilter::LINEAR, PVX::OpenGL::TextureWrap::REPEAT },
		PostSampler(PVX::OpenGL::TextureFilter::LINEAR, PVX::OpenGL::TextureWrap::CLAMP)
	{
		Position = FullFrameBuffer.AddColorAttachmentRGB32F();
		Color = FullFrameBuffer.AddColorAttachmentRGB8UB();
		Normal = FullFrameBuffer.AddColorAttachmentRGB16F();
		PBR = FullFrameBuffer.AddColorAttachment(PVX::OpenGL::InternalFormat::RG16F, PVX::OpenGL::TextureFormat::RG, PVX::OpenGL::TextureType::HALF_FLOAT);
		Depth = FullFrameBuffer.AddDepthAttachment();
		FullFrameBuffer.Build();

		FullFrameBuffer.Name("G Buffer");
		Position.Name("PositionBuffer");
		Color.Name("ColorBuffer");
		Normal.Name("NormalBuffer");
		PBR.Name("PBR_Buffer");
		Depth.Name("DepthBuffer");

		PostProcessPipeline.Shaders({
			{ PVX::OpenGL::Shader::ShaderType::VertexShader, PVX::IO::ReadText("shaders\\PostProcessVertex.glsl") },
			{ PVX::OpenGL::Shader::ShaderType::FragmentShader, PVX::IO::ReadText("shaders\\PostProcessLights.glsl") },
		});

		LightBuffer.Update(sizeof(LightRig), &Lights);

		PostProcessPipeline.BindBuffer("Lights", LightBuffer);

		PostProcessPipeline.Textures2D({
			{ "NormalTex", PostSampler, Normal },
			{ "ColorTex", PostSampler, Color },
			{ "PositionTex", PostSampler, Position },
			{ "MetallicRoughness", PostSampler, PBR },
		});
	}

	void Renderer::SetCameraBuffer(PVX::OpenGL::Buffer& CamBuffer) {
		CameraBuffer = CamBuffer;
		PostProcessPipeline.BindBuffer("Camera", CamBuffer);
	}

	void Renderer::Render(std::function<void()> RenderClb) {
		FullFrameBuffer.Bind();
		RenderClb();
		FullFrameBuffer.Unbind();

		gl.Viewport();

		LightBuffer.Update(sizeof(LightRig), &Lights);

		PostProcessPipeline.Bind();
		FrameGeometry.Draw();
		PostProcessPipeline.Unbind();
	}
	PVX::OpenGL::Texture2D Renderer::LoadTexture2D(const std::wstring& Filename) {
		if (!Textures.count(Filename)) {
			auto img = PVX::ImageData::Load(Filename.c_str());
			Textures[Filename] = PVX::OpenGL::Texture2D(img.Width, img.Height, img.Channels, 4, img.Data.data());
			Textures[Filename].Name(PVX::Encode::ToString(Filename).c_str());
		}
		return Textures[Filename];
	}
	PVX::OpenGL::Texture2D Renderer::LoadTexture2D(const std::string& Filename) {
		return LoadTexture2D(PVX::Encode::ToString(Filename));
	}

	Geometry ToGeomerty2(const Object3D::ObjectSubPart& so) {
		return {
			PrimitiveType::TRIANGLES,
			int(so.Index.size()),
			so.Index,
			{
				{
					so.VertexData,
					PVX::Map(so.Attributes, [](const auto& a) { return FromObjectAttribute(a); }),
					so.Stride
				}
			},
			false
		};
	}

	PVX::OpenGL::Program Renderer::GetDefaultProgram(unsigned int VertexFormat, unsigned int Fragment) {
		auto Id = (Fragment<<9) |VertexFormat;
		if (DefaultShaderPrograms.count(Id)) return DefaultShaderPrograms.at(Id);
		return DefaultShaderPrograms[Id] = {
			GetDefaultVertexShader(VertexFormat),
			GetDefaultFragmentShader(VertexFormat, Fragment)
		};
	}

	int Renderer::LoadObject(const std::string& Filename) {
		return LoadObject(PVX::Object3D::LoadFbx(Filename));
	}

	int Renderer::LoadObject(const Object3D::Object& obj) {
		int Id = NextObjectId++;
		ObjectGL& ret = *(Objects[Id] = std::make_unique<ObjectGL>()).get();
		ret.TransformConstants.reserve(obj.Heirarchy.size());
		for (auto& t : obj.Heirarchy) {
			ret.TransformConstants.emplace_back(t);
			ret.InitialTransform.push_back({ t.Position, t.Rotation, t.Scale });
		}

		std::map<std::string, int> MatLookup;
		for (auto& [Name, m]: obj.Material) {
			MatLookup[Name] = int(MatLookup.size());
			if (m.IsPBR) {
				auto& mat = ret.Materials.emplace_back(DataPBR{
					{ m.Color, 1.0f - m.Transparency },
					m.Metallic,
					m.Roughness,
					m.Bump,
					m.EmissiveFactor * (m.Emissive.x + m.Emissive.y + m.Emissive.z) * (1.0f/3.0f)
				});
				mat.Data.Name(Name.c_str());
				if (m.Textures.Diffuse.size())
					mat.Color_Tex = LoadTexture2D(m.Textures.Diffuse).Get();
				if (m.Textures.Bump.size())
					mat.Normal_Tex = LoadTexture2D(m.Textures.Bump).Get();
				if (m.Textures.PBR.size())
					mat.PBR_Tex = LoadTexture2D(m.Textures.PBR).Get();
			} else {
				auto& mat = ret.Materials.emplace_back(DataPBR{
					{ m.Color, 1.0f - m.Transparency },
					0,
					1.0f - m.SpecularFactor /(m.AmbientFactor + m.DiffuseFactor + m.SpecularFactor),
					m.Bump,
					m.EmissiveFactor * (m.Emissive.x + m.Emissive.y + m.Emissive.z) * (1.0f/3.0f)
				});
				mat.Data.Name(Name.c_str());
				if (m.Textures.Diffuse.size())
					mat.Color_Tex = LoadTexture2D(m.Textures.Diffuse).Get();
				if (m.Textures.Bump.size())
					mat.Normal_Tex = LoadTexture2D(m.Textures.Bump).Get();
			}
		}
		
		ret.Parts.reserve(obj.Parts.size());
		for (auto& p : obj.Parts) {
			auto& pp = ret.Parts.emplace_back();

			ret.MorphCount += (pp.MorphCount = int(p.BlendShapes.size()));

			if (!p.BoneNodes.size()) {
				pp.UseMatrices.push_back((int)IndexOf(obj.Heirarchy, [&](const Object3D::Transform& t) { return t.Name == p.TransformNode; }));
			} else {
				for (auto i = 0; i<p.BoneNodes.size(); i++) {
					pp.PostTransform.reserve(p.BoneNodes.size());
					pp.UseMatrices.reserve(p.BoneNodes.size());

					pp.UseMatrices.push_back((int)IndexOf(obj.Heirarchy, [&](const Object3D::Transform& t) { return p.BoneNodes[i] == t.Name; }));
					pp.PostTransform.push_back(p.BonePostTransform[i]);
				}
			}
			for (auto& [Mat, SubPart]: p.Parts) {
				auto& sp = pp.SubPart.emplace_back();
				sp.MaterialIndex = MatLookup[Mat];

				if (!SubPart.CustomShader) {
					auto& Mater = ret.Materials[sp.MaterialIndex];
					unsigned int MatFlags = (Mater.Color_Tex ? 1 : 0) | (Mater.PBR_Tex ? 2 : 0) | (Mater.Normal_Tex ? 4 : 0);

					unsigned int GeoFlags = PVX::Reduce(SubPart.Attributes, 0, [](unsigned int acc, const PVX::Object3D::VertexAttribute& attr) {
						return acc | unsigned int(attr.Usage);
					});

					std::vector<std::string> Filters;
					Filters.reserve(SubPart.Attributes.size());
					Filters.push_back("Position");
					if (GeoFlags & unsigned int(PVX::Object3D::ItemUsage::ItemUsage_Normal)) 
						Filters.push_back("Normal");
					if (GeoFlags & unsigned int(PVX::Object3D::ItemUsage::ItemUsage_UV) && MatFlags) {
						Filters.push_back("UV");
						Filters.push_back("TexCoord");
						if (GeoFlags & unsigned int(PVX::Object3D::ItemUsage::ItemUsage_Tangent) && (MatFlags&4)) Filters.push_back("Tangent");
					}
					if (GeoFlags & unsigned int(PVX::Object3D::ItemUsage::ItemUsage_Weight)) {
						Filters.push_back("Weights");
						Filters.push_back("WeightIndices");
					}
					auto fsp = SubPart.FilterAttributes(Filters);

					sp.Mesh = ToGeomerty2(fsp);
					GeoFlags = sp.Mesh.GetFlags();

					
					if (fsp.BlendAttributes.size()) {
						GeoFlags = PVX::Reduce(SubPart.BlendAttributes, GeoFlags, [](unsigned int acc, const PVX::Object3D::VertexAttribute& attr) {
							return acc | unsigned int(attr.Usage);
						});
						sp.Morph = PVX::OpenGL::Buffer::MakeImmutableShaderStorage((int)fsp.BlendShapeData.size(), fsp.BlendShapeData.data());
					}
					sp.ShaderProgram = { GetDefaultProgram(GeoFlags, MatFlags), GeneralSampler };
				} else {
					sp.Mesh = ToGeomerty2(SubPart);
					if (SubPart.BlendAttributes.size()) {
						sp.Morph = PVX::OpenGL::Buffer(SubPart.BlendShapeData.data(), SubPart.BlendShapeData.size(), false);
					}
					sp.ShaderProgram = { GetDefaultProgram(1, 0), GeneralSampler };
				}
			}
		}
		return Id;
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

	ProgramPlus::ProgramPlus(const PVX::OpenGL::Program& p, const PVX::OpenGL::Sampler& DefSampler) : Prog{ p } {
		glUseProgram(Prog.Get());
		Prog.SetUniformBlockIndex("Camera", 0);
		Prog.SetUniformBlockIndex("Material", 1);
		Prog.SetShaderStrorageIndex("Transform", 2);
		Prog.SetShaderStrorageIndex("MorphControl", 3);
		Prog.SetShaderStrorageIndex("MorphData", 4);
		MorphCountIndex = Prog.UniformLocation("MorphCount");
		BoneCountIndex = Prog.UniformLocation("BoneCount");

		Prog.SetTextureIndexAndSampler("Color_Tex", 0, DefSampler);
		Prog.SetTextureIndexAndSampler("PBR_Tex", 1, DefSampler);
		Prog.SetTextureIndexAndSampler("Bump_Tex", 2, DefSampler);
		glUseProgram(0);
	}
	void ProgramPlus::SetBoneCount(int n) {  if(BoneCountIndex!=-1) glUniform1i(BoneCountIndex, n); }
	void ProgramPlus::SetMorphCount(int n) { if (MorphCountIndex!=-1) glUniform1i(MorphCountIndex, n); }
}
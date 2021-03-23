#include "Include/PVX_OpenGL_Helpers.h"
#include <PVX_File.h>

#include "PostShaders.inl"

namespace PVX::OpenGL::Helpers {
	PVX::OpenGL::Shader* PostProcess_VertexShader;
	PVX::OpenGL::Geometry* PostProcess_Geometry;
	PVX::OpenGL::Sampler* PostProcess_Sampler;
	int PostProcess_VertexShaderRef = 0;
	int PostProcess_GeometryRef = 0;
	int PostProcess_SamplerRef = 0;
	const PVX::OpenGL::Shader& RefPostProcess_VertexShader() {
		if (!PostProcess_VertexShaderRef) {
			PostProcess_VertexShader = new PVX::OpenGL::Shader(PVX::OpenGL::Shader::ShaderType::VertexShader, R"POST_VERTEX(#version 440
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 uv;
out vec2 UV;
void main() { 
	UV = uv;
	gl_Position = Position; 
})POST_VERTEX");
		}
		PostProcess_VertexShaderRef++;
		return *PostProcess_VertexShader;
	}
	void DeRefPostProcess_VertexShader() {
		PostProcess_VertexShaderRef--;
		if (!PostProcess_VertexShaderRef) 
			delete PostProcess_VertexShader;
	}
	const PVX::OpenGL::Geometry& RefPostProcess_Geometry() {
		if (!PostProcess_GeometryRef) {
			PostProcess_Geometry = new PVX::OpenGL::Geometry([] {
				PVX::OpenGL::ObjectBuilder ob;
				ob.Begin(GL_QUADS); {
					ob.TexCoord(	0,		0);
					ob.Vertex(-1.0f, -1.0f, 0);
					ob.TexCoord(	1.0f,	0);
					ob.Vertex(1.0f, -1.0f, 0);
					ob.TexCoord(	1.0f,	1.0f);
					ob.Vertex(1.0f, 1.0f, 0);
					ob.TexCoord(	0,		1.0f);
					ob.Vertex(-1.0f, 1.0f, 0);
				} ob.End();
				return ob.Build();
			}());
		}
		PostProcess_GeometryRef++;
		return *PostProcess_Geometry;
	}
	void DeRefPostProcess_Geometry() {
		PostProcess_GeometryRef--;
		if (!PostProcess_GeometryRef) 
			delete PostProcess_Geometry;
	}
	const PVX::OpenGL::Sampler& RefPostProcess_Sampler() {
		if (!PostProcess_SamplerRef) {
			PostProcess_Sampler = new PVX::OpenGL::Sampler{ PVX::OpenGL::TextureFilter::LINEAR, PVX::OpenGL::TextureWrap::CLAMP };
		}
		PostProcess_SamplerRef++;
		return *PostProcess_Sampler;
	}
	void DeRefPostProcess_Sampler() {
		PostProcess_SamplerRef--;
		if (!PostProcess_SamplerRef) 
			delete PostProcess_Sampler;
	}

	void DeRef_ALL() {
		//DeRefPostProcess_Geometry();
		//DeRefPostProcess_VertexShader();
		//DeRefPostProcess_Sampler();
	}


	PostProcessor::PostProcessor(PVX::OpenGL::Context& gl) :
		gl{ gl }, 
		VertexShader{ RefPostProcess_VertexShader() },
		FrameGeometry{ RefPostProcess_Geometry() },
		TexSampler{ RefPostProcess_Sampler() }
	{}

	void PostProcessor::AddProcess(
		const PVX::OpenGL::Shader& FragmentShader, 
		const std::initializer_list<std::tuple<int, PVX::OpenGL::Texture2D>>& Inputs,
		const std::initializer_list<PVX::OpenGL::Texture2D>& Targets, float ScaleX, float ScaleY) {
		auto fbo = FBOs.emplace_back(std::make_unique<PVX::OpenGL::FrameBufferObject>(gl)).get();

		if (Targets.size()) {
			for (auto& t : Targets) {
				fbo->AddColorAttachment(t);
				if (!Scales.count(t.Get())) Scales[t.Get()] = { t, { ScaleX, ScaleY } };
			}
			fbo->Build();
		}

		for (auto& [unit, t] : Inputs) {
			if (BindSamplers.size()<=unit)BindSamplers.resize(unit + 1);
			BindSamplers[unit] = TexSampler.Get();
		}

		PVX::OpenGL::Program p{ VertexShader, FragmentShader };
		p.Bind();
		Processes.push_back({
			p,
			fbo,
			Inputs
		});
		p.Unbind();
	}

	void PostProcessor::AddProcess(
		const PVX::OpenGL::Shader& FragmentShader,
		const std::initializer_list<std::tuple<int, PVX::OpenGL::Texture2D>>& Inputs,
		PVX::OpenGL::FrameBufferObject& FBO) {

		for (auto& [unit, t] : Inputs) {
			if (BindSamplers.size()<=unit)BindSamplers.resize(unit + 1);
			BindSamplers[unit] = TexSampler.Get();
		}

		PVX::OpenGL::Program p{ VertexShader, FragmentShader };
		Processes.push_back({
			p,
			&FBO,
			Inputs,
		});
	}


	void PostProcessor::Resize(const PVX::iVector2D& Size) {
		for (auto& [id, it]: Scales) {
			auto& [Tex, Sc] = it;
			Tex.Resize(int(Size.Width * Sc.x), int(Size.Height * Sc.y));
		}
	}
	void PostProcessor::Process() {
		FrameGeometry.Bind();
		glBindSamplers(0, GLsizei(BindSamplers.size()), BindSamplers.data());
		int lastWidth = gl.Width, lastHeight = gl.Height;
		for (auto& p : Processes) {
			p.FBO->Bind();
			p.Shaders.Bind();
			for (auto& [unit, tex] : p.TextureInputs) tex.BindToUnit(unit);
			FrameGeometry.DrawBound();
		}
		Geometry::Unbind();
	}
	void PostProcessor::MakeSimple(
		int Width, int Height,
		const PVX::OpenGL::Texture2D& gPosition,
		const PVX::OpenGL::Texture2D& gAlbedo,
		const PVX::OpenGL::Texture2D& gNormal,
		const PVX::OpenGL::Texture2D& gMaterial,
		PVX::OpenGL::FrameBufferObject* Target
	) {
		Processes.clear();
		FBOs.clear();
		if (!Target) {
			AddProcess(
				PVX::OpenGL::Shader{ PVX::OpenGL::Shader::ShaderType::FragmentShader, POSTPROCESSLIGHTS_GLSL },
				{
					{ 0, gPosition},
					{ 1, gAlbedo },
					{ 2, gNormal },
					{ 3, gMaterial }
				},
				{}
			);
		} else {
			AddProcess(
				PVX::OpenGL::Shader{ PVX::OpenGL::Shader::ShaderType::FragmentShader, POSTPROCESSLIGHTS_GLSL },
				{
					{ 0, gPosition},
					{ 1, gAlbedo },
					{ 2, gNormal },
					{ 3, gMaterial }
				},
				*Target
			);
		}
	}

	void PostProcessor::MakeBloom(int Width, int Height, GBuffer& g, PVX::OpenGL::FrameBufferObject* Target) {
		MakeBloom(Width, Height, g.Position, g.Albedo, g.Normal, g.Material, Target);
	}

	void PostProcessor::MakeSimple(int Width, int Height, GBuffer& g, PVX::OpenGL::FrameBufferObject* Target) {
		MakeSimple(Width, Height, g.Position, g.Albedo, g.Normal, g.Material, Target);
	}

	void PostProcessor::MakeBloom(
		int Width, int Height,
		const PVX::OpenGL::Texture2D& gPosition,
		const PVX::OpenGL::Texture2D& gAlbedo,
		const PVX::OpenGL::Texture2D& gNormal,
		const PVX::OpenGL::Texture2D& gMaterial,
		PVX::OpenGL::FrameBufferObject* Target
	) {
		Processes.clear();
		FBOs.clear();
		auto FinalColor = PVX::OpenGL::Texture2D::MakeTextureRGB16F(Width, Height);
		auto BloomHorizontal = PVX::OpenGL::Texture2D::MakeTextureRGB16F(Width, Height);
		auto BloomVertical = PVX::OpenGL::Texture2D::MakeTextureRGB16F(Width, Height);

		FinalColor.Name("FinalColor");
		BloomHorizontal.Name("BloomHorizontal");
		BloomVertical.Name("BloomVertical");

		AddProcess(
			PVX::OpenGL::Shader{ PVX::OpenGL::Shader::ShaderType::FragmentShader, POSTPROCESSLIGHTS_PREPOST_GLSL },
			{
				{ 0, gPosition},
				{ 1, gAlbedo },
				{ 2, gNormal },
				{ 3, gMaterial }
			},
			{ FinalColor, BloomVertical }
		);

		AddProcess(
			PVX::OpenGL::Shader{ PVX::OpenGL::Shader::ShaderType::FragmentShader, GAUSSIANBLUR_H_GLSL },
			{ { 0, BloomVertical } },
			{ BloomHorizontal }
		);

		AddProcess(
			PVX::OpenGL::Shader{ PVX::OpenGL::Shader::ShaderType::FragmentShader, GAUSSIANBLUR_V_GLSL },
			{ { 0, BloomHorizontal } },
			{ BloomVertical }
		);

		if (!Target) {
			AddProcess(
				PVX::OpenGL::Shader{ PVX::OpenGL::Shader::ShaderType::FragmentShader, POSTPOST_GLSL },
				{
					{ 0, FinalColor },
					{ 1, BloomVertical },
				},
				{}
			);
		} else {
			AddProcess(
				PVX::OpenGL::Shader{ PVX::OpenGL::Shader::ShaderType::FragmentShader, POSTPOST_GLSL },
				{
					{ 0, FinalColor },
					{ 1, BloomVertical },
				},
				*Target
			);
		}
	}
}
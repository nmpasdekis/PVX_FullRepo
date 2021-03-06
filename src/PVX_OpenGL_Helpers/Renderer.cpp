#include "Include/PVX_OpenGL_Helpers.h"
#include <PVX_File.h>
#include <PVX_Encode.h>
#include <PVX_Image.h>

namespace PVX::OpenGL::Helpers {
	Renderer::Renderer(int Width, int Height, PVX::OpenGL::Context& gl) : gl{ gl },
		FrameGeometry{ PVX::OpenGL::MakeSquareWithUV() },
		FullFrameBuffer(Width, Height),
		GeneralSampler(PVX::OpenGL::TextureFilter::LINEAR, PVX::OpenGL::TextureWrap::CLAMP)
	{
		Position = FullFrameBuffer.AddColorAttachmentRGB32F();
		Color = FullFrameBuffer.AddColorAttachmentRGB8UB();
		Normal = FullFrameBuffer.AddColorAttachmentRGB16F();
		PBR = FullFrameBuffer.AddColorAttachment(PVX::OpenGL::InternalFormat::RG16F, PVX::OpenGL::TextureFormat::RG, PVX::OpenGL::TextureType::HALF_FLOAT);
		Depth = FullFrameBuffer.AddDepthAttachment();
		FullFrameBuffer.Build();

		PostProcessPipeline.Shaders({
			{ PVX::OpenGL::Shader::ShaderType::VertexShader, PVX::IO::ReadText("shaders\\PostProcessVertex.glsl") },
			{ PVX::OpenGL::Shader::ShaderType::FragmentShader, PVX::IO::ReadText("shaders\\PostProcessLights.glsl") },
		});
		LightBuffer.Update(&Lights, sizeof(LightRig));

		PostProcessPipeline.BindBuffer("Lights", LightBuffer);

		PostProcessPipeline.Textures2D({
			{ "NormalTex", GeneralSampler, Normal },
			{ "ColorTex", GeneralSampler, Color },
			{ "PositionTex", GeneralSampler, Position },
			{ "MetallicRoughness", GeneralSampler, PBR },
		});
	}

	void Renderer::SetCameraBuffer(PVX::OpenGL::Buffer& CamBuffer) {
		PostProcessPipeline.BindBuffer("Camera", CamBuffer);
	}

	void Renderer::Render(std::function<void()> RenderClb) {
		FullFrameBuffer.Bind();
		RenderClb();
		FullFrameBuffer.Unbind();

		gl.Viewport();

		LightBuffer.Update(&Lights, sizeof(LightRig));

		PostProcessPipeline.Bind();
		FrameGeometry.Draw();
		PostProcessPipeline.Unbind();
	}
	PVX::OpenGL::Texture2D Renderer::LoadTexture2D(const std::wstring& Filename) {
		if (!Textures.count(Filename)) {
			auto img = PVX::ImageData::Load(Filename.c_str());
			return Textures[Filename] = PVX::OpenGL::Texture2D(img.Width, img.Height, img.Channels, 4, img.Data.data());
		}
		return Textures[Filename];
	}
}
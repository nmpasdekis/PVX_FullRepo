#include <PVX_OpenGL.h>
namespace PVX::OpenGL {
	FrameBufferObject::FrameBufferObject(int Width, int Height) : Width{ Width }, Height{ Height } {}

	Texture2D FrameBufferObject::AddColorAttachment(int InternalFormat, int Format, int Type) {
		ColorBuffers.push_back(Texture2D(Width, Height, 3, 4, nullptr));
		//ColorBuffers.emplace_back(Width, Height, InternalFormat, Format, Type, (void*)0);
		return ColorBuffers.back();
	}
	Texture2D FrameBufferObject::AddDepthAttachment() {
		DepthStencilBuffer = Texture2D::MakeDepthBuffer32F(Width, Height);
		return DepthStencilBuffer;
	}
	Texture2D FrameBufferObject::AddDepthStencilAttachment() {
		DepthStencilBuffer = Texture2D::MakeDepthStencilBuffer24_8(Width, Height);
		return DepthStencilBuffer;
	}
	void FrameBufferObject::Resize(int Width, int Height) {
		this->Width = Width;
		this->Height = Height;
		for (auto& t: ColorBuffers) t.Resize(Width, Height);
		DepthStencilBuffer.Resize(Width, Height);
	}

	int FrameBufferObject::Build() {
		if (!Id)glGenFramebuffers(1, &Id);
		glBindFramebuffer(GL_FRAMEBUFFER, Id);
		for (int i = 0; i<ColorBuffers.size(); i++) {
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, ColorBuffers.back().Get(), 0);
		}
		if (DepthStencilBuffer.Format == GL_DEPTH_COMPONENT) {
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, DepthStencilBuffer.Get(), 0);
		} else if (DepthStencilBuffer.Format == GL_DEPTH_STENCIL) {
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, DepthStencilBuffer.Get(), 0);
		} else {
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_TEXTURE_2D, DepthStencilBuffer.Get(), 0);
		}
		auto ret = glCheckFramebufferStatus(GL_FRAMEBUFFER);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		return ret;
	}
	void FrameBufferObject::Bind() {
		glBindFramebuffer(GL_FRAMEBUFFER, Id);
		glViewport(0, 0, Width, Height);
	}
	void FrameBufferObject::Unbind() {
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}
}
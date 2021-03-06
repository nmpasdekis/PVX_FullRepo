#include <PVX_OpenGL.h>
namespace PVX::OpenGL {
	FrameBufferObject::FrameBufferObject(int Width, int Height) : Width{ Width }, Height{ Height } {}

	const Texture2D& FrameBufferObject::AddColorAttachment(InternalFormat ifmt, TextureFormat Format, TextureType Type) {
		//ColorBuffers.push_back(Texture2D(Width, Height, 3, 4, nullptr));
		GL_CHECK(ColorBuffers.emplace_back(Width, Height, (int)ifmt, (int)Format, (int)Type, (void*)0));
		return ColorBuffers.back();
	}
	const Texture2D& FrameBufferObject::AddDepthAttachment() {
		DepthStencilBuffer = Texture2D::MakeDepthBuffer32F(Width, Height);
		return DepthStencilBuffer;
	}
	const Texture2D& FrameBufferObject::AddDepthStencilAttachment() {
		DepthStencilBuffer = Texture2D::MakeDepthStencilBuffer24_8(Width, Height);
		return DepthStencilBuffer;
	}

	const Texture2D& FrameBufferObject::AddColorAttachment(const Texture2D& tex) {
		ColorBuffers.push_back(tex);
		return tex;
	}
	const Texture2D& FrameBufferObject::AddDepthStencilAttachment(const Texture2D& tex) {
		DepthStencilBuffer = tex;
		return tex;
	}

	void FrameBufferObject::Resize(int Width, int Height) {
		this->Width = Width;
		this->Height = Height;
		for (auto& t: ColorBuffers) t.Resize(Width, Height);
		if(DepthStencilBuffer.Id)
			DepthStencilBuffer.Resize(Width, Height);
	}

	int FrameBufferObject::Build() {
		if (!Id)glGenFramebuffers(1, &Id);
		glBindFramebuffer(GL_FRAMEBUFFER, Id);
		std::vector<GLenum> att;
		for (int i = 0; i<ColorBuffers.size(); i++) {
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, ColorBuffers[i].Get(), 0);
			att.push_back(GL_COLOR_ATTACHMENT0 + i);
		}
		glNamedFramebufferDrawBuffers(Id, int(att.size()), att.data());

		if (DepthStencilBuffer.Format == TextureFormat::DEPTH_COMPONENT) {
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, DepthStencilBuffer.Get(), 0);
		} else if (DepthStencilBuffer.Format == TextureFormat::DEPTH_STENCIL) {
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, DepthStencilBuffer.Get(), 0);
		} else {
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_TEXTURE_2D, DepthStencilBuffer.Get(), 0);
		}
		GLenum ret = (GLenum)glCheckFramebufferStatus(GL_FRAMEBUFFER);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		return ret == GL_FRAMEBUFFER_COMPLETE;
	}
	void FrameBufferObject::Bind() {
		glBindFramebuffer(GL_FRAMEBUFFER, Id);
		if (Id) {
			glViewport(0, 0, Width, Height);
		}
	}
	void FrameBufferObject::Unbind() {
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}
}
#include <PVX_OpenGL.h>
namespace PVX::OpenGL {
	FrameBufferObject::FrameBufferObject(PVX::OpenGL::Context& gl, int Width, int Height) : gl{ gl }, Size { Width, Height } {}

	const Texture2D& FrameBufferObject::AddColorAttachment(InternalFormat ifmt, TextureFormat Format, TextureType Type) {
		return ColorBuffers.emplace_back(Size.Width, Size.Height, (int)ifmt, (int)Format, (int)Type, (void*)0);
	}
	const Texture2D& FrameBufferObject::AddDepthAttachment() {
		return DepthStencilBuffer = Texture2D::MakeDepthBuffer32F(Size.Width, Size.Height);
	}
	const Texture2D& FrameBufferObject::AddDepthStencilAttachment() {
		return DepthStencilBuffer = Texture2D::MakeDepthStencilBuffer24_8(Size.Width, Size.Height);
	}

	const Texture2D& FrameBufferObject::AddColorAttachment(const Texture2D& tex) {
		ColorBuffers.push_back(tex);
		return tex;
	}
	void FrameBufferObject::AddColorAttachments(const std::initializer_list<Texture2D>& tex) {
		ColorBuffers.reserve(ColorBuffers.size() + tex.size());
		for (auto& t: tex) ColorBuffers.push_back(t);
	}

	const Texture2D& FrameBufferObject::AddDepthStencilAttachment(const Texture2D& tex) {
		return DepthStencilBuffer = tex;
	}

	void FrameBufferObject::Resize(int Width, int Height) {
		Size.Width = Width;
		Size.Height = Height;
		if (Id) {
			for (auto& t: ColorBuffers) t.Resize(Width, Height);
			if (DepthStencilBuffer.Get())
				DepthStencilBuffer.Resize(Width, Height);
		}
	}

	int FrameBufferObject::Build() {
		if (!Id)glGenFramebuffers(1, &Id);
		glBindFramebuffer(GL_FRAMEBUFFER, Id);

		if (ColorBuffers.size())
			Size = ColorBuffers[0].GetSize();
		else if (DepthStencilBuffer.Get())
			Size = DepthStencilBuffer.GetSize();

		std::vector<GLenum> att;
		for (int i = 0; i<ColorBuffers.size(); i++) {
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, ColorBuffers[i].Get(), 0);
			att.push_back(GL_COLOR_ATTACHMENT0 + i);
		}
		glNamedFramebufferDrawBuffers(Id, int(att.size()), att.data());

		if (DepthStencilBuffer.ptr->Format == TextureFormat::DEPTH_COMPONENT) {
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, DepthStencilBuffer.Get(), 0);
		} else if (DepthStencilBuffer.ptr->Format == TextureFormat::DEPTH_STENCIL) {
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, DepthStencilBuffer.Get(), 0);
		} else {
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_TEXTURE_2D, DepthStencilBuffer.Get(), 0);
		}
		GLenum ret = (GLenum)glCheckFramebufferStatus(GL_FRAMEBUFFER);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		return ret == GL_FRAMEBUFFER_COMPLETE;
	}
	void FrameBufferObject::Bind() const {
		glBindFramebuffer(GL_FRAMEBUFFER, Id);
		if (Id)
			gl.Viewport(Size);
		else 
			gl.Viewport();
	}
	void FrameBufferObject::Unbind() {
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}
}
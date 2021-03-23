#include <PVX_OpenGL.h>
#include <PVX_Image.h>
#include <array>

namespace PVX::OpenGL {
	Texture2D::Texture2D() : ptr{ new TextureData(), [](TextureData* p) {
		if (p->Id) 
			glDeleteTextures(1, &p->Id);
		delete p;
	} } {}

	Texture2D::Texture2D(const TextureData& dt) : ptr{ new TextureData(dt), [](TextureData* p) {
		if (p->Id)
			glDeleteTextures(1, &p->Id);
		delete p;
	} } {}

	Texture2D::Texture2D(int Width, int Height, int Channels, int BytesPerChannel): Texture2D() {
		if (!ptr->Id)glGenTextures(1, &ptr->Id);
		Update(Width, Height, Channels, BytesPerChannel, nullptr);
	}
	Texture2D::Texture2D(int Width, int Height, int Channels, int BytesPerChannel, void* Data): Texture2D() {
		Update(Width, Height, Channels, BytesPerChannel, Data);
	}
	Texture2D::Texture2D(int Width, int Height, int InternalFormat, int Format, int Type, void* Data): Texture2D() {
		Update(Width, Height, InternalFormat, Format, Type, Data);
	}
	Texture2D::Texture2D(int Width, int Height, PVX::OpenGL::InternalFormat internalFormat, TextureFormat Format, TextureType Type, void* Data): Texture2D() {
		Update(Width, Height, (int)internalFormat, (int)Format, (int)Type, Data);
	}
	void Texture2D::UpdateAndBind(int Width, int Height, int InternalFormat, int Format, int Type, void* Data) {
		if (!ptr->Id)glGenTextures(1, &ptr->Id);
		glBindTexture(GL_TEXTURE_2D, ptr->Id);
		ptr->Size = { Width, Height };
		ptr->InternalFormat = PVX::OpenGL::InternalFormat(InternalFormat);
		ptr->Format = PVX::OpenGL::TextureFormat(Format);
		ptr->Type = PVX::OpenGL::TextureType(Type);

		glTexStorage2D(GL_TEXTURE_2D, 1, InternalFormat, Width, Height);
		if(Data) glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, Width, Height, Format, Type, Data);
		//glTexImage2D(GL_TEXTURE_2D, 0, InternalFormat, Width, Height, 0, Format, Type, Data);
	}
	void Texture2D::Update(int Width, int Height, int InternalFormat, int Format, int Type, void* Data) {
		UpdateAndBind(Width, Height, InternalFormat, Format, Type, Data);
		glBindTexture(GL_TEXTURE_2D, 0);
	}
	void Texture2D::UpdateAndBind(const void* Data) {
		if (!ptr->Id)glGenTextures(1, &ptr->Id);
		glBindTexture(GL_TEXTURE_2D, ptr->Id);

		//glTexStorage2D(GL_TEXTURE_2D, 1, int(ptr->InternalFormat), ptr->Size.Width, ptr->Size.Height);
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, ptr->Size.Width, ptr->Size.Height, int(ptr->Format), int(ptr->Type), Data);

		//glTexImage2D(GL_TEXTURE_2D, 0, int(ptr->InternalFormat), ptr->Size.Width, ptr->Size.Height, 0, GLenum(ptr->Format), GLenum(ptr->Type), Data);
	}
	void Texture2D::Update(void* Data) {
		UpdateAndBind(Data);
		glBindTexture(GL_TEXTURE_2D, 0);
	}
	void Texture2D::UpdateAndBind(int Width, int Height, int Channels, int BytesPerChannel, void* Data) {
		if (!ptr->Id)glGenTextures(1, &ptr->Id);
		glBindTexture(GL_TEXTURE_2D, ptr->Id);

		ptr->Format = TextureFormat::RGB;
		ptr->Type = TextureType::FLOAT;
		ptr->InternalFormat = InternalFormat::RGB;

		if (Channels == 4) {
			ptr->Format = TextureFormat::RGBA;
			ptr->InternalFormat = InternalFormat::RGBA;
		} else if (Channels == 1) {
			ptr->Format = TextureFormat::RED;
			ptr->InternalFormat = InternalFormat::RED;
		}
		if (BytesPerChannel == 1) ptr->Type = TextureType::UNSIGNED_BYTE;

		ptr->Size = { Width, Height };

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	}
	void Texture2D::Update(int Width, int Height, int Channels, int BytesPerChannel, void* Data) {
		UpdateAndBind(Width, Height, Channels, BytesPerChannel, Data);
		glBindTexture(GL_TEXTURE_2D, 0);
	}

	void Texture2D::GenerateMipmaps() {
		Bind();
		glGenerateMipmap(GL_TEXTURE_2D);
		Unbind();
	}

	void Texture2D::Resize(int Width, int Height) {
		ptr->Size = { Width, Height };
		glBindTexture(GL_TEXTURE_2D, ptr->Id);


		glTexStorage2D(GL_TEXTURE_2D, 1, int(ptr->InternalFormat), ptr->Size.Width, ptr->Size.Height);

		//glTexImage2D(GL_TEXTURE_2D, 0, GLint(ptr->InternalFormat), Width, Height, 0, GLenum(ptr->Format), GLenum(ptr->Type), 0);
		glBindTexture(GL_TEXTURE_2D, 0);
	}
	void Texture2D::Bind(int Unit) const {
		glActiveTexture(GL_TEXTURE0 + Unit);
		glBindTexture(GL_TEXTURE_2D, ptr->Id);
	}
	void Texture2D::BindToUnit(int Unit) const {
		glBindTextureUnit(Unit, ptr->Id);
	}
	void Texture2D::Unbind() {
		glBindTexture(GL_TEXTURE_2D, 0);
	}
	Texture2D Texture2D::MakeDepthBuffer32F(int Width, int Height) {
		return std::move(Texture2D{ Width, Height, GL_DEPTH_COMPONENT32F, GL_DEPTH_COMPONENT, GL_FLOAT, 0 });
	}
	Texture2D Texture2D::MakeStencilBuffer(int Width, int Height) {
		return std::move(Texture2D{ Width, Height, 1, GL_STENCIL_COMPONENTS, GL_UNSIGNED_BYTE, 0 });
	}
	Texture2D Texture2D::MakeDepthStencilBuffer24_8(int Width, int Height) {
		return std::move(Texture2D{ Width, Height, GL_DEPTH24_STENCIL8, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, 0 });
	}

	Texture2D Texture2D::MakeTextureRGB8UB(int Width, int Height) { return std::move(Texture2D{ Width, Height, InternalFormat::RGB8, TextureFormat::RGB, TextureType::UNSIGNED_BYTE, nullptr }); }
	Texture2D Texture2D::MakeTextureRGBA8UB(int Width, int Height) { return std::move(Texture2D{ Width, Height, InternalFormat::RGBA8, TextureFormat::RGBA, TextureType::UNSIGNED_BYTE, nullptr }); }
	Texture2D Texture2D::MakeTextureRGB16F(int Width, int Height) { return std::move(Texture2D{ Width, Height, InternalFormat::RGB16F, TextureFormat::RGB, TextureType::HALF_FLOAT, nullptr }); }
	Texture2D Texture2D::MakeTextureRGBA16F(int Width, int Height) { return std::move(Texture2D{ Width, Height, InternalFormat::RGBA16F, TextureFormat::RGBA, TextureType::HALF_FLOAT, nullptr }); }
	Texture2D Texture2D::MakeTextureRGB32F(int Width, int Height) { return std::move(Texture2D{ Width, Height, InternalFormat::RGB32F, TextureFormat::RGB, TextureType::FLOAT, nullptr }); }
	Texture2D Texture2D::MakeTextureRGBA32F(int Width, int Height) { return std::move(Texture2D{ Width, Height, InternalFormat::RGBA32F, TextureFormat::RGBA, TextureType::FLOAT, nullptr }); }

	void Texture2D::Filter(TextureFilter Min, TextureFilter Max) {
		Bind();
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GLint(Min));
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GLint(Max));
		Unbind();
	}
	void Texture2D::WrapAll(TextureWrap w) {
		Bind();
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GLint(w));
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GLint(w));
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GLint(w));
		Unbind();
	}
	void Texture2D::FilterWrap(TextureFilter Min, TextureFilter Max, TextureWrap w) {
		Bind();
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GLint(Min));
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GLint(Max));
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GLint(w));
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GLint(w));
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GLint(w));
		Unbind();
	}

	Texture2D Texture2D::MakeTexture8UB(int width, int height, int channels, const float* data) {
		switch (channels) {
			case 1:	return  PVX::OpenGL::Texture2D(
				width, height,
				PVX::OpenGL::InternalFormat::R8UI,
				PVX::OpenGL::TextureFormat::RED,
				PVX::OpenGL::TextureType::FLOAT,
				(void*)data);
			case 2:	return PVX::OpenGL::Texture2D(
				width, height,
				PVX::OpenGL::InternalFormat::RG8UI,
				PVX::OpenGL::TextureFormat::RG,
				PVX::OpenGL::TextureType::FLOAT,
				(void*)data);
			case 3:	return PVX::OpenGL::Texture2D(
				width, height,
				PVX::OpenGL::InternalFormat::RGB8UI,
				PVX::OpenGL::TextureFormat::RGB,
				PVX::OpenGL::TextureType::FLOAT,
				(void*)data);
			case 4:	return PVX::OpenGL::Texture2D(
				width, height,
				PVX::OpenGL::InternalFormat::RGBA8UI,
				PVX::OpenGL::TextureFormat::RGBA,
				PVX::OpenGL::TextureType::FLOAT,
				(void*)data);
		}
	}

	Texture2D Texture2D::MakeTexture16F(int width, int height, int channels, const float* data) {
		switch (channels) {
			case 1:	return  PVX::OpenGL::Texture2D(
				width, height,
				PVX::OpenGL::InternalFormat::R16F,
				PVX::OpenGL::TextureFormat::RED,
				PVX::OpenGL::TextureType::FLOAT,
				(void*)data);
			case 2:	return PVX::OpenGL::Texture2D(
				width, height,
				PVX::OpenGL::InternalFormat::RG16F,
				PVX::OpenGL::TextureFormat::RG,
				PVX::OpenGL::TextureType::FLOAT,
				(void*)data);
			case 3:	return PVX::OpenGL::Texture2D(
				width, height,
				PVX::OpenGL::InternalFormat::RGB16F,
				PVX::OpenGL::TextureFormat::RGB,
				PVX::OpenGL::TextureType::FLOAT,
				(void*)data);
			case 4:	return PVX::OpenGL::Texture2D(
				width, height,
				PVX::OpenGL::InternalFormat::RGBA16F,
				PVX::OpenGL::TextureFormat::RGBA,
				PVX::OpenGL::TextureType::FLOAT,
				(void*)data);
		}
	}
	Texture2D Texture2D::MakeTexture32F(int width, int height, int channels, const float* data) {
		switch (channels) {
			case 1:	return  PVX::OpenGL::Texture2D(
				width, height,
				PVX::OpenGL::InternalFormat::R32F,
				PVX::OpenGL::TextureFormat::RED,
				PVX::OpenGL::TextureType::FLOAT,
				(void*)data);
			case 2:	return PVX::OpenGL::Texture2D(
				width, height,
				PVX::OpenGL::InternalFormat::RG32F,
				PVX::OpenGL::TextureFormat::RG,
				PVX::OpenGL::TextureType::FLOAT,
				(void*)data);
			case 3:	return PVX::OpenGL::Texture2D(
				width, height,
				PVX::OpenGL::InternalFormat::RGB32F,
				PVX::OpenGL::TextureFormat::RGB,
				PVX::OpenGL::TextureType::FLOAT,
				(void*)data);
			case 4:	return PVX::OpenGL::Texture2D(
				width, height,
				PVX::OpenGL::InternalFormat::RGBA32F,
				PVX::OpenGL::TextureFormat::RGBA,
				PVX::OpenGL::TextureType::FLOAT,
				(void*)data);
		}
	}
}

namespace PVX::OpenGL {
	TextureCube::~TextureCube() {
		if (!Ref&&Id)
			glDeleteTextures(1, &Id);
	}
	TextureCube::TextureCube() {}
	TextureCube::TextureCube(int Width, int Height, int TilesX, int TilesY, int Channels, int BytesPerChannel, void* Data, const std::initializer_list<int>& Tiles) {
		Update(Width, Height, TilesX, TilesY, Channels, BytesPerChannel, Data, Tiles);
	}
	TextureCube::TextureCube(int Width, int Height, int TilesX, int TilesY, int InternalFormat, int Format, int Type, void* Data, const std::initializer_list<int>& Tiles) {
		Update(Width, Height, TilesX, TilesY, InternalFormat, Format, Type, Data, Tiles);
	}
	TextureCube::TextureCube(int Width, int Height, int TilesX, int TilesY, PVX::OpenGL::InternalFormat InternalFormat, PVX::OpenGL::TextureFormat Format, PVX::OpenGL::TextureType Type, void* Data, const std::initializer_list<int>& Tiles) {
		Update(Width, Height, TilesX, TilesY, int(InternalFormat), int(Format), int(Type), Data, Tiles);
	}
	void TextureCube::Update(int Width, int Height, int TilesX, int TilesY, int Channels, int BytesPerChannel, void* Data, const std::initializer_list<int>& Tiles) {
		if (!Id)glGenTextures(1, &Id);
		GL_CHECK(glBindTexture(GL_TEXTURE_CUBE_MAP, Id));
		Format = GL_RGB;
		Type = GL_FLOAT;
		InternalFormat = Channels;

		if (Channels == 4) Format = GL_RGBA;
		else if (Channels == 1) Format = GL_LUMINANCE;
		if (BytesPerChannel == 1) Type = GL_UNSIGNED_BYTE;
		PixelSize = Channels * BytesPerChannel;

		Update(Data, Width, Height, TilesX, TilesY, Tiles);
		GL_CHECK(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
		GL_CHECK(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
		GL_CHECK(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
		GL_CHECK(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
		GL_CHECK(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE));
		GL_CHECK(glBindTexture(GL_TEXTURE_CUBE_MAP, 0));
	}
	void TextureCube::Update(int Width, int Height, int TilesX, int TilesY, int InternalFormat, int Format, int Type, void* Data, const std::initializer_list<int>& Tiles) {
	}

	template<int pxSize>
	void UpdateTiles(void* Data, int Width, int Height, int intFormat, int format, int type, int TilesX, int TilesY, const std::initializer_list<int>& Tiles) {
		using PixelType = std::array<unsigned char, pxSize>;

		int TileWidth = Width / TilesX;
		int TileHeight = Height / TilesY;

		auto tiler = PVX::Helpers::Tiler<PixelType>::FromLayout((PixelType*)Data, TileWidth, TileHeight, TilesX, TilesY);
		int pos = 0;
		std::vector<PixelType> buffer(TileWidth * TileHeight);
		for (auto Index : Tiles) {
			size_t cur = 0;
			tiler.ForEachPixelInTile(Index % TilesX, Index / TilesX, TileWidth, TileHeight, [&buffer, &cur](PixelType& p, int x, int y) {
				buffer[cur++] = p;
			});
			GL_CHECK(glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + (pos++), 0, intFormat, TileWidth, TileHeight, 0, format, type, &buffer[0][0]));
		}
	}


	void TextureCube::Update(void* Data, int Width, int Height, int TilesX, int TilesY, const std::initializer_list<int>& Tiles) {
		switch (PixelSize) {
			case 1:  UpdateTiles<1>(Data, Width, Height, InternalFormat, Format, Type, TilesX, TilesY, Tiles); break;
			case 3:  UpdateTiles<3>(Data, Width, Height, InternalFormat, Format, Type, TilesX, TilesY, Tiles); break;
			case 4:  UpdateTiles<4>(Data, Width, Height, InternalFormat, Format, Type, TilesX, TilesY, Tiles); break;
			case 12: UpdateTiles<12>(Data, Width, Height, InternalFormat, Format, Type, TilesX, TilesY, Tiles); break;
			case 16: UpdateTiles<16>(Data, Width, Height, InternalFormat, Format, Type, TilesX, TilesY, Tiles); break;
		}
	}
	void TextureCube::Bind(int Unit) {
		glActiveTexture(GL_TEXTURE0 + Unit);
		GL_CHECK(glBindTexture(GL_TEXTURE_CUBE_MAP, Id));
	}
	void TextureCube::Unbind() {
		GL_CHECK(glBindTexture(GL_TEXTURE_CUBE_MAP, 0));
	}
	void TextureCube::GenerateMipmaps() {
		Bind();
		glGenerateMipmap(GL_TEXTURE_2D);
		Unbind();
	}


	Texture2D Texture2D::MakeMultisampleTextureRGB8UB(int Width, int Height, int Samples) {
		TextureData ret{ 0, Samples, { Width, Height },	PVX::OpenGL::InternalFormat::RGB8 };
		glGenTextures(1, &ret.Id);
		glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, ret.Id);
		glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, Samples, GLenum(ret.InternalFormat), Width, Height, true);
		glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);
		return ret;
	}
	Texture2D Texture2D::MakeMultisampleTextureRGBA8UB(int Width, int Height, int Samples) {
		TextureData ret{ 0, Samples, { Width, Height },	PVX::OpenGL::InternalFormat::RGBA8 };
		glGenTextures(1, &ret.Id);
		glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, ret.Id);
		glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, Samples, GLenum(ret.InternalFormat), Width, Height, true);
		glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);
		return ret;
	}
	Texture2D Texture2D::MakeMultisampleTextureRGB16F(int Width, int Height, int Samples) {
		TextureData ret{ 0, Samples, { Width, Height },	PVX::OpenGL::InternalFormat::RGB16F };
		glGenTextures(1, &ret.Id);
		glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, ret.Id);
		glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, Samples, GLenum(ret.InternalFormat), Width, Height, true);
		glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);
		return ret;
	}
	Texture2D Texture2D::MakeMultisampleTextureRGBA16F(int Width, int Height, int Samples) {
		TextureData ret{ 0, Samples, { Width, Height },	PVX::OpenGL::InternalFormat::RGBA16F };
		glGenTextures(1, &ret.Id);
		glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, ret.Id);
		glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, Samples, GLenum(ret.InternalFormat), Width, Height, true);
		glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);
		return ret;
	}
	Texture2D Texture2D::MakeMultisampleTextureRGB32F(int Width, int Height, int Samples) {
		TextureData ret{ 0, Samples, { Width, Height },	PVX::OpenGL::InternalFormat::RGB32F };
		glGenTextures(1, &ret.Id);
		glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, ret.Id);
		glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, Samples, GLenum(ret.InternalFormat), Width, Height, true);
		glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);
		return ret;
	}
	Texture2D Texture2D::MakeMultisampleTextureRGBA32F(int Width, int Height, int Samples) {
		TextureData ret{ 0, Samples, { Width, Height },	PVX::OpenGL::InternalFormat::RGBA32F };
		glGenTextures(1, &ret.Id);
		glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, ret.Id);
		glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, Samples, GLenum(ret.InternalFormat), Width, Height, true);
		glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);
		return ret;
	}
	Texture2D Texture2D::MakeMultisampleDepthBuffer32F(int Width, int Height, int Samples) {
		TextureData ret{ 0, Samples, { Width, Height },	PVX::OpenGL::InternalFormat(GL_DEPTH_COMPONENT32F) };
		glGenTextures(1, &ret.Id);
		glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, ret.Id);
		glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, Samples, GLenum(ret.InternalFormat), Width, Height, true);
		glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);
		return ret;
	}
	Texture2D Texture2D::MakeMultisampleDepthStencilBuffer24_8(int Width, int Height, int Samples) {
		TextureData ret{ 0, Samples, { Width, Height },	PVX::OpenGL::InternalFormat(GL_DEPTH24_STENCIL8) };
		glGenTextures(1, &ret.Id);
		glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, ret.Id);
		glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, Samples, GLenum(ret.InternalFormat), Width, Height, true);
		glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);
		return ret;
	}
}
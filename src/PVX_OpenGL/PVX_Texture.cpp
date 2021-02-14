#include <PVX_OpenGL.h>
#include <PVX_Image.h>
#include <array>

namespace PVX::OpenGL {
	Texture2D::Texture2D(unsigned int Id) : Id{ Id } {}
	Texture2D::Texture2D() {}
	Texture2D::~Texture2D() {
		if (!Ref&&Id) {
			GL_CHECK(glDeleteTextures(1, &Id));
		}
	}
	Texture2D::Texture2D(int Width, int Height, int Channels, int BytesPerChannel) {
		if (!Id)glGenTextures(1, &Id);

		Format = GL_RGB;
		Type = GL_FLOAT;
		InternalFormat = Channels;

		if (Channels == 4) Format = GL_RGBA;
		else if (Channels == 1) Format = GL_LUMINANCE;
		if (BytesPerChannel == 1) Type = GL_UNSIGNED_BYTE;

		this->Width = Width;
		this->Height = Height;
		this->InternalFormat = InternalFormat;
		this->Format = Format;
		this->Type = Type;

		glBindTexture(GL_TEXTURE_2D, Id);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glBindTexture(GL_TEXTURE_2D, 0);
	}
	Texture2D::Texture2D(int Width, int Height, int Channels, int BytesPerChannel, void* Data) {
		Update(Width, Height, Channels, BytesPerChannel, Data);
	}
	Texture2D::Texture2D(int Width, int Height, int InternalFormat, int Format, int Type, void* Data) {
		Update(Width, Height, InternalFormat, Format, Type, Data);
	}
	Texture2D::Texture2D(int Width, int Height, PVX::OpenGL::InternalFormat internalFormat, TextureFormat Format, TextureType Type, void* Data) {
		Update(Width, Height, (int)internalFormat, (int)Format, (int)Type, Data);
	}
	void Texture2D::Update(int Width, int Height, int InternalFormat, int Format, int Type, void* Data) {
		if (!Id)glGenTextures(1, &Id);
		glBindTexture(GL_TEXTURE_2D, Id);
		this->Width = Width;
		this->Height = Height;
		this->InternalFormat = InternalFormat;
		this->Format = Format;
		this->Type = Type;
		glTexImage2D(GL_TEXTURE_2D, 0, InternalFormat, Width, Height, 0, Format, Type, Data);
		glBindTexture(GL_TEXTURE_2D, 0);
	}
	void Texture2D::Update(void* Data) {
		glBindTexture(GL_TEXTURE_2D, Id);
		glTexImage2D(GL_TEXTURE_2D, 0, InternalFormat, Width, Height, 0, Format, Type, Data);
		glBindTexture(GL_TEXTURE_2D, 0);
	}
	void Texture2D::Update(int Width, int Height, int Channels, int BytesPerChannel, void* Data) {
		if (!Id)glGenTextures(1, &Id);
		glBindTexture(GL_TEXTURE_2D, Id);
		Format = GL_RGB;
		Type = GL_FLOAT;
		InternalFormat = Channels;

		if (Channels == 4) Format = GL_RGBA;
		else if (Channels == 1) Format = GL_LUMINANCE;
		if (BytesPerChannel == 1) Type = GL_UNSIGNED_BYTE;

		glTexImage2D(GL_TEXTURE_2D, 0, InternalFormat, Width, Height, 0, Format, Type, Data);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glBindTexture(GL_TEXTURE_2D, 0);
	}


	void Texture2D::UpdateAndBind(int Width, int Height, int InternalFormat, int Format, int Type, void* Data) {
		if (!Id)glGenTextures(1, &Id);
		glBindTexture(GL_TEXTURE_2D, Id);
		this->Width = Width;
		this->Height = Height;
		this->InternalFormat = InternalFormat;
		this->Format = Format;
		this->Type = Type;
		glTexImage2D(GL_TEXTURE_2D, 0, InternalFormat, Width, Height, 0, Format, Type, Data);
	}
	void Texture2D::UpdateAndBind(const void* Data) {
		glBindTexture(GL_TEXTURE_2D, Id);
		glTexImage2D(GL_TEXTURE_2D, 0, InternalFormat, Width, Height, 0, Format, Type, Data);
	}
	void Texture2D::GenerateMipmaps() {
		Bind();
		glGenerateMipmap(GL_TEXTURE_2D);
		Unbind();
	}
	void Texture2D::UpdateAndBind(int Width, int Height, int Channels, int BytesPerChannel, void* Data) {
		if (!Id)glGenTextures(1, &Id);
		glBindTexture(GL_TEXTURE_2D, Id);
		Format = GL_RGB;
		Type = GL_FLOAT;
		InternalFormat = Channels;

		if (Channels == 4) Format = GL_RGBA;
		else if (Channels == 1) Format = GL_LUMINANCE;
		if (BytesPerChannel == 1) Type = GL_UNSIGNED_BYTE;

		glTexImage2D(GL_TEXTURE_2D, 0, InternalFormat, Width, Height, 0, Format, Type, Data);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	}

	void Texture2D::Resize(int Width, int Height) {
		this->Width = Width;
		this->Height = Height;
		glBindTexture(GL_TEXTURE_2D, Id);
		glTexImage2D(GL_TEXTURE_2D, 0, InternalFormat, Width, Height, 0, Format, Type, 0);
		glBindTexture(GL_TEXTURE_2D, 0);
	}
	void Texture2D::Bind(int Unit) {
		glActiveTexture(GL_TEXTURE0 + Unit);
		glBindTexture(GL_TEXTURE_2D, Id);
	}
	void Texture2D::Unbind() {
		glBindTexture(GL_TEXTURE_2D, 0);
	}
	Texture2D Texture2D::MakeDepthBuffer32F(int Width, int Height) {
		Texture2D ret;
		ret.Update(Width, Height, GL_DEPTH_COMPONENT32F, GL_DEPTH_COMPONENT, GL_FLOAT, 0);
		ret.Bind();
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		ret.Unbind();
		return ret;
	}
	Texture2D Texture2D::MakeStencilBuffer(int Width, int Height) {
		Texture2D ret;
		ret.Update(Width, Height, 1, GL_STENCIL_COMPONENTS, GL_UNSIGNED_BYTE, 0);
		ret.Bind();
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		ret.Unbind();
		return ret;
	}
	Texture2D Texture2D::MakeDepthStencilBuffer24_8(int Width, int Height) {
		Texture2D ret;
		ret.Update(Width, Height, GL_DEPTH24_STENCIL8, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, 0);
		ret.Bind();
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		ret.Unbind();
		return ret;
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
}
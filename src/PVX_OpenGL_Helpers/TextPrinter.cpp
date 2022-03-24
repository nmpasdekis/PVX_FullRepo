#include <PVX_OpenGL_Helpers.h>
#include <PVX_Image.h>
#include <PVX_File.h>

namespace PVX::OpenGL::Helpers {

	TextPrinter::TextPrinter(ResourceManager& mgr, const std::string& Texture, int xTiles, int yTiles, const PVX::iVector2D& ScreenSize) :
		rManager{ mgr },
		ScreenSize{ ScreenSize },
		Characters{ },
		Texts(false, BufferUsege::STREAM_DRAW),
		Shaders{
			mgr.Programs.Get("TextPrinter", [] {
				return PVX::OpenGL::Program {
					{ PVX::OpenGL::Shader::ShaderType::VertexShader, PVX::IO::ReadText("Shaders\\TextVertexShader.glsl") },
					{ PVX::OpenGL::Shader::ShaderType::FragmentShader, PVX::IO::ReadText("Shaders\\TextFragShader.glsl") }
				};
			})
	},
		Atlas{ mgr.Textures2D.Get(Texture, [&] {
			auto data = PVX::ImageData::LoadRaw(Texture.c_str());
			return PVX::OpenGL::Texture2D::MakeTexture32F(data.Width, data.Height, data.Channels, data.Data.data());
		}) },
		xTiles{ xTiles }, yTiles{ yTiles },
		TileSize{ 1.0f / xTiles, 1.0f / yTiles },
		geo{
			mgr.Geometry.Get("TextPrinter", [&]() -> PVX::OpenGL::Geometry {
				return {
					PrimitiveType::TRIANGLES,
					{ 0, 1, 2, 0, 2, 3 },
					{
						{
							[&] {
								float h = (Atlas.GetHeight() * xTiles) * 1.0f / (Atlas.GetWidth() * yTiles);
								CharInstance verts[4]{
									{ { 0.0f, 0.0f }, { 0.0f, 1.0f - TileSize.Height } },
									{ { 1.0f, 0.0f }, { TileSize.Width, 1.0f - TileSize.Height } },
									{ { 1.0f, h }, { TileSize.Width, 1.0f } },
									{ { 0.0f, h }, { 0.0f, 1.0f } },
								};
								return VertexBuffer(verts, sizeof(CharInstance)*4);
							}(),
							{
								{ AttribType::FLOAT, 2, 0 },
								{ AttribType::FLOAT, 2, 0 },
							}
						},
						{
							Characters,
							{
								{ AttribType::FLOAT, 2, 1 },
							}
						}
					}
				};
			})
	}
	{}

	void TextPrinter::AddText(const std::string_view& Text, const PVX::Vector2D& pos, float scale, const PVX::Vector4D& Color) {
		TextBufferData.push_back({ Color, {
				scale * 2.0f/ ScreenSize.Width, 0, 0, 0,
				0, scale * 2.0f / ScreenSize.Height, 0, 0,
				0, 0, 0, 0,
				(2.0f * pos.x / ScreenSize.Width - 1.0f), (2.0f * pos.y / ScreenSize.Height - 1.0f), 0, 1.0f
		} });
		for (auto& t : Text) {
			Stream.push_back({
				(t % xTiles)* TileSize.Width,
				-(t / xTiles) * TileSize.Height
			});
		}
		uint32_t off = 0;
		if (cmds.size()) off = cmds.back().baseInstance + cmds.back().instanceCount;

		cmds.push_back({ 6, uint32_t(Text.size()), 0, 0, off });
	}

	void TextPrinter::Render() {
		glEnable(GL_BLEND);
		glDisable(GL_DEPTH_TEST);
		Texts.Update(TextBufferData.size() * sizeof(DrawData), TextBufferData.data());
		Characters.Update(Stream.data(), Stream.size() * sizeof(PVX::Vector2D));
		Shaders.Bind();
		BindBuffer(0, Texts);
		Atlas.BindToUnit(0);
		geo.Bind();

		glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, cmds.data(), cmds.size(), 0);
		geo.Unbind();

		cmds.clear();
		TextBufferData.clear();
		Stream.clear();

		glDisable(GL_BLEND);
	}
}
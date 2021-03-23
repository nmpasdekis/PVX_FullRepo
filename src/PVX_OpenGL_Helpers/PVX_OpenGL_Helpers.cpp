#include "Include/PVX_OpenGL_Helpers.h"
#include <vector>
#include <functional>
#include <PVX.inl>
#include <PVX_Image.h>

namespace PVX::OpenGL::Helpers {
	using namespace PVX;
	using namespace PVX::OpenGL;

	constexpr uint32_t HasNormal = 1;
	constexpr uint32_t HasUV = 2;
	constexpr uint32_t HasSkin = 4;
	constexpr uint32_t HasMorphs = 8;

	constexpr uint32_t HasTexColor = 16;
	constexpr uint32_t HasTexPBR = 32;
	constexpr uint32_t HasTexBump = 64;

	PVX::JSON::Item Variable(const std::string& Name, const std::string& Type, const PVX::JSON::Item& Value) {
		PVX::JSON::Item ret = PVX::JSON::jsElementType::Object;
		ret[L"Name"] = Name;
		ret[L"Type"] = Type;
		if (!Value.IsNullOrUndefined())
			ret[L"Value"] = Value;
		return ret;
	}

	PVX::JSON::Item Describe(const PVX::Object3D::PlaneMaterial& Mat) {
		return {};
	}

	Attribute FromObjectAttribute(const Object3D::VertexAttribute& attr) {
		constexpr AttribType TypeMap[] = { AttribType::UNSIGNED_BYTE, AttribType::INT, AttribType::FLOAT, AttribType::DOUBLE };
		constexpr bool TypeIsIntMap[] = { true, true, false, false };
		const char* vecs[4][5] = {
			{ "", "int", "ivec2", "ivec3", "ivec4" },
			{ "", "int", "ivec2", "ivec3", "ivec4" },
			{ "", "float", "vec2", "vec3", "vec4" },
			{ "", "double", "dvec2", "dvec3", "dvec4" },
		};

		return {
			TypeIsIntMap[(int)attr.Type],
			attr.Name,
			attr.Count,
			TypeMap[(int)attr.Type],
			false,
			attr.Offset,
			std::string(vecs[(int)attr.Type][attr.Count])
		};
	}

	Geometry ToGeomerty(const PVX::OpenGL::InterleavedArrayObject& obj, bool OldVersion) {
		return {
			PVX::OpenGL::PrimitiveType(obj.Mode),
			int(obj.Index.size()),
			obj.Index,
			{
				{ obj.Data, obj.Attributes, obj.Stride }
			},
			OldVersion
		};
	}
	Geometry ToGeomerty(const PVX::OpenGL::BufferObject& obj, bool OldVersion) {
		return {
			PVX::OpenGL::PrimitiveType(obj.Mode),
			obj.IndexCount,
			obj.Indices,
			{
				{ obj.Vertices, obj.Attributes, obj.Stride }
			},
			OldVersion
		};
	}

	Geometry ToGeomerty(const Object3D::ObjectSubPart& so, bool old) {
		if (so.BlendAttributes.size()) {
			return {
				PrimitiveType::TRIANGLES,
				int(so.Index.size()),
				so.Index,
				{
					{
						so.VertexData,
						PVX::Map(so.Attributes, [](const auto& a) { return FromObjectAttribute(a); }),
						so.Stride
					},
					{
						so.BlendShapeData,
						PVX::Map(so.BlendAttributes, [](const auto& a) { return FromObjectAttribute(a); }),
						int(so.BlendShapeData.size() / so.VertexCount)
					}
				},
				old
			};
		} else {
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
				old
			};
		}
	}

	void Transform::TransformAll(TransformConstant& Const, const TransformConstant* All) const {
		Const.Result = All[Const.ParentIndex].Result *
			PVX::mTran(Position) * Const.PostTranslate *
			PVX::Rotate(Const.RotationOrder, Rotation) * Const.PostRotate *
			PVX::mScale(Scale) * Const.PostScale;
	}
	void Transform::TransformNoParent(TransformConstant& Const, const TransformConstant* All) const {
		Const.Result =
			PVX::mTran(Position) * Const.PostTranslate *
			PVX::Rotate(Const.RotationOrder, Rotation) * Const.PostRotate *
			PVX::mScale(Scale) * Const.PostScale;
	}
	void Transform::TransformAll_Identity(TransformConstant& Const, const TransformConstant* All) const {
		Const.Result = All[Const.ParentIndex].Result *
			PVX::mTran(Position) *
			PVX::Rotate(Const.RotationOrder, Rotation) *
			PVX::mScale(Scale);
	}
	void Transform::TransformNoParent_Identity(TransformConstant& Const, const TransformConstant* All) const {
		Const.Result =
			PVX::mTran(Position) *
			PVX::Rotate(Const.RotationOrder, Rotation) *
			PVX::mScale(Scale);
	}
	void Transform::PreTransformed(TransformConstant& Const, const TransformConstant* All) const {
		Const.Result = All[Const.ParentIndex].Result * Matrix;
	}
	void Transform::PreTransformedNoParent(TransformConstant& Const, const TransformConstant* All) const {
		Const.Result = Matrix;
	}

	void ObjectGL::UpdateInstances() {
		DrawCount = 0;
		if (!ActiveInstances) return;
		for (auto& p: Parts) {
			p.TransformBufferPtr = p.TransformBuffer.Map<PVX::Matrix4x4>();
			if (p.MorphCount) p.MorphPtr = p.MorphControlBuffer.Map<float>();
		}

		for (auto& instData: Instances) {
			if (!instData->Active) continue;

			auto& trans = instData->Transforms;
			int curPart = 0;
			for (auto& ct: TransformConstants) {
				auto& tran = trans[curPart++];

				tran.GetTransform(ct, TransformConstants.data());
			}
			for (auto& p : Parts) {
				if (!p.PostTransform.size()) {
					p.TransformBufferPtr[DrawCount] = TransformConstants[p.UseMatrices[0]].Result;
				} else {
					size_t index = p.UseMatrices.size() * DrawCount;
					for (auto i = 0; i < p.UseMatrices.size(); i++) {
						p.TransformBufferPtr[index++] = TransformConstants[p.UseMatrices[i]].Result *  p.PostTransform[i];
					}
				}
			}
			if (instData->MorphControls.size()) {
				float* Morphs = instData->MorphControls.data();
				for (auto& p : Parts) {
					memcpy(p.MorphPtr + p.MorphCount * DrawCount, Morphs, p.MorphCount * sizeof(float));
					Morphs += p.MorphCount;
				}
			}
			DrawCount++;
		}
		for (auto& p : Parts) {
			p.TransformBuffer.Unmap();
			if (p.MorphCount) p.MorphControlBuffer.Unmap();
		}
	}

	void Renderer::UpdateInstances() {
		for (auto& [n, o] : Objects) {
			o->UpdateInstances();
		}
	}

	void Renderer::DrawInstances() {
		//PVX::OpenGL::BindBuffer(0, CameraBuffer);
		glBindSampler(0, GeneralSampler.Get());
		glBindSampler(1, GeneralSampler.Get());
		glBindSampler(2, GeneralSampler.Get());

		for (auto& [oid, Object2]: Objects) {
			auto& Object = *Object2.get();
			if (!Object.DrawCount) continue;
			for (auto& p: Object.Parts) {
				BindBuffer(3, p.TransformBuffer);
				BindBuffer(4, p.MorphControlBuffer);
				for (auto& pp: p.SubPart) {
					auto& mat = Object.Materials[pp.MaterialIndex];
					if (mat.PBR.Color.a < 1.0f) continue;
					BindBuffer(2, mat.Data);
					BindBuffer(5, pp.Morph);
					if (mat.Color_Tex) {
						glBindTextureUnit(0, mat.Color_Tex);
					}
					if (mat.Normal_Tex) {
						glBindTextureUnit(2, mat.Normal_Tex);
					}

					pp.ShaderProgram.Bind();
					pp.ShaderProgram.BindUniform(pp.BoneCountIndex, int(p.PostTransform.size()));
					pp.ShaderProgram.BindUniform(pp.MorphCountIndex, int(p.MorphCount));					
					
					pp.Mesh.Draw(int(Object.DrawCount));
				}
			}
		}
		glEnable(GL_BLEND);
		for (auto& [oid, Object2]: Objects) {
			auto& Object = *Object2.get();
			if (!Object.DrawCount) continue;
			for (auto& p: Object.Parts) {
				BindBuffer(3, p.TransformBuffer);
				BindBuffer(4, p.MorphControlBuffer);
				for (auto& pp: p.SubPart) {
					auto& mat = Object.Materials[pp.MaterialIndex];
					if (mat.PBR.Color.a == 1.0f) continue;

					BindBuffer(2, mat.Data);
					BindBuffer(5, pp.Morph);
					if (mat.Color_Tex) {
						glBindTextureUnit(0, mat.Color_Tex);
					}
					if (mat.Normal_Tex) {
						glBindTextureUnit(2, mat.Normal_Tex);
					}

					pp.ShaderProgram.Bind();
					pp.ShaderProgram.BindUniform(pp.BoneCountIndex, int(p.PostTransform.size()));
					pp.ShaderProgram.BindUniform(pp.MorphCountIndex, int(p.MorphCount));

					pp.Mesh.Draw(int(Object.DrawCount));
				}
			}
		}
		glDisable(GL_BLEND);
		glUseProgram(0);
	}
	void InstanceData::Animate(float time) {
		if (Object.AnimationMaxFrame) {
			size_t f = (size_t(time * Object.FrameRate)) % Object.AnimationMaxFrame;
			size_t i = 0;
			for (auto& t : Transforms) {
				t.Position = Object.Animation[i][f].Position;
				t.Rotation = Object.Animation[i][f].Rotation;
				t.Scale = Object.Animation[i][f].Scale;
				i++;
			}
		}
	}
	void InstanceData::SetActive(bool isActive) {
		if (isActive!=Active) {
			Active = isActive;
			Object.ActiveInstances += Active ? 1 : -1;
		}
	}
	void ObjectGL_SubPart::SetProgram(const PVX::OpenGL::Program& prog) {
		ShaderProgram = prog;
		//ShaderProgram.Bind();
		ShaderProgram.SetUniformBlockIndex("Camera", 0);
		ShaderProgram.SetUniformBlockIndex("Material", 2);
		ShaderProgram.SetShaderStrorageIndex("Transform", 3);
		ShaderProgram.SetShaderStrorageIndex("MorphControl", 4);
		ShaderProgram.SetShaderStrorageIndex("MorphData", 5);
		MorphCountIndex = ShaderProgram.UniformLocation("MorphCount");
		BoneCountIndex = ShaderProgram.UniformLocation("BoneCount");

		ShaderProgram.SetTextureIndex("Color_Tex", 0);
		ShaderProgram.SetTextureIndex("PBR_Tex", 1);
		ShaderProgram.SetTextureIndex("Bump_Tex", 2);
		//ShaderProgram.Unbind();
	}
	TextPrinter::TextPrinter(const std::string& Texture, int xTiles, int yTiles, const PVX::iVector2D& ScreenSize) : Text(true, PVX::OpenGL::BufferUsege::STREAM_DRAW),
		xTiles{ xTiles }, yTiles{ yTiles }, ScreenSize { ScreenSize },
		Shaders{ {
			{ PVX::OpenGL::Shader::ShaderType::VertexShader, R"vShader(#version 440

layout(std140, binding = 0) uniform Text{
	mat4 Transform;
	vec4 textColor;
	int xTiles, yTiles;
	vec2 TileSize;
	ivec4 Chars[64];
};

in vec4 pos;
in vec2 UV;
in int gl_InstanceID;

out vec2 uv;

void main(){
	int printChar = Chars[gl_InstanceID/4][gl_InstanceID%4];

	int xTile = printChar % xTiles;
	int yTile = printChar / xTiles;
	uv = UV + vec2(xTile * (1.0/xTiles), -yTile * (1.0/yTiles));
	gl_Position = Transform * (pos + vec4(gl_InstanceID * TileSize.x, 0, 0, 0));
})vShader"},
			{ PVX::OpenGL::Shader::ShaderType::FragmentShader, R"vShader(#version 440
layout(binding = 0) uniform sampler2D Atlas;

layout(std140, binding = 0) uniform Text{
	mat4 Transform;
	vec4 textColor;
	int xTiles, yTiles;
	vec2 TileSize;
	ivec4 Chars[64];
};

in vec2 uv;
out vec4 Color;

void main(){
	Color = vec4(textColor.xyz, textColor.w * texture(Atlas, uv).r);
	//Color = vec4(1,0,0,1);
})vShader"},
		} }
	{
		GL_CHECK(;);
		Text.Update(2 * 16 + 4 * 4 + 4 * 256);
		GL_CHECK(;);
		auto data = PVX::ImageData::LoadRaw(Texture.c_str());
		GL_CHECK(;);
		Atlas = PVX::OpenGL::Texture2D::MakeTexture32F(data.Width, data.Height, data.Channels, data.Data.data());
		GL_CHECK(;);

		TileSize = { float(Atlas.GetWidth() / xTiles), float(Atlas.GetHeight()/ yTiles) };
		
		geo = [&] {
			float tw = 1.0f / xTiles;
			float th = 1.0f / yTiles;
			PVX::OpenGL::ObjectBuilder ob;
			
			ob.Begin(GL_QUADS);

			ob.TexCoord(0, 1.0f - th);
			ob.Vertex(0, 0, 0);

			ob.TexCoord(tw, 1.0f - th);
			ob.Vertex(TileSize.Width, 0, 0);

			ob.TexCoord(tw, 1.0f);
			ob.Vertex(TileSize.Width, TileSize.Height, 0);

			ob.TexCoord(0, 1.0f);
			ob.Vertex(0, TileSize.Height, 0);

			ob.End();

			return ToGeomerty(ob.Build(), false);
		}();
		GL_CHECK(;);
	}
	void TextPrinter::Render(const std::string& Text, const PVX::Vector2D& pos, const PVX::Vector4D& Color) {
		struct {
			PVX::Matrix4x4 Transform;
			PVX::Vector4D color;
			int xTiles, yTile;
			PVX::Vector2D TileSize;
			int Chars[256];
		} textData{
			{ 
				2.0f/ ScreenSize.Width, 0, 0, 0, 
				0, 2.0f / ScreenSize.Height, 0, 0,
				0, 0, 0, 0,
				2.0f * pos.x / ScreenSize.Width - 1.0f, 2.0f * pos.y / ScreenSize.Height - 1.0f, 0, 1.0f
			},
			Color,
			xTiles, yTiles,
			TileSize
		};
		int i = 0;
		for (auto& t : Text) {
			textData.Chars[i] = t;
			//textData.Chars[i] = 1;
			i++;
		}
		this->Text.Update(sizeof(textData), &textData);
		glEnable(GL_BLEND);
		glDisable(GL_DEPTH_TEST);
		Shaders.Bind();
		BindBuffer(0, this->Text);
		Atlas.BindToUnit(0);
		geo.Draw(Text.size());
		glDisable(GL_BLEND); 
		geo.Unbind();
		Shaders.Unbind();
	}
}
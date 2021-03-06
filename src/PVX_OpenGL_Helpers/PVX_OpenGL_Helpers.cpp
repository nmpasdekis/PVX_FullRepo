#include "Include/PVX_OpenGL_Helpers.h"
#include <vector>
#include <functional>
#include <PVX.inl>

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

	ObjectGL::ObjectGL(const Object3D::Object& obj) {
		TransformConstants.reserve(obj.Heirarchy.size());
		for (auto& t : obj.Heirarchy) {
			TransformConstants.emplace_back(t);
			InitialTransform.push_back({ t.Position, t.Rotation, t.Scale });
		}

		std::map<std::string, int> MatLookup;
		for (auto& [Name, m]: obj.Material) {
			MatLookup[Name] = int(MatLookup.size());
			auto& mat = Materials.emplace_back(DataPBR {
				{ m.Color, 1.0f - m.Transparency },
				0,
				1.0f - m.SpecularFactor /(m.AmbientFactor + m.DiffuseFactor + m.SpecularFactor),
				m.Bump,
				m.EmissiveFactor
			});
		}

		Parts.reserve(obj.Parts.size());
		for (auto& p : obj.Parts) {
			auto& pp = Parts.emplace_back();
			if (!p.BoneNodes.size()) {
				pp.UseMatrices.push_back(IndexOf(obj.Heirarchy, [&](const Object3D::Transform& t) { return t.Name == p.TransformNode; }));
			} else {
				for (auto i = 0; i<p.BoneNodes.size(); i++) {
					pp.PostTransform.reserve(p.BoneNodes.size());
					pp.UseMatrices.reserve(p.BoneNodes.size());

					pp.UseMatrices.push_back(IndexOf(obj.Heirarchy, [&](const Object3D::Transform& t) { return p.BoneNodes[i] == t.Name; }));
					pp.PostTransform.push_back(p.BonePostTransform[i]);
				}
			}
			for (auto& [Mat, SubPart]: p.Parts) {
				auto& sp = pp.SubPart.emplace_back();
				sp.MaterialIndex = MatLookup[Mat];
				sp.Mesh = ToGeomerty2(SubPart);
			}
		}
	}

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
		constexpr GLenum TypeMap[] = { GL_UNSIGNED_BYTE, GL_INT, GL_FLOAT, GL_DOUBLE };
		const char* vecs[4][5] = {
			{ "", "int", "ivec2", "ivec3", "ivec4" },
			{ "", "int", "ivec2", "ivec3", "ivec4" },
			{ "", "float", "vec2", "vec3", "vec4" },
			{ "", "double", "dvec2", "dvec3", "dvec4" },
		};

		return {
			attr.Name,
			attr.Count,
			TypeMap[(int)attr.Type],
			false,
			attr.Offset,
			std::string(vecs[(int)attr.Type][attr.Count])
		};
	}

	Geometry ToGeomerty(const PVX::OpenGL::InterleavedArrayObject& obj) {
		return {
			PVX::OpenGL::PrimitiveType(obj.Mode),
			int(obj.Index.size()),
			obj.Index,
			{
				{ obj.Data, obj.Attributes, obj.Stride }
			},
			true
		};
	}
	Geometry ToGeomerty(const PVX::OpenGL::BufferObject& obj) {
		return {
			PVX::OpenGL::PrimitiveType(obj.Mode),
			obj.IndexCount,
			obj.Indices,
			{
				{ obj.Vertices, obj.Attributes, obj.Stride }
			},
			true
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
}
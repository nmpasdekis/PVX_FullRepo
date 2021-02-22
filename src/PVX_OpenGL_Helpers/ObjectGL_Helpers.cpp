#include <PVX_OpenGL_Object.h>
#include <PVX_Object3D.h>

namespace PVX::OpenGL::Helpers{
	using namespace PVX;
	using namespace PVX::OpenGL;

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
						so.Stride
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
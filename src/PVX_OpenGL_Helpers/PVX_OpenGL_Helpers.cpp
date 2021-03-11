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
				if (ct.ParentIndex!=-1) {
					ct.Result = TransformConstants[ct.ParentIndex].Result *
						PVX::mTran(tran.Position) * ct.PostTranslate *
						PVX::Rotate(ct.RotationOrder, tran.Rotation) * ct.PostRotate *
						PVX::mScale(tran.Scale) * ct.PostScale;
				} else {
					ct.Result =
						PVX::mTran(tran.Position) * ct.PostTranslate *
						PVX::Rotate(ct.RotationOrder, tran.Rotation) * ct.PostRotate *
						PVX::mScale(tran.Scale) * ct.PostScale;
				}
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

	void Renderer::DrawInstances() {
		for (auto& [n, o] : Objects) {
			o->UpdateInstances();
		}
		PVX::OpenGL::BindBuffer(0, CameraBuffer);

		for (auto& [oid, Object2]: Objects) {
			auto& Object = *Object2.get();
			if (!Object.DrawCount) continue;
			for (auto& p: Object.Parts) {
				BindBuffer(2, p.TransformBuffer);
				BindBuffer(3, p.MorphControlBuffer);
				for (auto& pp: p.SubPart) {
					auto& mat = Object.Materials[pp.MaterialIndex];

					BindBuffer(1, mat.Data);
					BindBuffer(4, pp.Morph);
					if (mat.Color_Tex) glBindTextureUnit(0, mat.Color_Tex);
					

					pp.ShaderProgram.Use();
					pp.ShaderProgram.SetBoneCount(p.PostTransform.size());
					pp.ShaderProgram.SetMorphCount(p.MorphCount);
					
					
					pp.Mesh.Draw(Object.DrawCount);
				}
			}
		}
		glUseProgram(0);
	}
	void InstanceData::SetActive(bool isActive) {
		if (isActive!=Active) {
			Active = isActive;
			Object.ActiveInstances += Active ? 1 : -1;
		}
	}
}
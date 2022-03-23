#include <PVX_OpenGL_Helpers.h>
#include <vector>
#include <functional>
#include <PVX.inl>
#include <PVX_Image.h>
#include <PVX_File.h>

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

	void Transform::Transform_All(const TransformConstant& Const) {
		Matrix = PVX::mTran(Position) * Const.PostTranslate *
			PVX::Rotate(Const.RotationOrder, Rotation) * Const.PostRotate *
			PVX::mScale(Scale) * Const.PostScale;
	}

	void Transform::Transform_Identity(const TransformConstant& Const) {
		Matrix = PVX::mTran(Position) *
			PVX::Rotate(Const.RotationOrder, Rotation) *
			PVX::mScale(Scale);
	}

	void Transform::DoNothing(const TransformConstant& Const) {}

	PVX::Matrix4x4 Transform::GetMatrix_WithParent(const Transform* tr, int Parent) const {
		return tr[Parent].Matrix * Matrix;
	}

	PVX::Matrix4x4 Transform::GetMatrix_WithoutParent(const Transform* tr, int Parent) const {
		return Matrix;
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


	void ObjectGL::UpdateInstances_1() {
		//for (auto& inst: Instances) {
#pragma omp parallel for 
		for(auto i=0;i<Instances.size();i++){
			auto& inst = Instances[i];
			if (!inst->Active) continue;
			int c = 0;
			for (auto& tr : inst->Transforms)
				tr.GetTransform(TransformConstants[c++]);
		}
	}
	void ObjectGL::UpdateInstances_2() {
		//for (auto& inst: Instances) {
#pragma omp parallel for 
		for (auto i = 0; i<Instances.size(); i++) {
			auto& inst = Instances[i];
			if (!inst->Active) continue;
			int c = 0;
			for (auto& tr : inst->Transforms) {
				tr.Matrix = tr.GetMatrix(inst->Transforms.data(), TransformConstants[c++].ParentIndex);
			}
		}
	}
	void ObjectGL::UpdateInstances_3() {
		for (auto& p: Parts) {
			p.TransformBufferPtr = p.TransformBuffer.Map<PVX::Matrix4x4>();
		}

#pragma omp parallel for 
		for (auto ii = 0; ii<Parts.size(); ii++) {
			auto& p = Parts[ii];
			size_t index = 0;
			if (!p.PostTransform.size()) {
				auto m = p.UseMatrices[0];
				for (auto& inst: Instances) {
					if (!inst->Active) continue;
					p.TransformBufferPtr[index++] = inst->Transforms[m].Matrix;
				}
			} else {
				for (auto& inst: Instances) {
					if (!inst->Active) continue;
					int i = 0;
					for (auto& m : p.UseMatrices) {
						p.TransformBufferPtr[index++] = inst->Transforms[m].Matrix * p.PostTransform[i++];
					}
				}
			}
		}
		for (auto& p: Parts) {
			p.TransformBuffer.Unmap();
		}
	}

	void ObjectGL::UpdateInstances() {
		if (!ActiveInstances) return;
		for (auto& inst: Instances) {
			if (!inst->Active) continue;
			int c = 0;
			for (auto& tr : inst->Transforms)
				tr.GetTransform(TransformConstants[c++]);
		}
		for (auto& inst: Instances) {
			if (!inst->Active) continue;
			int c = 0;
			for (auto& tr : inst->Transforms) {
				tr.Matrix = tr.GetMatrix(inst->Transforms.data(), TransformConstants[c++].ParentIndex);
			}
		}

		for (auto& p: Parts) {
			auto tPtr = p.TransformBuffer.Map<PVX::Matrix4x4>();
			size_t index = 0;
			if (!p.PostTransform.size()) {
				auto m = p.UseMatrices[0];
				for (auto& inst: Instances) {
					if (!inst->Active) continue;
					tPtr[index++] = inst->Transforms[m].Matrix;
				}
			}else{
				for (auto& inst: Instances) {
					if (!inst->Active) continue;
					int i = 0;
					for (auto& m : p.UseMatrices) {
						tPtr[index++] = inst->Transforms[m].Matrix * p.PostTransform[i++];
					}
				}
			}
			p.TransformBuffer.Unmap();
		}
		if (MorphCount) {
			for (auto& p: Parts) {
				if (!p.MorphCount) continue;

				auto mPtr = p.MorphControlBuffer.Map<float>();



				p.MorphControlBuffer.Unmap();
			}
		}
	}

	//void ObjectGL::UpdateInstances() {
	//	DrawCount = 0;
	//	if (!ActiveInstances) return;
	//	for (auto& p: Parts) {
	//		p.TransformBufferPtr = p.TransformBuffer.Map<PVX::Matrix4x4>();
	//		if (p.MorphCount) p.MorphPtr = p.MorphControlBuffer.Map<float>();
	//	}

	//	for (auto& instData: Instances) {
	//		if (!instData->Active) continue;

	//		auto& trans = instData->Transforms;
	//		int curPart = 0;
	//		for (auto& ct: TransformConstants) {
	//			auto& tran = trans[curPart++];

	//			tran.GetTransform(ct, TransformConstants.data());
	//		}
	//		for (auto& p : Parts) {
	//			if (!p.PostTransform.size()) {
	//				p.TransformBufferPtr[DrawCount] = TransformConstants[p.UseMatrices[0]].Result;
	//			} else {
	//				size_t index = p.UseMatrices.size() * DrawCount;
	//				for (auto i = 0; i < p.UseMatrices.size(); i++) {
	//					p.TransformBufferPtr[index++] = TransformConstants[p.UseMatrices[i]].Result *  p.PostTransform[i];
	//				}
	//			}
	//		}
	//		if (instData->MorphControls.size()) {
	//			float* Morphs = instData->MorphControls.data();
	//			for (auto& p : Parts) {
	//				memcpy(p.MorphPtr + p.MorphCount * DrawCount, Morphs, p.MorphCount * sizeof(float));
	//				Morphs += p.MorphCount;
	//			}
	//		}
	//		DrawCount++;
	//	}
	//	for (auto& p : Parts) {
	//		p.TransformBuffer.Unmap();
	//		if (p.MorphCount) p.MorphControlBuffer.Unmap();
	//	}
	//}

	void Renderer::UpdateInstances() {
		for (auto& [n, o] : Objects) {
			o->UpdateInstances_1();
		}
		for (auto& [n, o] : Objects) {
			o->UpdateInstances_2();
		}
		for (auto& [n, o] : Objects) {
			o->UpdateInstances_3();
		}
	}

	void Renderer::DrawInstances() {
		//PVX::OpenGL::BindBuffer(0, CameraBuffer);
		glBindSampler(0, GeneralSampler.Get());
		glBindSampler(1, GeneralSampler.Get());
		glBindSampler(2, GeneralSampler.Get());

		for (auto& [oid, Object2]: Objects) {
			auto& Object = *Object2.get();
			if (!Object.ActiveInstances) continue;
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
					
					pp.Mesh.Draw(int(Object.ActiveInstances));
				}
			}
		}
		glEnable(GL_BLEND);
		for (auto& [oid, Object2]: Objects) {
			auto& Object = *Object2.get();
			if (!Object.ActiveInstances) continue;
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

					pp.Mesh.Draw(int(Object.ActiveInstances));
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
			for (i = 1; i<Transforms.size(); i++) {
				auto& t = Transforms[i];
				t.Position = Object.Animation[i][f].Position;
				t.Rotation = Object.Animation[i][f].Rotation;
				t.Scale = Object.Animation[i][f].Scale;
			}
			//for (auto& t : Transforms) {
			//	t.Position = Object.Animation[i][f].Position;
			//	t.Rotation = Object.Animation[i][f].Rotation;
			//	t.Scale = Object.Animation[i][f].Scale;
			//	i++;
			//}
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
	{
	}

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
				- (t / xTiles) * TileSize.Height
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
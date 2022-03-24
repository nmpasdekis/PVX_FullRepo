#include <PVX_OpenGL_Helpers.h>
#include <vector>
#include <functional>
#include <PVX.inl>
#include <PVX_Image.h>
#include <PVX_File.h>
#include <limits>

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

	//void Transform::Transform_All(const TransformConstant& Const) {
	//	Matrix = PVX::mTran(Position) * Const.PostTranslate *
	//		PVX::Rotate(Const.RotationOrder, Rotation) * Const.PostRotate *
	//		PVX::mScale(Scale) * Const.PostScale;
	//}

	//void Transform::Transform_Identity(const TransformConstant& Const) {
	//	Matrix = PVX::mTran(Position) *
	//		PVX::Rotate(Const.RotationOrder, Rotation) *
	//		PVX::mScale(Scale);
	//}

	void Transform::DoNothing(const TransformConstant& Const) {}

	PVX::Matrix4x4 Transform::GetMatrix_WithParent(const Transform* tr, int Parent) const {
		return tr[Parent].Matrix * Matrix;
	}

	PVX::Matrix4x4 Transform::GetMatrix_WithoutParent(const Transform* tr, int Parent) const {
		return Matrix;
	}


	//void Transform::TransformAll(TransformConstant& Const, const TransformConstant* All) const {
	//	Const.Result = All[Const.ParentIndex].Result *
	//		PVX::mTran(Position) * Const.PostTranslate *
	//		PVX::Rotate(Const.RotationOrder, Rotation) * Const.PostRotate *
	//		PVX::mScale(Scale) * Const.PostScale;
	//}
	//void Transform::TransformNoParent(TransformConstant& Const, const TransformConstant* All) const {
	//	Const.Result =
	//		PVX::mTran(Position) * Const.PostTranslate *
	//		PVX::Rotate(Const.RotationOrder, Rotation) * Const.PostRotate *
	//		PVX::mScale(Scale) * Const.PostScale;
	//}
	//void Transform::TransformAll_Identity(TransformConstant& Const, const TransformConstant* All) const {
	//	Const.Result = All[Const.ParentIndex].Result *
	//		PVX::mTran(Position) *
	//		PVX::Rotate(Const.RotationOrder, Rotation) *
	//		PVX::mScale(Scale);
	//}
	//void Transform::TransformNoParent_Identity(TransformConstant& Const, const TransformConstant* All) const {
	//	Const.Result =
	//		PVX::mTran(Position) *
	//		PVX::Rotate(Const.RotationOrder, Rotation) *
	//		PVX::mScale(Scale);
	//}
	void Transform::PreTransformed(TransformConstant& Const, const TransformConstant* All) const {
		Const.Result = All[Const.ParentIndex].Result * Matrix;
	}
	void Transform::PreTransformedNoParent(TransformConstant& Const, const TransformConstant* All) const {
		Const.Result = Matrix;
	}


	ObjectGL::ObjectGL(Renderer& r) :renderer{ r } {}

	void ObjectGL::UpdateInstances_1() {
		if (MorphCount) {
			int offset = 0;
			for (auto& p : Parts) {
				if (p.MorphCount) {
					float* mPtr = p.MorphControlBuffer.Map<float>();

					for (auto& inst : Instances) {
						if (inst->Active) {
							memcpy(mPtr, inst->MorphControls.data() + offset, p.MorphCount * sizeof(float));

							mPtr += p.MorphCount;
						}
					}

					p.MorphControlBuffer.Unmap();
					offset += p.MorphCount;
				}
			}
		}

		for (auto& inst: Instances) {
			if (!inst->Active) continue;
			int c = 0;
			for (auto j = 0; j < inst->TransformCount(); j++) {
				inst->Transform(j).GetTransform(TransformConstants[c++]);
			}
		}
	}
	void ObjectGL::UpdateInstances_2() {
		for (auto& inst: Instances) {
			if (!inst->Active) continue;
			int c = 0;
			for (auto j = 0; j < inst->TransformCount(); j++) {
				auto& tr = inst->Transform(j);
				tr.Matrix = tr.GetMatrix(&inst->Transform(0), TransformConstants[c++].ParentIndex);
			}
		}
	}
	void ObjectGL::UpdateInstances_3() {
		DrawCount = 0;

		for(auto& p : Parts) {
			PVX::Matrix4x4 *Ptr = p.TransformBuffer.Map<PVX::Matrix4x4>();
			size_t index = 0;
			if (!p.PostTransform.size()) {
				auto m = p.UseMatrices[0];
				for (auto& o : DrawOrder) {
					auto& inst = Instances[o.Index];
					if (!inst->Active || o.CamAngle<0) continue;
					DrawCount++;
					Ptr[index++] = inst->Transform(m).Matrix;
				}
			} else {
				for (auto& o : DrawOrder) {
					auto& inst = Instances[o.Index];
					if (!inst->Active || o.CamAngle<0) continue;
					DrawCount++;
					int i = 0;
					for (auto& m : p.UseMatrices) {
						Ptr[index++] = inst->Transform(m).Matrix * p.PostTransform[i++];
					}
				}
			}
			p.TransformBuffer.Unmap();
		}
	}

	void ObjectGL::OrderInstances(const PVX::Vector3D& CamPos, const PVX::Vector3D& CamLook) {
		size_t i = 0;
		for (auto& inst : Instances) {
			auto& o = DrawOrder[i];
			o.Index = i++;
			o.CamDist2 = std::numeric_limits<float>::infinity();
			o.CamAngle = -std::numeric_limits<float>::infinity();
			if (!inst->Active) continue;
			auto& pos = inst->Transform(0).Position;
			auto sub = pos.Vec3 - CamPos;
			o.CamDist2 = sub.Dot(CamLook);
			o.CamAngle = sub.Normalized().Dot(CamLook);
		}
		PVX::SortInplace(DrawOrder, [](const DrawOrderData& a, const DrawOrderData& b) {
			return a.CamDist2 < b.CamDist2;
		});
		i = 0;
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

	void Renderer::UpdateInstances(const PVX::Vector3D& CamPos, const PVX::Vector3D& CamLook) {
		for (auto& [n, o] : Objects) {
			o->UpdateInstances_1();
		}
		for (auto& [n, o] : Objects) {
			o->UpdateInstances_2();
		}
		for (auto& [n, o] : Objects) {
			o->OrderInstances(CamPos, CamLook);
		}
		for (auto& [n, o] : Objects) {
			if(o->InstanceCount)
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
					
					pp.Mesh.Draw(int(Object.DrawCount));
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
	inline PVX::OpenGL::Helpers::Transform& InstanceData::Transform(size_t index) {
		return Object.renderer.Transforms[TransformOffset + index];
	}
	inline size_t InstanceData::TransformCount() {
		return Object.InitialTransform.size();
	}
	void InstanceData::Animate(float time) {
		if (Object.AnimationMaxFrame) {
			size_t f = (size_t(time * Object.FrameRate)) % Object.AnimationMaxFrame;
			size_t f2 = (f+1)&Object.AnimationMaxFrame;
			float inter = time * Object.FrameRate - f;
			size_t i = 0;
			for (i = 1; i<TransformCount(); i++) {
				auto& t = Transform(i);
				t.Position = Object.Animation[i][f].Position;
				t.Rotate(Object.Animation[i][f].Rotation);
				//t.Rotation = Object.Animation[i][f].Rotation;
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


	//void ParticleEmitter::Spawn(float count) {
	//	count += Data.RemainingBirthRate;
	//	while (count > 1.0f && Data.LiveCount < Particles.size()) {
	//		float Pitch = Data.MinAngle + rndf.Next() * (Data.MaxAngle - Data.MinAngle);
	//		float Yaw = rndf.Next() * PVX::ToRAD(360.0f);
	//		float cy = cosf(Yaw);
	//		float sy = sinf(Yaw);
	//		float cp = cosf(Pitch);
	//		float sp = sinf(Pitch);
	//		PVX::Vector4D Velocity{ 
	//			(PVX::Vector3D{ sp*sy, cp, cy*sp } *Data.Initial) * 
	//			Data.Speed * (1.0f + rndf.Next() * Data.SpeedVariancePc), 
	//			0 
	//		};
	//		Particle& p = Particles[Data.LiveCount++];
	//		p.Position = Data.Position;
	//		p.Velocity = Velocity;
	//		p.Life = p.MaxLife = Data.LifeSpan * (1.0f + Data.LifeSpanVariancePc * rndf.Next());
	//		count -= 1.0f;
	//	}
	//	Data.RemainingBirthRate = std::modf(count, &count);
	//	//RemainingBirthRate -= int(RemainingBirthRate);
	//}


	void ParticleEmitter::Update(float dt, const PVX::Vector3D& CamPosition) {
		std::lock_guard<std::mutex> lock{ LockUpdate };
		Data.dt = dt;
		Data.CamPosition = CamPosition;
		Data.Random = rndf.Next();
		DataBuffer.Update(sizeof(ParticleData), &Data);
		ComputeParticles.Execute({ DataBuffer, pBuffer }, 1);
		DataBuffer.Read(&Data);
		//pBuffer.Read(Particles);
	}


	//void ParticleEmitter::Update(float dt) {
	//	size_t next = 0;
	//	for (auto i = 0; i<Data.LiveCount; i++) {
	//		auto& pIn = Particles[i];
	//		auto& pOut = Particles[next];
	//		pOut.Life = pIn.Life - dt;
	//		if (pOut.Life <= 0) continue;
	//		pOut.MaxLife = pIn.MaxLife;
	//		pOut.Velocity = pIn.Velocity * (1.0f - (1.0f-Data.Resistance) * dt) + Data.Gravity * dt;
	//		pOut.Position = pIn.Position + pOut.Velocity * dt;
	//		next++;
	//	}
	//	Data.LiveCount = next;
	//	Spawn(dt * Data.BirthRate);
	//}

	void ParticleEmitter::SetDirectionAngle(const PVX::Vector3D& Rot) {
		Data.Initial = PVX::Rotate_XYZ(Rot);
	}
	//void ParticleEmitter::Sort(const PVX::Vector3D& CamPos) {
	//	//for (int i = 0; i<Data.LiveCount; i++) { 
	//	//	Particles[i].CamDist2 = (Particles[i].Position.Vec3 - CamPos).Length2(); 
	//	//}
	//	std::sort(Particles.begin(), Particles.begin() + Data.LiveCount, [](const auto& a, const auto& b) {
	//		return b.CamDist2 < a.CamDist2;
	//	});
	//}

	ParticleRenderer::ParticleRenderer(ResourceManager& mgr, ParticleEmitter& emitter, PVX::OpenGL::Texture3D& Tex):
	Shaders{ 
		mgr.Programs.Get("ParticleEmitter", [] { return PVX::OpenGL::Program{
			{ PVX::OpenGL::Shader::ShaderType::VertexShader, PVX::IO::ReadText("Shaders\\ParticleVertex.glsl") },
			{ PVX::OpenGL::Shader::ShaderType::FragmentShader, PVX::IO::ReadText("Shaders\\ParticleFragment.glsl") }
		}; })
	},
	LiveCount{ emitter.Data.LiveCount },
	geo{
		mgr.Geometry.Get("Particle", [&]() -> Geometry { return {
			PrimitiveType::TRIANGLES,
			{ 0, 1, 2, 0, 2, 3 },
			{
				{
					[&] {
						constexpr float verts[]{
							-0.5f, -0.5f, 0.0f, 0.0f,
							0.5f, -0.5f, 1.0f, 0.0f,
							0.5f,  0.5f, 1.0f, 1.0f,
							-0.5f,  0.5f, 0.0f, 1.0f,
						};
						return VertexBuffer(verts, sizeof(verts));
					}(),
					{
						{ AttribType::FLOAT, 2, 0 }, // 0
						{ AttribType::FLOAT, 2, 0 }, // 1
					}
				},
				{
					emitter.pBuffer,
					{
						{ AttribType::FLOAT, 4, 1, 16 },	// 2
						{ AttribType::FLOAT, 1, 1, 4 },		// 3
						{ AttribType::FLOAT, 2, 1, 8 },		// 4
						{ AttribType::FLOAT, 2, 1 },		// 5
					}
				}
			} };
		})
	},
	Texture{ Tex } {}

	void ParticleRenderer::Render() {
		glDepthMask(false);
		glEnable(GL_BLEND);
		glBindTextureUnit(0, Texture.Get());
		Shaders.Bind();
		geo.Draw(LiveCount);
		glDisable(GL_BLEND);
		glDepthMask(true);
	}


	ParticleEmitter::ParticleEmitter(ResourceManager& mgr, int Max) :
		//Particles{ Max },
		pBuffer{
			Buffer::MakeBuffer(BufferType::SHADER_STORAGE_BUFFER, BufferFlags::DYNAMIC_STORAGE_BIT, size_t(Max) * sizeof(Particle))
		},
		DataBuffer{
			Buffer::MakeBuffer(BufferType::SHADER_STORAGE_BUFFER, BufferFlags::DYNAMIC_STORAGE_BIT, sizeof(ParticleData))
		},
		ComputeParticles{ mgr.Programs.Get("ParticleCompute", [] {
			return Program {
				{ PVX::OpenGL::Shader::ShaderType::ComputeShader, PVX::IO::ReadText("shaders\\ParticleCompute.glsl") }
			};
		}) }
	{
		GL_CHECK(;);
	}
}
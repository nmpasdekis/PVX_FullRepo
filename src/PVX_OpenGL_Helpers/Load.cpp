#include <PVX_OpenGL_Helpers.h>
#include <PVX_File.h>
#include <PVX_Encode.h>
#include <PVX_Image.h>
#include <PVX.inl>

namespace PVX::OpenGL::Helpers {
	int Renderer::LoadObject(const std::string& Filename) {
		return LoadObject(PVX::Object3D::LoadFbx(Filename));
	}

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

	int Renderer::LoadObject(const Object3D::Object& obj) {
		int Id = NextObjectId++;
		ObjectGL& ret = *(Objects[Id] = std::make_unique<ObjectGL>()).get();
		ret.TransformConstants.reserve(obj.Heirarchy.size());
		ret.Animation.reserve(obj.Heirarchy.size());

		for (const auto& t : obj.Heirarchy) {
			ret.TransformConstants.emplace_back(t);
			auto IsIdentity = t.PostTranslate.IsIdentity()&&t.PostRotate.IsIdentity()&&t.PostScale.IsIdentity();
			auto HasParent = t.ParentIndex != -1;
			ret.InitialTransform.emplace_back(t.Position, t.Rotation, t.Scale, HasParent, IsIdentity);

			ret.FrameRate = t.Animation.FrameRate;
			ret.AnimationMaxFrame = t.Animation.Position.size();

			ret.Animation.push_back(PVX::Map(t.Animation.Position.size(), [&](size_t i) {
				return AnimationFrame{ t.Animation.Position[i], t.Animation.Rotation[i], t.Animation.Scale[i] };
			}));
		}

		std::map<std::string, int> MatLookup;
		for (auto& [Name, m]: obj.Material) {
			MatLookup[Name] = int(MatLookup.size());
			auto& mat = ret.Materials.emplace_back(DataPBR{
				{ m.Color, m.Alpha },
				m.Metallic,
				m.Roughness,
				m.Bump,
				m.EmissiveFactor * (m.Emissive.x + m.Emissive.y + m.Emissive.z) * (1.0f/3.0f)
			});
			mat.Data.Name(Name.c_str());
			if (m.Textures.Diffuse.size())
				mat.Color_Tex = LoadTexture2D(m.Textures.Diffuse).Get();
			if (m.Textures.Bump.size())
				mat.Normal_Tex = LoadTexture2D(m.Textures.Bump).Get();
			if (m.Textures.PBR.size())
				mat.PBR_Tex = LoadTexture2D(m.Textures.PBR).Get();
		}
		ret.Parts.reserve(obj.Parts.size());
		for (auto& p : obj.Parts) {
			auto& pp = ret.Parts.emplace_back();

			ret.MorphCount += (pp.MorphCount = int(p.BlendShapes.size()));

			if (!p.BoneNodes.size()) {
				pp.UseMatrices.push_back((int)IndexOf(obj.Heirarchy, [&](const Object3D::Transform& t) { return t.Name == p.TransformNode; }));
			} else {
				for (auto i = 0; i<p.BoneNodes.size(); i++) {
					pp.PostTransform.reserve(p.BoneNodes.size());
					pp.UseMatrices.reserve(p.BoneNodes.size());

					pp.UseMatrices.push_back((int)IndexOf(obj.Heirarchy, [&](const Object3D::Transform& t) { return p.BoneNodes[i] == t.Name; }));
					pp.PostTransform.push_back(p.BonePostTransform[i]);
				}
			}
			for (auto& [Mat, SubPart]: p.Parts) {
				auto& sp = pp.SubPart.emplace_back();
				sp.MaterialIndex = MatLookup[Mat];

				if (!SubPart.CustomShader) {
					auto& Mater = ret.Materials[sp.MaterialIndex];
					unsigned int MatFlags = (Mater.Color_Tex ? 1 : 0) | (Mater.PBR_Tex ? 2 : 0) | (Mater.Normal_Tex ? 4 : 0);

					unsigned int GeoFlags = PVX::Reduce(SubPart.Attributes, 0, [](unsigned int acc, const PVX::Object3D::VertexAttribute& attr) {
						return acc | unsigned int(attr.Usage);
					});

					std::vector<std::string> Filters;
					Filters.reserve(SubPart.Attributes.size());
					Filters.push_back("Position");
					if (GeoFlags & unsigned int(PVX::Object3D::ItemUsage::ItemUsage_Normal))
						Filters.push_back("Normal");
					if (GeoFlags & unsigned int(PVX::Object3D::ItemUsage::ItemUsage_UV) && MatFlags) {
						Filters.push_back("UV");
						Filters.push_back("TexCoord");
						if (GeoFlags & unsigned int(PVX::Object3D::ItemUsage::ItemUsage_Tangent) && (MatFlags&4))
							Filters.push_back("Tangent");
					}
					if (GeoFlags & unsigned int(PVX::Object3D::ItemUsage::ItemUsage_Weight)) {
						Filters.push_back("Weights");
						Filters.push_back("WeightIndices");
					}
					auto fsp = SubPart.FilterAttributes(Filters);



					sp.Mesh = ToGeomerty2(fsp);

					GeoFlags = sp.Mesh.GetFlags();


					if (fsp.BlendAttributes.size()) {
						GeoFlags = PVX::Reduce(SubPart.BlendAttributes, GeoFlags, [](unsigned int acc, const PVX::Object3D::VertexAttribute& attr) {
							return acc | unsigned int(attr.Usage);
						});
						sp.Morph = PVX::OpenGL::Buffer::MakeImmutableShaderStorage((int)fsp.BlendShapeData.size(), fsp.BlendShapeData.data());
					}
					sp.SetProgram(GetDefaultProgram(GeoFlags, MatFlags));
				} else {
					sp.Mesh = ToGeomerty2(SubPart);
					if (SubPart.BlendAttributes.size()) {
						sp.Morph = PVX::OpenGL::Buffer(SubPart.BlendShapeData.data(), int(SubPart.BlendShapeData.size()), false);
					}
					sp.SetProgram(GetDefaultProgram(1, 0));
				}
			}
		}
		return Id;
	}
}
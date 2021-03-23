#pragma once

#include <PVX_Math3D.h>

#include <string>
#include <vector>
#include <map>
#include <PVX_Reflect.h>
#include <PVX_Animation.h>
#include <PVX_BinSaver.h>


namespace PVX::Object3D {
	enum class ItemUsage : unsigned int {
		ItemUsage_Position = 0,
		ItemUsage_Normal = 1,
		ItemUsage_UV = 2,
		ItemUsage_Tangent = 4,
		ItemUsage_Weight = 8,
		ItemUsage_WeightIndex = 16,

		ItemUsage_MorphPosition = 32,
		ItemUsage_MorphNormal = 64,
		ItemUsage_MorphUV = 128,
		ItemUsage_MorphTangent = 256,
		
		ItemUsage_General = 0xffffffff
	};

	struct PlaneMaterial {
		PVX::Vector3D Color;
		PVX::Vector3D Ambient;
		PVX::Vector3D Diffuse;
		PVX::Vector3D Specular;
		PVX::Vector3D Emissive;

		float Metallic;
		float Roughness;

		float AmbientFactor;
		float DiffuseFactor;
		float SpecularFactor;
		float EmissiveFactor;

		float SpecularPower;
		float Alpha;
		float Bump;
		struct {
			std::string Ambient;
			std::string Diffuse;
			std::string Specular;
			std::string Emissive;
			std::string Bump;
			std::string Transparency;
			std::string PBR;
		} Textures;
	protected:
		void Save(BinSaver& bin, const std::string& Name);
		friend class Object;
	};

	struct VertexAttribute {
		enum class _Type : int {
			Byte,
			Int,
			Float,
			Double
		};
		_Type Type;
		int Count;
		int Offset;
		ItemUsage Usage;
		std::string Name;
	};

	class ObjectSubPart {
		void Save(BinSaver& bin);
		friend class ObjectPart;
	public:
		std::vector<unsigned char> VertexData;
		std::vector<int> Index;
		std::vector<VertexAttribute> Attributes;
		std::vector<VertexAttribute> BlendAttributes;
		std::vector<unsigned char> BlendShapeData;
		int Stride;
		int VertexCount;
		int CustomShader = 0;
		ObjectSubPart FilterAttributes(const std::vector<std::string>& Attrs) const;
	};

	class ObjectPart {
		void Save(BinSaver& bin);
		friend class Object;
	public:
		std::map<std::string, ObjectSubPart> Parts;
		std::string TransformNode;
		std::vector<Matrix4x4> BonePostTransform;
		std::vector<std::string> BoneNodes;
		std::vector<std::string> BlendShapes;
	};

	struct Transform {
		long long						ParentIndex;
		Matrix4x4						PostTranslate;
		Matrix4x4						PostRotate;
		Matrix4x4						PostScale;

		Vector3D						Position;
		Vector3D						Rotation;
		Vector3D						Scale;

		std::string						Name;
		int								RotationOrder;

		struct {
			float						FrameRate = 0;
			std::vector<PVX::Vector3D>	Position;
			std::vector<PVX::Vector3D>	Rotation;
			std::vector<PVX::Vector3D>	Scale;
		} Animation;
	protected:
		void Save(BinSaver& bin);
		friend class Object;
	};

	class Object {
		void Save(BinSaver& bin);
	public:
		std::map<std::string, PlaneMaterial> Material;
		std::vector<ObjectPart> Parts;
		std::vector<Transform> Heirarchy;
		void Save(const std::string& Filename);
		static Object Load(const std::string& Filename);
	};

	Object LoadFbx(const std::string& Filename);
	size_t Reindex(std::vector<unsigned char>& VertexData, std::vector<int>& Index, int Stride);
}
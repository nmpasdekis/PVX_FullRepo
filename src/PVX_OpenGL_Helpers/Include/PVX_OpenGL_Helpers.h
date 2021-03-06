#pragma once

#include <PVX_OpenGL_Object.h>
#include <PVX_Object3D.h>
#include <PVX_json.h>


namespace PVX::OpenGL::Helpers {

	Attribute FromObjectAttribute(const PVX::Object3D::VertexAttribute& attr);
	Geometry ToGeomerty(const PVX::OpenGL::InterleavedArrayObject& obj);
	Geometry ToGeomerty(const PVX::OpenGL::BufferObject& obj);
	Geometry ToGeomerty(const PVX::Object3D::ObjectSubPart& so, bool OldVersion = false);


	struct TransformConstant {
		PVX::Matrix4x4	PostTranslate;
		PVX::Matrix4x4	PostRotate;
		PVX::Matrix4x4	PostScale;

		long long		ParentIndex;
		long long		RotationOrder;

		inline TransformConstant(const PVX::Object3D::Transform& t) :
			PostTranslate{ t.PostTranslate },
			PostRotate{ t.PostRotate },
			PostScale{ t.PostScale },
			ParentIndex{ t.ParentIndex },
			RotationOrder{ t.RotationOrder }
		{}
	};

	struct Transform {
		PVX::Vector3D Position;
		PVX::Vector3D Rotation;
		PVX::Vector3D Scale;
	};

	struct ObjectGL_SubPart {
		PVX::OpenGL::Geometry Mesh;
		PVX::OpenGL::Program ShaderProgram;
		int MaterialIndex;
	};

	struct ObjectGL_Part {
		std::vector<ObjectGL_SubPart> SubPart;
		std::vector<int> UseMatrices;
		std::vector<PVX::Matrix4x4> TransformBufferData;
		std::vector<PVX::Matrix4x4> PostTransform;
		PVX::OpenGL::Buffer TransformBuffer{ nullptr, sizeof(PVX::Matrix4x4), false };
	};

	struct DataPBR {
		PVX::Vector4D Color;
		float Metallic;
		float Roughness;
		float Bump;
		float Emissive;
	};

	struct Material {
		DataPBR PBR;
		PVX::OpenGL::Buffer Data{ &PBR, sizeof(PBR), true };
		Material(const DataPBR& dt) : PBR{ dt }, Data{ &PBR, sizeof(PBR), true }{}
	};

	struct ObjectGL {
		std::vector<TransformConstant> TransformConstants;
		std::vector<Transform> InitialTransform;
		std::vector<std::vector<Transform>> Transform;
		std::vector<PVX::Matrix4x4> HelperMatrices;

		std::vector<ObjectGL_Part> Parts;
		std::vector<std::vector<unsigned char>> PerInstanceMaterialData;

		std::vector<Material> Materials;
		std::vector<PVX::OpenGL::Buffer> InstanceData;

		ObjectGL(const PVX::Object3D::Object& obj);
	};

	struct aLight {
		PVX::Vector4D Position;
		PVX::Vector4D Color;
	};

	struct LightRig {
		int Count, padd1, padd2, padd3;
		float Attenuation3 = 0;
		float Attenuation2 = 0.001;
		float Attenuation1 = 0;
		float Attenuation0 = 1.0f;
		aLight light[128];
	};

	class Renderer {
		PVX::OpenGL::FrameBufferObject FullFrameBuffer;
		PVX::OpenGL::Context& gl;
		PVX::OpenGL::Geometry FrameGeometry;
		PVX::OpenGL::Pipeline PostProcessPipeline;
		PVX::OpenGL::Sampler GeneralSampler;

		PVX::OpenGL::Texture2D Position;
		PVX::OpenGL::Texture2D Color;
		PVX::OpenGL::Texture2D Normal;
		PVX::OpenGL::Texture2D PBR;
		PVX::OpenGL::Texture2D Depth;

		PVX::OpenGL::Buffer LightBuffer;
		std::map<std::wstring, PVX::OpenGL::Texture2D> Textures;
	public:
		LightRig Lights;
		Renderer(int Width, int Height, PVX::OpenGL::Context& gl);
		void SetCameraBuffer(PVX::OpenGL::Buffer& CamBuffer);
		void Render(std::function<void()> RenderClb);

		PVX::OpenGL::Texture2D LoadTexture2D(const std::wstring& Filename);
		int LoadObject(const std::wstring& Filename);
	};
}
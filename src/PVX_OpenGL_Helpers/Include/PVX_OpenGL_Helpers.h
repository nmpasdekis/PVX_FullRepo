#pragma once

#include <PVX_OpenGL_Object.h>
#include <PVX_Object3D.h>
#include <PVX_json.h>
#include <memory>


namespace PVX::OpenGL::Helpers {

	Attribute FromObjectAttribute(const PVX::Object3D::VertexAttribute& attr);
	Geometry ToGeomerty(const PVX::OpenGL::InterleavedArrayObject& obj);
	Geometry ToGeomerty(const PVX::OpenGL::BufferObject& obj);
	Geometry ToGeomerty(const PVX::Object3D::ObjectSubPart& so, bool OldVersion = false);


	struct TransformConstant {
		PVX::Matrix4x4	PostTranslate;
		PVX::Matrix4x4	PostRotate;
		PVX::Matrix4x4	PostScale;
		PVX::Matrix4x4	Result;

		long long			ParentIndex;
		PVX::RotationOrder	RotationOrder;

		inline TransformConstant(const PVX::Object3D::Transform& t) :
			PostTranslate{ t.PostTranslate },
			PostRotate{ t.PostRotate },
			PostScale{ t.PostScale },
			ParentIndex{ t.ParentIndex },
			RotationOrder{ t.RotationOrder },
			Result{ 0 }
		{}
	};

	struct Transform {
		PVX::Vector3D Position;
		PVX::Vector3D Rotation;
		PVX::Vector3D Scale;
	};


	class ProgramPlus {
		PVX::OpenGL::Program Prog;
		int MorphCountIndex = -1;
		int BoneCountIndex = -1;
	public:
		ProgramPlus(const PVX::OpenGL::Program& p, const PVX::OpenGL::Sampler& DefSampler);
		ProgramPlus(){}
		inline void Use() { Prog.Bind(); }
		inline void SetCamera(const PVX::OpenGL::Buffer& Cam) { Prog.BindBuffer(0, Cam); }
		inline void SetMaterial(const PVX::OpenGL::Buffer& Mat) { Prog.BindBuffer(1, Mat); }
		inline void SetTransform(const PVX::OpenGL::Buffer& Tran) { Prog.BindBuffer(2, Tran); }
		void SetBoneCount(int n);
		void SetMorphCount(int n);
	};

	struct ObjectGL_SubPart {
		PVX::OpenGL::Geometry Mesh;
		ProgramPlus ShaderProgram;
		//PVX::OpenGL::Pipeline Pip;
		PVX::OpenGL::Buffer Morph;
		int MaterialIndex;
	};

	struct ObjectGL_Part {
		std::vector<ObjectGL_SubPart> SubPart;
		std::vector<int> UseMatrices;
		//std::vector<float> MorphControls;
		//std::vector<PVX::Matrix4x4> TransformBufferData;
		std::vector<PVX::Matrix4x4> PostTransform;
		PVX::Matrix4x4* TransformBufferPtr = nullptr;

		int MorphCount = 0;
		float* MorphPtr = nullptr;


		PVX::OpenGL::Buffer TransformBuffer{ false, PVX::OpenGL::BufferUsege::STREAM_DRAW };
		PVX::OpenGL::Buffer MorphControlBuffer{ false, PVX::OpenGL::BufferUsege::STREAM_DRAW };
		//ObjectGL_Part() { TransformBuffer.Name("TransformBuffer"); }
	};

	struct DataPBR {
		PVX::Vector4D Color;
		float Metallic;
		float Roughness;
		float Bump;
		float Emissive;
	};

	struct Material {
		Material(const DataPBR& dt) : PBR{ dt }, Data{ &PBR, sizeof(PBR), true }{}
		DataPBR PBR;
		PVX::OpenGL::Buffer Data{ &PBR, sizeof(PBR), true };
		int Color_Tex = 0;
		int PBR_Tex = 0;
		int Normal_Tex = 0;
	};

	struct ObjectGL;

	class InstanceData {
		ObjectGL& Object;
		std::vector<PVX::OpenGL::Helpers::Transform> Transforms;
		std::vector<float> MorphControls;
		friend struct ObjectGL;
		friend class Renderer;
		bool Active = false;
	public:
		inline InstanceData(ObjectGL& obj, const std::vector<PVX::OpenGL::Helpers::Transform>& tran, bool Active) :Object{ obj }, Transforms{ tran } { SetActive(Active); }

		inline PVX::OpenGL::Helpers::Transform& Transform(size_t index) { return Transforms[index]; }
		inline size_t TransformCount() { return Transforms.size(); }

		inline float& Morph(size_t index) { return MorphControls[index]; }
		inline size_t MorphCount() { return MorphControls.size(); }

		void SetActive(bool isActive);
	};

	struct ObjectGL {
		std::vector<TransformConstant> TransformConstants;
		std::vector<Transform> InitialTransform;
		std::vector<std::unique_ptr<InstanceData>> Instances;
		std::vector<PVX::Matrix4x4> HelperMatrices;

		std::vector<ObjectGL_Part> Parts;
		std::vector<std::vector<unsigned char>> PerInstanceMaterialData;

		std::vector<Material> Materials;
		std::vector<PVX::OpenGL::Buffer> InstanceData;
		size_t InstanceCount = 0;
		size_t DrawCount = 0;
		size_t ActiveInstances = 0;
		int MorphCount = 0;

		void UpdateInstances();
	};

	struct aLight {
		PVX::Vector4D Position;
		PVX::Vector4D Color;
	};

	struct LightRig {
		int Count, padd1, padd2, padd3;
		float Attenuation3 = 0;
		float Attenuation2 = 0.001f;
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
		PVX::OpenGL::Sampler PostSampler;

		PVX::OpenGL::Texture2D Position;
		PVX::OpenGL::Texture2D Color;
		PVX::OpenGL::Texture2D Normal;
		PVX::OpenGL::Texture2D PBR;
		PVX::OpenGL::Texture2D Depth;

		PVX::OpenGL::Buffer CameraBuffer;
		PVX::OpenGL::Buffer LightBuffer;
		std::map<std::wstring, PVX::OpenGL::Texture2D> Textures;

		int NextObjectId = 1;
		int NextInstanceId = 1;
		std::map<int, std::unique_ptr<ObjectGL>> Objects;
		std::map<int, std::tuple<int, int>> Instances;
		//int LoadObject(const Object3D::Object& obj);
		PVX::OpenGL::Program GetDefaultProgram(unsigned int VertexFormat, unsigned int Fragment);
		PVX::OpenGL::Shader GetDefaultVertexShader(unsigned int VertexFormat);
		PVX::OpenGL::Shader GetDefaultFragmentShader(unsigned int VertexFormat, unsigned int Fragment);

		std::map<unsigned int, PVX::OpenGL::Shader> DefaultVertexShaders;
		std::map<unsigned int, PVX::OpenGL::Shader> DefaultFragmentShaders;
		std::map<unsigned int, PVX::OpenGL::Program> DefaultShaderPrograms;
	public:
		LightRig Lights;
		Renderer(int Width, int Height, PVX::OpenGL::Context& gl);
		void SetCameraBuffer(PVX::OpenGL::Buffer& CamBuffer);
		void Render(std::function<void()> RenderClb);

		PVX::OpenGL::Texture2D LoadTexture2D(const std::wstring& Filename);
		PVX::OpenGL::Texture2D LoadTexture2D(const std::string& Filename);
		int LoadObject(const std::string& Filename);
		int LoadObject(const Object3D::Object& obj);

		InstanceData& GetInstance(int InstanceId);

		int CreateInstance(int Id, bool Active = true);

		void DrawInstances();
	};
}
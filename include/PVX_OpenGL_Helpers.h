#pragma once

#include <PVX_OpenGL_Object.h>
#include <PVX_Object3D.h>
#include <PVX_json.h>
//#include <PVX_Threading.h>
#include <memory>


namespace PVX::OpenGL::Helpers {

	Attribute FromObjectAttribute(const PVX::Object3D::VertexAttribute& attr);
	Geometry ToGeomerty(const PVX::OpenGL::InterleavedArrayObject& obj, bool OldVersion = true);
	Geometry ToGeomerty(const PVX::OpenGL::BufferObject& obj, bool OldVersion = true);
	Geometry ToGeomerty(const PVX::Object3D::ObjectSubPart& so, bool OldVersion = false);

	class Reflector140 {
		std::vector<uint8_t> Data;
		std::unordered_map<std::string, std::tuple<int, int>> Variables;
	public:
		template<typename T>
		T& Add(const std::string& Name) {
			constexpr int vSize = sizeof(T) <= 8 ? sizeof(T) : (16 * ((sizeof(T) + 15) / 16));
			constexpr int align = sizeof(T) <= 8 ? sizeof(T) : 16;
			int offset = align * ((int(Data.size()) + align - 1) / align);
			Data.resize(offset + vSize);
			Variables[Name] = { offset, sizeof(T) };
			return *(T*)&Data[offset];
		}
		template<typename T>
		T& Get(const std::string& Name) { 
			return *(T*)&Data[std::get<0>(Variables[Name])]; 
		}
		template<typename T>
		int SizeOf(const std::string& Name) { 
			return *(T*)&Data[std::get<1>(Variables[Name])]; 
		}
	};

	template<typename T>
	class SingleResourceManager {
		std::unordered_map<std::string, T> Resources;
	public:
		T Get(const std::string& Name, std::function<T()> LoadFunc) {
			if (auto f = Resources.find(Name); f == Resources.end())
				return Resources[Name] = LoadFunc();
			return Resources.at(Name);
		}
	};

	class ResourceManager {
	public:
		SingleResourceManager<PVX::OpenGL::Geometry> Geometry;
		SingleResourceManager<PVX::OpenGL::Shader> VertexShaders;
		SingleResourceManager<PVX::OpenGL::Shader> FragmentShaders;
		SingleResourceManager<PVX::OpenGL::Program> Programs;
		SingleResourceManager<PVX::OpenGL::Texture2D> Textures2D;
		SingleResourceManager<PVX::OpenGL::Sampler> Samplers;
	};

	class TextPrinter {
		struct CharInstance {
			PVX::Vector2D Position;
			PVX::Vector2D UVOffset;
		};
		struct DrawData {
			PVX::Vector4D Color;
			PVX::Matrix4x4 Transform;
		};
		struct DrawElementsIndirectCommand {
			uint32_t  count;
			uint32_t  instanceCount;
			uint32_t  firstIndex;
			uint32_t  baseVertex;
			uint32_t  baseInstance;
		};
		ResourceManager& rManager;
		PVX::OpenGL::Texture2D Atlas;
		PVX::Vector2D TileSize;
		PVX::OpenGL::StreamingVertexShader Characters;
		PVX::OpenGL::Buffer Texts;
		PVX::OpenGL::Geometry geo;
		PVX::OpenGL::Program Shaders;
		int xTiles, yTiles;

		PVX::iVector2D ScreenSize;
		std::vector<DrawData> TextBufferData;
		std::vector<PVX::Vector2D> Stream;
		std::vector<DrawElementsIndirectCommand> cmds;
	public:
		TextPrinter(ResourceManager& mgr, const std::string& Texture,
			int xTiles, int yTiles, 
			const PVX::iVector2D& ScreenSize);
		void AddText(const std::string_view& Text, 
			const PVX::Vector2D& pos, 
			float scale = 1.0f, 
			const PVX::Vector4D& Color = { 1.0f, 1.0f, 1.0f, 1.0f });
		inline void SetScreenSize(int Width, int Height) { ScreenSize ={ Width, Height }; }
		void Render();
	};

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
		{
		}
	};

	struct Transform {
		PVX::Matrix4x4 Matrix;
		PVX::Vector3D Position;
		PVX::Vector3D Rotation;
		PVX::Vector3D Scale;
		Transform(const Transform&) = default;

		void Transform_All(const TransformConstant& Const);
		void Transform_Identity(const TransformConstant& Const);
		void DoNothing(const TransformConstant& Const);

		void TransformAll(TransformConstant& Const, const TransformConstant* All) const;
		void TransformNoParent(TransformConstant& Const, const TransformConstant* All) const;
		void TransformAll_Identity(TransformConstant& Const, const TransformConstant* All) const;
		void TransformNoParent_Identity(TransformConstant& Const, const TransformConstant* All) const;
		void PreTransformed(TransformConstant& Const, const TransformConstant* All) const;
		void PreTransformedNoParent(TransformConstant& Const, const TransformConstant* All) const;



		PVX::Matrix4x4 GetMatrix_WithParent(const Transform* tr, int Parent) const;
		PVX::Matrix4x4 GetMatrix_WithoutParent(const Transform* tr, int Parent) const;

		void(Transform::* curTransform)(TransformConstant& Const, const TransformConstant* All) const;

		void(Transform::* cTransform)(const TransformConstant& Const);
		PVX::Matrix4x4 (Transform::* cGetTransform)(const Transform* tr, int Parent) const;

		inline void GetTransform(TransformConstant& Const, const TransformConstant* All) const {
			(this->*curTransform)(Const, All);
		}

		inline void GetTransform(TransformConstant& Const) {
			(this->*cTransform)(Const);
		}

		inline PVX::Matrix4x4 GetMatrix(const Transform* tr, int Parent) {
			return (this->*cGetTransform)(tr, Parent);
		}

		inline Transform(const PVX::Vector3D& Pos, const PVX::Vector3D& Rot, const PVX::Vector3D& Scl, bool Parent, bool Identity) :
			Position{ Pos }, Rotation{ Rot }, Scale{ Scl },
			Matrix{ PVX::Matrix4x4::Identity() }
		{
			if (Identity) cTransform = &Transform::Transform_Identity;
			else cTransform = &Transform::Transform_All;

			if (Parent) cGetTransform = &Transform::GetMatrix_WithParent;
			else cGetTransform = &Transform::GetMatrix_WithoutParent;

			if (Identity) {
				if (Parent) {
					curTransform = &Transform::TransformAll_Identity;
				} else {
					curTransform = &Transform::TransformNoParent_Identity;
				}
			} else {
				if (Parent) {
					curTransform = &Transform::TransformAll;
				} else {
					curTransform = &Transform::TransformNoParent;
				}
			}
		}
	};

	struct AnimationFrame {
		PVX::Vector3D Position;
		PVX::Vector3D Rotation;
		PVX::Vector3D Scale;
	};

	struct ObjectGL_SubPart {
		PVX::OpenGL::Geometry Mesh;
		PVX::OpenGL::Program ShaderProgram;
		//PVX::OpenGL::Pipeline Pip;
		PVX::OpenGL::Buffer Morph;
		int MaterialIndex = -1;
		int MorphCountIndex = -1;
		int BoneCountIndex = -1;

		void SetProgram(const PVX::OpenGL::Program& prog);
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
		std::vector<Transform> Transforms;
		std::vector<float> MorphControls;
		friend struct ObjectGL;
		friend class Renderer;
		bool Active = false;
	public:
		inline InstanceData(ObjectGL& obj, const std::vector<Transform>& tran, bool Active) :
			Object{ obj }, Transforms{ tran } { SetActive(Active); }

		inline PVX::OpenGL::Helpers::Transform& Transform(size_t index) { return Transforms[index]; }
		inline size_t TransformCount() { return Transforms.size(); }

		inline float& Morph(size_t index) { return MorphControls[index]; }
		inline size_t MorphCount() { return MorphControls.size(); }

		void Animate(float time);

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
		size_t ActiveInstances = 0;
		int MorphCount = 0;
		float FrameRate;
		size_t AnimationMaxFrame;

		std::vector<std::vector<AnimationFrame>> Animation;

		void UpdateInstances();
		void UpdateInstances_1();
		void UpdateInstances_2();
		void UpdateInstances_3();
	};

	struct aLight {
		PVX::Vector4D Position;
		PVX::Vector4D Color;
	};

	template<int LightCount = 16>
	struct LightRig {
		float Attenuation3 = 0.01f;
		float Attenuation2 = 0.01f;
		float Attenuation1 = 0.1f;
		int Count = 0;

		aLight light[LightCount];
	};

	struct GBuffer {		
		PVX::OpenGL::Texture2D Position;
		PVX::OpenGL::Texture2D Albedo;
		PVX::OpenGL::Texture2D Normal;
		PVX::OpenGL::Texture2D Material;
		PVX::OpenGL::Texture2D Depth;
	};

	class PostProcessStep {
		PVX::OpenGL::Shader FragmentShader;
		PVX::OpenGL::FrameBufferObject* FBO{ nullptr };
		std::vector<std::tuple<int, PVX::OpenGL::Texture2D>> TextureInputs;
		std::vector<PVX::OpenGL::Texture2D> Targets{};
		float ScaleX;
		float ScaleY;

		PostProcessStep(
			const std::string& FragmentCode,
			const std::initializer_list<std::tuple<int, PVX::OpenGL::Texture2D>>& Inputs,
			const std::initializer_list<PVX::OpenGL::Texture2D>& Targets,
			float ScaleX = 1.0f, float ScaleY = 0
		) : 
			FragmentShader{ PVX::OpenGL::Shader::ShaderType::FragmentShader, FragmentCode },
			TextureInputs{ Inputs },
			Targets{ Targets },
			ScaleX{ ScaleX },
			ScaleY{ ScaleY ? ScaleY : ScaleX }
		{};
		PostProcessStep(
			const std::string& FragmentCode,
			PVX::OpenGL::FrameBufferObject& FBO,
			const std::initializer_list<std::tuple<int, PVX::OpenGL::Texture2D>>& Inputs,
			float ScaleX = 1.0f, float ScaleY = 0
		) :
			FragmentShader{ PVX::OpenGL::Shader::ShaderType::FragmentShader, FragmentCode },
			TextureInputs{ Inputs },
			FBO{ &FBO },
			ScaleX{ ScaleX },
			ScaleY{ ScaleY ? ScaleY : ScaleX }
		{};
		PostProcessStep(
			const PVX::OpenGL::Shader & FragmentShader,
			const std::initializer_list<std::tuple<int, PVX::OpenGL::Texture2D>>& Inputs,
			const std::initializer_list<PVX::OpenGL::Texture2D>& Targets,
			float ScaleX = 1.0f, float ScaleY = 0
		) :
			FragmentShader{ FragmentShader },
			TextureInputs{ Inputs },
			Targets{ Targets },
			ScaleX{ ScaleX },
			ScaleY{ ScaleY ? ScaleY : ScaleX }
		{};
		PostProcessStep(
			const PVX::OpenGL::Shader& FragmentShader,
			PVX::OpenGL::FrameBufferObject& FBO,
			const std::initializer_list<std::tuple<int, PVX::OpenGL::Texture2D>>& Inputs,
			float ScaleX = 1.0f, float ScaleY = 0
		) :
			FragmentShader{ FragmentShader },
			TextureInputs{ Inputs },
			FBO{ &FBO },
			ScaleX{ ScaleX },
			ScaleY{ ScaleY ? ScaleY : ScaleX }
		{};
	};

	class PostProcessor {
		struct Process_t {
			PVX::OpenGL::Program Shaders;
			PVX::OpenGL::FrameBufferObject* FBO;
			std::vector<std::tuple<int, PVX::OpenGL::Texture2D>> TextureInputs;
		};
		ResourceManager& rManager;
		std::map<int, std::pair<PVX::OpenGL::Texture2D, PVX::Vector2D>> Scales;
		PVX::OpenGL::Context& gl;
		PVX::OpenGL::Shader VertexShader;
		PVX::OpenGL::Geometry FrameGeometry;
		PVX::OpenGL::Sampler TexSampler;
		std::vector<Process_t> Processes;
		std::vector<GLuint> BindSamplers;
		std::vector<std::unique_ptr<PVX::OpenGL::FrameBufferObject>> FBOs;
	public:
		PostProcessor(ResourceManager& mgr, PVX::OpenGL::Context& gl);
		void AddProcess(
			const PVX::OpenGL::Shader& FragmentShader, 
			const std::initializer_list<std::tuple<int, PVX::OpenGL::Texture2D>>& Inputs,
			const std::initializer_list<PVX::OpenGL::Texture2D>& Targets,
			float ScaleX = 1.0f, float ScaleY = 1.0f);

		void AddProcess(
			const PVX::OpenGL::Shader& FragmentShader,
			const std::initializer_list<std::tuple<int, PVX::OpenGL::Texture2D>>& Inputs,
			PVX::OpenGL::FrameBufferObject& FBO);

		void Resize(const PVX::iVector2D& Size);
		inline void Resize(int Width, int Height) { Resize({ Width, Height }); };
		void Process();
		void MakeBloom(
			int Width, int Height,
			const PVX::OpenGL::Texture2D& gPosition,
			const PVX::OpenGL::Texture2D& gAlbedo,
			const PVX::OpenGL::Texture2D& gNormal,
			const PVX::OpenGL::Texture2D& gMaterial, 
			PVX::OpenGL::FrameBufferObject* Target = nullptr
		);
		void MakeSimple(
			int Width, int Height,
			const PVX::OpenGL::Texture2D& gPosition,
			const PVX::OpenGL::Texture2D& gAlbedo,
			const PVX::OpenGL::Texture2D& gNormal,
			const PVX::OpenGL::Texture2D& gMaterial, 
			PVX::OpenGL::FrameBufferObject* Target = nullptr
		);
		void MakeBloom(
			int Width, int Height,
			GBuffer& gBuffer,
			PVX::OpenGL::FrameBufferObject* Target = nullptr
		);
		void MakeSimple(
			int Width, int Height,
			GBuffer& gBuffer,
			PVX::OpenGL::FrameBufferObject* Target = nullptr
		);
	};

	struct Particle {
		PVX::Vector4D Position;
		PVX::Vector4D Velocity;
		float Life;
		float pad1, pad2, pad3;
	};

	class ParticleEmitter {
	public:
		PVX::Vector4D Position;
		PVX::Vector4D InitialVelocity;
		std::vector<Particle> Particles;
		float AngleVariance;
		float VelocityVariance;
		float LifeSpan;
		float LifeSpanVariance;
		float BirthRate;
		PVX::Vector4D Gravity;
		PVX::Vector4D Resistance;
	};

	class Renderer {
		PVX::OpenGL::Context& gl;
		ResourceManager& rManager;

		PVX::OpenGL::FrameBufferObject gBufferFBO;

		GBuffer gBuffer;

		PVX::OpenGL::Sampler GeneralSampler;

		PVX::OpenGL::Buffer CameraBuffer;
		PVX::OpenGL::Buffer LightBuffer;
		std::map<std::wstring, PVX::OpenGL::Texture2D> Textures;
		std::map<int, std::unique_ptr<ObjectGL>> Objects;
		std::map<int, std::tuple<int, int>> Instances;

		PVX::OpenGL::Program GetDefaultProgram(unsigned int VertexFormat, unsigned int Fragment);
		std::string GetDefaultVertexShader(unsigned int VertexFormat);
		std::string GetDefaultFragmentShader(unsigned int VertexFormat, unsigned int Fragment);

		std::map<unsigned int, PVX::OpenGL::Shader> DefaultVertexShaders;
		std::map<unsigned int, PVX::OpenGL::Shader> DefaultFragmentShaders;
		std::map<unsigned int, PVX::OpenGL::Program> DefaultShaderPrograms;
		//PVX::Threading::TaskPump Threads;
		
		//std::vector<Transform> Transforms;

		int NextObjectId = 1;
		int NextInstanceId = 1;
	public:
		PostProcessor PostProcesses;
		LightRig<16> Lights;
		Renderer(ResourceManager& mgr, int Width, int Height, PVX::OpenGL::Context& gl, PVX::OpenGL::FrameBufferObject* Target = nullptr);
		void SetCameraBuffer(PVX::OpenGL::Buffer& CamBuffer);
		void Render(std::function<void()> RenderClb);

		PVX::OpenGL::Texture2D LoadTexture2D(const std::wstring& Filename);
		PVX::OpenGL::Texture2D LoadTexture2D(const std::string& Filename);
		PVX::OpenGL::Texture2D LoadNormalTexture2D(const std::string& Filename, bool flipY = false);
		PVX::OpenGL::Texture2D LoadNormalTexture2D(const std::wstring& Filename, bool flipY = false);
		int LoadObject(const std::string& Filename);
		int LoadObject(const Object3D::Object& obj);

		InstanceData& GetInstance(int InstanceId);

		int CreateInstance(int Id, bool Active = true);

		void DrawInstances();
		void UpdateInstances();

		inline std::vector<int> GetInstenceIds() const { 
			std::vector<int> ret;
			ret.reserve(Instances.size());
			for (const auto& [k, v]:Instances)ret.push_back(k);
			return ret;
		}
		inline int InstanceOf(int instId) const { return std::get<0>(Instances.at(instId)); }
	};
}
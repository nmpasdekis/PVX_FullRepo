#pragma once
//#include <Windows.h>

#include <gl/glcorearb.h>
#include <gl/GL.h>
#include <gl/GLU.h>
#include <gl/glext.h>
#include <gl/wglext.h>
#include <thread>
#include <functional>
#include <PVX_Math3D.h>
#include <PVX.inl>
#include <string>
#include <map>

#include "PVX_OpenGL_Extern.inl"



namespace PVX::OpenGL {
	enum class PrimitiveType {
		POINTS = GL_POINTS,
		LINE_STRIP = GL_LINE_STRIP,
		LINE_LOOP = GL_LINE_LOOP,
		LINES = GL_LINES,
		LINE_STRIP_ADJACENCY = GL_LINE_STRIP_ADJACENCY,
		LINES_ADJACENCY = GL_LINES_ADJACENCY,
		TRIANGLE_STRIP = GL_TRIANGLE_STRIP,
		TRIANGLE_FAN = GL_TRIANGLE_FAN,
		TRIANGLES = GL_TRIANGLES,
		TRIANGLE_STRIP_ADJACENCY = GL_TRIANGLE_STRIP_ADJACENCY,
		TRIANGLES_ADJACENCY = GL_TRIANGLES_ADJACENCY,
		PATCHES = GL_PATCHES,
	};

	enum class TextureType {
		UNSIGNED_BYTE = GL_UNSIGNED_BYTE,
		BYTE = GL_BYTE,
		UNSIGNED_SHORT = GL_UNSIGNED_SHORT,
		SHORT = GL_SHORT,
		UNSIGNED_INT = GL_UNSIGNED_INT,
		INT = GL_INT,
		HALF_FLOAT = GL_HALF_FLOAT,
		FLOAT = GL_FLOAT,
		UNSIGNED_BYTE_3_3_2 = GL_UNSIGNED_BYTE_3_3_2,
		UNSIGNED_BYTE_2_3_3_REV = GL_UNSIGNED_BYTE_2_3_3_REV,
		UNSIGNED_SHORT_5_6_5 = GL_UNSIGNED_SHORT_5_6_5,
		UNSIGNED_SHORT_5_6_5_REV = GL_UNSIGNED_SHORT_5_6_5_REV,
		UNSIGNED_SHORT_4_4_4_4 = GL_UNSIGNED_SHORT_4_4_4_4,
		UNSIGNED_SHORT_4_4_4_4_REV = GL_UNSIGNED_SHORT_4_4_4_4_REV,
		UNSIGNED_SHORT_5_5_5_1 = GL_UNSIGNED_SHORT_5_5_5_1,
		UNSIGNED_SHORT_1_5_5_5_REV = GL_UNSIGNED_SHORT_1_5_5_5_REV,
		UNSIGNED_INT_8_8_8_8 = GL_UNSIGNED_INT_8_8_8_8,
		UNSIGNED_INT_8_8_8_8_REV = GL_UNSIGNED_INT_8_8_8_8_REV,
		UNSIGNED_INT_10_10_10_2 = GL_UNSIGNED_INT_10_10_10_2,
		UNSIGNED_INT_2_10_10_10_REV = GL_UNSIGNED_INT_2_10_10_10_REV
	};

	enum class TextureFormat {
		RED = GL_RED,
		RG = GL_RG,
		RGB = GL_RGB,
		BGR = GL_BGR,
		RGBA = GL_RGBA,
		BGRA = GL_BGRA,
		RED_INTEGER = GL_RED_INTEGER,
		RG_INTEGER = GL_RG_INTEGER,
		RGB_INTEGER = GL_RGB_INTEGER,
		BGR_INTEGER = GL_BGR_INTEGER,
		RGBA_INTEGER = GL_RGBA_INTEGER,
		BGRA_INTEGER = GL_BGRA_INTEGER,
		STENCIL_INDEX = GL_STENCIL_INDEX,
		DEPTH_COMPONENT = GL_DEPTH_COMPONENT,
		DEPTH_STENCIL = GL_DEPTH_STENCIL
	};
	enum class InternalFormat {
		DEPTH_COMPONENT = GL_DEPTH_COMPONENT,
		DEPTH_STENCIL = GL_DEPTH_STENCIL,
		RED = GL_RED,
		RG = GL_RG,
		RGB = GL_RGB,
		RGBA = GL_RGBA,
		R8 = GL_R8,
		R8_SNORM = GL_R8_SNORM,
		R16 = GL_R16,
		R16_SNORM = GL_R16_SNORM,
		RG8 = GL_RG8,
		RG8_SNORM = GL_RG8_SNORM,
		RG16 = GL_RG16,
		RG16_SNORM = GL_RG16_SNORM,
		R3_G3_B2 = GL_R3_G3_B2,
		RGB4 = GL_RGB4,
		RGB5 = GL_RGB5,
		RGB8 = GL_RGB8,
		RGB8_SNORM = GL_RGB8_SNORM,
		RGB10 = GL_RGB10,
		RGB12 = GL_RGB12,
		RGB16_SNORM = GL_RGB16_SNORM,
		RGBA2 = GL_RGBA2,
		RGBA4 = GL_RGBA4,
		RGB5_A1 = GL_RGB5_A1,
		RGBA8 = GL_RGBA8,
		RGBA8_SNORM = GL_RGBA8_SNORM,
		RGB10_A2 = GL_RGB10_A2,
		RGB10_A2UI = GL_RGB10_A2UI,
		RGBA12 = GL_RGBA12,
		RGBA16 = GL_RGBA16,
		SRGB8 = GL_SRGB8,
		SRGB8_ALPHA8 = GL_SRGB8_ALPHA8,
		R16F = GL_R16F,
		RG16F = GL_RG16F,
		RGB16F = GL_RGB16F,
		RGBA16F = GL_RGBA16F,
		R32F = GL_R32F,
		RG32F = GL_RG32F,
		RGB32F = GL_RGB32F,
		RGBA32F = GL_RGBA32F,
		R11F_G11F_B10F = GL_R11F_G11F_B10F,
		RGB9_E5 = GL_RGB9_E5,
		R8I = GL_R8I,
		R8UI = GL_R8UI,
		R16I = GL_R16I,
		R16UI = GL_R16UI,
		R32I = GL_R32I,
		R32UI = GL_R32UI,
		RG8I = GL_RG8I,
		RG8UI = GL_RG8UI,
		RG16I = GL_RG16I,
		RG16UI = GL_RG16UI,
		RG32I = GL_RG32I,
		RG32UI = GL_RG32UI,
		RGB8I = GL_RGB8I,
		RGB8UI = GL_RGB8UI,
		RGB16I = GL_RGB16I,
		RGB16UI = GL_RGB16UI,
		RGB32I = GL_RGB32I,
		RGB32UI = GL_RGB32UI,
		RGBA8I = GL_RGBA8I,
		RGBA8UI = GL_RGBA8UI,
		RGBA16I = GL_RGBA16I,
		RGBA16UI = GL_RGBA16UI,
		RGBA32I = GL_RGBA32I,
		RGBA32UI = GL_RGBA32UI,
		COMPRESSED_RED = GL_COMPRESSED_RED,
		COMPRESSED_RG = GL_COMPRESSED_RG,
		COMPRESSED_RGB = GL_COMPRESSED_RGB,
		COMPRESSED_RGBA = GL_COMPRESSED_RGBA,
		COMPRESSED_SRGB = GL_COMPRESSED_SRGB,
		COMPRESSED_SRGB_ALPHA = GL_COMPRESSED_SRGB_ALPHA,
		COMPRESSED_RED_RGTC1 = GL_COMPRESSED_RED_RGTC1,
		COMPRESSED_SIGNED_RED_RGTC1 = GL_COMPRESSED_SIGNED_RED_RGTC1,
		COMPRESSED_RG_RGTC2 = GL_COMPRESSED_RG_RGTC2,
		COMPRESSED_SIGNED_RG_RGTC2 = GL_COMPRESSED_SIGNED_RG_RGTC2,
		COMPRESSED_RGBA_BPTC_UNORM = GL_COMPRESSED_RGBA_BPTC_UNORM,
		COMPRESSED_SRGB_ALPHA_BPTC_UNORM = GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM,
		COMPRESSED_RGB_BPTC_SIGNED_FLOAT = GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT,
		COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT = GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT
	};

	enum class TextureFilter {
		NEAREST = GL_NEAREST,
		LINEAR = GL_LINEAR,
		NEAREST_MIPMAP_NEAREST = GL_NEAREST_MIPMAP_NEAREST,
		LINEAR_MIPMAP_NEAREST = GL_LINEAR_MIPMAP_NEAREST,
		NEAREST_MIPMAP_LINEAR = GL_NEAREST_MIPMAP_LINEAR,
		LINEAR_MIPMAP_LINEAR = GL_LINEAR_MIPMAP_LINEAR,
	};

	enum class TextureWrap {
		CLAMP =  GL_CLAMP,
		REPEAT = GL_REPEAT,
	};

	enum class TextureProperty {
		TEXTURE_WRAP_S = GL_TEXTURE_WRAP_S,
		TEXTURE_WRAP_T = GL_TEXTURE_WRAP_T,
		TEXTURE_WRAP_R = GL_TEXTURE_WRAP_R,
		TEXTURE_MIN_FILTER = GL_TEXTURE_MIN_FILTER,
		TEXTURE_MAG_FILTER = GL_TEXTURE_MAG_FILTER,
		TEXTURE_MIN_LOD = GL_TEXTURE_MIN_LOD,
		TEXTURE_MAX_LOD = GL_TEXTURE_MAX_LOD,
		TEXTURE_LOD_BIAS = GL_TEXTURE_LOD_BIAS,
		TEXTURE_COMPARE_MODE = GL_TEXTURE_COMPARE_MODE,
		TEXTURE_COMPARE_FUNC = GL_TEXTURE_COMPARE_FUNC,
	};

	typedef struct Attribute {
		std::string Name;
		int Size;
		GLenum Type;
		int Normalized;
		int Offset;
		std::string GLSL;
	} Attribute;

	class Context {
	public:
		~Context();
		Context(HWND hWnd);
		Context(HWND hWnd, std::function<void(Context& gl)> Function);
		Context(HWND hWnd, std::function<void(Context& gl, void*)> Function, void * Data);

		void MakeCurrent() const;
		void UnMakeCurrent() const;
		void Present() const;
		void Stop() { Running = false; };
		inline HWND Window() { return hWnd; };
		bool Running = true, Active = true;
		int Width, Height;
		void Viewport();
		void Resized();
		void AddTask(std::function<void(PVX::OpenGL::Context&)> Task);
		void DoTasks();

		void Enable(GLenum What, bool enalbe = true);

		void EnableLighting();
		void DisableLighting();
		void Enable3D();
		void Disable3D();

		struct MaterialFunctions {
			static void Diffuse(const PVX::Vector3D& Color);
			static void Specular(const PVX::Vector3D& Color);
			static void Ambient(const PVX::Vector3D& Color);
			static void Emissive(const PVX::Vector3D& Color);

			static void Diffuse(const PVX::Vector4D& Color);
			static void Specular(const PVX::Vector4D& Color);
			static void Ambient(const PVX::Vector4D& Color);
			static void Emissive(const PVX::Vector4D& Color);

			static void SpecularPower(float p);
		} Material;

	protected:
		void glThread_Func1(HWND hWnd, std::function<void(Context& gl)> Function);
		void glThread_Func2(HWND hWnd, std::function<void(Context& gl, void*)> Function, void * Data);
		std::thread glThread;
		void Init(HWND hWnd, int Flags = 0);
		HWND hWnd;
		HDC dc;
		HGLRC rc;
		std::mutex AsyncTaskMutex;
		std::vector<std::function<void(PVX::OpenGL::Context&)>> Tasks;
		int UniformAlignSize;
		void InitExtensions();
	};

	class Light {
		int Index;
	public:
		Light(const int Index);
		void Position(const Vector4D& v);
		void Diffuse(const Vector4D& v);
		void Ambient(const Vector4D& v);
		void Specular(const Vector4D& v);
		void Attenuation(const Vector3D& v);
		void Enable(int enable = 1);
	};

	class Camera {
	public:
		Camera();
		Camera(int Width, int Height, float FovDeg, float Near, float Far);
		Vector3D Position;
		Vector3D OrbitCenter;
		Vector3D Rotation;
		float Width, Height, OrbitDistance;
		Vector3D& GetLookVector(Vector3D& Look);
		Vector3D& GetUpVector(Vector3D& Up);
		Vector3D& GetRightVector(Vector3D& Right);
		Matrix4x4& UpdateView();
		Matrix4x4& UpdateView_Orbit();

		Vector3D& Move(float x, float y, float z);
		Vector3D& Move(const Vector3D& xyz);
		Vector3D& MoveLevel(const Vector3D& xyz);

		Vector3D& MoveCenter(float x, float y, float z);
		Vector3D& MoveCenter(const Vector3D& xyz);
		Vector3D& MoveCenterLevel(const Vector3D& xyz);

		Matrix4x4& SetPerspective(float FovDeg, float Near, float Far);
		Ray& CastScreenRay(float x, float y, Ray& Ray);
		Ray& CastRay(float x, float y, Ray& Ray);
		Ray CastScreenRay(float x, float y);
		Ray CastRay(float x, float y);
		void SetSize(int Width, int Heght);
		void SetProjectionMatrix(Matrix4x4& Mat);
		void SetViewMatrix(Matrix4x4& Mat);
		Matrix4x4& GetViewMatrix();
		Matrix4x4& GetProjectionMatrix();

		void OrbitRelative(float u, float v, float Distance = 1.0f);
	protected:
		Matrix4x4* _View, * _Perspective;
		struct {
			Matrix4x4 View, Perspective;
		} Storage;
	};

	class ConstantBuffer {
	public:
		~ConstantBuffer();
		ConstantBuffer();
		ConstantBuffer(const void* Data, int Size);
		ConstantBuffer(const std::vector<unsigned char>& Data);
		void Update(const void* Data, int Size);
		void Update(const std::vector<unsigned char>& Data);
		unsigned int Get() const { return Id; }
	private:
		unsigned int Id = 0;
		PVX::RefCounter Ref;
	};

	class VertexBuffer {
	public:
		~VertexBuffer();
		VertexBuffer() : Id{ 0 } {};
		VertexBuffer(const void* Data, int SizeInBytes);
		inline VertexBuffer(const std::vector<unsigned char>& Data) :VertexBuffer{ Data.data(), Data.size() } {};
		void Update(const void* Data, int SizeInBytes);
		inline void Update(const std::vector<unsigned char>& Data) { Update(Data.data(), Data.size()); };
		unsigned int Get() const { return Id; }
	private:
		unsigned int Id;
		PVX::RefCounter Ref;
	};
	class IndexBuffer {
	public:
		~IndexBuffer();
		IndexBuffer(const unsigned int* Data, int Count);
		inline IndexBuffer(const std::vector<unsigned int>& Data) :IndexBuffer{ Data.data(), Data.size() } {};
		unsigned int Get() const { return Id; }
	private:
		IndexBuffer() : Id{ 0 } {};
		void Update(const unsigned int* Data, int Count);
		inline void Update(const std::vector<unsigned int>& Data) { Update(Data.data(), Data.size()); };
		unsigned int Id;
		PVX::RefCounter Ref;
	};
	class Shader {
	public:
		enum class ShaderType : unsigned int {
			Uninitialized = 0,
			FragmentShader = GL_FRAGMENT_SHADER,
			VertexShader = GL_VERTEX_SHADER,
			GeomentryShader = GL_GEOMETRY_SHADER,
			TessellationEvaluationShader = GL_TESS_EVALUATION_SHADER,
			TessellationControlShader = GL_TESS_CONTROL_SHADER,
			ComputeShader = GL_COMPUTE_SHADER,
		};
		~Shader();
		Shader(ShaderType Type): Type{ Type }, Id{ 0 }{};
		Shader(ShaderType Type, const std::string& Source);
		void Source(const std::string& Source);

		std::string Debug(const std::string& Code);

		unsigned int Get() const { return Id; }
	private:
		ShaderType Type = ShaderType::Uninitialized;
		unsigned int Id = 0;
		PVX::RefCounter Ref;
	};

	class Texture2D {
	public:
		~Texture2D();
		Texture2D();
		Texture2D(int Width, int Height, int Channels, int BytesPerChannel);
		Texture2D(int Width, int Height, int Channels, int BytesPerChannel, void* Data);
		Texture2D(int Width, int Height, int InternalFormat, int Format, int Type, void* Data);
		Texture2D(int Width, int Height, InternalFormat internalFormat, TextureFormat Format, TextureType Type, void* Data);

		void Update(int Width, int Height, int Channels, int BytesPerChannel, void* Data);
		void Update(int Width, int Height, int InternalFormat, int Format, int Type, void * Data);
		void Update(void* Data);

		void UpdateAndBind(int Width, int Height, int Channels, int BytesPerChannel, void* Data);
		void UpdateAndBind(int Width, int Height, int InternalFormat, int Format, int Type, void* Data);
		void UpdateAndBind(void* Data);

		void GenerateMipmaps();
		void Bind(int Unit = 0);
		void Unbind();
		static Texture2D MakeDepthBuffer32F(int Width, int Height);
		static Texture2D MakeStencilBuffer(int Width, int Height);
		static Texture2D MakeDepthStencilBuffer24_8(int Width, int Height);
		unsigned int Get() const { return Id; }
	private:
		Texture2D(unsigned int Id);
		void Resize(int Width, int Height);
		unsigned int Id = 0;
		int Width = 0, Height = 0, InternalFormat = 0, Format = 0, Type = 0;
		PVX::RefCounter Ref;
		friend class FrameBufferObject;
	};

	class TextureCube {
	public:
		~TextureCube();
		TextureCube();
		TextureCube(int Width, int Height, int TilesX, int TilesY, int Channels, int BytesPerChannel, void* Data, const std::initializer_list<int>& Tiles);
		TextureCube(int Width, int Height, int TilesX, int TilesY, int InternalFormat, int Format, int Type, void* Data, const std::initializer_list<int>& Tiles);
		void Update(int Width, int Height, int TilesX, int TilesY, int Channels, int BytesPerChannel, void* Data, const std::initializer_list<int>& Tiles);
		void Update(int Width, int Height, int TilesX, int TilesY, int InternalFormat, int Format, int Type, void* Data, const std::initializer_list<int>& Tiles);
		void Update(void* Data, int Width, int Height, int TilesX, int TylesY, const std::initializer_list<int>& Tiles);
		void Bind(int Unit = 0);
		void Unbind();
		void GenerateMipmaps();
		unsigned int Get() const { return Id; }
	private:
		TextureCube(unsigned int Id);
		unsigned int Id = 0;
		int Side = 0, InternalFormat = 0, Format = 0, Type = 0, PixelSize = 0;
		PVX::RefCounter Ref;
		friend class FrameBufferObject;
	};

	class Sampler {
		unsigned int Id = 0;
		PVX::RefCounter Ref;
	public:
		Sampler(TextureFilter MinAndMag = TextureFilter::LINEAR, TextureWrap Wrap = TextureWrap::REPEAT);
		Sampler(TextureFilter Min, TextureFilter Mag, TextureWrap Wrap = TextureWrap::REPEAT);
		unsigned int Get() const { return Id; }
	};

	class FrameBufferObject {
	public:
		FrameBufferObject(int Width, int Height);
		FrameBufferObject() {};

		Texture2D AddColorAttachment(InternalFormat iFormat, TextureFormat Format, TextureType Type);
		Texture2D AddDepthAttachment();
		Texture2D AddDepthStencilAttachment();

		int Build();

		void Resize(int Width, int Height);
		void Bind();
		void Unbind();
		unsigned int Get() const { return Id; }
	private:
		int Width = 0;
		int Height = 0;
		std::vector<Texture2D> ColorBuffers;
		Texture2D DepthStencilBuffer;
		unsigned int Id = 0;
		PVX::RefCounter Ref;
	};

	class Geometry {
	public:
		void Draw();
		static void Unbind();
		Geometry(const Geometry&) = default;
		Geometry(PrimitiveType Type, int Stride, int iCount, const VertexBuffer& vBuffer, const IndexBuffer& iBuffer, const std::vector<Attribute>& attributes);
	protected:
		int& LastAtrribCount;
		int Mode;
		int Stride;
		int IndexCount;
		std::vector<Attribute> Attributes;
		VertexBuffer Vertices;
		IndexBuffer Indices;
		friend class Pipeline;
	};


	class Program {
	public:
		~Program();
		Program() = default;
		Program(const std::initializer_list<Shader>& sh);
		void AddShader(const Shader& sh);
		void AddShaders(const std::initializer_list<Shader>& sh);
		void Build();
		void Bind() const;
		void Unbind() const;
		unsigned int Get() const { return Id; }

		unsigned int UniformBlockIndex(const char* Name);
		unsigned int UniformLocation(const char* Name);
		unsigned int UniformIndex(const char* Name);
		void BindUniformBlock(int BlockIndex, const ConstantBuffer& Buffer);

		void BindUniform(int Index, float Value);
		void BindUniform(int Index, const PVX::Vector2D& Value);
		void BindUniform(int Index, const PVX::Vector3D& Value);
		void BindUniform(int Index, const PVX::Vector4D& Value);

		void BindUniform(int Index, int Value);
		void BindUniform(int Index, const PVX::iVector2D& Value);
		void BindUniform(int Index, const PVX::iVector3D& Value);
		void BindUniform(int Index, const PVX::iVector4D& Value);

		void BindUniform(int Index, const PVX::Matrix4x4& Value);

		void BindUniform(int Index, const std::vector<float>& Value);
		void BindUniform(int Index, const std::vector<PVX::Vector2D>& Value);
		void BindUniform(int Index, const std::vector<PVX::Vector3D>& Value);
		void BindUniform(int Index, const std::vector<PVX::Vector4D>& Value);

		void BindUniform(int Index, const std::vector<int>& Value);
		void BindUniform(int Index, const std::vector<PVX::iVector2D>& Value);
		void BindUniform(int Index, const std::vector<PVX::iVector3D>& Value);
		void BindUniform(int Index, const std::vector<PVX::iVector4D>& Value);

		void BindUniform(int Index, const std::vector<PVX::Matrix4x4>& Value);

		void BindUniform(int Index, const float* Value, int Count);
		void BindUniform(int Index, const PVX::Vector2D* Value, int Count);
		void BindUniform(int Index, const PVX::Vector3D* Value, int Count);
		void BindUniform(int Index, const PVX::Vector4D* Value, int Count);

		void BindUniform(int Index, const int* Value, int Count);
		void BindUniform(int Index, const PVX::iVector2D* Value, int Count);
		void BindUniform(int Index, const PVX::iVector3D* Value, int Count);
		void BindUniform(int Index, const PVX::iVector4D* Value, int Count);

		void BindUniform(int Index, const PVX::Matrix4x4* Value, int Count);
	private:
		std::vector<Shader> Shaders;
		unsigned int Id = 0;
		PVX::RefCounter Ref;
		std::vector<std::string> UniformNames;
		std::vector<std::string> UniformBlockNames;
	};

	class Pipeline {
	public:
		void Bind();
		void Unbind();
		void Shaders(const Program&);
		void Textures2D(const std::string& Name, const Sampler& sampler, const Texture2D& Tex);
		void Textures2D(const std::initializer_list<std::tuple<std::string, Sampler, Texture2D>>& Tex);
		void TexturesCube(const std::string& Name, const Sampler& sampler, const TextureCube& Tex);
		void TexturesCube(const std::initializer_list<std::tuple<std::string, Sampler, TextureCube>>& Tex);
		void UniformBlock(const std::string& Name, const ConstantBuffer& Buf);
		void UniformBlock(const std::initializer_list<std::pair<std::string, ConstantBuffer>>& Buf);

		inline unsigned int UniformLocation(const char* Name) { return Prog.UniformLocation(Name); }
		template<typename T> inline void BindUniform(int Index, const T& Value) { Prog.BindUniform(Index, Value); }
		template<typename T> inline void BindUniform(int Index, const T* Value, int Count) { Prog.BindUniform(Index, Value, Count); }
	private:
		Program Prog;
		FrameBufferObject FBuffer;
		std::vector<std::tuple<unsigned int, Texture2D>> Tex2D;
		std::vector<std::tuple<unsigned int, TextureCube>> TexCube;
		std::unordered_map<int, ConstantBuffer> Consts;
	};
}

inline void glLoadMatrix(const PVX::Matrix4x4& mat) {
	glLoadMatrixf(mat.m16);
}
inline void glMultMatrix(const PVX::Matrix4x4& mat) {
	glMultMatrixf(mat.m16);
}


#ifndef _NO_GL_DEBUG_
#ifdef _DEBUG

static std::map<unsigned int, const char*> glErrors{
	{GL_INVALID_ENUM, "An unacceptable value is specified for an enumerated argument. The offending function is ignored, having no side effect other than to set the error flag."},
	{GL_INVALID_VALUE, "A numeric argument is out of range. The offending function is ignored, having no side effect other than to set the error flag."},
	{GL_INVALID_OPERATION, "The specified operation is not allowed in the current state. The offending function is ignored, having no side effect other than to set the error flag."},
	{GL_NO_ERROR, "No error has been recorded. The value of this symbolic constant is guaranteed to be zero."},
	{GL_STACK_OVERFLOW, "This function would cause a stack overflow. The offending function is ignored, having no side effect other than to set the error flag."},
	{GL_STACK_UNDERFLOW, "This function would cause a stack underflow. The offending function is ignored, having no side effect other than to set the error flag."},
	{GL_OUT_OF_MEMORY, "There is not enough memory left to execute the function. The state of OpenGL is undefined, except for the state of the error flags, after this error is recorded."}
};

#define GL_CHECK(x) \
	x; \
{ \
	GLenum glError = glGetError(); \
	if(glError != GL_NO_ERROR) { \
	fprintf(stderr, "glGetError() = %i (0x%.8x) \"%s\" at File %s line %i\n", glError, glError, glErrors[glError], __FILE__, __LINE__); \
		} \
}
#else
#define GL_CHECK(x) x

#endif
#else
#define GL_CHECK(x) x
#endif
#pragma once
//#include <Windows.h>

//#define _NO_GL_DEBUG_

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
		QUADS = GL_QUADS,
		QUAD_STRIP = GL_QUAD_STRIP,
		POLYGON = GL_POLYGON,
	};

	enum class TextureType {
		NONE = 0,
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
		NONE = 0,
		LUMINANCE = GL_LUMINANCE,
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
		NONE = 0,
		DEPTH_COMPONENT = GL_DEPTH_COMPONENT,
		DEPTH_COMPONENT32F = GL_DEPTH_COMPONENT32F,
		DEPTH24_STENCIL8 = GL_DEPTH24_STENCIL8,
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
		CLAMP_TO_EDGE = GL_CLAMP_TO_EDGE
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

	enum class BufferUsege {
		Unspecified = 0,
		STREAM_DRAW = GL_STREAM_DRAW,
		STREAM_READ = GL_STREAM_READ,
		STREAM_COPY = GL_STREAM_COPY,
		STATIC_DRAW = GL_STATIC_DRAW,
		STATIC_READ = GL_STATIC_READ,
		STATIC_COPY = GL_STATIC_COPY,
		DYNAMIC_DRAW = GL_DYNAMIC_DRAW,
		DYNAMIC_READ = GL_DYNAMIC_READ,
		DYNAMIC_COPY = GL_DYNAMIC_COPY
	};

	enum class AttribType {
		BYTE = GL_BYTE,
		UNSIGNED_BYTE = GL_UNSIGNED_BYTE,
		SHORT = GL_SHORT,
		UNSIGNED_SHORT = GL_UNSIGNED_SHORT,
		INT = GL_INT,
		UNSIGNED_INT = GL_UNSIGNED_INT,
		HALF_FLOAT = GL_HALF_FLOAT,
		FLOAT = GL_FLOAT,
		DOUBLE = GL_DOUBLE,
		FIXED = GL_FIXED,
		INT_2_10_10_10_REV = GL_INT_2_10_10_10_REV,
		UNSIGNED_INT_2_10_10_10_REV = GL_UNSIGNED_INT_2_10_10_10_REV,
		UNSIGNED_INT_10F_11F_11F_REV = GL_UNSIGNED_INT_10F_11F_11F_REV,
	};

	inline int AttribSize(AttribType a) {
		switch (a) 	{
			case PVX::OpenGL::AttribType::BYTE:
			case PVX::OpenGL::AttribType::UNSIGNED_BYTE:
				return 1;
			case PVX::OpenGL::AttribType::SHORT:
			case PVX::OpenGL::AttribType::UNSIGNED_SHORT:
			case PVX::OpenGL::AttribType::HALF_FLOAT:
				return 2;
			case PVX::OpenGL::AttribType::INT:
			case PVX::OpenGL::AttribType::UNSIGNED_INT:
			case PVX::OpenGL::AttribType::FLOAT:
			case PVX::OpenGL::AttribType::FIXED:
			case PVX::OpenGL::AttribType::INT_2_10_10_10_REV:
			case PVX::OpenGL::AttribType::UNSIGNED_INT_2_10_10_10_REV:
			case PVX::OpenGL::AttribType::UNSIGNED_INT_10F_11F_11F_REV:
				return 4;
			case PVX::OpenGL::AttribType::DOUBLE:
				return 8;
		}
	}
	inline int IsInt(AttribType a) {
		switch (a) 	{
			case PVX::OpenGL::AttribType::BYTE:
			case PVX::OpenGL::AttribType::UNSIGNED_BYTE:
			case PVX::OpenGL::AttribType::SHORT:
			case PVX::OpenGL::AttribType::UNSIGNED_SHORT:
			case PVX::OpenGL::AttribType::INT:
			case PVX::OpenGL::AttribType::UNSIGNED_INT:
				return 1;
		}
		return 0;
	}

	struct Attribute {
		bool IsInt;
		std::string Name;
		int Size;
		AttribType Type;
		int Normalized;
		int Offset;
		std::string GLSL;
		//static Attribute Float(const std::string& Name) {
		//	return { 0, Name, 1, AttribType::FLOAT, 0, -1, "float" };
		//}
		//static Attribute Vec2(const std::string& Name) {
		//	return { 0, Name, 2, AttribType::FLOAT, 0, -1, "vec2" };
		//}
		//static Attribute Vec3(const std::string& Name) {
		//	return { 0, Name, 3, AttribType::FLOAT, 0, -1, "vec3" };
		//}
		//static Attribute Vec4(const std::string& Name) {
		//	return { 0, Name, 4, AttribType::FLOAT, 0, -1, "vec4" };
		//}
		//static Attribute Int(const std::string& Name) {
		//	return { 1, Name, 1, AttribType::INT, 0, -1, "int" };
		//}
		//static Attribute iVec2(const std::string& Name) {
		//	return { 1, Name, 2, AttribType::INT, 0, -1, "ivec2" };
		//}
		//static Attribute iVec3(const std::string& Name) {
		//	return { 1, Name, 3, AttribType::INT, 0, -1, "ivec3" };
		//}
		//static Attribute iVec4(const std::string& Name) {
		//	return { 1, Name, 4, AttribType::INT, 0, -1, "ivec4" };
		//}
	};

	class Context {
	public:
		~Context();
		Context(HWND hWnd, bool DepthStencil);
		Context(HWND hWnd, bool DepthStencil, std::function<void(Context& gl)> Function);
		Context(HWND hWnd, bool DepthStencil, std::function<void(Context& gl, void*)> Function, void * Data);

		void MakeCurrent() const;
		void UnMakeCurrent() const;
		void Present() const;
		void Stop() { Running = false; };
		inline HWND Window() { return hWnd; };
		bool Running = true, Active = true;
		union {
			PVX::iVector2D Size;
			struct {
				int Width, Height;
			};
		};
		void Viewport();
		void Viewport(int Width, int Height);
		void Viewport(const PVX::iVector2D& Size);
		void Resized();
		void Resized(int Width, int Height);
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

		struct GroupLimits {
			PVX::iVector3D Count;
			PVX::iVector3D Size;
		};

		GroupLimits GetComputeGroupLimits() const;

	protected:
		void glThread_Func1(HWND hWnd, bool DepthStencil, std::function<void(Context& gl)> Function);
		void glThread_Func2(HWND hWnd, bool DepthStencil, std::function<void(Context& gl, void*)> Function, void * Data);
		std::thread glThread;
		void Init(HWND hWnd, bool DepthStencil);
		HWND hWnd;
		HDC dc;
		HGLRC rc;
		std::mutex AsyncTaskMutex;
		std::vector<std::function<void(PVX::OpenGL::Context&)>> Tasks;
		int UniformAlignSize;
		void InitExtensions();
		int CurrentViewportWidth = 0;
		int CurrentViewportHeight = 0;
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

	enum class BufferType {
		ARRAY_BUFFER = GL_ARRAY_BUFFER,
		ATOMIC_COUNTER_BUFFER = GL_ATOMIC_COUNTER_BUFFER,
		COPY_READ_BUFFER = GL_COPY_READ_BUFFER,
		COPY_WRITE_BUFFER = GL_COPY_WRITE_BUFFER,
		DISPATCH_INDIRECT_BUFFER = GL_DISPATCH_INDIRECT_BUFFER,
		DRAW_INDIRECT_BUFFER = GL_DRAW_INDIRECT_BUFFER,
		ELEMENT_ARRAY_BUFFER = GL_ELEMENT_ARRAY_BUFFER,
		PIXEL_PACK_BUFFER = GL_PIXEL_PACK_BUFFER,
		PIXEL_UNPACK_BUFFER = GL_PIXEL_UNPACK_BUFFER,
		QUERY_BUFFER = GL_QUERY_BUFFER,
		SHADER_STORAGE_BUFFER = GL_SHADER_STORAGE_BUFFER,
		TEXTURE_BUFFER = GL_TEXTURE_BUFFER,
		TRANSFORM_FEEDBACK_BUFFER = GL_TRANSFORM_FEEDBACK_BUFFER,
		UNIFORM_BUFFER = GL_UNIFORM_BUFFER
	};

	enum class BufferFlags {
		NONE = 0,
		DYNAMIC_STORAGE_BIT = GL_DYNAMIC_STORAGE_BIT,
		MAP_READ_BIT = GL_MAP_READ_BIT,
		MAP_WRITE_BIT = GL_MAP_WRITE_BIT,
		MAP_PERSISTENT_BIT = GL_MAP_PERSISTENT_BIT,
		MAP_COHERENT_BIT = GL_MAP_COHERENT_BIT,
		CLIENT_STORAGE_BIT = GL_CLIENT_STORAGE_BIT
	};

	class Buffer {
		struct Buffer_Data {
			unsigned int Id = 0;
			int Size = 0;
			BufferType Type = BufferType::UNIFORM_BUFFER;
			BufferUsege Usage = BufferUsege::STATIC_DRAW;
			BufferFlags Flags;
			bool OnlyGrow = true;
		};
		Buffer(const Buffer_Data& dt);
	public:
		enum class MapAccess {
			READ_ONLY = GL_READ_ONLY,
			WRITE_ONLY = GL_WRITE_ONLY,
			READ_WRITE = GL_READ_WRITE
		};
		Buffer();
		Buffer(BufferUsege Usage);
		Buffer(bool IsUniformBlock, BufferUsege Usage);
		Buffer(const Buffer&) = default;
		Buffer(const void* Data, int Size, bool IsUniformBlock, BufferUsege Usage = BufferUsege::STATIC_DRAW);
		Buffer(const std::vector<unsigned char>& Data, bool IsUniformBlock, BufferUsege Usage = BufferUsege::STATIC_DRAW);

		inline void Name(const char* nm) { glObjectLabel(GL_BUFFER, ptr->Id, -1, nm); }

		void* Map(MapAccess access = MapAccess::WRITE_ONLY);
		void Unmap();
		template<typename T>
		T* Map(MapAccess access = MapAccess::WRITE_ONLY) { return (T*)Map(access); }

		void Update(int Size, const void* Data = nullptr);
		template<typename T>
		void Update(const std::vector<T>& Data) { Update(Data.size() * sizeof(T), Data.data()); }

		inline unsigned int Get() const { return ptr->Id; }
		inline GLenum GetType() const { return GLenum(ptr->Type); }

		static Buffer MakeImmutableShaderStorage(int Size, void* Data);
		static Buffer MakeBuffer(BufferType Type, BufferFlags Flags, size_t SizeInBytes, const void* Data=nullptr);
		template<typename T>
		static Buffer MakeBuffer(BufferType Type, BufferFlags Flags, const std::vector<T>& Data) {
			return MakeBuffer(Type, Flags, Data.size() * sizeof(T), Data.data());
		}

		void Read(void* Data);
		template<typename T>
		std::vector<T> Read() {
			std::vector<T> ret(ptr->Size / sizeof(T));
			Read(ret.data());
			return std::move(ret);
		}
		template<typename T>
		void Read(std::vector<T>& Data) {
			Read(Data.data());
		}
	protected:
		std::shared_ptr<Buffer_Data> ptr;
	};

	class VertexBuffer {
	public:
		~VertexBuffer();
		VertexBuffer() : Id{ 0 } {};
		VertexBuffer(const void* Data, int SizeInBytes);
		inline VertexBuffer(const std::vector<unsigned char>& Data) :VertexBuffer{ Data.data(), int(Data.size()) } {};
		inline unsigned int Get() const { return Id; }
	private:
		void Update(const void* Data, int SizeInBytes);
		inline void Update(const std::vector<unsigned char>& Data) { Update(Data.data(), int(Data.size())); };

		unsigned int Id;
		PVX::RefCounter Ref;
		friend class StreamingVertexShader;
	};

	class StreamingVertexShader {
	public:
		StreamingVertexShader();
		void Update(const void* Data, size_t Size);
		inline void Update(const std::vector<unsigned char>& Data) { Update(Data.data(), Data.size()); }
		operator VertexBuffer();
	private:
		unsigned int Id = 0;
		PVX::RefCounter Ref;
		size_t Size;
	};

	class IndexBuffer {
	public:
		~IndexBuffer();
		IndexBuffer() : Id{ 0 } {};
		IndexBuffer(const unsigned int* Data, int Count);
		inline IndexBuffer(const std::vector<unsigned int>& Data) :IndexBuffer{ Data.data(), int(Data.size()) } {};
		inline IndexBuffer(const std::vector<int>& Data) :IndexBuffer{ (const unsigned int*)Data.data(), int(Data.size()) } {};
		
		inline unsigned int Get() const { return Id; }
	private:
		void Update(const unsigned int* Data, int Count);
		inline void Update(const std::vector<unsigned int>& Data) { Update(Data.data(), int(Data.size())); };
		inline void Update(const std::vector<int>& Data) { Update((const unsigned int*)Data.data(), int(Data.size())); };
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
		Shader() = default;
		Shader(const Shader&) = default;
		Shader(ShaderType Type): Type{ Type }, Id{ 0 }{};
		Shader(ShaderType Type, const std::string& Source);
		void Source(const std::string& Source);
		inline void Name(const char* nm) { glObjectLabel(GL_SHADER, Id, -1, nm); }

		std::string Debug(const std::string& Code);

		inline unsigned int Get() const { return Id; }
	private:
		ShaderType Type = ShaderType::Uninitialized;
		unsigned int Id = 0;
		PVX::RefCounter Ref;
	};

	class Texture2D {
	public:
		~Texture2D();
		Texture2D();
		Texture2D(const Texture2D&) = default;
		//Texture2D(Texture2D&&) = default;
		Texture2D& operator=(const Texture2D&) = default;
		Texture2D(int Width, int Height, int Channels, int BytesPerChannel);
		Texture2D(int Width, int Height, int Channels, int BytesPerChannel, void* Data);
		Texture2D(int Width, int Height, int InternalFormat, int Format, int Type, void* Data);
		Texture2D(int Width, int Height, InternalFormat internalFormat, TextureFormat Format, TextureType Type, void* Data);
		Texture2D(int Width, int Height, int Samples, InternalFormat internalFormat);

		void Update(int Width, int Height, int Channels, int BytesPerChannel, void* Data);
		void Update(int Width, int Height, int InternalFormat, int Format, int Type, void * Data);
		void Update(void* Data);

		void UpdateAndBind(int Width, int Height, int Channels, int BytesPerChannel, void* Data);
		void UpdateAndBind(int Width, int Height, int InternalFormat, int Format, int Type, void* Data);
		void UpdateAndBind(const void* Data);

		void GenerateMipmaps();
		void Bind(int Unit = 0) const;
		void BindToUnit(int Unit) const;
		void Unbind();
		static Texture2D MakeDepthBuffer32F(int Width, int Height);
		static Texture2D MakeStencilBuffer(int Width, int Height);
		static Texture2D MakeDepthStencilBuffer24_8(int Width, int Height);

		static Texture2D MakeTextureRGB8UB(int Width, int Height);
		static Texture2D MakeTextureRGBA8UB(int Width, int Height);
		static Texture2D MakeTextureRGB16F(int Width, int Height);
		static Texture2D MakeTextureRGBA16F(int Width, int Height);
		static Texture2D MakeTextureRGB32F(int Width, int Height);
		static Texture2D MakeTextureRGBA32F(int Width, int Height);

		static Texture2D MakeMultisampleTextureRGB8UB(int Width, int Height, int Samples);
		static Texture2D MakeMultisampleTextureRGBA8UB(int Width, int Height, int Samples);
		static Texture2D MakeMultisampleTextureRGB16F(int Width, int Height, int Samples);
		static Texture2D MakeMultisampleTextureRGBA16F(int Width, int Height, int Samples);
		static Texture2D MakeMultisampleTextureRGB32F(int Width, int Height, int Samples);
		static Texture2D MakeMultisampleTextureRGBA32F(int Width, int Height, int Samples);

		static Texture2D MakeMultisampleDepthBuffer32F(int Width, int Height, int Samples);
		static Texture2D MakeMultisampleDepthStencilBuffer24_8(int Width, int Height, int Samples);


		static Texture2D MakeTexture8UB(int width, int height, int channels, const float* data);
		static Texture2D MakeTexture16F(int width, int height, int channels, const float* data);
		static Texture2D MakeTexture32F(int width, int height, int channels, const float* data);

		void Filter(TextureFilter Min = TextureFilter::LINEAR, TextureFilter Max = TextureFilter::LINEAR);
		void WrapAll(TextureWrap w = TextureWrap::REPEAT);
		void FilterWrap(TextureFilter Min = TextureFilter::LINEAR, TextureFilter Max = TextureFilter::LINEAR, TextureWrap w = TextureWrap::REPEAT);

		inline unsigned int Get() const { return ptr->Id; }

		inline void Name(const char* nm) { glObjectLabel(GL_TEXTURE, ptr->Id, -1, nm); }
		inline int GetWidth() const { return ptr->Size.Width; }
		inline int GetHeight() const { return ptr->Size.Height; }
		inline GLenum GetTargetType() {	return ptr->Samples>1 ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D; }
		inline const PVX::iVector2D& GetSize() const { return ptr->Size; }
		void Resize(int Width, int Height);
	private:
		struct TextureData {
			unsigned int Id = 0;
			int Samples = 1;
			PVX::iVector2D Size;
			InternalFormat InternalFormat = PVX::OpenGL::InternalFormat(0);
			TextureFormat Format = PVX::OpenGL::TextureFormat(0);
			TextureType Type = PVX::OpenGL::TextureType(0);
		};
		std::shared_ptr<TextureData> ptr;
		//Texture2D(const TextureData&);
		friend class FrameBufferObject;
	};

	class TextureCube {
	public:
		~TextureCube();
		TextureCube();
		TextureCube(int Width, int Height, int TilesX, int TilesY, int Channels, int BytesPerChannel, void* Data, const std::initializer_list<int>& Tiles);
		TextureCube(int Width, int Height, int TilesX, int TilesY, int InternalFormat, int Format, int Type, void* Data, const std::initializer_list<int>& Tiles);
		TextureCube(int Width, int Height, int TilesX, int TilesY, InternalFormat InternalFormat, TextureFormat Format, TextureType Type, void* Data, const std::initializer_list<int>& Tiles);

		void Update(int Width, int Height, int TilesX, int TilesY, int Channels, int BytesPerChannel, void* Data, const std::initializer_list<int>& Tiles);
		void Update(int Width, int Height, int TilesX, int TilesY, int InternalFormat, int Format, int Type, void* Data, const std::initializer_list<int>& Tiles);
		void Update(void* Data, int Width, int Height, int TilesX, int TylesY, const std::initializer_list<int>& Tiles);
		void Bind(int Unit = 0);
		void Unbind();
		void GenerateMipmaps();
		inline unsigned int Get() const { return Id; }

		inline void Name(const char* nm) { glObjectLabel(GL_TEXTURE, Id, -1, nm); }
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
		Sampler() {}
		~Sampler();
		Sampler(TextureFilter MinAndMag, TextureWrap Wrap = TextureWrap::REPEAT);
		Sampler(TextureFilter Min, TextureFilter Mag, TextureWrap Wrap = TextureWrap::REPEAT);
		inline unsigned int Get() const { return Id; }
		inline void Name(const char* nm) { glObjectLabel(GL_SAMPLER, Id, -1, nm); }
	};

	class FrameBufferObject {
	public:
		FrameBufferObject(PVX::OpenGL::Context& gl, int Width, int Height);
		FrameBufferObject(PVX::OpenGL::Context& gl) : gl{ gl } {};

		const Texture2D& AddColorAttachment(InternalFormat iFormat, int Samples);
		const Texture2D& AddColorAttachment(InternalFormat iFormat, TextureFormat Format, TextureType Type);
		const Texture2D& AddDepthAttachment();
		const Texture2D& AddDepthStencilAttachment();

		const Texture2D& AddColorAttachment(const Texture2D& tex);
		void AddColorAttachments(const std::initializer_list<Texture2D>& tex);
		const Texture2D& AddDepthStencilAttachment(const Texture2D& tex);

		inline const Texture2D& AddColorAttachmentRGB8UB() { return AddColorAttachment(InternalFormat::RGB8, TextureFormat::RGB, TextureType::UNSIGNED_BYTE); }
		inline const Texture2D& AddColorAttachmentRGBA8UB() { return AddColorAttachment(InternalFormat::RGBA8, TextureFormat::RGBA, TextureType::UNSIGNED_BYTE); }
		inline const Texture2D& AddColorAttachmentRGB16F() { return AddColorAttachment(InternalFormat::RGB16F, TextureFormat::RGB, TextureType::HALF_FLOAT); }
		inline const Texture2D& AddColorAttachmentRGBA16F() { return AddColorAttachment(InternalFormat::RGBA16F, TextureFormat::RGBA, TextureType::HALF_FLOAT); }
		inline const Texture2D& AddColorAttachmentRGB32F() { return AddColorAttachment(InternalFormat::RGB32F, TextureFormat::RGB, TextureType::FLOAT); }
		inline const Texture2D& AddColorAttachmentRGBA32F() { return AddColorAttachment(InternalFormat::RGBA32F, TextureFormat::RGBA, TextureType::FLOAT); }

		inline const Texture2D& AddColorAttachmentRGB8UB(int Samples) { return AddColorAttachment(InternalFormat::RGB8, Samples); }
		inline const Texture2D& AddColorAttachmentRGBA8UB(int Samples) { return AddColorAttachment(InternalFormat::RGBA8, Samples); }
		inline const Texture2D& AddColorAttachmentRGB16F(int Samples) { return AddColorAttachment(InternalFormat::RGB16F, Samples); }
		inline const Texture2D& AddColorAttachmentRGBA16F(int Samples) { return AddColorAttachment(InternalFormat::RGBA16F, Samples); }
		inline const Texture2D& AddColorAttachmentRGB32F(int Samples) { return AddColorAttachment(InternalFormat::RGB32F, Samples); }
		inline const Texture2D& AddColorAttachmentRGBA32F(int Samples) { return AddColorAttachment(InternalFormat::RGBA32F, Samples); }

		const Texture2D& AddDepthAttachment(int Samples);


		int Build();

		void Resize(int Width, int Height);
		void Bind() const;
		void Unbind();
		inline unsigned int Get() const { return Id; }
		inline void Name(const char* nm) { glObjectLabel(GL_FRAMEBUFFER, Id, -1, nm); }
		inline const PVX::iVector2D& GetSize() const { return Id? Size: gl.Size; }
	private:
		PVX::OpenGL::Context& gl;
		PVX::iVector2D Size{};
		std::vector<Texture2D> ColorBuffers;
		Texture2D DepthStencilBuffer;
		unsigned int Id = 0;
		PVX::RefCounter Ref;
	};

	struct Geometry_init {
		VertexBuffer Buffer;
		std::vector<Attribute> Attributes;
		int Stride;
	};
	struct Geometry_init2 {
		VertexBuffer Buffer;
		std::vector<std::tuple<AttribType, int, int>> Attributes;
	};

	class Geometry {
		std::vector<VertexBuffer> VertexBuffers;
		IndexBuffer Indices;
		PVX::RefCounter Ref;
		PrimitiveType Type{ PrimitiveType(0)};
		unsigned int Id = 0;
		int IndexCount = 0;
		unsigned int Flags = 0;
	public:
		inline Geometry() {}
		Geometry(const Geometry&) = default;
		Geometry(Geometry&&) noexcept = default;
		Geometry& operator=(const Geometry&) = default;
		~Geometry();
		Geometry(PrimitiveType Type, int IndexCount, const IndexBuffer& Indices, const std::initializer_list<Geometry_init>& Buffers, bool OldVersion=false);
		Geometry(PrimitiveType Type, const std::vector<int>& Index, const std::initializer_list<Geometry_init2>& Vertex);
		void Draw() const;
		void Draw(int Count) const;
		unsigned int Get() const { return Id; }
		inline unsigned int GetFlags() const { return Flags; }

		void Bind();
		void DrawBound();
		static void Unbind();
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
		inline void Name(const char* nm) { glObjectLabel(GL_PROGRAM, Id, -1, nm); }

		unsigned int UniformBlockIndex(const char* Name) const;
		unsigned int UniformLocation(const char* Name) const;
		unsigned int StorageIndex(const char* Name) const;

		void BindUniform(int Index, float Value) const;
		void BindUniform(int Index, const PVX::Vector2D& Value) const;
		void BindUniform(int Index, const PVX::Vector3D& Value) const;
		void BindUniform(int Index, const PVX::Vector4D& Value) const;

		void BindUniform(int Index, int Value) const;
		void BindUniform(int Index, const PVX::iVector2D& Value) const;
		void BindUniform(int Index, const PVX::iVector3D& Value) const;
		void BindUniform(int Index, const PVX::iVector4D& Value) const;

		void BindUniform(int Index, const PVX::Matrix4x4& Value) const;

		void BindUniform(int Index, const std::vector<float>& Value) const;
		void BindUniform(int Index, const std::vector<PVX::Vector2D>& Value) const;
		void BindUniform(int Index, const std::vector<PVX::Vector3D>& Value) const;
		void BindUniform(int Index, const std::vector<PVX::Vector4D>& Value) const;

		void BindUniform(int Index, const std::vector<int>& Value) const;
		void BindUniform(int Index, const std::vector<PVX::iVector2D>& Value) const;
		void BindUniform(int Index, const std::vector<PVX::iVector3D>& Value) const;
		void BindUniform(int Index, const std::vector<PVX::iVector4D>& Value) const;

		void BindUniform(int Index, const std::vector<PVX::Matrix4x4>& Value) const;

		void BindUniform(int Index, const float* Value, int Count) const;
		void BindUniform(int Index, const PVX::Vector2D* Value, int Count) const;
		void BindUniform(int Index, const PVX::Vector3D* Value, int Count) const;
		void BindUniform(int Index, const PVX::Vector4D* Value, int Count) const;

		void BindUniform(int Index, const int* Value, int Count) const;
		void BindUniform(int Index, const PVX::iVector2D* Value, int Count) const;
		void BindUniform(int Index, const PVX::iVector3D* Value, int Count) const;
		void BindUniform(int Index, const PVX::iVector4D* Value, int Count) const;

		void BindUniform(int Index, const PVX::Matrix4x4* Value, int Count) const;

		bool SetUniformBlockIndex(const char* Name, int Index) const;
		bool SetShaderStrorageIndex(const char* Name, int Index) const; 
		bool SetTextureIndex(const char* Name, int Index) const;
	private:
		std::vector<Shader> Shaders;
		unsigned int Id = 0;
		PVX::RefCounter Ref;
		std::vector<std::string> UniformNames;
		std::vector<std::string> UniformBlockNames;

		// Deprecated
		unsigned int UniformIndex(const char* Name) const;
		void BindUniformBlock(int BlockIndex, const Buffer& Buffer);
	};

	inline void BindBuffer(int Index, const PVX::OpenGL::Buffer& buf) {
		glBindBufferBase(buf.GetType(), Index, buf.Get());
	}
	inline void BindTextureAndSampler(int Index, const PVX::OpenGL::Texture2D& Texture, const PVX::OpenGL::Sampler& Sampler) {
		glBindTextureUnit(Index, Texture.Get());
		glBindSampler(Index, Sampler.Get());
	}

	class Pipeline {
	public:
		void Bind();
		void Unbind();
		void Shaders(const Program&);

		void Textures2D(const std::string& Name, const Sampler& sampler, const Texture2D& Tex);
		void Textures2D(const std::initializer_list<std::tuple<std::string, Sampler, Texture2D>>& Tex);
		void TexturesCube(const std::string& Name, const Sampler& sampler, const TextureCube& Tex);
		void TexturesCube(const std::initializer_list<std::tuple<std::string, Sampler, TextureCube>>& Tex);

		void BindBuffer(const std::string& Name, const Buffer& Buf);
		void BindBuffer(const std::initializer_list<std::pair<std::string, Buffer>>& Buf);

		inline unsigned int UniformLocation(const char* Name) { return Prog.UniformLocation(Name); }
		template<typename T> inline void BindUniform(int Index, const T& Value) { Prog.BindUniform(Index, Value); }
		template<typename T> inline void BindUniform(int Index, const T* Value, int Count) { Prog.BindUniform(Index, Value, Count); }
		inline unsigned int GetProgramId() { return Prog.Get(); }
	private:
		Program Prog;
		//FrameBufferObject FBuffer;
		std::vector<std::tuple<unsigned int, unsigned int, unsigned int>> Tex;
		std::vector<Texture2D> Tex2D;
		std::vector<TextureCube> TexCube;
		std::unordered_map<int, Buffer> UniformBuffers;
		std::unordered_map<int, Buffer> StorageBuffers;

		void UniformBlock(const std::string& Name, const Buffer& Buf);
		void UniformBlock(const std::initializer_list<std::pair<std::string, Buffer>>& Buf);
		void UniformStorage(const std::string& Name, const Buffer& Buf);
		void UniformStorage(const std::initializer_list<std::pair<std::string, Buffer>>& Buf);
	};

	class ComputeProgram {
		Program p;
	public:
		ComputeProgram(const std::string& Code) : p{ { Shader::ShaderType::ComputeShader, Code } } {};
		void Execute(const std::initializer_list<Buffer>& Buffers, uint32_t CountX, uint32_t CountY = 1, uint32_t CountZ = 1);
	};

	class Camera {
	public:
		Camera(PVX::Matrix4x4* View = nullptr, PVX::Matrix4x4* Projection = nullptr);
		Camera(int Width, int Height, float FovDeg, float Near, float Far, PVX::Matrix4x4* View = nullptr, PVX::Matrix4x4* Projection = nullptr);
		Vector3D &Position;
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
		void SetSize(int Width, int Height);
		Matrix4x4& SetSizePerspective(int Width, int Height);
		void SetProjectionMatrix(Matrix4x4& Mat);
		void SetViewMatrix(Matrix4x4& Mat);
		Matrix4x4& GetViewMatrix();
		Matrix4x4& GetProjectionMatrix();

		void OrbitRelative(float u, float v, float Distance = 1.0f);

	protected:
		float _fov, _near, _far;
		Matrix4x4* _View, * _Perspective;
		struct {
			Matrix4x4 View = Matrix4x4::Identity();
			Matrix4x4 Perspective = Matrix4x4::Identity();
			Vector4D Position = { 0, 0, 0, 1 };
		} Storage;
	public:
		inline Buffer GetConstantBuffer() {
			return { &Storage, sizeof(Storage), true };
		}
		inline void UpdateConstantBuffer(Buffer& Buffer) { Buffer.Update(sizeof(Storage), &Storage); };
//#endif
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
		DebugBreak(); \
		} \
}
#else
#define GL_CHECK(x) x

#endif
#else
#define GL_CHECK(x) x
#endif
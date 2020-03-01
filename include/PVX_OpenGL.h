#pragma once
//#include <Windows.h>
#include <gl/glcorearb.h>
#include<gl/GL.h>
#include<gl/GLU.h>
#include <gl/glext.h>
#include <gl/wglext.h>
#include <thread>
#include <functional>
#include <PVX_Math3D.h>
#include <PVX.inl>
#include <string>

#ifdef _DEBUG
#include <map>
#endif

namespace PVX::OpenGL {
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
		unsigned int Id;
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
		enum ShaderType {
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
		ShaderType Type;
		unsigned int Id;
		PVX::RefCounter Ref;
	};
	class Program {
	public:
		~Program();
		void BindShader(const Shader& sh);
		void BindShaders(const std::initializer_list<Shader>& sh);
		void Build();
		void Bind() const;
		void Unbind() const;
		unsigned int Get() const { return Id; }
	private:
		std::vector<Shader> Shaders;
		unsigned int Id = 0;
		PVX::RefCounter Ref;
	};

	class Texture2D {
	public:
		~Texture2D();
		Texture2D();
		Texture2D(int Width, int Height, int Channels, int BytesPerChannel, void* Data);
		Texture2D(int Width, int Height, int InternalFormat, int Format, int Type, void* Data);
		void Update(int Width, int Height, int Channels, int BytesPerChannel, void* Data);
		void Update(int Width, int Height, int InternalFormat, int Format, int Type, void * Data);
		void Update(void* Data);
		void Bind();
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
		void Bind();
		void Unbind();
		unsigned int Get() const { return Id; }
	private:
		TextureCube(unsigned int Id);
		unsigned int Id = 0;
		int Side = 0, InternalFormat = 0, Format = 0, Type = 0, PixelSize = 0;
		PVX::RefCounter Ref;
		friend class FrameBufferObject;
	};

	class FrameBufferObject {
	public:
		FrameBufferObject(int Width, int Height);

		Texture2D AddColorAttachment(int InternalFormat, int Format, int Type);
		Texture2D AddDepthAttachment();
		Texture2D AddDepthStencilAttachment();

		int Build();

		void Resize(int Width, int Height);
		void Bind();
		void Unbind();
		unsigned int Get() const { return Id; }
	private:
		int Width;
		int Height;
		std::vector<Texture2D> ColorBuffers;
		Texture2D DepthStencilBuffer;
		unsigned int Id = 0;
		PVX::RefCounter Ref;
	};

	class Pipeline {
	public:
		Pipeline();
	private:
		FrameBufferObject FrameBuffer;
	};
}

extern PFNGLACTIVETEXTUREPROC glActiveTexture;
extern PFNGLATTACHSHADERPROC glAttachShader;
extern PFNGLBINDATTRIBLOCATIONPROC glBindAttribLocation;
extern PFNGLBINDBUFFERPROC glBindBuffer;
extern PFNGLBINDBUFFERBASEPROC glBindBufferBase;
extern PFNGLBINDFRAMEBUFFERPROC glBindFramebuffer;
extern PFNGLBINDRENDERBUFFERPROC glBindRenderbuffer;
extern PFNGLBUFFERDATAPROC glBufferData;
extern PFNGLBUFFERSUBDATAPROC glBufferSubData;
extern PFNGLCHECKFRAMEBUFFERSTATUSPROC glCheckFramebufferStatus;
extern PFNGLCOMPILESHADERPROC glCompileShader;
extern PFNGLCREATEPROGRAMPROC glCreateProgram;
extern PFNGLCREATESHADERPROC glCreateShader;
extern PFNGLDELETEBUFFERSPROC glDeleteBuffers;
extern PFNGLDELETEFRAMEBUFFERSPROC glDeleteFramebuffers;
extern PFNGLDELETEPROGRAMPROC glDeleteProgram;
extern PFNGLDELETESHADERPROC glDeleteShader;
extern PFNGLDETACHSHADERPROC glDetachShader;
extern PFNGLDISABLEVERTEXATTRIBARRAYPROC glDisableVertexAttribArray;
extern PFNGLENABLEVERTEXATTRIBARRAYPROC glEnableVertexAttribArray;
extern PFNGLFRAMEBUFFERRENDERBUFFERPROC glFramebufferRenderbuffer;
extern PFNGLFRAMEBUFFERTEXTUREPROC glFramebufferTexture;
extern PFNGLFRAMEBUFFERTEXTURE2DPROC glFramebufferTexture2D;
extern PFNGLGENBUFFERSPROC glGenBuffers;
extern PFNGLGENFRAMEBUFFERSPROC glGenFramebuffers;
extern PFNGLGETATTRIBLOCATIONPROC glGetAttribLocation;
extern PFNGLGETBUFFERPARAMETERIVPROC glGetBufferParameteriv;
extern PFNGLGETPROGRAMINFOLOGPROC glGetProgramInfoLog;
extern PFNGLGETPROGRAMIVPROC glGetProgramiv;
extern PFNGLGETSHADERINFOLOGPROC glGetShaderInfoLog;
extern PFNGLGETSHADERIVPROC glGetShaderiv;
extern PFNGLGETUNIFORMBLOCKINDEXPROC glGetUniformBlockIndex;
extern PFNGLGETUNIFORMLOCATIONPROC glGetUniformLocation;
extern PFNGLLINKPROGRAMPROC glLinkProgram;
extern PFNGLMAPBUFFERPROC glMapBuffer;
extern PFNGLMAPBUFFERRANGEPROC glMapBufferRange;
extern PFNGLRENDERBUFFERSTORAGEPROC glRenderbufferStorage;
extern PFNGLSHADERSOURCEPROC glShaderSource;
extern PFNGLTEXIMAGE3DPROC glTexImage3D;
extern PFNGLUNIFORM1FPROC glUniform1f;
extern PFNGLUNIFORM1IPROC glUniform1i;
extern PFNGLUNIFORM2FVPROC glUniform2fv;
extern PFNGLUNIFORM3FVPROC glUniform3fv;
extern PFNGLUNIFORM4FVPROC glUniform4fv;
extern PFNGLUNIFORMBLOCKBINDINGPROC glUniformBlockBinding;
extern PFNGLUNIFORMMATRIX4FVPROC glUniformMatrix4fv;
extern PFNGLUNMAPBUFFERPROC glUnmapBuffer;
extern PFNGLUSEPROGRAMPROC glUseProgram;
extern PFNGLVERTEXATTRIBPOINTERPROC glVertexAttribPointer;

extern PFNWGLSWAPINTERVALEXTPROC wglSwapInterval;


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
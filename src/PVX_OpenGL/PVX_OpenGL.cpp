#include <PVX_OpenGL.h>
#include <Windows.h>

namespace PVX::OpenGL {
	void Context::Init(HWND hWnd, int Flags) {
		this->hWnd = hWnd;
		PIXELFORMATDESCRIPTOR pfd = {
			sizeof(PIXELFORMATDESCRIPTOR),	// size of this pfd 
			1,								// version number
			PFD_DRAW_TO_WINDOW |			// support window
			PFD_SUPPORT_OPENGL |			// support OpenGL
			PFD_GENERIC_ACCELERATED |
			PFD_DOUBLEBUFFER,
			PFD_TYPE_RGBA,					// RGBA type
			32,								// 32-bit color depth
			0, 0, 0, 0, 0, 0,				// color bits ignored
			0,								// no alpha buffer
			0,								// shift bit ignored
			0,								// no accumulation buffer
			0, 0, 0, 0,						// accum bits ignored
			32,								// 32-bit z-buffer
			8,								// Stencil buffer
			0,								// no auxiliary buffer
			PFD_MAIN_PLANE,					// main layer
			0,								// reserved
			0, 0, 0							// layer masks ignored
		};
		pfd.dwFlags |= Flags;
		dc = GetDC(hWnd);
		int  iPixelFormat = ChoosePixelFormat(dc, &pfd);
		SetPixelFormat(dc, iPixelFormat, &pfd);
		rc = wglCreateContext(dc);
		this->MakeCurrent();
		int iAttributes[]{
			// WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
			WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
			WGL_COLOR_BITS_ARB, 32,
			WGL_DEPTH_BITS_ARB, 16,
			WGL_DOUBLE_BUFFER_ARB, GL_TRUE,
			WGL_FRAMEBUFFER_SRGB_CAPABLE_ARB, GL_TRUE,
			0, 0
		};
		float fAttributes[] = { 0, 0 };
		int   pf = 0;
		UINT  numFormats = 0;

		auto wglChoosePixelFormat = (PFNWGLCHOOSEPIXELFORMATARBPROC)wglGetProcAddress("wglChoosePixelFormatARB");
		auto wglCreateContextAttribs = (PFNWGLCREATECONTEXTATTRIBSARBPROC)wglGetProcAddress("wglCreateContextAttribsARB");
		//wglDeleteContext(rc);

		wglChoosePixelFormat(dc, iAttributes, fAttributes, 1, &pf, &numFormats);
		GLint attribs[16];
		int   attribCount = 0;

		attribs[attribCount] = 0;

		//rc = wglCreateContextAttribs(dc, 0, attribs);
		wglMakeCurrent(dc, rc);

		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &UniformAlignSize);

		RECT cl;
		GetClientRect(hWnd, &cl);
		Width = cl.right - cl.left;
		Height = cl.bottom - cl.top;

		if (!glActiveTexture) InitExtensions();
	}

	void Context::Present() const {
		SwapBuffers(dc);
	}

	void Context::glThread_Func1(HWND hWnd, std::function<void(Context& gl)> Function) {
		Init(hWnd);
		Function(*this);
		Running = 0;
	}
	void Context::glThread_Func2(HWND hWnd, std::function<void(Context& gl, void*)> Function, void* Data) {
		Init(hWnd);
		Function(*this, Data);
		Running = 0;
	}

	Context::~Context() {
		if (glThread.joinable()) {
			Running = false;
			glThread.join();
		}
	}

	Context::Context(HWND hWnd) {
		Init(hWnd);
	}

	Context::Context(HWND hWnd, std::function<void(Context& gl)> Function) {
		Running = true;
		glThread = std::thread(&Context::glThread_Func1, this, hWnd, Function);
	}

	Context::Context(HWND hWnd, std::function<void(Context& gl, void*)> Function, void* Data) {
		Running = true;
		glThread = std::thread(&Context::glThread_Func2, this, hWnd, Function, Data);
	}

	void Context::MakeCurrent() const {
		wglMakeCurrent(dc, rc);
	}
	void Context::UnMakeCurrent() const {
		wglMakeCurrent(0, 0);
	}
	void Context::Viewport() {
		glViewport(0, 0, Width, Height);
	}
	void Context::Resized() {
		RECT cl;
		GetClientRect(hWnd, &cl);
		Width = cl.right - cl.left;
		Height = cl.bottom - cl.top;
		glViewport(0, 0, Width, Height);
	}
	void Context::AddTask(std::function<void(PVX::OpenGL::Context&)> Task) {
		std::lock_guard<std::mutex> lock{ AsyncTaskMutex };
		Tasks.push_back(Task);
	}
	void Context::DoTasks() {
		if (Tasks.size()) {
			std::lock_guard<std::mutex> lock{ AsyncTaskMutex };
			for (auto& t : Tasks) t(*this);
			Tasks.clear();
		}
	}

	void Context::MaterialFunctions::Diffuse(const Vector3D& Color) {
		float c[]{ Color.r, Color.g, Color.b, 1.0f };
		glMaterialfv(GL_FRONT, GL_DIFFUSE, c);
	}

	void Context::MaterialFunctions::Specular(const Vector3D& Color) {
		float c[]{ Color.r, Color.g, Color.b, 1.0f };
		glMaterialfv(GL_FRONT, GL_SPECULAR, c);
	}

	void Context::MaterialFunctions::Ambient(const Vector3D& Color) {
		float c[]{ Color.r, Color.g, Color.b, 1.0f };
		glMaterialfv(GL_FRONT, GL_AMBIENT, c);
	}

	void Context::MaterialFunctions::Emissive(const Vector3D& Color) {
		float c[]{ Color.r, Color.g, Color.b, 1.0f };
		glMaterialfv(GL_FRONT, GL_EMISSION, c);
	}

	void Context::MaterialFunctions::Diffuse(const Vector4D& Color) {
		glMaterialfv(GL_FRONT, GL_DIFFUSE, Color.Array);
	}

	void Context::MaterialFunctions::Specular(const Vector4D& Color) {
		glMaterialfv(GL_FRONT, GL_SPECULAR, Color.Array);
	}

	void Context::MaterialFunctions::Ambient(const Vector4D& Color) {
		glMaterialfv(GL_FRONT, GL_AMBIENT, Color.Array);
	}

	void Context::MaterialFunctions::Emissive(const Vector4D& Color) {
		glMaterialfv(GL_FRONT, GL_EMISSION, Color.Array);
	}

	void Context::MaterialFunctions::SpecularPower(float p) {
		glMaterialf(GL_FRONT, GL_SHININESS, p);
	}

	void Context::EnableLighting() {
		glEnable(GL_LIGHTING);
	}

	void Context::DisableLighting() {
		glDisable(GL_LIGHTING);
	}

	void Context::Enable3D() {
		glEnable(GL_LIGHTING);
		glEnable(GL_DEPTH_TEST);
		glEnable(GL_NORMALIZE);
	}

	void Context::Disable3D() {
		glDisable(GL_LIGHTING);
		glDisable(GL_DEPTH_TEST);
		glDisable(GL_NORMALIZE);
	}

	Light::Light(const int i) : Index(i) {}

	void Light::Position(const Vector4D& v) {
		glLightfv(GL_LIGHT0 + Index, GL_POSITION, v.Array);
	}

	void Light::Diffuse(const Vector4D& v) {
		glLightfv(GL_LIGHT0 + Index, GL_DIFFUSE, v.Array);
	}

	void Light::Ambient(const Vector4D& v) {
		glLightfv(GL_LIGHT0 + Index, GL_AMBIENT, v.Array);
	}

	void Light::Specular(const Vector4D& v) {
		glLightfv(GL_LIGHT0 + Index, GL_SPECULAR, v.Array);
	}

	void Light::Attenuation(const Vector3D& v) {
		glLightf(GL_LIGHT0 + Index, GL_CONSTANT_ATTENUATION, v.x);
		glLightf(GL_LIGHT0 + Index, GL_LINEAR_ATTENUATION, v.y);
		glLightf(GL_LIGHT0 + Index, GL_QUADRATIC_ATTENUATION, v.z);
	}

	void Light::Enable(int Enable) {
		if (Enable) glEnable(GL_LIGHT0 + Index);
		else glDisable(GL_LIGHT0 + Index);
	}

	void Context::InitExtensions() {
		glActiveTexture = (PFNGLACTIVETEXTUREPROC)wglGetProcAddress("glActiveTexture");
		glAttachShader = (PFNGLATTACHSHADERPROC)wglGetProcAddress("glAttachShader");
		glBindAttribLocation = (PFNGLBINDATTRIBLOCATIONPROC)wglGetProcAddress("glBindAttribLocation");
		glBindBuffer = (PFNGLBINDBUFFERPROC)wglGetProcAddress("glBindBuffer");
		glBindBufferBase = (PFNGLBINDBUFFERBASEPROC)wglGetProcAddress("glBindBufferBase");
		glBindFramebuffer = (PFNGLBINDFRAMEBUFFERPROC)wglGetProcAddress("glBindFramebuffer");
		glBindRenderbuffer = (PFNGLBINDRENDERBUFFERPROC)wglGetProcAddress("glBindRenderbuffer");
		glBufferData = (PFNGLBUFFERDATAPROC)wglGetProcAddress("glBufferData");
		glBufferSubData = (PFNGLBUFFERSUBDATAPROC)wglGetProcAddress("glBufferSubData");
		glCheckFramebufferStatus = (PFNGLCHECKFRAMEBUFFERSTATUSPROC)wglGetProcAddress("glCheckFramebufferStatus");
		glCompileShader = (PFNGLCOMPILESHADERPROC)wglGetProcAddress("glCompileShader");
		glCreateProgram = (PFNGLCREATEPROGRAMPROC)wglGetProcAddress("glCreateProgram");
		glCreateShader = (PFNGLCREATESHADERPROC)wglGetProcAddress("glCreateShader");
		glDeleteBuffers = (PFNGLDELETEBUFFERSPROC)wglGetProcAddress("glDeleteBuffers");
		glDeleteFramebuffers = (PFNGLDELETEFRAMEBUFFERSPROC)wglGetProcAddress("glDeleteFramebuffers");
		glDeleteProgram = (PFNGLDELETEPROGRAMPROC)wglGetProcAddress("glDeleteProgram");
		glDeleteShader = (PFNGLDELETESHADERPROC)wglGetProcAddress("glDeleteShader");
		glDetachShader = (PFNGLDETACHSHADERPROC)wglGetProcAddress("glDetachShader");
		glDisableVertexAttribArray = (PFNGLDISABLEVERTEXATTRIBARRAYPROC)wglGetProcAddress("glDisableVertexAttribArray");
		glEnableVertexAttribArray = (PFNGLENABLEVERTEXATTRIBARRAYPROC)wglGetProcAddress("glEnableVertexAttribArray");
		glFramebufferRenderbuffer = (PFNGLFRAMEBUFFERRENDERBUFFERPROC)wglGetProcAddress("glFramebufferRenderbuffer");
		glFramebufferTexture = (PFNGLFRAMEBUFFERTEXTUREPROC)wglGetProcAddress("glFramebufferTexture");
		glFramebufferTexture2D = (PFNGLFRAMEBUFFERTEXTURE2DPROC)wglGetProcAddress("glFramebufferTexture2D");
		glGenBuffers = (PFNGLGENBUFFERSPROC)wglGetProcAddress("glGenBuffers");
		glGenFramebuffers = (PFNGLGENFRAMEBUFFERSPROC)wglGetProcAddress("glGenFramebuffers");
		glGetAttribLocation = (PFNGLGETATTRIBLOCATIONPROC)wglGetProcAddress("glGetAttribLocation");
		glGetBufferParameteriv = (PFNGLGETBUFFERPARAMETERIVPROC)wglGetProcAddress("glGetBufferParameteriv");
		glGetProgramInfoLog = (PFNGLGETPROGRAMINFOLOGPROC)wglGetProcAddress("glGetProgramInfoLog");
		glGetProgramiv = (PFNGLGETPROGRAMIVPROC)wglGetProcAddress("glGetProgramiv");
		glGetShaderInfoLog = (PFNGLGETSHADERINFOLOGPROC)wglGetProcAddress("glGetShaderInfoLog");
		glGetShaderiv = (PFNGLGETSHADERIVPROC)wglGetProcAddress("glGetShaderiv");
		glGetUniformBlockIndex = (PFNGLGETUNIFORMBLOCKINDEXPROC)wglGetProcAddress("glGetUniformBlockIndex");
		glGetUniformLocation = (PFNGLGETUNIFORMLOCATIONPROC)wglGetProcAddress("glGetUniformLocation");
		glLinkProgram = (PFNGLLINKPROGRAMPROC)wglGetProcAddress("glLinkProgram");
		glMapBuffer = (PFNGLMAPBUFFERPROC)wglGetProcAddress("glMapBuffer");
		glMapBufferRange = (PFNGLMAPBUFFERRANGEPROC)wglGetProcAddress("glMapBufferRange");
		glRenderbufferStorage = (PFNGLRENDERBUFFERSTORAGEPROC)wglGetProcAddress("glRenderbufferStorage");
		glShaderSource = (PFNGLSHADERSOURCEPROC)wglGetProcAddress("glShaderSource");
		glTexImage3D = (PFNGLTEXIMAGE3DPROC)wglGetProcAddress("glTexImage3D");
		glUniform1f = (PFNGLUNIFORM1FPROC)wglGetProcAddress("glUniform1f");
		glUniform1i = (PFNGLUNIFORM1IPROC)wglGetProcAddress("glUniform1i");
		glUniform2fv = (PFNGLUNIFORM2FVPROC)wglGetProcAddress("glUniform2fv");
		glUniform3fv = (PFNGLUNIFORM3FVPROC)wglGetProcAddress("glUniform3fv");
		glUniform4fv = (PFNGLUNIFORM4FVPROC)wglGetProcAddress("glUniform4fv");
		glUniformBlockBinding = (PFNGLUNIFORMBLOCKBINDINGPROC)wglGetProcAddress("glUniformBlockBinding");
		glUniformMatrix4fv = (PFNGLUNIFORMMATRIX4FVPROC)wglGetProcAddress("glUniformMatrix4fv");
		glUnmapBuffer = (PFNGLUNMAPBUFFERPROC)wglGetProcAddress("glUnmapBuffer");
		glUseProgram = (PFNGLUSEPROGRAMPROC)wglGetProcAddress("glUseProgram");
		glVertexAttribPointer = (PFNGLVERTEXATTRIBPOINTERPROC)wglGetProcAddress("glVertexAttribPointer");

		wglSwapInterval = (PFNWGLSWAPINTERVALEXTPROC)wglGetProcAddress("wglSwapIntervalEXT");
	}

}
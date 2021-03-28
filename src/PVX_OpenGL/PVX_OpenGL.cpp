#include <PVX_OpenGL.h>
#include <Windows.h>

namespace PVX::OpenGL {
	const std::function<void(GLenum)> glEnableDisable[]{
		glDisable,
		glEnable
	};

	void Context::Init(HWND hWnd, bool DepthStencil) {
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
			DepthStencil?32:0,				// 32-bit z-buffer
			DepthStencil?8:0,				// Stencil buffer
			0,								// no auxiliary buffer
			PFD_MAIN_PLANE,					// main layer
			0,								// reserved
			0, 0, 0							// layer masks ignored
		};

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

	Context::GroupLimits Context::GetComputeGroupLimits() const {
		GroupLimits ret;
		glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 0, &ret.Count.x);
		glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 1, &ret.Count.y);
		glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 2, &ret.Count.z);
		glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 0, &ret.Size.x);
		glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 1, &ret.Size.y);
		glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 2, &ret.Size.z);
		return ret;
	}

	void Context::glThread_Func1(HWND hWnd, bool DepthStencil, std::function<void(Context& gl)> Function) {
		Init(hWnd, DepthStencil);
		Function(*this);
		Running = 0;
	}
	void Context::glThread_Func2(HWND hWnd, bool DepthStencil, std::function<void(Context& gl, void*)> Function, void* Data) {
		Init(hWnd, DepthStencil);
		Function(*this, Data);
		Running = 0;
	}

	Context::~Context() {
		if (glThread.joinable()) {
			Running = false;
			glThread.join();
		}
	}

	Context::Context(HWND hWnd, bool DepthStencil) {
		Init(hWnd, DepthStencil);
	}

	Context::Context(HWND hWnd, bool DepthStencil, std::function<void(Context& gl)> Function) {
		Running = true;
		glThread = std::thread(&Context::glThread_Func1, this, hWnd, DepthStencil, Function);
	}

	Context::Context(HWND hWnd, bool DepthStencil, std::function<void(Context& gl, void*)> Function, void* Data) {
		Running = true;
		glThread = std::thread(&Context::glThread_Func2, this, hWnd, DepthStencil, Function, Data);
	}

	void Context::MakeCurrent() const {
		wglMakeCurrent(dc, rc);
	}
	void Context::UnMakeCurrent() const {
		wglMakeCurrent(0, 0);
	}
	void Context::Viewport() {
		if (CurrentViewportWidth != Width || CurrentViewportHeight != Height) {
			CurrentViewportWidth = Width;
			CurrentViewportHeight = Height;
			glViewport(0, 0, Width, Height);
		}
	}
	void Context::Viewport(int Width, int Height) {
		if (CurrentViewportWidth != Width || CurrentViewportHeight != Height) {
			CurrentViewportWidth = Width;
			CurrentViewportHeight = Height;
			glViewport(0, 0, Width, Height);
		}
	}
	void Context::Viewport(const PVX::iVector2D& Size) {
		if (CurrentViewportWidth != Size.Width || CurrentViewportHeight != Size.Height) {
			CurrentViewportWidth = Size.Width;
			CurrentViewportHeight = Size.Height;
			glViewport(0, 0, Size.Width, Size.Height);
		}
	}
	void Context::Resized() {
		RECT cl;
		GetClientRect(hWnd, &cl);
		Viewport(Width = cl.right - cl.left, Height = cl.bottom - cl.top);
	}

	void Context::Resized(int w, int h) {
		Viewport(Width = w, Height = h);
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

	void Context::Enable(GLenum What, bool enable) {
		glEnableDisable[(int)enable](What);
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
		glEnableDisable[Enable](GL_LIGHT0 + Index);
	}

	//void Light::Enable(int Enable) {
	//	if (Enable) glEnable(GL_LIGHT0 + Index);
	//	else glDisable(GL_LIGHT0 + Index);
	//}

	void Context::InitExtensions() {
		glActiveShaderProgram = (PFNGLACTIVESHADERPROGRAMPROC)wglGetProcAddress("glActiveShaderProgram");
		glActiveTexture = (PFNGLACTIVETEXTUREPROC)wglGetProcAddress("glActiveTexture");
		glAttachShader = (PFNGLATTACHSHADERPROC)wglGetProcAddress("glAttachShader");
		glBeginConditionalRender = (PFNGLBEGINCONDITIONALRENDERPROC)wglGetProcAddress("glBeginConditionalRender");
		glBeginQuery = (PFNGLBEGINQUERYPROC)wglGetProcAddress("glBeginQuery");
		glBeginQueryIndexed = (PFNGLBEGINQUERYINDEXEDPROC)wglGetProcAddress("glBeginQueryIndexed");
		glBeginTransformFeedback = (PFNGLBEGINTRANSFORMFEEDBACKPROC)wglGetProcAddress("glBeginTransformFeedback");
		glBindAttribLocation = (PFNGLBINDATTRIBLOCATIONPROC)wglGetProcAddress("glBindAttribLocation");
		glBindBuffer = (PFNGLBINDBUFFERPROC)wglGetProcAddress("glBindBuffer");
		glBindBufferBase = (PFNGLBINDBUFFERBASEPROC)wglGetProcAddress("glBindBufferBase");
		glBindBufferRange = (PFNGLBINDBUFFERRANGEPROC)wglGetProcAddress("glBindBufferRange");
		glBindBuffersBase = (PFNGLBINDBUFFERSBASEPROC)wglGetProcAddress("glBindBuffersBase");
		glBindBuffersRange = (PFNGLBINDBUFFERSRANGEPROC)wglGetProcAddress("glBindBuffersRange");
		glBindFragDataLocation = (PFNGLBINDFRAGDATALOCATIONPROC)wglGetProcAddress("glBindFragDataLocation");
		glBindFragDataLocationIndexed = (PFNGLBINDFRAGDATALOCATIONINDEXEDPROC)wglGetProcAddress("glBindFragDataLocationIndexed");
		glBindFramebuffer = (PFNGLBINDFRAMEBUFFERPROC)wglGetProcAddress("glBindFramebuffer");
		glBindImageTexture = (PFNGLBINDIMAGETEXTUREPROC)wglGetProcAddress("glBindImageTexture");
		glBindImageTextures = (PFNGLBINDIMAGETEXTURESPROC)wglGetProcAddress("glBindImageTextures");
		glBindProgramPipeline = (PFNGLBINDPROGRAMPIPELINEPROC)wglGetProcAddress("glBindProgramPipeline");
		glBindRenderbuffer = (PFNGLBINDRENDERBUFFERPROC)wglGetProcAddress("glBindRenderbuffer");
		glBindSampler = (PFNGLBINDSAMPLERPROC)wglGetProcAddress("glBindSampler");
		glBindSamplers = (PFNGLBINDSAMPLERSPROC)wglGetProcAddress("glBindSamplers");
		glBindTextures = (PFNGLBINDTEXTURESPROC)wglGetProcAddress("glBindTextures");
		glBindTextureUnit = (PFNGLBINDTEXTUREUNITPROC)wglGetProcAddress("glBindTextureUnit");
		glBindTransformFeedback = (PFNGLBINDTRANSFORMFEEDBACKPROC)wglGetProcAddress("glBindTransformFeedback");
		glBindVertexArray = (PFNGLBINDVERTEXARRAYPROC)wglGetProcAddress("glBindVertexArray");
		glBindVertexBuffer = (PFNGLBINDVERTEXBUFFERPROC)wglGetProcAddress("glBindVertexBuffer");
		glBindVertexBuffers = (PFNGLBINDVERTEXBUFFERSPROC)wglGetProcAddress("glBindVertexBuffers");
		glBlendColor = (PFNGLBLENDCOLORPROC)wglGetProcAddress("glBlendColor");
		glBlendEquation = (PFNGLBLENDEQUATIONPROC)wglGetProcAddress("glBlendEquation");
		glBlendEquationi = (PFNGLBLENDEQUATIONIPROC)wglGetProcAddress("glBlendEquationi");
		glBlendEquationSeparate = (PFNGLBLENDEQUATIONSEPARATEPROC)wglGetProcAddress("glBlendEquationSeparate");
		glBlendEquationSeparatei = (PFNGLBLENDEQUATIONSEPARATEIPROC)wglGetProcAddress("glBlendEquationSeparatei");
		glBlendFunci = (PFNGLBLENDFUNCIPROC)wglGetProcAddress("glBlendFunci");
		glBlendFuncSeparate = (PFNGLBLENDFUNCSEPARATEPROC)wglGetProcAddress("glBlendFuncSeparate");
		glBlendFuncSeparatei = (PFNGLBLENDFUNCSEPARATEIPROC)wglGetProcAddress("glBlendFuncSeparatei");
		glBlitFramebuffer = (PFNGLBLITFRAMEBUFFERPROC)wglGetProcAddress("glBlitFramebuffer");
		glBlitNamedFramebuffer = (PFNGLBLITNAMEDFRAMEBUFFERPROC)wglGetProcAddress("glBlitNamedFramebuffer");
		glBufferData = (PFNGLBUFFERDATAPROC)wglGetProcAddress("glBufferData");
		glBufferStorage = (PFNGLBUFFERSTORAGEPROC)wglGetProcAddress("glBufferStorage");
		glBufferSubData = (PFNGLBUFFERSUBDATAPROC)wglGetProcAddress("glBufferSubData");
		glCheckFramebufferStatus = (PFNGLCHECKFRAMEBUFFERSTATUSPROC)wglGetProcAddress("glCheckFramebufferStatus");
		glCheckNamedFramebufferStatus = (PFNGLCHECKNAMEDFRAMEBUFFERSTATUSPROC)wglGetProcAddress("glCheckNamedFramebufferStatus");
		glClampColor = (PFNGLCLAMPCOLORPROC)wglGetProcAddress("glClampColor");
		glClearDepthf = (PFNGLCLEARDEPTHFPROC)wglGetProcAddress("glClearDepthf");
		glClearNamedBufferData = (PFNGLCLEARNAMEDBUFFERDATAPROC)wglGetProcAddress("glClearNamedBufferData");
		glClearNamedBufferSubData = (PFNGLCLEARNAMEDBUFFERSUBDATAPROC)wglGetProcAddress("glClearNamedBufferSubData");
		glClearNamedFramebufferfi = (PFNGLCLEARNAMEDFRAMEBUFFERFIPROC)wglGetProcAddress("glClearNamedFramebufferfi");
		glClearNamedFramebufferfv = (PFNGLCLEARNAMEDFRAMEBUFFERFVPROC)wglGetProcAddress("glClearNamedFramebufferfv");
		glClearNamedFramebufferiv = (PFNGLCLEARNAMEDFRAMEBUFFERIVPROC)wglGetProcAddress("glClearNamedFramebufferiv");
		glClearNamedFramebufferuiv = (PFNGLCLEARNAMEDFRAMEBUFFERUIVPROC)wglGetProcAddress("glClearNamedFramebufferuiv");
		glClearTexImage = (PFNGLCLEARTEXIMAGEPROC)wglGetProcAddress("glClearTexImage");
		glClearTexSubImage = (PFNGLCLEARTEXSUBIMAGEPROC)wglGetProcAddress("glClearTexSubImage");
		glClientWaitSync = (PFNGLCLIENTWAITSYNCPROC)wglGetProcAddress("glClientWaitSync");
		glClipControl = (PFNGLCLIPCONTROLPROC)wglGetProcAddress("glClipControl");
		glColorMaski = (PFNGLCOLORMASKIPROC)wglGetProcAddress("glColorMaski");
		glCompileShader = (PFNGLCOMPILESHADERPROC)wglGetProcAddress("glCompileShader");
		glCompressedTexImage1D = (PFNGLCOMPRESSEDTEXIMAGE1DPROC)wglGetProcAddress("glCompressedTexImage1D");
		glCompressedTexImage2D = (PFNGLCOMPRESSEDTEXIMAGE2DPROC)wglGetProcAddress("glCompressedTexImage2D");
		glCompressedTexImage3D = (PFNGLCOMPRESSEDTEXIMAGE3DPROC)wglGetProcAddress("glCompressedTexImage3D");
		glCompressedTexSubImage1D = (PFNGLCOMPRESSEDTEXSUBIMAGE1DPROC)wglGetProcAddress("glCompressedTexSubImage1D");
		glCompressedTexSubImage2D = (PFNGLCOMPRESSEDTEXSUBIMAGE2DPROC)wglGetProcAddress("glCompressedTexSubImage2D");
		glCompressedTexSubImage3D = (PFNGLCOMPRESSEDTEXSUBIMAGE3DPROC)wglGetProcAddress("glCompressedTexSubImage3D");
		glCompressedTextureSubImage1D = (PFNGLCOMPRESSEDTEXTURESUBIMAGE1DPROC)wglGetProcAddress("glCompressedTextureSubImage1D");
		glCompressedTextureSubImage2D = (PFNGLCOMPRESSEDTEXTURESUBIMAGE2DPROC)wglGetProcAddress("glCompressedTextureSubImage2D");
		glCompressedTextureSubImage3D = (PFNGLCOMPRESSEDTEXTURESUBIMAGE3DPROC)wglGetProcAddress("glCompressedTextureSubImage3D");
		glCopyBufferSubData = (PFNGLCOPYBUFFERSUBDATAPROC)wglGetProcAddress("glCopyBufferSubData");
		glCopyImageSubData = (PFNGLCOPYIMAGESUBDATAPROC)wglGetProcAddress("glCopyImageSubData");
		glCopyNamedBufferSubData = (PFNGLCOPYNAMEDBUFFERSUBDATAPROC)wglGetProcAddress("glCopyNamedBufferSubData");
		glCopyTexSubImage3D = (PFNGLCOPYTEXSUBIMAGE3DPROC)wglGetProcAddress("glCopyTexSubImage3D");
		glCopyTextureSubImage1D = (PFNGLCOPYTEXTURESUBIMAGE1DPROC)wglGetProcAddress("glCopyTextureSubImage1D");
		glCopyTextureSubImage2D = (PFNGLCOPYTEXTURESUBIMAGE2DPROC)wglGetProcAddress("glCopyTextureSubImage2D");
		glCopyTextureSubImage3D = (PFNGLCOPYTEXTURESUBIMAGE3DPROC)wglGetProcAddress("glCopyTextureSubImage3D");
		glCreateBuffers = (PFNGLCREATEBUFFERSPROC)wglGetProcAddress("glCreateBuffers");
		glCreateFramebuffers = (PFNGLCREATEFRAMEBUFFERSPROC)wglGetProcAddress("glCreateFramebuffers");
		glCreateProgram = (PFNGLCREATEPROGRAMPROC)wglGetProcAddress("glCreateProgram");
		glCreateProgramPipelines = (PFNGLCREATEPROGRAMPIPELINESPROC)wglGetProcAddress("glCreateProgramPipelines");
		glCreateQueries = (PFNGLCREATEQUERIESPROC)wglGetProcAddress("glCreateQueries");
		glCreateRenderbuffers = (PFNGLCREATERENDERBUFFERSPROC)wglGetProcAddress("glCreateRenderbuffers");
		glCreateSamplers = (PFNGLCREATESAMPLERSPROC)wglGetProcAddress("glCreateSamplers");
		glCreateShader = (PFNGLCREATESHADERPROC)wglGetProcAddress("glCreateShader");
		glCreateShaderProgramv = (PFNGLCREATESHADERPROGRAMVPROC)wglGetProcAddress("glCreateShaderProgramv");
		glCreateTextures = (PFNGLCREATETEXTURESPROC)wglGetProcAddress("glCreateTextures");
		glCreateTransformFeedbacks = (PFNGLCREATETRANSFORMFEEDBACKSPROC)wglGetProcAddress("glCreateTransformFeedbacks");
		glCreateVertexArrays = (PFNGLCREATEVERTEXARRAYSPROC)wglGetProcAddress("glCreateVertexArrays");
		glDebugMessageCallback = (PFNGLDEBUGMESSAGECALLBACKPROC)wglGetProcAddress("glDebugMessageCallback");
		glDebugMessageControl = (PFNGLDEBUGMESSAGECONTROLPROC)wglGetProcAddress("glDebugMessageControl");
		glDebugMessageInsert = (PFNGLDEBUGMESSAGEINSERTPROC)wglGetProcAddress("glDebugMessageInsert");
		glDeleteBuffers = (PFNGLDELETEBUFFERSPROC)wglGetProcAddress("glDeleteBuffers");
		glDeleteFramebuffers = (PFNGLDELETEFRAMEBUFFERSPROC)wglGetProcAddress("glDeleteFramebuffers");
		glDeleteProgram = (PFNGLDELETEPROGRAMPROC)wglGetProcAddress("glDeleteProgram");
		glDeleteProgramPipelines = (PFNGLDELETEPROGRAMPIPELINESPROC)wglGetProcAddress("glDeleteProgramPipelines");
		glDeleteQueries = (PFNGLDELETEQUERIESPROC)wglGetProcAddress("glDeleteQueries");
		glDeleteRenderbuffers = (PFNGLDELETERENDERBUFFERSPROC)wglGetProcAddress("glDeleteRenderbuffers");
		glDeleteSamplers = (PFNGLDELETESAMPLERSPROC)wglGetProcAddress("glDeleteSamplers");
		glDeleteShader = (PFNGLDELETESHADERPROC)wglGetProcAddress("glDeleteShader");
		glDeleteSync = (PFNGLDELETESYNCPROC)wglGetProcAddress("glDeleteSync");
		glDeleteTransformFeedbacks = (PFNGLDELETETRANSFORMFEEDBACKSPROC)wglGetProcAddress("glDeleteTransformFeedbacks");
		glDeleteVertexArrays = (PFNGLDELETEVERTEXARRAYSPROC)wglGetProcAddress("glDeleteVertexArrays");
		glDepthRangeArrayv = (PFNGLDEPTHRANGEARRAYVPROC)wglGetProcAddress("glDepthRangeArrayv");
		glDepthRangef = (PFNGLDEPTHRANGEFPROC)wglGetProcAddress("glDepthRangef");
		glDepthRangeIndexed = (PFNGLDEPTHRANGEINDEXEDPROC)wglGetProcAddress("glDepthRangeIndexed");
		glDetachShader = (PFNGLDETACHSHADERPROC)wglGetProcAddress("glDetachShader");
		glDisablei = (PFNGLDISABLEIPROC)wglGetProcAddress("glDisablei");
		glDisableVertexArrayAttrib = (PFNGLDISABLEVERTEXARRAYATTRIBPROC)wglGetProcAddress("glDisableVertexArrayAttrib");
		glDisableVertexAttribArray = (PFNGLDISABLEVERTEXATTRIBARRAYPROC)wglGetProcAddress("glDisableVertexAttribArray");
		glDispatchCompute = (PFNGLDISPATCHCOMPUTEPROC)wglGetProcAddress("glDispatchCompute");
		glDispatchComputeIndirect = (PFNGLDISPATCHCOMPUTEINDIRECTPROC)wglGetProcAddress("glDispatchComputeIndirect");
		glDrawArraysIndirect = (PFNGLDRAWARRAYSINDIRECTPROC)wglGetProcAddress("glDrawArraysIndirect");
		glDrawArraysInstanced = (PFNGLDRAWARRAYSINSTANCEDPROC)wglGetProcAddress("glDrawArraysInstanced");
		glDrawArraysInstancedBaseInstance = (PFNGLDRAWARRAYSINSTANCEDBASEINSTANCEPROC)wglGetProcAddress("glDrawArraysInstancedBaseInstance");
		glDrawBuffers = (PFNGLDRAWBUFFERSPROC)wglGetProcAddress("glDrawBuffers");
		glDrawElementsBaseVertex = (PFNGLDRAWELEMENTSBASEVERTEXPROC)wglGetProcAddress("glDrawElementsBaseVertex");
		glDrawElementsIndirect = (PFNGLDRAWELEMENTSINDIRECTPROC)wglGetProcAddress("glDrawElementsIndirect");
		glDrawElementsInstanced = (PFNGLDRAWELEMENTSINSTANCEDPROC)wglGetProcAddress("glDrawElementsInstanced");
		glDrawElementsInstancedBaseInstance = (PFNGLDRAWELEMENTSINSTANCEDBASEINSTANCEPROC)wglGetProcAddress("glDrawElementsInstancedBaseInstance");
		glDrawElementsInstancedBaseVertex = (PFNGLDRAWELEMENTSINSTANCEDBASEVERTEXPROC)wglGetProcAddress("glDrawElementsInstancedBaseVertex");
		glDrawElementsInstancedBaseVertexBaseInstance = (PFNGLDRAWELEMENTSINSTANCEDBASEVERTEXBASEINSTANCEPROC)wglGetProcAddress("glDrawElementsInstancedBaseVertexBaseInstance");
		glDrawRangeElements = (PFNGLDRAWRANGEELEMENTSPROC)wglGetProcAddress("glDrawRangeElements");
		glDrawRangeElementsBaseVertex = (PFNGLDRAWRANGEELEMENTSBASEVERTEXPROC)wglGetProcAddress("glDrawRangeElementsBaseVertex");
		glDrawTransformFeedback = (PFNGLDRAWTRANSFORMFEEDBACKPROC)wglGetProcAddress("glDrawTransformFeedback");
		glDrawTransformFeedbackInstanced = (PFNGLDRAWTRANSFORMFEEDBACKINSTANCEDPROC)wglGetProcAddress("glDrawTransformFeedbackInstanced");
		glDrawTransformFeedbackStream = (PFNGLDRAWTRANSFORMFEEDBACKSTREAMPROC)wglGetProcAddress("glDrawTransformFeedbackStream");
		glDrawTransformFeedbackStreamInstanced = (PFNGLDRAWTRANSFORMFEEDBACKSTREAMINSTANCEDPROC)wglGetProcAddress("glDrawTransformFeedbackStreamInstanced");
		glEnablei = (PFNGLENABLEIPROC)wglGetProcAddress("glEnablei");
		glEnableVertexArrayAttrib = (PFNGLENABLEVERTEXARRAYATTRIBPROC)wglGetProcAddress("glEnableVertexArrayAttrib");
		glEnableVertexAttribArray = (PFNGLENABLEVERTEXATTRIBARRAYPROC)wglGetProcAddress("glEnableVertexAttribArray");
		glEndConditionalRender = (PFNGLENDCONDITIONALRENDERPROC)wglGetProcAddress("glEndConditionalRender");
		glEndQuery = (PFNGLENDQUERYPROC)wglGetProcAddress("glEndQuery");
		glEndQueryIndexed = (PFNGLENDQUERYINDEXEDPROC)wglGetProcAddress("glEndQueryIndexed");
		glEndTransformFeedback = (PFNGLENDTRANSFORMFEEDBACKPROC)wglGetProcAddress("glEndTransformFeedback");
		glFenceSync = (PFNGLFENCESYNCPROC)wglGetProcAddress("glFenceSync");
		glFlushMappedBufferRange = (PFNGLFLUSHMAPPEDBUFFERRANGEPROC)wglGetProcAddress("glFlushMappedBufferRange");
		glFlushMappedNamedBufferRange = (PFNGLFLUSHMAPPEDNAMEDBUFFERRANGEPROC)wglGetProcAddress("glFlushMappedNamedBufferRange");
		glFramebufferParameteri = (PFNGLFRAMEBUFFERPARAMETERIPROC)wglGetProcAddress("glFramebufferParameteri");
		glFramebufferRenderbuffer = (PFNGLFRAMEBUFFERRENDERBUFFERPROC)wglGetProcAddress("glFramebufferRenderbuffer");
		glFramebufferTexture = (PFNGLFRAMEBUFFERTEXTUREPROC)wglGetProcAddress("glFramebufferTexture");
		glFramebufferTexture1D = (PFNGLFRAMEBUFFERTEXTURE1DPROC)wglGetProcAddress("glFramebufferTexture1D");
		glFramebufferTexture2D = (PFNGLFRAMEBUFFERTEXTURE2DPROC)wglGetProcAddress("glFramebufferTexture2D");
		glFramebufferTexture3D = (PFNGLFRAMEBUFFERTEXTURE3DPROC)wglGetProcAddress("glFramebufferTexture3D");
		glFramebufferTextureLayer = (PFNGLFRAMEBUFFERTEXTURELAYERPROC)wglGetProcAddress("glFramebufferTextureLayer");
		glGenBuffers = (PFNGLGENBUFFERSPROC)wglGetProcAddress("glGenBuffers");
		glGenerateMipmap = (PFNGLGENERATEMIPMAPPROC)wglGetProcAddress("glGenerateMipmap");
		glGenerateTextureMipmap = (PFNGLGENERATETEXTUREMIPMAPPROC)wglGetProcAddress("glGenerateTextureMipmap");
		glGenFramebuffers = (PFNGLGENFRAMEBUFFERSPROC)wglGetProcAddress("glGenFramebuffers");
		glGenProgramPipelines = (PFNGLGENPROGRAMPIPELINESPROC)wglGetProcAddress("glGenProgramPipelines");
		glGenQueries = (PFNGLGENQUERIESPROC)wglGetProcAddress("glGenQueries");
		glGenRenderbuffers = (PFNGLGENRENDERBUFFERSPROC)wglGetProcAddress("glGenRenderbuffers");
		glGenSamplers = (PFNGLGENSAMPLERSPROC)wglGetProcAddress("glGenSamplers");
		glGenTransformFeedbacks = (PFNGLGENTRANSFORMFEEDBACKSPROC)wglGetProcAddress("glGenTransformFeedbacks");
		glGenVertexArrays = (PFNGLGENVERTEXARRAYSPROC)wglGetProcAddress("glGenVertexArrays");
		glGetActiveAtomicCounterBufferiv = (PFNGLGETACTIVEATOMICCOUNTERBUFFERIVPROC)wglGetProcAddress("glGetActiveAtomicCounterBufferiv");
		glGetActiveAttrib = (PFNGLGETACTIVEATTRIBPROC)wglGetProcAddress("glGetActiveAttrib");
		glGetActiveSubroutineName = (PFNGLGETACTIVESUBROUTINENAMEPROC)wglGetProcAddress("glGetActiveSubroutineName");
		glGetActiveSubroutineUniformiv = (PFNGLGETACTIVESUBROUTINEUNIFORMIVPROC)wglGetProcAddress("glGetActiveSubroutineUniformiv");
		glGetActiveSubroutineUniformName = (PFNGLGETACTIVESUBROUTINEUNIFORMNAMEPROC)wglGetProcAddress("glGetActiveSubroutineUniformName");
		glGetActiveUniform = (PFNGLGETACTIVEUNIFORMPROC)wglGetProcAddress("glGetActiveUniform");
		glGetActiveUniformBlockiv = (PFNGLGETACTIVEUNIFORMBLOCKIVPROC)wglGetProcAddress("glGetActiveUniformBlockiv");
		glGetActiveUniformBlockName = (PFNGLGETACTIVEUNIFORMBLOCKNAMEPROC)wglGetProcAddress("glGetActiveUniformBlockName");
		glGetActiveUniformName = (PFNGLGETACTIVEUNIFORMNAMEPROC)wglGetProcAddress("glGetActiveUniformName");
		glGetActiveUniformsiv = (PFNGLGETACTIVEUNIFORMSIVPROC)wglGetProcAddress("glGetActiveUniformsiv");
		glGetAttachedShaders = (PFNGLGETATTACHEDSHADERSPROC)wglGetProcAddress("glGetAttachedShaders");
		glGetAttribLocation = (PFNGLGETATTRIBLOCATIONPROC)wglGetProcAddress("glGetAttribLocation");
		glGetBooleani_v = (PFNGLGETBOOLEANI_VPROC)wglGetProcAddress("glGetBooleani_v");
		glGetBufferParameteri64v = (PFNGLGETBUFFERPARAMETERI64VPROC)wglGetProcAddress("glGetBufferParameteri64v");
		glGetBufferParameteriv = (PFNGLGETBUFFERPARAMETERIVPROC)wglGetProcAddress("glGetBufferParameteriv");
		glGetBufferPointerv = (PFNGLGETBUFFERPOINTERVPROC)wglGetProcAddress("glGetBufferPointerv");
		glGetBufferSubData = (PFNGLGETBUFFERSUBDATAPROC)wglGetProcAddress("glGetBufferSubData");
		glGetCompressedTexImage = (PFNGLGETCOMPRESSEDTEXIMAGEPROC)wglGetProcAddress("glGetCompressedTexImage");
		glGetCompressedTextureImage = (PFNGLGETCOMPRESSEDTEXTUREIMAGEPROC)wglGetProcAddress("glGetCompressedTextureImage");
		glGetCompressedTextureSubImage = (PFNGLGETCOMPRESSEDTEXTURESUBIMAGEPROC)wglGetProcAddress("glGetCompressedTextureSubImage");
		glGetDebugMessageLog = (PFNGLGETDEBUGMESSAGELOGPROC)wglGetProcAddress("glGetDebugMessageLog");
		glGetDoublei_v = (PFNGLGETDOUBLEI_VPROC)wglGetProcAddress("glGetDoublei_v");
		glGetFloati_v = (PFNGLGETFLOATI_VPROC)wglGetProcAddress("glGetFloati_v");
		glGetFragDataIndex = (PFNGLGETFRAGDATAINDEXPROC)wglGetProcAddress("glGetFragDataIndex");
		glGetFragDataLocation = (PFNGLGETFRAGDATALOCATIONPROC)wglGetProcAddress("glGetFragDataLocation");
		glGetFramebufferAttachmentParameteriv = (PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVPROC)wglGetProcAddress("glGetFramebufferAttachmentParameteriv");
		glGetFramebufferParameteriv = (PFNGLGETFRAMEBUFFERPARAMETERIVPROC)wglGetProcAddress("glGetFramebufferParameteriv");
		glGetGraphicsResetStatus = (PFNGLGETGRAPHICSRESETSTATUSPROC)wglGetProcAddress("glGetGraphicsResetStatus");
		glGetInteger64i_v = (PFNGLGETINTEGER64I_VPROC)wglGetProcAddress("glGetInteger64i_v");
		glGetInteger64v = (PFNGLGETINTEGER64VPROC)wglGetProcAddress("glGetInteger64v");
		glGetIntegeri_v = (PFNGLGETINTEGERI_VPROC)wglGetProcAddress("glGetIntegeri_v");
		glGetInternalformati64v = (PFNGLGETINTERNALFORMATI64VPROC)wglGetProcAddress("glGetInternalformati64v");
		glGetInternalformativ = (PFNGLGETINTERNALFORMATIVPROC)wglGetProcAddress("glGetInternalformativ");
		glGetMultisamplefv = (PFNGLGETMULTISAMPLEFVPROC)wglGetProcAddress("glGetMultisamplefv");
		glGetNamedBufferParameteri64v = (PFNGLGETNAMEDBUFFERPARAMETERI64VPROC)wglGetProcAddress("glGetNamedBufferParameteri64v");
		glGetNamedBufferParameteriv = (PFNGLGETNAMEDBUFFERPARAMETERIVPROC)wglGetProcAddress("glGetNamedBufferParameteriv");
		glGetNamedBufferPointerv = (PFNGLGETNAMEDBUFFERPOINTERVPROC)wglGetProcAddress("glGetNamedBufferPointerv");
		glGetNamedBufferSubData = (PFNGLGETNAMEDBUFFERSUBDATAPROC)wglGetProcAddress("glGetNamedBufferSubData");
		glGetNamedFramebufferAttachmentParameteriv = (PFNGLGETNAMEDFRAMEBUFFERATTACHMENTPARAMETERIVPROC)wglGetProcAddress("glGetNamedFramebufferAttachmentParameteriv");
		glGetNamedFramebufferParameteriv = (PFNGLGETNAMEDFRAMEBUFFERPARAMETERIVPROC)wglGetProcAddress("glGetNamedFramebufferParameteriv");
		glGetNamedRenderbufferParameteriv = (PFNGLGETNAMEDRENDERBUFFERPARAMETERIVPROC)wglGetProcAddress("glGetNamedRenderbufferParameteriv");
		glGetnCompressedTexImage = (PFNGLGETNCOMPRESSEDTEXIMAGEPROC)wglGetProcAddress("glGetnCompressedTexImage");
		glGetnTexImage = (PFNGLGETNTEXIMAGEPROC)wglGetProcAddress("glGetnTexImage");
		glGetnUniformdv = (PFNGLGETNUNIFORMDVPROC)wglGetProcAddress("glGetnUniformdv");
		glGetnUniformfv = (PFNGLGETNUNIFORMFVPROC)wglGetProcAddress("glGetnUniformfv");
		glGetnUniformiv = (PFNGLGETNUNIFORMIVPROC)wglGetProcAddress("glGetnUniformiv");
		glGetnUniformuiv = (PFNGLGETNUNIFORMUIVPROC)wglGetProcAddress("glGetnUniformuiv");
		glGetObjectLabel = (PFNGLGETOBJECTLABELPROC)wglGetProcAddress("glGetObjectLabel");
		glGetObjectPtrLabel = (PFNGLGETOBJECTPTRLABELPROC)wglGetProcAddress("glGetObjectPtrLabel");
		glGetProgramBinary = (PFNGLGETPROGRAMBINARYPROC)wglGetProcAddress("glGetProgramBinary");
		glGetProgramInfoLog = (PFNGLGETPROGRAMINFOLOGPROC)wglGetProcAddress("glGetProgramInfoLog");
		glGetProgramInterfaceiv = (PFNGLGETPROGRAMINTERFACEIVPROC)wglGetProcAddress("glGetProgramInterfaceiv");
		glGetProgramiv = (PFNGLGETPROGRAMIVPROC)wglGetProcAddress("glGetProgramiv");
		glGetProgramPipelineInfoLog = (PFNGLGETPROGRAMPIPELINEINFOLOGPROC)wglGetProcAddress("glGetProgramPipelineInfoLog");
		glGetProgramPipelineiv = (PFNGLGETPROGRAMPIPELINEIVPROC)wglGetProcAddress("glGetProgramPipelineiv");
		glGetProgramResourceIndex = (PFNGLGETPROGRAMRESOURCEINDEXPROC)wglGetProcAddress("glGetProgramResourceIndex");
		glGetProgramResourceiv = (PFNGLGETPROGRAMRESOURCEIVPROC)wglGetProcAddress("glGetProgramResourceiv");
		glGetProgramResourceLocation = (PFNGLGETPROGRAMRESOURCELOCATIONPROC)wglGetProcAddress("glGetProgramResourceLocation");
		glGetProgramResourceLocationIndex = (PFNGLGETPROGRAMRESOURCELOCATIONINDEXPROC)wglGetProcAddress("glGetProgramResourceLocationIndex");
		glGetProgramResourceName = (PFNGLGETPROGRAMRESOURCENAMEPROC)wglGetProcAddress("glGetProgramResourceName");
		glGetProgramStageiv = (PFNGLGETPROGRAMSTAGEIVPROC)wglGetProcAddress("glGetProgramStageiv");
		glGetQueryBufferObjecti64v = (PFNGLGETQUERYBUFFEROBJECTI64VPROC)wglGetProcAddress("glGetQueryBufferObjecti64v");
		glGetQueryBufferObjectiv = (PFNGLGETQUERYBUFFEROBJECTIVPROC)wglGetProcAddress("glGetQueryBufferObjectiv");
		glGetQueryBufferObjectui64v = (PFNGLGETQUERYBUFFEROBJECTUI64VPROC)wglGetProcAddress("glGetQueryBufferObjectui64v");
		glGetQueryBufferObjectuiv = (PFNGLGETQUERYBUFFEROBJECTUIVPROC)wglGetProcAddress("glGetQueryBufferObjectuiv");
		glGetQueryIndexediv = (PFNGLGETQUERYINDEXEDIVPROC)wglGetProcAddress("glGetQueryIndexediv");
		glGetQueryiv = (PFNGLGETQUERYIVPROC)wglGetProcAddress("glGetQueryiv");
		glGetQueryObjecti64v = (PFNGLGETQUERYOBJECTI64VPROC)wglGetProcAddress("glGetQueryObjecti64v");
		glGetQueryObjectiv = (PFNGLGETQUERYOBJECTIVPROC)wglGetProcAddress("glGetQueryObjectiv");
		glGetQueryObjectui64v = (PFNGLGETQUERYOBJECTUI64VPROC)wglGetProcAddress("glGetQueryObjectui64v");
		glGetQueryObjectuiv = (PFNGLGETQUERYOBJECTUIVPROC)wglGetProcAddress("glGetQueryObjectuiv");
		glGetRenderbufferParameteriv = (PFNGLGETRENDERBUFFERPARAMETERIVPROC)wglGetProcAddress("glGetRenderbufferParameteriv");
		glGetSamplerParameterfv = (PFNGLGETSAMPLERPARAMETERFVPROC)wglGetProcAddress("glGetSamplerParameterfv");
		glGetSamplerParameterIiv = (PFNGLGETSAMPLERPARAMETERIIVPROC)wglGetProcAddress("glGetSamplerParameterIiv");
		glGetSamplerParameterIuiv = (PFNGLGETSAMPLERPARAMETERIUIVPROC)wglGetProcAddress("glGetSamplerParameterIuiv");
		glGetSamplerParameteriv = (PFNGLGETSAMPLERPARAMETERIVPROC)wglGetProcAddress("glGetSamplerParameteriv");
		glGetShaderInfoLog = (PFNGLGETSHADERINFOLOGPROC)wglGetProcAddress("glGetShaderInfoLog");
		glGetShaderiv = (PFNGLGETSHADERIVPROC)wglGetProcAddress("glGetShaderiv");
		glGetShaderPrecisionFormat = (PFNGLGETSHADERPRECISIONFORMATPROC)wglGetProcAddress("glGetShaderPrecisionFormat");
		glGetShaderSource = (PFNGLGETSHADERSOURCEPROC)wglGetProcAddress("glGetShaderSource");
		glGetStringi = (PFNGLGETSTRINGIPROC)wglGetProcAddress("glGetStringi");
		glGetSubroutineIndex = (PFNGLGETSUBROUTINEINDEXPROC)wglGetProcAddress("glGetSubroutineIndex");
		glGetSubroutineUniformLocation = (PFNGLGETSUBROUTINEUNIFORMLOCATIONPROC)wglGetProcAddress("glGetSubroutineUniformLocation");
		glGetSynciv = (PFNGLGETSYNCIVPROC)wglGetProcAddress("glGetSynciv");
		glGetTexParameterIiv = (PFNGLGETTEXPARAMETERIIVPROC)wglGetProcAddress("glGetTexParameterIiv");
		glGetTexParameterIuiv = (PFNGLGETTEXPARAMETERIUIVPROC)wglGetProcAddress("glGetTexParameterIuiv");
		glGetTextureImage = (PFNGLGETTEXTUREIMAGEPROC)wglGetProcAddress("glGetTextureImage");
		glGetTextureLevelParameterfv = (PFNGLGETTEXTURELEVELPARAMETERFVPROC)wglGetProcAddress("glGetTextureLevelParameterfv");
		glGetTextureLevelParameteriv = (PFNGLGETTEXTURELEVELPARAMETERIVPROC)wglGetProcAddress("glGetTextureLevelParameteriv");
		glGetTextureParameterfv = (PFNGLGETTEXTUREPARAMETERFVPROC)wglGetProcAddress("glGetTextureParameterfv");
		glGetTextureParameterIiv = (PFNGLGETTEXTUREPARAMETERIIVPROC)wglGetProcAddress("glGetTextureParameterIiv");
		glGetTextureParameterIuiv = (PFNGLGETTEXTUREPARAMETERIUIVPROC)wglGetProcAddress("glGetTextureParameterIuiv");
		glGetTextureParameteriv = (PFNGLGETTEXTUREPARAMETERIVPROC)wglGetProcAddress("glGetTextureParameteriv");
		glGetTextureSubImage = (PFNGLGETTEXTURESUBIMAGEPROC)wglGetProcAddress("glGetTextureSubImage");
		glGetTransformFeedbacki_v = (PFNGLGETTRANSFORMFEEDBACKI_VPROC)wglGetProcAddress("glGetTransformFeedbacki_v");
		glGetTransformFeedbacki64_v = (PFNGLGETTRANSFORMFEEDBACKI64_VPROC)wglGetProcAddress("glGetTransformFeedbacki64_v");
		glGetTransformFeedbackiv = (PFNGLGETTRANSFORMFEEDBACKIVPROC)wglGetProcAddress("glGetTransformFeedbackiv");
		glGetTransformFeedbackVarying = (PFNGLGETTRANSFORMFEEDBACKVARYINGPROC)wglGetProcAddress("glGetTransformFeedbackVarying");
		glGetUniformBlockIndex = (PFNGLGETUNIFORMBLOCKINDEXPROC)wglGetProcAddress("glGetUniformBlockIndex");
		glGetUniformdv = (PFNGLGETUNIFORMDVPROC)wglGetProcAddress("glGetUniformdv");
		glGetUniformfv = (PFNGLGETUNIFORMFVPROC)wglGetProcAddress("glGetUniformfv");
		glGetUniformIndices = (PFNGLGETUNIFORMINDICESPROC)wglGetProcAddress("glGetUniformIndices");
		glGetUniformiv = (PFNGLGETUNIFORMIVPROC)wglGetProcAddress("glGetUniformiv");
		glGetUniformLocation = (PFNGLGETUNIFORMLOCATIONPROC)wglGetProcAddress("glGetUniformLocation");
		glGetUniformSubroutineuiv = (PFNGLGETUNIFORMSUBROUTINEUIVPROC)wglGetProcAddress("glGetUniformSubroutineuiv");
		glGetUniformuiv = (PFNGLGETUNIFORMUIVPROC)wglGetProcAddress("glGetUniformuiv");
		glGetVertexArrayIndexed64iv = (PFNGLGETVERTEXARRAYINDEXED64IVPROC)wglGetProcAddress("glGetVertexArrayIndexed64iv");
		glGetVertexArrayIndexediv = (PFNGLGETVERTEXARRAYINDEXEDIVPROC)wglGetProcAddress("glGetVertexArrayIndexediv");
		glGetVertexArrayiv = (PFNGLGETVERTEXARRAYIVPROC)wglGetProcAddress("glGetVertexArrayiv");
		glGetVertexAttribdv = (PFNGLGETVERTEXATTRIBDVPROC)wglGetProcAddress("glGetVertexAttribdv");
		glGetVertexAttribfv = (PFNGLGETVERTEXATTRIBFVPROC)wglGetProcAddress("glGetVertexAttribfv");
		glGetVertexAttribIiv = (PFNGLGETVERTEXATTRIBIIVPROC)wglGetProcAddress("glGetVertexAttribIiv");
		glGetVertexAttribIuiv = (PFNGLGETVERTEXATTRIBIUIVPROC)wglGetProcAddress("glGetVertexAttribIuiv");
		glGetVertexAttribiv = (PFNGLGETVERTEXATTRIBIVPROC)wglGetProcAddress("glGetVertexAttribiv");
		glGetVertexAttribLdv = (PFNGLGETVERTEXATTRIBLDVPROC)wglGetProcAddress("glGetVertexAttribLdv");
		glGetVertexAttribPointerv = (PFNGLGETVERTEXATTRIBPOINTERVPROC)wglGetProcAddress("glGetVertexAttribPointerv");
		glInvalidateBufferData = (PFNGLINVALIDATEBUFFERDATAPROC)wglGetProcAddress("glInvalidateBufferData");
		glInvalidateBufferSubData = (PFNGLINVALIDATEBUFFERSUBDATAPROC)wglGetProcAddress("glInvalidateBufferSubData");
		glInvalidateFramebuffer = (PFNGLINVALIDATEFRAMEBUFFERPROC)wglGetProcAddress("glInvalidateFramebuffer");
		glInvalidateNamedFramebufferData = (PFNGLINVALIDATENAMEDFRAMEBUFFERDATAPROC)wglGetProcAddress("glInvalidateNamedFramebufferData");
		glInvalidateNamedFramebufferSubData = (PFNGLINVALIDATENAMEDFRAMEBUFFERSUBDATAPROC)wglGetProcAddress("glInvalidateNamedFramebufferSubData");
		glInvalidateSubFramebuffer = (PFNGLINVALIDATESUBFRAMEBUFFERPROC)wglGetProcAddress("glInvalidateSubFramebuffer");
		glInvalidateTexImage = (PFNGLINVALIDATETEXIMAGEPROC)wglGetProcAddress("glInvalidateTexImage");
		glInvalidateTexSubImage = (PFNGLINVALIDATETEXSUBIMAGEPROC)wglGetProcAddress("glInvalidateTexSubImage");
		glIsBuffer = (PFNGLISBUFFERPROC)wglGetProcAddress("glIsBuffer");
		glIsEnabledi = (PFNGLISENABLEDIPROC)wglGetProcAddress("glIsEnabledi");
		glIsFramebuffer = (PFNGLISFRAMEBUFFERPROC)wglGetProcAddress("glIsFramebuffer");
		glIsProgram = (PFNGLISPROGRAMPROC)wglGetProcAddress("glIsProgram");
		glIsProgramPipeline = (PFNGLISPROGRAMPIPELINEPROC)wglGetProcAddress("glIsProgramPipeline");
		glIsQuery = (PFNGLISQUERYPROC)wglGetProcAddress("glIsQuery");
		glIsRenderbuffer = (PFNGLISRENDERBUFFERPROC)wglGetProcAddress("glIsRenderbuffer");
		glIsSampler = (PFNGLISSAMPLERPROC)wglGetProcAddress("glIsSampler");
		glIsShader = (PFNGLISSHADERPROC)wglGetProcAddress("glIsShader");
		glIsSync = (PFNGLISSYNCPROC)wglGetProcAddress("glIsSync");
		glIsTransformFeedback = (PFNGLISTRANSFORMFEEDBACKPROC)wglGetProcAddress("glIsTransformFeedback");
		glIsVertexArray = (PFNGLISVERTEXARRAYPROC)wglGetProcAddress("glIsVertexArray");
		glLinkProgram = (PFNGLLINKPROGRAMPROC)wglGetProcAddress("glLinkProgram");
		glMapBuffer = (PFNGLMAPBUFFERPROC)wglGetProcAddress("glMapBuffer");
		glMapBufferRange = (PFNGLMAPBUFFERRANGEPROC)wglGetProcAddress("glMapBufferRange");
		glMapNamedBuffer = (PFNGLMAPNAMEDBUFFERPROC)wglGetProcAddress("glMapNamedBuffer");
		glMapNamedBufferRange = (PFNGLMAPNAMEDBUFFERRANGEPROC)wglGetProcAddress("glMapNamedBufferRange");
		glMemoryBarrier = (PFNGLMEMORYBARRIERPROC)wglGetProcAddress("glMemoryBarrier");
		glMemoryBarrierByRegion = (PFNGLMEMORYBARRIERBYREGIONPROC)wglGetProcAddress("glMemoryBarrierByRegion");
		glMinSampleShading = (PFNGLMINSAMPLESHADINGPROC)wglGetProcAddress("glMinSampleShading");
		glMultiDrawArrays = (PFNGLMULTIDRAWARRAYSPROC)wglGetProcAddress("glMultiDrawArrays");
		glMultiDrawArraysIndirect = (PFNGLMULTIDRAWARRAYSINDIRECTPROC)wglGetProcAddress("glMultiDrawArraysIndirect");
		glMultiDrawElements = (PFNGLMULTIDRAWELEMENTSPROC)wglGetProcAddress("glMultiDrawElements");
		glMultiDrawElementsBaseVertex = (PFNGLMULTIDRAWELEMENTSBASEVERTEXPROC)wglGetProcAddress("glMultiDrawElementsBaseVertex");
		glMultiDrawElementsIndirect = (PFNGLMULTIDRAWELEMENTSINDIRECTPROC)wglGetProcAddress("glMultiDrawElementsIndirect");
		glNamedBufferData = (PFNGLNAMEDBUFFERDATAPROC)wglGetProcAddress("glNamedBufferData");
		glNamedBufferPageCommitmentEXT = (PFNGLNAMEDBUFFERPAGECOMMITMENTEXTPROC)wglGetProcAddress("glNamedBufferPageCommitmentEXT");
		glNamedBufferStorage = (PFNGLNAMEDBUFFERSTORAGEPROC)wglGetProcAddress("glNamedBufferStorage");
		glNamedBufferSubData = (PFNGLNAMEDBUFFERSUBDATAPROC)wglGetProcAddress("glNamedBufferSubData");
		glNamedFramebufferDrawBuffer = (PFNGLNAMEDFRAMEBUFFERDRAWBUFFERPROC)wglGetProcAddress("glNamedFramebufferDrawBuffer");
		glNamedFramebufferDrawBuffers = (PFNGLNAMEDFRAMEBUFFERDRAWBUFFERSPROC)wglGetProcAddress("glNamedFramebufferDrawBuffers");
		glNamedFramebufferParameteri = (PFNGLNAMEDFRAMEBUFFERPARAMETERIPROC)wglGetProcAddress("glNamedFramebufferParameteri");
		glNamedFramebufferReadBuffer = (PFNGLNAMEDFRAMEBUFFERREADBUFFERPROC)wglGetProcAddress("glNamedFramebufferReadBuffer");
		glNamedFramebufferRenderbuffer = (PFNGLNAMEDFRAMEBUFFERRENDERBUFFERPROC)wglGetProcAddress("glNamedFramebufferRenderbuffer");
		glNamedFramebufferTexture = (PFNGLNAMEDFRAMEBUFFERTEXTUREPROC)wglGetProcAddress("glNamedFramebufferTexture");
		glNamedFramebufferTextureLayer = (PFNGLNAMEDFRAMEBUFFERTEXTURELAYERPROC)wglGetProcAddress("glNamedFramebufferTextureLayer");
		glNamedRenderbufferStorage = (PFNGLNAMEDRENDERBUFFERSTORAGEPROC)wglGetProcAddress("glNamedRenderbufferStorage");
		glNamedRenderbufferStorageMultisample = (PFNGLNAMEDRENDERBUFFERSTORAGEMULTISAMPLEPROC)wglGetProcAddress("glNamedRenderbufferStorageMultisample");
		glObjectLabel = (PFNGLOBJECTLABELPROC)wglGetProcAddress("glObjectLabel");
		glObjectPtrLabel = (PFNGLOBJECTPTRLABELPROC)wglGetProcAddress("glObjectPtrLabel");
		glPatchParameterfv = (PFNGLPATCHPARAMETERFVPROC)wglGetProcAddress("glPatchParameterfv");
		glPatchParameteri = (PFNGLPATCHPARAMETERIPROC)wglGetProcAddress("glPatchParameteri");
		glPauseTransformFeedback = (PFNGLPAUSETRANSFORMFEEDBACKPROC)wglGetProcAddress("glPauseTransformFeedback");
		glPointParameterf = (PFNGLPOINTPARAMETERFPROC)wglGetProcAddress("glPointParameterf");
		glPointParameterfv = (PFNGLPOINTPARAMETERFVPROC)wglGetProcAddress("glPointParameterfv");
		glPointParameteri = (PFNGLPOINTPARAMETERIPROC)wglGetProcAddress("glPointParameteri");
		glPointParameteriv = (PFNGLPOINTPARAMETERIVPROC)wglGetProcAddress("glPointParameteriv");
		glPopDebugGroup = (PFNGLPOPDEBUGGROUPPROC)wglGetProcAddress("glPopDebugGroup");
		glPrimitiveRestartIndex = (PFNGLPRIMITIVERESTARTINDEXPROC)wglGetProcAddress("glPrimitiveRestartIndex");
		glProgramBinary = (PFNGLPROGRAMBINARYPROC)wglGetProcAddress("glProgramBinary");
		glProgramParameteri = (PFNGLPROGRAMPARAMETERIPROC)wglGetProcAddress("glProgramParameteri");
		glProgramUniform1d = (PFNGLPROGRAMUNIFORM1DPROC)wglGetProcAddress("glProgramUniform1d");
		glProgramUniform1dv = (PFNGLPROGRAMUNIFORM1DVPROC)wglGetProcAddress("glProgramUniform1dv");
		glProgramUniform1f = (PFNGLPROGRAMUNIFORM1FPROC)wglGetProcAddress("glProgramUniform1f");
		glProgramUniform1fv = (PFNGLPROGRAMUNIFORM1FVPROC)wglGetProcAddress("glProgramUniform1fv");
		glProgramUniform1i = (PFNGLPROGRAMUNIFORM1IPROC)wglGetProcAddress("glProgramUniform1i");
		glProgramUniform1iv = (PFNGLPROGRAMUNIFORM1IVPROC)wglGetProcAddress("glProgramUniform1iv");
		glProgramUniform1ui = (PFNGLPROGRAMUNIFORM1UIPROC)wglGetProcAddress("glProgramUniform1ui");
		glProgramUniform1uiv = (PFNGLPROGRAMUNIFORM1UIVPROC)wglGetProcAddress("glProgramUniform1uiv");
		glProgramUniform2d = (PFNGLPROGRAMUNIFORM2DPROC)wglGetProcAddress("glProgramUniform2d");
		glProgramUniform2dv = (PFNGLPROGRAMUNIFORM2DVPROC)wglGetProcAddress("glProgramUniform2dv");
		glProgramUniform2f = (PFNGLPROGRAMUNIFORM2FPROC)wglGetProcAddress("glProgramUniform2f");
		glProgramUniform2fv = (PFNGLPROGRAMUNIFORM2FVPROC)wglGetProcAddress("glProgramUniform2fv");
		glProgramUniform2i = (PFNGLPROGRAMUNIFORM2IPROC)wglGetProcAddress("glProgramUniform2i");
		glProgramUniform2iv = (PFNGLPROGRAMUNIFORM2IVPROC)wglGetProcAddress("glProgramUniform2iv");
		glProgramUniform2ui = (PFNGLPROGRAMUNIFORM2UIPROC)wglGetProcAddress("glProgramUniform2ui");
		glProgramUniform2uiv = (PFNGLPROGRAMUNIFORM2UIVPROC)wglGetProcAddress("glProgramUniform2uiv");
		glProgramUniform3d = (PFNGLPROGRAMUNIFORM3DPROC)wglGetProcAddress("glProgramUniform3d");
		glProgramUniform3dv = (PFNGLPROGRAMUNIFORM3DVPROC)wglGetProcAddress("glProgramUniform3dv");
		glProgramUniform3f = (PFNGLPROGRAMUNIFORM3FPROC)wglGetProcAddress("glProgramUniform3f");
		glProgramUniform3fv = (PFNGLPROGRAMUNIFORM3FVPROC)wglGetProcAddress("glProgramUniform3fv");
		glProgramUniform3i = (PFNGLPROGRAMUNIFORM3IPROC)wglGetProcAddress("glProgramUniform3i");
		glProgramUniform3iv = (PFNGLPROGRAMUNIFORM3IVPROC)wglGetProcAddress("glProgramUniform3iv");
		glProgramUniform3ui = (PFNGLPROGRAMUNIFORM3UIPROC)wglGetProcAddress("glProgramUniform3ui");
		glProgramUniform3uiv = (PFNGLPROGRAMUNIFORM3UIVPROC)wglGetProcAddress("glProgramUniform3uiv");
		glProgramUniform4d = (PFNGLPROGRAMUNIFORM4DPROC)wglGetProcAddress("glProgramUniform4d");
		glProgramUniform4dv = (PFNGLPROGRAMUNIFORM4DVPROC)wglGetProcAddress("glProgramUniform4dv");
		glProgramUniform4f = (PFNGLPROGRAMUNIFORM4FPROC)wglGetProcAddress("glProgramUniform4f");
		glProgramUniform4fv = (PFNGLPROGRAMUNIFORM4FVPROC)wglGetProcAddress("glProgramUniform4fv");
		glProgramUniform4i = (PFNGLPROGRAMUNIFORM4IPROC)wglGetProcAddress("glProgramUniform4i");
		glProgramUniform4iv = (PFNGLPROGRAMUNIFORM4IVPROC)wglGetProcAddress("glProgramUniform4iv");
		glProgramUniform4ui = (PFNGLPROGRAMUNIFORM4UIPROC)wglGetProcAddress("glProgramUniform4ui");
		glProgramUniform4uiv = (PFNGLPROGRAMUNIFORM4UIVPROC)wglGetProcAddress("glProgramUniform4uiv");
		glProgramUniformMatrix2dv = (PFNGLPROGRAMUNIFORMMATRIX2DVPROC)wglGetProcAddress("glProgramUniformMatrix2dv");
		glProgramUniformMatrix2fv = (PFNGLPROGRAMUNIFORMMATRIX2FVPROC)wglGetProcAddress("glProgramUniformMatrix2fv");
		glProgramUniformMatrix2x3dv = (PFNGLPROGRAMUNIFORMMATRIX2X3DVPROC)wglGetProcAddress("glProgramUniformMatrix2x3dv");
		glProgramUniformMatrix2x3fv = (PFNGLPROGRAMUNIFORMMATRIX2X3FVPROC)wglGetProcAddress("glProgramUniformMatrix2x3fv");
		glProgramUniformMatrix2x4dv = (PFNGLPROGRAMUNIFORMMATRIX2X4DVPROC)wglGetProcAddress("glProgramUniformMatrix2x4dv");
		glProgramUniformMatrix2x4fv = (PFNGLPROGRAMUNIFORMMATRIX2X4FVPROC)wglGetProcAddress("glProgramUniformMatrix2x4fv");
		glProgramUniformMatrix3dv = (PFNGLPROGRAMUNIFORMMATRIX3DVPROC)wglGetProcAddress("glProgramUniformMatrix3dv");
		glProgramUniformMatrix3fv = (PFNGLPROGRAMUNIFORMMATRIX3FVPROC)wglGetProcAddress("glProgramUniformMatrix3fv");
		glProgramUniformMatrix3x2dv = (PFNGLPROGRAMUNIFORMMATRIX3X2DVPROC)wglGetProcAddress("glProgramUniformMatrix3x2dv");
		glProgramUniformMatrix3x2fv = (PFNGLPROGRAMUNIFORMMATRIX3X2FVPROC)wglGetProcAddress("glProgramUniformMatrix3x2fv");
		glProgramUniformMatrix3x4dv = (PFNGLPROGRAMUNIFORMMATRIX3X4DVPROC)wglGetProcAddress("glProgramUniformMatrix3x4dv");
		glProgramUniformMatrix3x4fv = (PFNGLPROGRAMUNIFORMMATRIX3X4FVPROC)wglGetProcAddress("glProgramUniformMatrix3x4fv");
		glProgramUniformMatrix4dv = (PFNGLPROGRAMUNIFORMMATRIX4DVPROC)wglGetProcAddress("glProgramUniformMatrix4dv");
		glProgramUniformMatrix4fv = (PFNGLPROGRAMUNIFORMMATRIX4FVPROC)wglGetProcAddress("glProgramUniformMatrix4fv");
		glProgramUniformMatrix4x2dv = (PFNGLPROGRAMUNIFORMMATRIX4X2DVPROC)wglGetProcAddress("glProgramUniformMatrix4x2dv");
		glProgramUniformMatrix4x2fv = (PFNGLPROGRAMUNIFORMMATRIX4X2FVPROC)wglGetProcAddress("glProgramUniformMatrix4x2fv");
		glProgramUniformMatrix4x3dv = (PFNGLPROGRAMUNIFORMMATRIX4X3DVPROC)wglGetProcAddress("glProgramUniformMatrix4x3dv");
		glProgramUniformMatrix4x3fv = (PFNGLPROGRAMUNIFORMMATRIX4X3FVPROC)wglGetProcAddress("glProgramUniformMatrix4x3fv");
		glProvokingVertex = (PFNGLPROVOKINGVERTEXPROC)wglGetProcAddress("glProvokingVertex");
		glPushDebugGroup = (PFNGLPUSHDEBUGGROUPPROC)wglGetProcAddress("glPushDebugGroup");
		glQueryCounter = (PFNGLQUERYCOUNTERPROC)wglGetProcAddress("glQueryCounter");
		glReadnPixels = (PFNGLREADNPIXELSPROC)wglGetProcAddress("glReadnPixels");
		glReleaseShaderCompiler = (PFNGLRELEASESHADERCOMPILERPROC)wglGetProcAddress("glReleaseShaderCompiler");
		glRenderbufferStorage = (PFNGLRENDERBUFFERSTORAGEPROC)wglGetProcAddress("glRenderbufferStorage");
		glRenderbufferStorageMultisample = (PFNGLRENDERBUFFERSTORAGEMULTISAMPLEPROC)wglGetProcAddress("glRenderbufferStorageMultisample");
		glResumeTransformFeedback = (PFNGLRESUMETRANSFORMFEEDBACKPROC)wglGetProcAddress("glResumeTransformFeedback");
		glSampleCoverage = (PFNGLSAMPLECOVERAGEPROC)wglGetProcAddress("glSampleCoverage");
		glSampleMaski = (PFNGLSAMPLEMASKIPROC)wglGetProcAddress("glSampleMaski");
		glSamplerParameterf = (PFNGLSAMPLERPARAMETERFPROC)wglGetProcAddress("glSamplerParameterf");
		glSamplerParameterfv = (PFNGLSAMPLERPARAMETERFVPROC)wglGetProcAddress("glSamplerParameterfv");
		glSamplerParameteri = (PFNGLSAMPLERPARAMETERIPROC)wglGetProcAddress("glSamplerParameteri");
		glSamplerParameterIiv = (PFNGLSAMPLERPARAMETERIIVPROC)wglGetProcAddress("glSamplerParameterIiv");
		glSamplerParameterIuiv = (PFNGLSAMPLERPARAMETERIUIVPROC)wglGetProcAddress("glSamplerParameterIuiv");
		glSamplerParameteriv = (PFNGLSAMPLERPARAMETERIVPROC)wglGetProcAddress("glSamplerParameteriv");
		glScissorArrayv = (PFNGLSCISSORARRAYVPROC)wglGetProcAddress("glScissorArrayv");
		glScissorIndexed = (PFNGLSCISSORINDEXEDPROC)wglGetProcAddress("glScissorIndexed");
		glScissorIndexedv = (PFNGLSCISSORINDEXEDVPROC)wglGetProcAddress("glScissorIndexedv");
		glShaderBinary = (PFNGLSHADERBINARYPROC)wglGetProcAddress("glShaderBinary");
		glShaderSource = (PFNGLSHADERSOURCEPROC)wglGetProcAddress("glShaderSource");
		glShaderStorageBlockBinding = (PFNGLSHADERSTORAGEBLOCKBINDINGPROC)wglGetProcAddress("glShaderStorageBlockBinding");
		glStencilFuncSeparate = (PFNGLSTENCILFUNCSEPARATEPROC)wglGetProcAddress("glStencilFuncSeparate");
		glStencilMaskSeparate = (PFNGLSTENCILMASKSEPARATEPROC)wglGetProcAddress("glStencilMaskSeparate");
		glStencilOpSeparate = (PFNGLSTENCILOPSEPARATEPROC)wglGetProcAddress("glStencilOpSeparate");
		glTexBuffer = (PFNGLTEXBUFFERPROC)wglGetProcAddress("glTexBuffer");
		glTexBufferRange = (PFNGLTEXBUFFERRANGEPROC)wglGetProcAddress("glTexBufferRange");
		glTexImage2DMultisample = (PFNGLTEXIMAGE2DMULTISAMPLEPROC)wglGetProcAddress("glTexImage2DMultisample");
		glTexImage3D = (PFNGLTEXIMAGE3DPROC)wglGetProcAddress("glTexImage3D");
		glTexImage3DMultisample = (PFNGLTEXIMAGE3DMULTISAMPLEPROC)wglGetProcAddress("glTexImage3DMultisample");
		glTexParameterIiv = (PFNGLTEXPARAMETERIIVPROC)wglGetProcAddress("glTexParameterIiv");
		glTexParameterIuiv = (PFNGLTEXPARAMETERIUIVPROC)wglGetProcAddress("glTexParameterIuiv");
		glTexStorage1D = (PFNGLTEXSTORAGE1DPROC)wglGetProcAddress("glTexStorage1D");
		glTexStorage2D = (PFNGLTEXSTORAGE2DPROC)wglGetProcAddress("glTexStorage2D");
		glTexStorage2DMultisample = (PFNGLTEXSTORAGE2DMULTISAMPLEPROC)wglGetProcAddress("glTexStorage2DMultisample");
		glTexStorage3D = (PFNGLTEXSTORAGE3DPROC)wglGetProcAddress("glTexStorage3D");
		glTexStorage3DMultisample = (PFNGLTEXSTORAGE3DMULTISAMPLEPROC)wglGetProcAddress("glTexStorage3DMultisample");
		glTexSubImage3D = (PFNGLTEXSUBIMAGE3DPROC)wglGetProcAddress("glTexSubImage3D");
		glTextureBarrier = (PFNGLTEXTUREBARRIERPROC)wglGetProcAddress("glTextureBarrier");
		glTextureBuffer = (PFNGLTEXTUREBUFFERPROC)wglGetProcAddress("glTextureBuffer");
		glTextureBufferRange = (PFNGLTEXTUREBUFFERRANGEPROC)wglGetProcAddress("glTextureBufferRange");
		glTextureParameterf = (PFNGLTEXTUREPARAMETERFPROC)wglGetProcAddress("glTextureParameterf");
		glTextureParameterfv = (PFNGLTEXTUREPARAMETERFVPROC)wglGetProcAddress("glTextureParameterfv");
		glTextureParameteri = (PFNGLTEXTUREPARAMETERIPROC)wglGetProcAddress("glTextureParameteri");
		glTextureParameterIiv = (PFNGLTEXTUREPARAMETERIIVPROC)wglGetProcAddress("glTextureParameterIiv");
		glTextureParameterIuiv = (PFNGLTEXTUREPARAMETERIUIVPROC)wglGetProcAddress("glTextureParameterIuiv");
		glTextureParameteriv = (PFNGLTEXTUREPARAMETERIVPROC)wglGetProcAddress("glTextureParameteriv");
		glTextureStorage1D = (PFNGLTEXTURESTORAGE1DPROC)wglGetProcAddress("glTextureStorage1D");
		glTextureStorage2D = (PFNGLTEXTURESTORAGE2DPROC)wglGetProcAddress("glTextureStorage2D");
		glTextureStorage2DMultisample = (PFNGLTEXTURESTORAGE2DMULTISAMPLEPROC)wglGetProcAddress("glTextureStorage2DMultisample");
		glTextureStorage3D = (PFNGLTEXTURESTORAGE3DPROC)wglGetProcAddress("glTextureStorage3D");
		glTextureStorage3DMultisample = (PFNGLTEXTURESTORAGE3DMULTISAMPLEPROC)wglGetProcAddress("glTextureStorage3DMultisample");
		glTextureSubImage1D = (PFNGLTEXTURESUBIMAGE1DPROC)wglGetProcAddress("glTextureSubImage1D");
		glTextureSubImage2D = (PFNGLTEXTURESUBIMAGE2DPROC)wglGetProcAddress("glTextureSubImage2D");
		glTextureSubImage3D = (PFNGLTEXTURESUBIMAGE3DPROC)wglGetProcAddress("glTextureSubImage3D");
		glTextureView = (PFNGLTEXTUREVIEWPROC)wglGetProcAddress("glTextureView");
		glTransformFeedbackBufferBase = (PFNGLTRANSFORMFEEDBACKBUFFERBASEPROC)wglGetProcAddress("glTransformFeedbackBufferBase");
		glTransformFeedbackBufferRange = (PFNGLTRANSFORMFEEDBACKBUFFERRANGEPROC)wglGetProcAddress("glTransformFeedbackBufferRange");
		glTransformFeedbackVaryings = (PFNGLTRANSFORMFEEDBACKVARYINGSPROC)wglGetProcAddress("glTransformFeedbackVaryings");
		glUniform1d = (PFNGLUNIFORM1DPROC)wglGetProcAddress("glUniform1d");
		glUniform1dv = (PFNGLUNIFORM1DVPROC)wglGetProcAddress("glUniform1dv");
		glUniform1f = (PFNGLUNIFORM1FPROC)wglGetProcAddress("glUniform1f");
		glUniform1fv = (PFNGLUNIFORM1FVPROC)wglGetProcAddress("glUniform1fv");
		glUniform1i = (PFNGLUNIFORM1IPROC)wglGetProcAddress("glUniform1i");
		glUniform1iv = (PFNGLUNIFORM1IVPROC)wglGetProcAddress("glUniform1iv");
		glUniform1ui = (PFNGLUNIFORM1UIPROC)wglGetProcAddress("glUniform1ui");
		glUniform1uiv = (PFNGLUNIFORM1UIVPROC)wglGetProcAddress("glUniform1uiv");
		glUniform2d = (PFNGLUNIFORM2DPROC)wglGetProcAddress("glUniform2d");
		glUniform2dv = (PFNGLUNIFORM2DVPROC)wglGetProcAddress("glUniform2dv");
		glUniform2f = (PFNGLUNIFORM2FPROC)wglGetProcAddress("glUniform2f");
		glUniform2fv = (PFNGLUNIFORM2FVPROC)wglGetProcAddress("glUniform2fv");
		glUniform2i = (PFNGLUNIFORM2IPROC)wglGetProcAddress("glUniform2i");
		glUniform2iv = (PFNGLUNIFORM2IVPROC)wglGetProcAddress("glUniform2iv");
		glUniform2ui = (PFNGLUNIFORM2UIPROC)wglGetProcAddress("glUniform2ui");
		glUniform2uiv = (PFNGLUNIFORM2UIVPROC)wglGetProcAddress("glUniform2uiv");
		glUniform3d = (PFNGLUNIFORM3DPROC)wglGetProcAddress("glUniform3d");
		glUniform3dv = (PFNGLUNIFORM3DVPROC)wglGetProcAddress("glUniform3dv");
		glUniform3f = (PFNGLUNIFORM3FPROC)wglGetProcAddress("glUniform3f");
		glUniform3fv = (PFNGLUNIFORM3FVPROC)wglGetProcAddress("glUniform3fv");
		glUniform3i = (PFNGLUNIFORM3IPROC)wglGetProcAddress("glUniform3i");
		glUniform3iv = (PFNGLUNIFORM3IVPROC)wglGetProcAddress("glUniform3iv");
		glUniform3ui = (PFNGLUNIFORM3UIPROC)wglGetProcAddress("glUniform3ui");
		glUniform3uiv = (PFNGLUNIFORM3UIVPROC)wglGetProcAddress("glUniform3uiv");
		glUniform4d = (PFNGLUNIFORM4DPROC)wglGetProcAddress("glUniform4d");
		glUniform4dv = (PFNGLUNIFORM4DVPROC)wglGetProcAddress("glUniform4dv");
		glUniform4f = (PFNGLUNIFORM4FPROC)wglGetProcAddress("glUniform4f");
		glUniform4fv = (PFNGLUNIFORM4FVPROC)wglGetProcAddress("glUniform4fv");
		glUniform4i = (PFNGLUNIFORM4IPROC)wglGetProcAddress("glUniform4i");
		glUniform4iv = (PFNGLUNIFORM4IVPROC)wglGetProcAddress("glUniform4iv");
		glUniform4ui = (PFNGLUNIFORM4UIPROC)wglGetProcAddress("glUniform4ui");
		glUniform4uiv = (PFNGLUNIFORM4UIVPROC)wglGetProcAddress("glUniform4uiv");
		glUniformBlockBinding = (PFNGLUNIFORMBLOCKBINDINGPROC)wglGetProcAddress("glUniformBlockBinding");
		glUniformMatrix2dv = (PFNGLUNIFORMMATRIX2DVPROC)wglGetProcAddress("glUniformMatrix2dv");
		glUniformMatrix2fv = (PFNGLUNIFORMMATRIX2FVPROC)wglGetProcAddress("glUniformMatrix2fv");
		glUniformMatrix2x3dv = (PFNGLUNIFORMMATRIX2X3DVPROC)wglGetProcAddress("glUniformMatrix2x3dv");
		glUniformMatrix2x3fv = (PFNGLUNIFORMMATRIX2X3FVPROC)wglGetProcAddress("glUniformMatrix2x3fv");
		glUniformMatrix2x4dv = (PFNGLUNIFORMMATRIX2X4DVPROC)wglGetProcAddress("glUniformMatrix2x4dv");
		glUniformMatrix2x4fv = (PFNGLUNIFORMMATRIX2X4FVPROC)wglGetProcAddress("glUniformMatrix2x4fv");
		glUniformMatrix3dv = (PFNGLUNIFORMMATRIX3DVPROC)wglGetProcAddress("glUniformMatrix3dv");
		glUniformMatrix3fv = (PFNGLUNIFORMMATRIX3FVPROC)wglGetProcAddress("glUniformMatrix3fv");
		glUniformMatrix3x2dv = (PFNGLUNIFORMMATRIX3X2DVPROC)wglGetProcAddress("glUniformMatrix3x2dv");
		glUniformMatrix3x2fv = (PFNGLUNIFORMMATRIX3X2FVPROC)wglGetProcAddress("glUniformMatrix3x2fv");
		glUniformMatrix3x4dv = (PFNGLUNIFORMMATRIX3X4DVPROC)wglGetProcAddress("glUniformMatrix3x4dv");
		glUniformMatrix3x4fv = (PFNGLUNIFORMMATRIX3X4FVPROC)wglGetProcAddress("glUniformMatrix3x4fv");
		glUniformMatrix4dv = (PFNGLUNIFORMMATRIX4DVPROC)wglGetProcAddress("glUniformMatrix4dv");
		glUniformMatrix4fv = (PFNGLUNIFORMMATRIX4FVPROC)wglGetProcAddress("glUniformMatrix4fv");
		glUniformMatrix4x2dv = (PFNGLUNIFORMMATRIX4X2DVPROC)wglGetProcAddress("glUniformMatrix4x2dv");
		glUniformMatrix4x2fv = (PFNGLUNIFORMMATRIX4X2FVPROC)wglGetProcAddress("glUniformMatrix4x2fv");
		glUniformMatrix4x3dv = (PFNGLUNIFORMMATRIX4X3DVPROC)wglGetProcAddress("glUniformMatrix4x3dv");
		glUniformMatrix4x3fv = (PFNGLUNIFORMMATRIX4X3FVPROC)wglGetProcAddress("glUniformMatrix4x3fv");
		glUniformSubroutinesuiv = (PFNGLUNIFORMSUBROUTINESUIVPROC)wglGetProcAddress("glUniformSubroutinesuiv");
		glUnmapBuffer = (PFNGLUNMAPBUFFERPROC)wglGetProcAddress("glUnmapBuffer");
		glUnmapNamedBuffer = (PFNGLUNMAPNAMEDBUFFERPROC)wglGetProcAddress("glUnmapNamedBuffer");
		glUseProgram = (PFNGLUSEPROGRAMPROC)wglGetProcAddress("glUseProgram");
		glUseProgramStages = (PFNGLUSEPROGRAMSTAGESPROC)wglGetProcAddress("glUseProgramStages");
		glValidateProgram = (PFNGLVALIDATEPROGRAMPROC)wglGetProcAddress("glValidateProgram");
		glValidateProgramPipeline = (PFNGLVALIDATEPROGRAMPIPELINEPROC)wglGetProcAddress("glValidateProgramPipeline");
		glVertexArrayAttribBinding = (PFNGLVERTEXARRAYATTRIBBINDINGPROC)wglGetProcAddress("glVertexArrayAttribBinding");
		glVertexArrayAttribFormat = (PFNGLVERTEXARRAYATTRIBFORMATPROC)wglGetProcAddress("glVertexArrayAttribFormat");
		glVertexArrayAttribIFormat = (PFNGLVERTEXARRAYATTRIBIFORMATPROC)wglGetProcAddress("glVertexArrayAttribIFormat");
		glVertexArrayAttribLFormat = (PFNGLVERTEXARRAYATTRIBLFORMATPROC)wglGetProcAddress("glVertexArrayAttribLFormat");
		glVertexArrayBindingDivisor = (PFNGLVERTEXARRAYBINDINGDIVISORPROC)wglGetProcAddress("glVertexArrayBindingDivisor");
		glVertexArrayElementBuffer = (PFNGLVERTEXARRAYELEMENTBUFFERPROC)wglGetProcAddress("glVertexArrayElementBuffer");
		glVertexArrayVertexBuffer = (PFNGLVERTEXARRAYVERTEXBUFFERPROC)wglGetProcAddress("glVertexArrayVertexBuffer");
		glVertexArrayVertexBuffers = (PFNGLVERTEXARRAYVERTEXBUFFERSPROC)wglGetProcAddress("glVertexArrayVertexBuffers");
		glVertexAttrib1d = (PFNGLVERTEXATTRIB1DPROC)wglGetProcAddress("glVertexAttrib1d");
		glVertexAttrib1dv = (PFNGLVERTEXATTRIB1DVPROC)wglGetProcAddress("glVertexAttrib1dv");
		glVertexAttrib1f = (PFNGLVERTEXATTRIB1FPROC)wglGetProcAddress("glVertexAttrib1f");
		glVertexAttrib1fv = (PFNGLVERTEXATTRIB1FVPROC)wglGetProcAddress("glVertexAttrib1fv");
		glVertexAttrib1s = (PFNGLVERTEXATTRIB1SPROC)wglGetProcAddress("glVertexAttrib1s");
		glVertexAttrib1sv = (PFNGLVERTEXATTRIB1SVPROC)wglGetProcAddress("glVertexAttrib1sv");
		glVertexAttrib2d = (PFNGLVERTEXATTRIB2DPROC)wglGetProcAddress("glVertexAttrib2d");
		glVertexAttrib2dv = (PFNGLVERTEXATTRIB2DVPROC)wglGetProcAddress("glVertexAttrib2dv");
		glVertexAttrib2f = (PFNGLVERTEXATTRIB2FPROC)wglGetProcAddress("glVertexAttrib2f");
		glVertexAttrib2fv = (PFNGLVERTEXATTRIB2FVPROC)wglGetProcAddress("glVertexAttrib2fv");
		glVertexAttrib2s = (PFNGLVERTEXATTRIB2SPROC)wglGetProcAddress("glVertexAttrib2s");
		glVertexAttrib2sv = (PFNGLVERTEXATTRIB2SVPROC)wglGetProcAddress("glVertexAttrib2sv");
		glVertexAttrib3d = (PFNGLVERTEXATTRIB3DPROC)wglGetProcAddress("glVertexAttrib3d");
		glVertexAttrib3dv = (PFNGLVERTEXATTRIB3DVPROC)wglGetProcAddress("glVertexAttrib3dv");
		glVertexAttrib3f = (PFNGLVERTEXATTRIB3FPROC)wglGetProcAddress("glVertexAttrib3f");
		glVertexAttrib3fv = (PFNGLVERTEXATTRIB3FVPROC)wglGetProcAddress("glVertexAttrib3fv");
		glVertexAttrib3s = (PFNGLVERTEXATTRIB3SPROC)wglGetProcAddress("glVertexAttrib3s");
		glVertexAttrib3sv = (PFNGLVERTEXATTRIB3SVPROC)wglGetProcAddress("glVertexAttrib3sv");
		glVertexAttrib4bv = (PFNGLVERTEXATTRIB4BVPROC)wglGetProcAddress("glVertexAttrib4bv");
		glVertexAttrib4d = (PFNGLVERTEXATTRIB4DPROC)wglGetProcAddress("glVertexAttrib4d");
		glVertexAttrib4dv = (PFNGLVERTEXATTRIB4DVPROC)wglGetProcAddress("glVertexAttrib4dv");
		glVertexAttrib4f = (PFNGLVERTEXATTRIB4FPROC)wglGetProcAddress("glVertexAttrib4f");
		glVertexAttrib4fv = (PFNGLVERTEXATTRIB4FVPROC)wglGetProcAddress("glVertexAttrib4fv");
		glVertexAttrib4iv = (PFNGLVERTEXATTRIB4IVPROC)wglGetProcAddress("glVertexAttrib4iv");
		glVertexAttrib4Nbv = (PFNGLVERTEXATTRIB4NBVPROC)wglGetProcAddress("glVertexAttrib4Nbv");
		glVertexAttrib4Niv = (PFNGLVERTEXATTRIB4NIVPROC)wglGetProcAddress("glVertexAttrib4Niv");
		glVertexAttrib4Nsv = (PFNGLVERTEXATTRIB4NSVPROC)wglGetProcAddress("glVertexAttrib4Nsv");
		glVertexAttrib4Nub = (PFNGLVERTEXATTRIB4NUBPROC)wglGetProcAddress("glVertexAttrib4Nub");
		glVertexAttrib4Nubv = (PFNGLVERTEXATTRIB4NUBVPROC)wglGetProcAddress("glVertexAttrib4Nubv");
		glVertexAttrib4Nuiv = (PFNGLVERTEXATTRIB4NUIVPROC)wglGetProcAddress("glVertexAttrib4Nuiv");
		glVertexAttrib4Nusv = (PFNGLVERTEXATTRIB4NUSVPROC)wglGetProcAddress("glVertexAttrib4Nusv");
		glVertexAttrib4s = (PFNGLVERTEXATTRIB4SPROC)wglGetProcAddress("glVertexAttrib4s");
		glVertexAttrib4sv = (PFNGLVERTEXATTRIB4SVPROC)wglGetProcAddress("glVertexAttrib4sv");
		glVertexAttrib4ubv = (PFNGLVERTEXATTRIB4UBVPROC)wglGetProcAddress("glVertexAttrib4ubv");
		glVertexAttrib4uiv = (PFNGLVERTEXATTRIB4UIVPROC)wglGetProcAddress("glVertexAttrib4uiv");
		glVertexAttrib4usv = (PFNGLVERTEXATTRIB4USVPROC)wglGetProcAddress("glVertexAttrib4usv");
		glVertexAttribBinding = (PFNGLVERTEXATTRIBBINDINGPROC)wglGetProcAddress("glVertexAttribBinding");
		glVertexAttribDivisor = (PFNGLVERTEXATTRIBDIVISORPROC)wglGetProcAddress("glVertexAttribDivisor");
		glVertexAttribFormat = (PFNGLVERTEXATTRIBFORMATPROC)wglGetProcAddress("glVertexAttribFormat");
		glVertexAttribI1i = (PFNGLVERTEXATTRIBI1IPROC)wglGetProcAddress("glVertexAttribI1i");
		glVertexAttribI1iv = (PFNGLVERTEXATTRIBI1IVPROC)wglGetProcAddress("glVertexAttribI1iv");
		glVertexAttribI1ui = (PFNGLVERTEXATTRIBI1UIPROC)wglGetProcAddress("glVertexAttribI1ui");
		glVertexAttribI1uiv = (PFNGLVERTEXATTRIBI1UIVPROC)wglGetProcAddress("glVertexAttribI1uiv");
		glVertexAttribI2i = (PFNGLVERTEXATTRIBI2IPROC)wglGetProcAddress("glVertexAttribI2i");
		glVertexAttribI2iv = (PFNGLVERTEXATTRIBI2IVPROC)wglGetProcAddress("glVertexAttribI2iv");
		glVertexAttribI2ui = (PFNGLVERTEXATTRIBI2UIPROC)wglGetProcAddress("glVertexAttribI2ui");
		glVertexAttribI2uiv = (PFNGLVERTEXATTRIBI2UIVPROC)wglGetProcAddress("glVertexAttribI2uiv");
		glVertexAttribI3i = (PFNGLVERTEXATTRIBI3IPROC)wglGetProcAddress("glVertexAttribI3i");
		glVertexAttribI3iv = (PFNGLVERTEXATTRIBI3IVPROC)wglGetProcAddress("glVertexAttribI3iv");
		glVertexAttribI3ui = (PFNGLVERTEXATTRIBI3UIPROC)wglGetProcAddress("glVertexAttribI3ui");
		glVertexAttribI3uiv = (PFNGLVERTEXATTRIBI3UIVPROC)wglGetProcAddress("glVertexAttribI3uiv");
		glVertexAttribI4bv = (PFNGLVERTEXATTRIBI4BVPROC)wglGetProcAddress("glVertexAttribI4bv");
		glVertexAttribI4i = (PFNGLVERTEXATTRIBI4IPROC)wglGetProcAddress("glVertexAttribI4i");
		glVertexAttribI4iv = (PFNGLVERTEXATTRIBI4IVPROC)wglGetProcAddress("glVertexAttribI4iv");
		glVertexAttribI4sv = (PFNGLVERTEXATTRIBI4SVPROC)wglGetProcAddress("glVertexAttribI4sv");
		glVertexAttribI4ubv = (PFNGLVERTEXATTRIBI4UBVPROC)wglGetProcAddress("glVertexAttribI4ubv");
		glVertexAttribI4ui = (PFNGLVERTEXATTRIBI4UIPROC)wglGetProcAddress("glVertexAttribI4ui");
		glVertexAttribI4uiv = (PFNGLVERTEXATTRIBI4UIVPROC)wglGetProcAddress("glVertexAttribI4uiv");
		glVertexAttribI4usv = (PFNGLVERTEXATTRIBI4USVPROC)wglGetProcAddress("glVertexAttribI4usv");
		glVertexAttribIFormat = (PFNGLVERTEXATTRIBIFORMATPROC)wglGetProcAddress("glVertexAttribIFormat");
		glVertexAttribIPointer = (PFNGLVERTEXATTRIBIPOINTERPROC)wglGetProcAddress("glVertexAttribIPointer");
		glVertexAttribL1d = (PFNGLVERTEXATTRIBL1DPROC)wglGetProcAddress("glVertexAttribL1d");
		glVertexAttribL1dv = (PFNGLVERTEXATTRIBL1DVPROC)wglGetProcAddress("glVertexAttribL1dv");
		glVertexAttribL2d = (PFNGLVERTEXATTRIBL2DPROC)wglGetProcAddress("glVertexAttribL2d");
		glVertexAttribL2dv = (PFNGLVERTEXATTRIBL2DVPROC)wglGetProcAddress("glVertexAttribL2dv");
		glVertexAttribL3d = (PFNGLVERTEXATTRIBL3DPROC)wglGetProcAddress("glVertexAttribL3d");
		glVertexAttribL3dv = (PFNGLVERTEXATTRIBL3DVPROC)wglGetProcAddress("glVertexAttribL3dv");
		glVertexAttribL4d = (PFNGLVERTEXATTRIBL4DPROC)wglGetProcAddress("glVertexAttribL4d");
		glVertexAttribL4dv = (PFNGLVERTEXATTRIBL4DVPROC)wglGetProcAddress("glVertexAttribL4dv");
		glVertexAttribLFormat = (PFNGLVERTEXATTRIBLFORMATPROC)wglGetProcAddress("glVertexAttribLFormat");
		glVertexAttribLPointer = (PFNGLVERTEXATTRIBLPOINTERPROC)wglGetProcAddress("glVertexAttribLPointer");
		glVertexAttribP1ui = (PFNGLVERTEXATTRIBP1UIPROC)wglGetProcAddress("glVertexAttribP1ui");
		glVertexAttribP1uiv = (PFNGLVERTEXATTRIBP1UIVPROC)wglGetProcAddress("glVertexAttribP1uiv");
		glVertexAttribP2ui = (PFNGLVERTEXATTRIBP2UIPROC)wglGetProcAddress("glVertexAttribP2ui");
		glVertexAttribP2uiv = (PFNGLVERTEXATTRIBP2UIVPROC)wglGetProcAddress("glVertexAttribP2uiv");
		glVertexAttribP3ui = (PFNGLVERTEXATTRIBP3UIPROC)wglGetProcAddress("glVertexAttribP3ui");
		glVertexAttribP3uiv = (PFNGLVERTEXATTRIBP3UIVPROC)wglGetProcAddress("glVertexAttribP3uiv");
		glVertexAttribP4ui = (PFNGLVERTEXATTRIBP4UIPROC)wglGetProcAddress("glVertexAttribP4ui");
		glVertexAttribP4uiv = (PFNGLVERTEXATTRIBP4UIVPROC)wglGetProcAddress("glVertexAttribP4uiv");
		glVertexAttribPointer = (PFNGLVERTEXATTRIBPOINTERPROC)wglGetProcAddress("glVertexAttribPointer");
		glVertexBindingDivisor = (PFNGLVERTEXBINDINGDIVISORPROC)wglGetProcAddress("glVertexBindingDivisor");
		glViewportArrayv = (PFNGLVIEWPORTARRAYVPROC)wglGetProcAddress("glViewportArrayv");
		glViewportIndexedf = (PFNGLVIEWPORTINDEXEDFPROC)wglGetProcAddress("glViewportIndexedf");
		glViewportIndexedfv = (PFNGLVIEWPORTINDEXEDFVPROC)wglGetProcAddress("glViewportIndexedfv");
		glWaitSync = (PFNGLWAITSYNCPROC)wglGetProcAddress("glWaitSync");
		wglSwapInterval = (PFNWGLSWAPINTERVALEXTPROC)wglGetProcAddress("wglSwapIntervalEXT");
	}

	Sampler::~Sampler() {
		if (!Ref&&Id) {
			GL_CHECK(glDeleteSamplers(1, &Id));
		}
	}
	Sampler::Sampler(TextureFilter MinAndMax, TextureWrap Wrap) {
		glGenSamplers(1, &Id);
		glSamplerParameteri(Id, (int)TextureProperty::TEXTURE_MIN_FILTER, (int)MinAndMax);
		glSamplerParameteri(Id, (int)TextureProperty::TEXTURE_MAG_FILTER, (int)MinAndMax);
		glSamplerParameteri(Id, (int)TextureProperty::TEXTURE_WRAP_S, (int)Wrap);
		glSamplerParameteri(Id, (int)TextureProperty::TEXTURE_WRAP_T, (int)Wrap);
		glSamplerParameteri(Id, (int)TextureProperty::TEXTURE_WRAP_R, (int)Wrap);
	}

	Sampler::Sampler(TextureFilter Min, TextureFilter Mag, TextureWrap Wrap) {
		glGenSamplers(1, &Id);
		glSamplerParameteri(Id, (int)TextureProperty::TEXTURE_MIN_FILTER, (int)Min);
		glSamplerParameteri(Id, (int)TextureProperty::TEXTURE_MAG_FILTER, (int)Mag);
		glSamplerParameteri(Id, (int)TextureProperty::TEXTURE_WRAP_S, (int)Wrap);
		glSamplerParameteri(Id, (int)TextureProperty::TEXTURE_WRAP_T, (int)Wrap);
		glSamplerParameteri(Id, (int)TextureProperty::TEXTURE_WRAP_R, (int)Wrap);
	}
}
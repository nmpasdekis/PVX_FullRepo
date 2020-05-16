#include<PVX_OpenGL.h>

namespace PVX::OpenGL {
	Shader::~Shader() {
		if (!Ref&&Id) glDeleteShader(Id);
	}
	Shader::Shader(ShaderType Type, const std::string& src) : Type{ Type } {
		Source(src);
	}
	void Shader::Source(const std::string& Source) {
		char* src = (char*)Source.c_str();
		int sz = Source.size();
		if(!Id) Id = glCreateShader((unsigned int)Type);
		glShaderSource(Id, 1, &src, &sz);
		glCompileShader(Id);
#ifdef _DEBUG
		int Status;
		glGetShaderiv(Id, GL_COMPILE_STATUS, &Status);
		if (!Status) {
			char msg[1024];
			glGetShaderInfoLog(Id, 1023, &sz, msg);
			throw std::exception(msg);
		}
#endif
	}

	std::string Shader::Debug(const std::string& Source) {
		char* src = (char*)Source.c_str();
		int sz = Source.size();
		if (!Id) Id = glCreateShader((unsigned int)Type);
		glShaderSource(Id, 1, &src, &sz);
		glCompileShader(Id);

		int Status;
		glGetShaderiv(Id, GL_COMPILE_STATUS, &Status);
		if (!Status) {
			glGetShaderiv(Id, GL_INFO_LOG_LENGTH, &sz);
			std::string ret;
			ret.resize(sz);
			glGetShaderInfoLog(Id, sz, NULL, &ret[0]);
			return ret;
		} else {
			return "Shader Compiled Correctly!";
		}
	}

	Program::~Program() {
		if (!Ref&&Id) glDeleteProgram(Id);
	}
	Program::Program(const std::initializer_list<Shader>& sh) {
		AddShaders(sh);
		Build();
	}
	void Program::AddShader(const Shader& sh) {
		Shaders.push_back(sh);
	}
	void Program::AddShaders(const std::initializer_list<Shader>& sh) {
		for (auto& s : sh) Shaders.push_back(s);
	}
	void Program::Build() {
		if(!Id)Id = glCreateProgram();
		for (auto& s : Shaders)	glAttachShader(Id, s.Get());
		glLinkProgram(Id);

#ifdef _DEBUG
		int status;
		glGetProgramiv(Id, GL_LINK_STATUS, &status);
		if (!status) {
			char msg[1024];
			int sz;
			glGetProgramInfoLog(Id, 1023, &sz, msg);
			throw std::exception(msg);
		}
#endif
		for (auto& s : Shaders)	glDetachShader(Id, s.Get());
	}
	void Program::Bind() const {
		glUseProgram(Id);
	}
	void Program::Unbind() const {
		glUseProgram(0);
	}

	void Pipeline::Bind() {
		Prog.Bind();
		FBuffer.Bind();
		for (auto& t : Tex2D) t.Bind();
		for (auto& t : TexCube) t.Bind();
	}
	void Pipeline::Unbind() {
		Prog.Unbind();
		FBuffer.Unbind();
		for (auto& t : Tex2D) t.Unbind();
		for (auto& t : TexCube) t.Unbind();
		for (auto& b : Consts) {
			//glBindBuffer(GL_CONSTANT_BU)
		}
	}
	void Pipeline::Shaders(const Program& p) {
		Prog = p;
	}
	void Pipeline::Textures2D(const std::initializer_list<Texture2D>& Tex) {
		for (auto& t : Tex) Tex2D.push_back(t);
	}
	void Pipeline::TexturesCube(const std::initializer_list<TextureCube>& Tex) {
		for (auto& t : Tex) TexCube.push_back(t);
	}
	void Pipeline::Constants(const std::initializer_list<std::pair<std::string, ConstantBuffer>>& Buf) {
		auto p = Prog.Get();
		for (auto & [Name, Const] : Buf) {
			int index = glGetUniformBlockIndex(p, Name.c_str());
			Consts[index] = Const;
		}
	}
}
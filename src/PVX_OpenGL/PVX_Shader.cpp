#include<PVX_OpenGL.h>

namespace PVX::OpenGL {
	Shader::~Shader() {
		if (!Ref&&Id) glDeleteShader(Id);
	}
	Shader::Shader(ShaderType Type, const std::string& src) : Type{ Type }, Id{ 0 } {
		Source(src);
	}
	void Shader::Source(const std::string& Source) {
		char* src = (char*)Source.c_str();
		int sz = Source.size();
		if(!Id) Id = glCreateShader(Type);
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
		if (!Id) Id = glCreateShader(Type);
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
	void Program::BindShader(const Shader& sh) {
		Shaders.push_back(sh);
	}
	void Program::BindShaders(const std::initializer_list<Shader>& sh) {
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
}
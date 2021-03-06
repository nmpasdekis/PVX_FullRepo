#include<PVX_OpenGL.h>
#include<variant>

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
		if (!Id) Id = glCreateShader((unsigned int)Type);
		glShaderSource(Id, 1, &src, &sz);
		glCompileShader(Id);
#ifdef _DEBUG
		int Status;
		glGetShaderiv(Id, GL_COMPILE_STATUS, &Status);
		if (!Status) {
			char msg[1024];
			glGetShaderInfoLog(Id, 1023, &sz, msg);
			printf(msg);
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
		if (!Id)Id = glCreateProgram();
		for (auto& s : Shaders)	glAttachShader(Id, s.Get());
		glLinkProgram(Id);
		for (auto& s : Shaders)	glDetachShader(Id, s.Get());

#ifdef _DEBUG
		int status;
		glGetProgramiv(Id, GL_LINK_STATUS, &status);
		if (!status) {
			char msg[1024];
			int sz;
			glGetProgramInfoLog(Id, 1023, &sz, msg);
			printf(msg);
			throw std::exception(msg);
		}
#endif
		//glGetProgramiv(Id, GL_ACTIVE_UNIFORM_BLOCK_MAX_NAME_LENGTH, &Count);
		//char* nameBuffer = new char[Count + 1];
		std::unordered_map<std::string, bool> Names;
		int Count;

		glGetProgramiv(Id, GL_ACTIVE_UNIFORM_BLOCKS, &Count);

		for (int i = 0; i < Count; i++) {
			int nameLength;
			glGetActiveUniformBlockiv(Id, i, GL_UNIFORM_BLOCK_NAME_LENGTH, &nameLength);
			std::string name;
			name.resize(nameLength - 1);
			glGetActiveUniformBlockName(Id, i, name.size() + 1, &nameLength, &name[0]);
			Names[name] = true;
		}

		glGetProgramiv(Id, GL_ACTIVE_UNIFORMS, &Count);

		for (int i = 0; i < Count; i++) {
			int nameLength;
			char _name[1024];
			glGetActiveUniformName(Id, i, 1023, &nameLength, _name);
			std::string name(_name);
			Names[name];
		}

		for (auto& [n, tp] : Names)
			if (tp) UniformBlockNames.push_back(n);
			else UniformNames.push_back(n);
	}
	void Program::Bind() const {
		glUseProgram(Id);
	}
	void Program::Unbind() const {
		glUseProgram(0);
	}

	void Pipeline::Bind() {
		Prog.Bind();
		//FBuffer.Bind();
		{
			int aTex = 0;
			for (auto& [tp, SamplerIndex, t] : Tex) {
				glActiveTexture(GL_TEXTURE0 + aTex);
				glUniform1i(SamplerIndex, aTex++);
				glBindTexture(tp, t);
			}
			GL_CHECK();
			for (auto& [i, c] : UniformBuffers) {
				GL_CHECK(glBindBufferBase(GL_UNIFORM_BUFFER, i, c.Get()));
			}
			for (auto& [i, c] : StorageBuffers) {
				GL_CHECK(glBindBufferBase(GL_SHADER_STORAGE_BUFFER, i, c.Get()));
			}
		}
	}
	void Pipeline::Unbind() {
		for (auto i = int(Tex.size()) - 1; i >= 0 ; i--) {
			glActiveTexture(GL_TEXTURE0 + i);
			glBindTexture(GL_TEXTURE_2D, 0);
		};
		Prog.Unbind();
	}
	void Pipeline::Shaders(const Program& p) {
		Prog = p;
	}

	void Pipeline::Textures2D(const std::string& n, const Sampler& s, const Texture2D& t) {
		int texCount = int(Tex.size());
		auto si = Prog.UniformLocation(n.c_str());
		GL_CHECK();
		if (si!=~0u) {
			glUseProgram(Prog.Get());
			GL_CHECK(glUniform1i(si, texCount));
			GL_CHECK(glBindSampler(texCount, s.Get()));
			glUseProgram(0);
			Tex.push_back(std::make_tuple(GL_TEXTURE_2D, si, t.Get()));
			Tex2D.push_back(t);
		}
	}
	void Pipeline::TexturesCube(const std::string& n, const Sampler& s, const TextureCube& t) {
		int texCount = int(Tex.size());
		auto si = Prog.UniformLocation(n.c_str());
		if (si!=~0u) {
			glUseProgram(Prog.Get());
			GL_CHECK(glUniform1i(si, texCount));
			GL_CHECK(glBindSampler(texCount, s.Get()));
			glUseProgram(0);
			Tex.push_back(std::make_tuple(GL_TEXTURE_CUBE_MAP, si, t.Get()));
			TexCube.push_back(t);
		}
	}

	void Pipeline::Textures2D(const std::initializer_list<std::tuple<std::string, Sampler, Texture2D>>& Tex) {
		for (auto& [n, s, t] : Tex) Textures2D(n, s, t);
	}
	void Pipeline::TexturesCube(const std::initializer_list<std::tuple<std::string, Sampler, TextureCube>>& Tex) {
		for (auto& [n, s, t] : Tex) TexturesCube(n, s, t);
	}
	void Pipeline::UniformBlock(const std::string& Name, const Buffer& Const) {
		auto index = Prog.UniformBlockIndex(Name.c_str());
		GL_CHECK(glUniformBlockBinding(Prog.Get(), index, index));
		UniformBuffers[index] = Const;
	}
	void Pipeline::UniformBlock(const std::initializer_list<std::pair<std::string, Buffer>>& Buf) {
		for (auto& [Name, Const] : Buf) UniformBlock(Name, Const);
	}

	void Pipeline::UniformStorage(const std::string& Name, const Buffer& Const) {
		auto index = glGetProgramResourceIndex(Prog.Get(), GL_SHADER_STORAGE_BLOCK, Name.c_str());
		GL_CHECK(glShaderStorageBlockBinding(Prog.Get(), index, index));
		StorageBuffers[index] = Const;
	}
	void Pipeline::UniformStorage(const std::initializer_list<std::pair<std::string, Buffer>>& Buf) {
		for (auto& [Name, Const] : Buf) UniformStorage(Name, Const);
	}

	void Pipeline::BindBuffer(const std::string& Name, const Buffer& Buf) {
		if (Buf.GetType() == GL_UNIFORM_BUFFER) UniformBlock(Name, Buf);
		else UniformStorage(Name, Buf);
	}

	void Pipeline::BindBuffer(const std::initializer_list<std::pair<std::string, Buffer>>& Buf) {}


	unsigned int Program::UniformBlockIndex(const char* Name) {
		return glGetUniformBlockIndex(Id, Name);
	}
	unsigned int Program::UniformLocation(const char* Name) {
		return glGetUniformLocation(Id, Name);
	}

	unsigned int Program::UniformIndex(const char* Name) {
		const char* nms[]{ Name };
		int unsigned ret;
		GL_CHECK(glGetUniformIndices(Id, 1, nms, &ret));
		return ret;
	}

	unsigned int Program::StorageIndex(const char* Name) {
		return glGetProgramResourceIndex(Id, GL_SHADER_STORAGE_BLOCK, Name);
	}


	void Program::BindUniformBlock(int BlockIndex, const Buffer& Buffer) {
		glUniformBlockBinding(Id, BlockIndex, Buffer.Get());
	}

	void Program::BindUniform(int index, float vec) { glUniform1f(index, vec); }
	void Program::BindUniform(int index, const PVX::Vector2D& vec) { glUniform2fv(index, 1, vec.Array); }
	void Program::BindUniform(int index, const PVX::Vector3D& vec) { glUniform3fv(index, 1, vec.Array); }
	void Program::BindUniform(int index, const PVX::Vector4D& vec) { glUniform4fv(index, 1, vec.Array); }
	void Program::BindUniform(int index, const PVX::Matrix4x4& mat) { glUniformMatrix4fv(index, 1, false, mat.m16); }
	void Program::BindUniform(int Index, int Value) { glUniform1i(Index, Value); }
	void Program::BindUniform(int Index, const PVX::iVector2D& Value) { glUniform2iv(Index, 1, Value.Array); }
	void Program::BindUniform(int Index, const PVX::iVector3D& Value) { glUniform3iv(Index, 1, Value.Array); }
	void Program::BindUniform(int Index, const PVX::iVector4D& Value) { glUniform4iv(Index, 1, Value.Array); }
	void Program::BindUniform(int Index, const std::vector<float>& Value) { glUniform1fv(Index, Value.size(), Value.data()); }
	void Program::BindUniform(int Index, const std::vector<PVX::Vector2D>& Value) { glUniform2fv(Index, Value.size(), (float*)Value.data()); }
	void Program::BindUniform(int Index, const std::vector<PVX::Vector3D>& Value) { glUniform3fv(Index, Value.size(), (float*)Value.data()); }
	void Program::BindUniform(int Index, const std::vector<PVX::Vector4D>& Value) { glUniform4fv(Index, Value.size(), (float*)Value.data()); }
	void Program::BindUniform(int Index, const std::vector<int>& Value) { glUniform1iv(Index, Value.size(), Value.data()); }
	void Program::BindUniform(int Index, const std::vector<PVX::iVector2D>& Value) { glUniform2iv(Index, Value.size(), (int*)Value.data()); }
	void Program::BindUniform(int Index, const std::vector<PVX::iVector3D>& Value) { glUniform3iv(Index, Value.size(), (int*)Value.data()); }
	void Program::BindUniform(int Index, const std::vector<PVX::iVector4D>& Value) { glUniform4iv(Index, Value.size(), (int*)Value.data()); }
	void Program::BindUniform(int Index, const std::vector<PVX::Matrix4x4>& Value) { glUniformMatrix4fv(Index, Value.size(), false, (float*)Value.data()); }
	void Program::BindUniform(int Index, const float* Value, int Count) { glUniform1fv(Index, Count, Value); }
	void Program::BindUniform(int Index, const PVX::Vector2D* Value, int Count) { glUniform2fv(Index, Count, (float*)Value); }
	void Program::BindUniform(int Index, const PVX::Vector3D* Value, int Count) { glUniform3fv(Index, Count, (float*)Value); }
	void Program::BindUniform(int Index, const PVX::Vector4D* Value, int Count) { glUniform4fv(Index, Count, (float*)Value); }
	void Program::BindUniform(int Index, const int* Value, int Count) { glUniform1iv(Index, Count, Value); }
	void Program::BindUniform(int Index, const PVX::iVector2D* Value, int Count) { glUniform2iv(Index, Count, (int*)Value); }
	void Program::BindUniform(int Index, const PVX::iVector3D* Value, int Count) { glUniform3iv(Index, Count, (int*)Value); }
	void Program::BindUniform(int Index, const PVX::iVector4D* Value, int Count) { glUniform4iv(Index, Count, (int*)Value); }
	void Program::BindUniform(int Index, const PVX::Matrix4x4* Value, int Count) { glUniformMatrix4fv(Index, Count, false, (float*)Value); }

	Geometry::Geometry(PrimitiveType Type, int IndexCount, const IndexBuffer& Indices, const std::initializer_list<Geometry_init>& Buffers, bool old) : IndexCount{ IndexCount }, Type{ Type }, Indices{ Indices } {
		glGenVertexArrays(1, &Id);
		glBindVertexArray(Id);
		if (!old) {
			int i = 0;
			for (auto& b : Buffers) {
				VertexBuffers.push_back(b.Buffer);
				glBindBuffer(GL_ARRAY_BUFFER, b.Buffer.Get());
				for (auto& a: b.Attributes) {
					glEnableVertexAttribArray(i);
					glVertexAttribPointer(i++, a.Size, a.Type, a.Normalized, b.Stride, (void*)a.Offset);
				}
			}
		} else {
			for (auto& b : Buffers) {
				VertexBuffers.push_back(b.Buffer);
				glBindBuffer(GL_ARRAY_BUFFER, b.Buffer.Get());
				for (auto& a: b.Attributes) {
					if (a.Name == "Position") {
						glEnableClientState(GL_VERTEX_ARRAY);
						glVertexPointer(a.Size, a.Type, b.Stride, (void*)a.Offset);
					} else if (a.Name == "Normal") {
						glEnableClientState(GL_NORMAL_ARRAY);
						glNormalPointer(a.Type, b.Stride, (void*)a.Offset);
					} else if (a.Name=="UV") {
						glEnableClientState(GL_TEXTURE_COORD_ARRAY);
						glTexCoordPointer(a.Size, a.Type, b.Stride, (void*)a.Offset);
					} else if (a.Name=="Color") {
						glEnableClientState(GL_COLOR_ARRAY);
						glColorPointer(a.Size, a.Type, b.Stride, (void*)a.Offset);
					}
				}
			}
		}
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);
	}
	Geometry::~Geometry() {
		if (!Ref && Id) glDeleteVertexArrays(1, &Id);
	}
	void Geometry::Draw() {
		glBindVertexArray(Id);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, Indices.Get());
		glDrawElements(int(Type), IndexCount, GL_UNSIGNED_INT, 0);
		glBindVertexArray(0);
	}
	void Geometry::Draw(int Count) {
		glBindVertexArray(Id);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, Indices.Get());
		glDrawElementsInstanced(int(Type), IndexCount, GL_UNSIGNED_INT, 0, Count);
		glBindVertexArray(0);
	}
}
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
		//AddShaders(sh);
		Build(sh);
	}
	//void Program::AddShader(const Shader& sh) {
	//	Shaders.push_back(sh);
	//}
	//void Program::AddShaders(const std::initializer_list<Shader>& sh) {
	//	for (auto& s : sh) Shaders.push_back(s);
	//}
	void Program::Build(const std::initializer_list<Shader>& Shaders) {
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
		for (auto i = int(Tex.size()) - 1; i >= 0; i--) {
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

	void Pipeline::BindBuffer(const std::initializer_list<std::pair<std::string, Buffer>>& Buf) {
		for (auto& [Name, b] : Buf)
			BindBuffer(Name, b);
	}


	unsigned int Program::UniformBlockIndex(const char* Name) const {
		return glGetUniformBlockIndex(Id, Name);
	}
	unsigned int Program::UniformLocation(const char* Name) const {
		return glGetUniformLocation(Id, Name);
	}

	unsigned int Program::UniformIndex(const char* Name) const {
		const char* nms[]{ Name };
		int unsigned ret;
		GL_CHECK(glGetUniformIndices(Id, 1, nms, &ret));
		return ret;
	}

	unsigned int Program::StorageIndex(const char* Name) const {
		return glGetProgramResourceIndex(Id, GL_SHADER_STORAGE_BLOCK, Name);
	}


	void Program::BindUniformBlock(int BlockIndex, const Buffer& Buffer) {
		glUniformBlockBinding(Id, BlockIndex, Buffer.Get());
	}

	void Program::BindUniform(int index, float vec) const { glUniform1f(index, vec); }
	void Program::BindUniform(int index, const PVX::Vector2D& vec) const { glUniform2fv(index, 1, vec.Array); }
	void Program::BindUniform(int index, const PVX::Vector3D& vec) const { glUniform3fv(index, 1, vec.Array); }
	void Program::BindUniform(int index, const PVX::Vector4D& vec) const { glUniform4fv(index, 1, vec.Array); }
	void Program::BindUniform(int index, const PVX::Matrix4x4& mat) const { glUniformMatrix4fv(index, 1, false, mat.m16); }
	void Program::BindUniform(int Index, int Value) const { glUniform1i(Index, Value); }
	void Program::BindUniform(int Index, const PVX::iVector2D& Value) const { glUniform2iv(Index, 1, Value.Array); }
	void Program::BindUniform(int Index, const PVX::iVector3D& Value) const { glUniform3iv(Index, 1, Value.Array); }
	void Program::BindUniform(int Index, const PVX::iVector4D& Value) const { glUniform4iv(Index, 1, Value.Array); }
	void Program::BindUniform(int Index, const std::vector<float>& Value) const { glUniform1fv(Index, Value.size(), Value.data()); }
	void Program::BindUniform(int Index, const std::vector<PVX::Vector2D>& Value) const { glUniform2fv(Index, Value.size(), (float*)Value.data()); }
	void Program::BindUniform(int Index, const std::vector<PVX::Vector3D>& Value) const { glUniform3fv(Index, Value.size(), (float*)Value.data()); }
	void Program::BindUniform(int Index, const std::vector<PVX::Vector4D>& Value) const { glUniform4fv(Index, Value.size(), (float*)Value.data()); }
	void Program::BindUniform(int Index, const std::vector<int>& Value) const { glUniform1iv(Index, Value.size(), Value.data()); }
	void Program::BindUniform(int Index, const std::vector<PVX::iVector2D>& Value) const { glUniform2iv(Index, Value.size(), (int*)Value.data()); }
	void Program::BindUniform(int Index, const std::vector<PVX::iVector3D>& Value) const { glUniform3iv(Index, Value.size(), (int*)Value.data()); }
	void Program::BindUniform(int Index, const std::vector<PVX::iVector4D>& Value) const { glUniform4iv(Index, Value.size(), (int*)Value.data()); }
	void Program::BindUniform(int Index, const std::vector<PVX::Matrix4x4>& Value) const { glUniformMatrix4fv(Index, Value.size(), false, (float*)Value.data()); }
	void Program::BindUniform(int Index, const float* Value, int Count) const { glUniform1fv(Index, Count, Value); }
	void Program::BindUniform(int Index, const PVX::Vector2D* Value, int Count) const { glUniform2fv(Index, Count, (float*)Value); }
	void Program::BindUniform(int Index, const PVX::Vector3D* Value, int Count) const { glUniform3fv(Index, Count, (float*)Value); }
	void Program::BindUniform(int Index, const PVX::Vector4D* Value, int Count) const { glUniform4fv(Index, Count, (float*)Value); }
	void Program::BindUniform(int Index, const int* Value, int Count) const { glUniform1iv(Index, Count, Value); }
	void Program::BindUniform(int Index, const PVX::iVector2D* Value, int Count) const { glUniform2iv(Index, Count, (int*)Value); }
	void Program::BindUniform(int Index, const PVX::iVector3D* Value, int Count) const { glUniform3iv(Index, Count, (int*)Value); }
	void Program::BindUniform(int Index, const PVX::iVector4D* Value, int Count) const { glUniform4iv(Index, Count, (int*)Value); }
	void Program::BindUniform(int Index, const PVX::Matrix4x4* Value, int Count) const { glUniformMatrix4fv(Index, Count, false, (float*)Value); }

	bool Program::SetUniformBlockIndex(const char* Name, int Index) const {
		auto loc = UniformBlockIndex(Name);
		if (loc != ~0) {
			glUniformBlockBinding(Id, loc, Index);
			return true;
		}
		return false;
	}
	bool Program::SetShaderStrorageIndex(const char* Name, int Index) const {
		auto loc = glGetProgramResourceIndex(Id, GL_SHADER_STORAGE_BLOCK, Name);
		if (loc != ~0) {
			glShaderStorageBlockBinding(Id, loc, Index);
			return true;
		}
		return false;
	}
	bool Program::SetTextureIndex(const char* Name, int Index) const {
		auto loc = UniformLocation(Name);
		if (loc != ~0) {
			glProgramUniform1i(Id, loc, Index);
			return true;
		}
		return false;
	}

	Geometry::Geometry(PrimitiveType Type, const std::vector<int>& Index, const std::initializer_list<Geometry_init2>& Vertex) :
		Type{ Type },
		Indices{ Index },
		IndexCount{ int(Index.size()) }

	{
		glGenVertexArrays(1, &Id);
		glBindVertexArray(Id);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, Indices.Get());
		int i = 0;
		for (auto& v : Vertex) {
			VertexBuffers.push_back(v.Buffer);
			glBindBuffer(GL_ARRAY_BUFFER, v.Buffer.Get());
			int Stride = PVX::Reduce(v.Attributes, 0, [](int acc, const Geometry_init2_Attrib& aa) {
				return acc + aa.Count * AttribSize(aa.Type) + aa.SkipBytes;
			});
			int Offset = 0;
			for (auto& aa : v.Attributes) {
				glEnableVertexAttribArray(i);
				if (IsInt(aa.Type))
					glVertexAttribIPointer(i, aa.Count, GLenum(aa.Type), Stride, (void*)Offset);
				else
					glVertexAttribPointer(i, aa.Count, GLenum(aa.Type), 0, Stride, (void*)Offset);
				glVertexAttribDivisor(i++, aa.Instance);
				Offset += AttribSize(aa.Type) * aa.Count + aa.SkipBytes;
			}
		}
		//glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);
	}

	Geometry::Geometry(PrimitiveType Type, int IndexCount, const IndexBuffer& Indices, const std::initializer_list<Geometry_init>& Buffers, bool old) :
		IndexCount{ IndexCount },
		Type{ Type },
		Indices{ Indices }
	{
		glGenVertexArrays(1, &Id);
		glBindVertexArray(Id);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, Indices.Get());
		if (!old) {
			int i = 0;
			for (auto& b : Buffers) {
				VertexBuffers.push_back(b.Buffer);
				glBindBuffer(GL_ARRAY_BUFFER, b.Buffer.Get());
				for (auto& a: b.Attributes) {
					glEnableVertexAttribArray(i);
					if (a.IsInt)
						glVertexAttribIPointer(i++, a.Size, GLenum(a.Type), b.Stride, (void*)a.Offset);
					else
						glVertexAttribPointer(i++, a.Size, GLenum(a.Type), a.Normalized, b.Stride, (void*)a.Offset);
					if (a.Name == "Normal") Flags |= 1;
					else if (a.Name == "UV" || a.Name == "TexCoord") Flags |= 2;
					else if (a.Name == "Tangent") Flags |= 4;
					else if (a.Name == "Weights") Flags |= 8;
				}
			}
		} else {
			for (auto& b : Buffers) {
				VertexBuffers.push_back(b.Buffer);
				glBindBuffer(GL_ARRAY_BUFFER, b.Buffer.Get());
				for (auto& a: b.Attributes) {
					if (a.Name == "Position") {
						glEnableClientState(GL_VERTEX_ARRAY);
						glVertexPointer(a.Size, GLenum(a.Type), b.Stride, (void*)a.Offset);
					} else if (a.Name == "Normal") {
						glEnableClientState(GL_NORMAL_ARRAY);
						glNormalPointer(GLenum(a.Type), b.Stride, (void*)a.Offset);
						Flags |= 1;
					} else if (a.Name == "UV" || a.Name == "TexCoord") {
						glEnableClientState(GL_TEXTURE_COORD_ARRAY);
						glTexCoordPointer(a.Size, GLenum(a.Type), b.Stride, (void*)a.Offset);
						Flags |= 2;
					} else if (a.Name == "Color") {
						glEnableClientState(GL_COLOR_ARRAY);
						glColorPointer(a.Size, GLenum(a.Type), b.Stride, (void*)a.Offset);
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
	void Geometry::Draw() const {
		glBindVertexArray(Id);
		//glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, Indices.Get());
		glDrawElements(int(Type), IndexCount, GL_UNSIGNED_INT, 0);
		glBindVertexArray(0);
	}
	void Geometry::Draw(int Count) const {
		glBindVertexArray(Id);
		//glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, Indices.Get());
		glDrawElementsInstanced(int(Type), IndexCount, GL_UNSIGNED_INT, 0, Count);
		glBindVertexArray(0);
	}
	void Geometry::Bind() {
		glBindVertexArray(Id);
		//glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, Indices.Get());
	}
	void Geometry::DrawBound() {
		glDrawElements(int(Type), IndexCount, GL_UNSIGNED_INT, 0);
	}

	void Geometry::DrawBoundInstance(int i) {
		glDrawElementsInstancedBaseInstance(int(Type), IndexCount, GL_UNSIGNED_INT, 0, 1, i);
	}

	void Geometry::Unbind() {
		glBindVertexArray(0);
	}

	void ComputeProgram::Execute(const std::initializer_list<Buffer>& Buffers, uint32_t CountX, uint32_t CountY, uint32_t CountZ) {
		p.Bind();
		int i = 0;
		for (auto& b: Buffers)
			BindBuffer(i++, b);
		glDispatchCompute(CountX, CountY, CountZ);
		p.Unbind();
	}
}
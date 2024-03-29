#include <PVX_OpenGL.h>

namespace PVX::OpenGL {
	IndexBuffer::~IndexBuffer() {
		if (!Ref&&Id)
			glDeleteBuffers(1, &Id);
	}
	IndexBuffer::IndexBuffer(const unsigned int* Data, int Count) {
		GL_CHECK(glGenBuffers(1, &Id));
		Update(Data, Count);
	}
	void IndexBuffer::Update(const unsigned int* Data, int Count) {
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, Id);
		//GL_CHECK(glBufferData(GL_ELEMENT_ARRAY_BUFFER, Count * sizeof(int), Data, GL_STATIC_DRAW));
		glBufferStorage(GL_ELEMENT_ARRAY_BUFFER, Count * sizeof(int), Data, 0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	}

	VertexBuffer::~VertexBuffer() {
		if (!Ref&&Id)
			glDeleteBuffers(1, &Id);
	}
	VertexBuffer::VertexBuffer(const void* Data, int SizeInBytes) {
		GL_CHECK(glGenBuffers(1, &Id));
		Update(Data, SizeInBytes);
	}
	void VertexBuffer::Update(const void* Data, int SizeInBytes) {
		glBindBuffer(GL_ARRAY_BUFFER, Id);
		//GL_CHECK(glBufferData(GL_ARRAY_BUFFER, SizeInBytes, Data, GL_STATIC_DRAW));
		glBufferStorage(GL_ARRAY_BUFFER, SizeInBytes, Data, 0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}
	StreamingVertexShader::StreamingVertexShader() {
		Size = 4;
		glGenBuffers(1, &Id);
		int dt = 0;
		glBindBuffer(GL_ARRAY_BUFFER, Id);
		glBufferData(GL_ARRAY_BUFFER, Size, &dt, GLenum(BufferUsege::STREAM_DRAW));
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}
	void StreamingVertexShader::Update(const void* Data, size_t sz) {
		if (sz<=Size) {
			glNamedBufferSubData(Id, 0, sz, Data);
		} else {
			glNamedBufferData(Id, sz, Data, GLenum(BufferUsege::STREAM_DRAW));
			Size = sz;
		}
	}
	StreamingVertexShader::operator VertexBuffer() {
		VertexBuffer ret;
		ret.Id = Id;
		ret.Ref = Ref;
		return ret;
	}

	Buffer::Buffer() : ptr{ new Buffer_Data(), [](Buffer_Data* dt) { if (dt->Id) glDeleteBuffers(1, &(dt->Id)); delete dt; } } {}
	Buffer::Buffer(const Buffer_Data& dt) : ptr{ new Buffer_Data(dt), [](Buffer_Data* dt) { if (dt->Id) glDeleteBuffers(1, &(dt->Id)); delete dt; } } {}
	Buffer::Buffer(BufferUsege Usage) : Buffer() { ptr->Usage = Usage; }
	Buffer::Buffer(bool IsUniformBlock, BufferUsege Usage) : Buffer() {
		if (!IsUniformBlock) ptr->Type = BufferType::SHADER_STORAGE_BUFFER;
		ptr->Usage = Usage;
	}
	Buffer::Buffer(const void* Data, int Size, bool IsUniformBlock, BufferUsege Usage) : Buffer(IsUniformBlock, Usage) {
		Update(Size, Data);
	}
	Buffer::Buffer(const std::vector<unsigned char>& Data, bool IsUniformBlock, BufferUsege Usage) : Buffer(IsUniformBlock, Usage) {
		Update(int(Data.size()), Data.data());
	}
	void* Buffer::Map(MapAccess access) {
		return glMapNamedBuffer(ptr->Id, GLenum(access));
	}
	void Buffer::Unmap() {
		glUnmapNamedBuffer(ptr->Id);
	}
	void Buffer::Update(int Size, const void* Data) {
		if (this->ptr->Id) {
			if (ptr->Size == Size || (ptr->OnlyGrow && ptr->Size > Size))
				glNamedBufferSubData(ptr->Id, 0, Size, Data);
			else {
				glNamedBufferData(ptr->Id, Size, Data, GLenum(ptr->Usage));
				ptr->Size = Size;
			}
		} else {
			ptr->Size = Size;
			glGenBuffers(1, &(ptr->Id));
			glBindBuffer(GLenum(ptr->Type), ptr->Id);
			glBufferData(GLenum(ptr->Type), Size, Data, GLenum(ptr->Usage));
			glBindBuffer(GLenum(ptr->Type), 0);
		}
	}
	void Buffer::UpdateSubData(int Offset, void* Data, int Size) {
		glNamedBufferSubData(ptr->Id, Offset, Size, Data);
	}
	Buffer Buffer::MakeImmutableShaderStorage(int Size, void* Data) {
		Buffer_Data ret{
			0,
			Size,
			BufferType::SHADER_STORAGE_BUFFER,
			BufferUsege::STATIC_DRAW,
			BufferFlags::NONE
		};
		glGenBuffers(1, &ret.Id);
		glBindBuffer(GLenum(BufferType::SHADER_STORAGE_BUFFER), ret.Id);
		glBufferStorage(GLenum(BufferType::SHADER_STORAGE_BUFFER), Size, Data, 0);
		glBindBuffer(GLenum(BufferType::SHADER_STORAGE_BUFFER), 0);
		return { ret };
	}
	Buffer Buffer::MakeBuffer(BufferType Type, BufferFlags Flags, size_t SizeInBytes, const void* Data) {
		Buffer_Data ret{
			0,
			SizeInBytes,
			Type,
			BufferUsege::Unspecified,
			Flags
		};
		glCreateBuffers(1, &ret.Id);
		GL_CHECK(glNamedBufferStorage(ret.Id, SizeInBytes, Data, GLbitfield(Flags)));
		return { ret };
	}
	void Buffer::Read(void* Data) {
		glGetNamedBufferSubData(ptr->Id, 0, ptr->Size, Data);
	}
	Buffer::operator VertexBuffer() {
		VertexBuffer ret;
		ret.Id = ptr->Id;
		return ret;
	}
}
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
		GL_CHECK(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, Id));
		GL_CHECK(glBufferData(GL_ELEMENT_ARRAY_BUFFER, Count * sizeof(int), Data, GL_STATIC_DRAW));
		GL_CHECK(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));
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
		GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, Id));
		GL_CHECK(glBufferData(GL_ARRAY_BUFFER, SizeInBytes, Data, GL_STATIC_DRAW));
		GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, 0));
	}


	Buffer::~Buffer() {
		if (!Ref&&Id)
			glDeleteBuffers(1, &Id);
	}
	Buffer::Buffer() {}
	Buffer::Buffer(const void* Data, int Size, bool IsUniformBlock, BufferUsege Usage) {
		if (!IsUniformBlock)Type = GL_SHADER_STORAGE_BUFFER;
		Update(Data, Size, Usage);
	}
	Buffer::Buffer(const std::vector<unsigned char>& Data, bool IsUniformBlock, BufferUsege Usage) {
		if (!IsUniformBlock)Type = GL_SHADER_STORAGE_BUFFER;
		Update(Data.data(), int(Data.size()), Usage);
	}
	void Buffer::Update(const void* Data, int Size, BufferUsege Usage) {
		if (!Id) glGenBuffers(1, &Id);
		GL_CHECK(glBindBuffer(Type, Id));
		GL_CHECK(glNamedBufferData(Id, Size, Data, int(Usage)));
		GL_CHECK(glBindBuffer(Type, 0));
	}
	void Buffer::Update(const std::vector<unsigned char>& Data, BufferUsege Usage) {
		Update(Data.data(), int(Data.size()), Usage);
	}
}
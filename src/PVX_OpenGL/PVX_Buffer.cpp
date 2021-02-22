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


	ConstantBuffer::~ConstantBuffer() {
		if (!Ref&&Id)
			glDeleteBuffers(1, &Id);
	}
	ConstantBuffer::ConstantBuffer() {}
	ConstantBuffer::ConstantBuffer(const void* Data, int Size) {
		Update(Data, Size);
	}
	ConstantBuffer::ConstantBuffer(const std::vector<unsigned char>& Data) {
		Update(Data.data(), int(Data.size()));
	}
	void ConstantBuffer::Update(const void* Data, int Size) {
		if (!Id) glGenBuffers(1, &Id);
		glBindBuffer(GL_UNIFORM_BUFFER, Id);
		glBufferData(GL_UNIFORM_BUFFER, Size, Data, GL_STATIC_DRAW);
		glBindBuffer(GL_UNIFORM_BUFFER, 0);
	}
	void ConstantBuffer::Update(const std::vector<unsigned char>& Data) {
		Update(Data.data(), int(Data.size()));
	}
}
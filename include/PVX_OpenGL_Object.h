#ifndef _PVX_OPENGL_OBJECT_BUILDER_H__
#define _PVX_OPENGL_OBJECT_BUILDER_H__

#include <PVX_Math3D.h>
//#include <PVX_DataContainer.h>
#include <PVX_OpenGL.h>
#include <vector>

namespace PVX {
	namespace OpenGL {
		typedef struct ObjectData {
			int Mode;
			std::vector<int> Index;
			std::vector<Vector4D> Color;
			std::vector<Vector3D> Vertex;
			std::vector<Vector3D> Normal;
			std::vector<Vector2D> TexCoord;
			std::vector<Vector3D> TexCoord3D;
		} ObjectData;

		struct SimpleMatrial {
			Vector4D Diffuse = { 0.8f, 0.8f, 0.8f, 1.0f};
			Vector4D Ambient = { 0.2f, 0.2f, 0.2f, 1.0f };
			Vector4D Specular = { 0.2f, 0.2f, 0.2f, 1.0f };
			Vector4D Emission = { 0.0f, 0.0f, 0.0f, 0.0f };
			float SpecularPower = 10, Transparency = 0;
			struct {
				Texture2D Ambient;
				Texture2D Diffuse;
				Texture2D Specular;
				Texture2D Emission;
				Texture2D Bump;
			} Textures;
			void Bind(PVX::OpenGL::Context& gl) {
				//glColor3fv(Ambient.Array);
				gl.Material.Ambient(Ambient);
				gl.Material.Diffuse(Diffuse);
				gl.Material.Specular(Specular);
				gl.Material.Emissive(Emission);
				gl.Material.SpecularPower(SpecularPower);
			}
		};

		class InterleavedArrayObject {
		public:
			InterleavedArrayObject(GLenum Type, int VertexCount, Vector3D * Position, int IndexCount, int * Index, Vector3D * Normal = 0, Vector3D * UV = 0, Vector3D * Color = 0);
			int Stride = 0;
			int NormalOffset = 0;
			int ColorOffset = 0;
			int TexCoordOffset = 0;
			int TexCoordOffset3D = 0;
			int TangentOffset = 0;
			SimpleMatrial Material;
			int Mode = 0;
			std::vector<unsigned char> Data;
			std::vector<unsigned int> Index;
			void Bind();
			void Draw();
			void Unbind();
			void BindDrawUnbind();
			InterleavedArrayObject& MakeTangents();
			InterleavedArrayObject() {}
			operator Geometry();
		protected:
			std::vector<Attribute> Attributes;
			friend class BufferObject;
			friend class ObjectBuilder;
		};

		class BufferObject {
		public:
			int Mode;
			int Stride;
			int NormalOffset;
			int ColorOffset;
			int TexCoordOffset;
			int TexCoordOffset3D;
			int TangentOffset;
			int IndexCount;

			VertexBuffer Vertices;
			IndexBuffer Indices;

			void Draw();
			void Bind();
			void Unbind();
			void BindDrawUnbind();
			void BindAttributes();
			void UnbindAttributes();
			void BindAttributesDrawUnbind();
			SimpleMatrial Material;

			operator Geometry();

			BufferObject(const InterleavedArrayObject & ao);
		protected:
			std::vector<Attribute> Attributes;
			friend class Pipeline;
		};

		class ObjectBuilder {
		public:
			ObjectBuilder();
			void Color(const Vector4D& v);
			void Color(const Vector3D & v);
			void Vertex(const Vector3D & v);
			void Normal(const Vector3D & v);
			void TexCoord(const Vector2D& v);
			void TexCoord3D(const Vector3D & v);

			void Color(float r, float g, float b, float a);
			void Color(float r, float g, float b);
			void Vertex(float x, float y, float z);
			void Normal(float x, float y, float z);
			void TexCoord(float u, float v);
			void TexCoord3D(float u, float v, float w);

			void Begin(unsigned int GL_PrimitiveType);
			void End();
			void Reset();
			InterleavedArrayObject Build();
		protected:
			struct {
				Vector4D Color;
				Vector3D Normal;
				Vector2D TexCoord;
				Vector3D TexCoord3D;
			} Current;
			void(*MakePrimitive)(ObjectData&, int);
			ObjectData Data;
			int Flags, Mode, Count, Max, Min, Start;
		};

		InterleavedArrayObject MakeSphere(int segH, int segV);
		InterleavedArrayObject MakeSphereUV(int segH, int segV);
		InterleavedArrayObject MakeCube();
		InterleavedArrayObject MakeCubeWithUV();
		InterleavedArrayObject MakeCubeWithUVW();
		InterleavedArrayObject MakeCubeWithUV(int Width, int Height = 0);
		InterleavedArrayObject MakeSquareWithUV();
		InterleavedArrayObject MakeSquareWithUV_Flipped();
		InterleavedArrayObject MakeGrid(int size = 100, float dist = 1.0f);
		InterleavedArrayObject MakeAxis(float size = 1.0f);

		std::vector<InterleavedArrayObject> LoadObj(const wchar_t* fn);
	}
}

#endif
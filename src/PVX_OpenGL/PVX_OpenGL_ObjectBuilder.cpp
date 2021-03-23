#include <PVX_OpenGL_Object.h>
#include <PVX_Encrypt.h>
#include <unordered_map>
#include <PVX_String.h>
#include <PVX_File.h>

namespace PVX {
	namespace OpenGL {

#define HasNormal 2
#define HasColor 4
#define HasTex 1
#define HasTex3D 8

		static void AddQuad(ObjectData& data, int Start) {
			int i = int(data.Vertex.size()) - 4;
			data.Index.push_back(i + 0);
			data.Index.push_back(i + 1);
			data.Index.push_back(i + 2);

			data.Index.push_back(i + 0);
			data.Index.push_back(i + 2);
			data.Index.push_back(i + 3);
		}
		static void AddQuad2(ObjectData& data, int Start) {
			int i = int(data.Vertex.size()) - 4;
			data.Index.push_back(i + 1);
			data.Index.push_back(i + 0);
			data.Index.push_back(i + 2);

			data.Index.push_back(i + 1);
			data.Index.push_back(i + 2);
			data.Index.push_back(i + 3);
		}

		static void AddTriangleFan(ObjectData& data, int Start) {
			int i = int(data.Vertex.size()) - 2;
			data.Index.push_back(Start);
			data.Index.push_back(i + 0);
			data.Index.push_back(i + 1);
		}

		static void AddTriangle(ObjectData& data, int Start) {
			int i = int(data.Vertex.size()) - 1;
			data.Index.push_back(i - 2);
			data.Index.push_back(i - 1);
			data.Index.push_back(i - 0);
		}
		static void AddTriangleStrip1(ObjectData& data, int Start) {
			int i = int(data.Vertex.size()) - 1;
			data.Index.push_back(i - 3);
			data.Index.push_back(i - 1);
			data.Index.push_back(i - 0);
		}
		static void AddTriangleStrip2(ObjectData& data, int Start) {
			int i = int(data.Vertex.size()) - 1;
			data.Index.push_back(i - 1);
			data.Index.push_back(i - 2);
			data.Index.push_back(i - 0);
		}
		static void AddPoint(ObjectData& data, int Start) {
			int i = int(data.Vertex.size()) - 1;
			data.Index.push_back(i);
		}
		static void AddLine(ObjectData& data, int Start) {
			int i = int(data.Vertex.size()) - 1;
			data.Index.push_back(i - 1);
			data.Index.push_back(i);
		}
		static void AddLineStrip(ObjectData& data, int Start) {
			int i = int(data.Vertex.size()) - 1;
			data.Index.push_back(data.Index.back());
			data.Index.push_back(i);
		}
		static void AddLineLoop(ObjectData& data, int Start) {
			int i = int(data.Vertex.size()) - 1;
			data.Index[data.Index.size() - 2] = i;
			data.Index[data.Index.size() - 1] = i + 1;
			data.Index.push_back(i + 1);
			data.Index.push_back(Start);
		}

		struct AddLookupItem {
			void(*MakePrimitive)(ObjectData&, int);
			int Min, Max, Next, Mode;
		};
		static const AddLookupItem AddLookup[]{
			{ AddPoint, 0, 1, 0, GL_POINTS },					//  0 GL_POINTS
			{ AddLine, 0, 2, 1, GL_LINES },						//  1 GL_LINES
			{ AddLine, 1, 2, 14, GL_LINES },					//  2 GL_LINE_LOOP
			{ AddLine, 1, 2, 15, GL_LINES },					//  3 GL_LINE_STRIP
			{ AddTriangle, 0, 3, 4, GL_TRIANGLES },				//  4 GL_TRIANGLES
			{ AddTriangle, 2, 3, 10, GL_TRIANGLES },			//  5 GL_TRIANGLE_STRIP
			{ AddTriangleFan, 2, 3, 6, GL_TRIANGLES },			//  6 GL_TRIANGLE_FAN
			{ AddQuad, 0, 4, 7, GL_TRIANGLES },					//  7 GL_QUADS
			{ AddQuad, 2, 4, 13, GL_TRIANGLES },				//  8 GL_QUAD_STRIP
			{ NULL, 0, 0x7fffffff, 9, GL_TRIANGLES },			//  9 GL_POLYGON

			{ AddTriangleStrip1, 2, 3, 11, GL_TRIANGLES },		// 10 GL_TRIANGLE_STRIP
			{ AddTriangleStrip2, 2, 3, 12, GL_TRIANGLES },		// 11 GL_TRIANGLE_STRIP
			{ AddTriangle, 2, 3, 11, GL_TRIANGLES },			// 12 GL_TRIANGLE_STRIP
			{ AddQuad, 2, 4, 11, GL_TRIANGLES },				// 13 GL_QUAD_STRIP
			{ AddLineLoop, 1, 2, 2, GL_LINES },					// 14 GL_LINE_LOOP
			{ AddLineStrip, 1, 2, 3, GL_LINES },				// 15 GL_LINE_STRIP
		};

		ObjectBuilder::ObjectBuilder() {
			Flags = Mode = Count = Max = 0;
			memset(&Current, 0, sizeof(Current));
		}
		void ObjectBuilder::Color(const Vector4D& v) {
			Flags |= HasColor;
			Current.Color = v;
		}
		void ObjectBuilder::Color(const Vector3D& v) {
			Flags |= HasColor;
			Current.Color = { v.r, v.g, v.b, 1.0f };
		}
		void ObjectBuilder::Vertex(const Vector3D& v) {
			Data.Vertex.push_back(v);
			Data.Color.push_back(Current.Color);
			Data.Normal.push_back(Current.Normal);
			Data.TexCoord.push_back(Current.TexCoord);
			Data.TexCoord3D.push_back(Current.TexCoord3D);
			Count++;

			if (Count == Max) {
				auto& mode = AddLookup[Mode];
				mode.MakePrimitive(Data, Start);
				Mode = mode.Next;
				Count = Min;
			}
		}
		void ObjectBuilder::Normal(const Vector3D& v) {
			Flags |= HasNormal;
			Current.Normal = v;
		}
		void ObjectBuilder::TexCoord(const Vector2D& v) {
			Flags |= HasTex;
			Current.TexCoord = v;
		}

		void ObjectBuilder::TexCoord3D(const Vector3D& v) {
			Flags |= HasTex3D;
			Current.TexCoord3D = v;
		}

		void ObjectBuilder::Color(float r, float g, float b, float a) {
			Color({ r, g, b, a });
		}
		void ObjectBuilder::Color(float r, float g, float b) {
			Color({ r, g, b, 1.0f });
		}
		void ObjectBuilder::Vertex(float x, float y, float z) {
			Vertex({ x, y, z });
		}
		void ObjectBuilder::Normal(float x, float y, float z) {
			Normal({ x, y, z });
		}
		void ObjectBuilder::TexCoord(float u, float v) {
			TexCoord({ u, v });
		}

		void ObjectBuilder::TexCoord3D(float u, float v, float w) {
			TexCoord3D({ u, v, w });
		}

		void ObjectBuilder::Begin(unsigned int GL_PrimitiveType) {
			Mode = GL_PrimitiveType;
			auto& mode = AddLookup[Mode];
			Data.Mode = mode.Mode;
			Min = mode.Min;
			Max = mode.Max;
			Count = 0;
			Start = int(Data.Vertex.size());
		}
		void ObjectBuilder::End() {
			if (Max != 0x7fffffff && Count != Min) {
				Count = int(Data.Vertex.size() - Count + Min);
				Data.Vertex.resize(Count);
				Data.Color.resize(Count);
				Data.Normal.resize(Count);
				Data.TexCoord.resize(Count);
				Data.TexCoord3D.resize(Count);
			}
		}
		void ObjectBuilder::Reset() {
			Flags = Mode = Count = Max = 0;
			memset(&Current, 0, sizeof(Current));
			Data.Vertex.clear();
			Data.Color.clear();
			Data.Normal.clear();
			Data.TexCoord.clear();
			Data.TexCoord3D.clear();
		}

		static int InsertUnique(std::vector<unsigned char>& list, unsigned char* item, int stride) {
			unsigned char* dt = (unsigned char*)list.data();
			auto sz = list.size();
			size_t i;
			for (i = 0; i < sz && memcmp(dt, item, stride); i += stride, dt += stride);
			if (i == sz) {
				list.resize(sz + stride);
				memcpy(&list[i], item, stride);
			}
			return int(i / stride);
		}

		static int InsertUnique(std::vector<unsigned char>& list, std::vector<unsigned int>& Hash, unsigned char* item, int stride) {
			unsigned int itemCRC = PVX::Encrypt::CRC32_Algorithm().Update(item, stride).Get();
			unsigned char* dt = (unsigned char*)list.data();
			auto sz = list.size();
			size_t i;

			unsigned int* h = Hash.data();
			size_t hsz = Hash.size();
			for (i = 0; i < hsz; i++) {
				if ((h[i] == itemCRC) && (!memcmp(item, dt + stride*i, stride)))
					break;
			}
			if (i == hsz) {
				Hash.push_back(itemCRC);
				list.resize(sz + stride);
				memcpy(&list[i*stride], item, stride);
			}
			return int(i);
		}

		static int InsertUnique(std::vector<unsigned char>& list, std::vector<unsigned int>& Hash, unsigned char* item, unsigned int itemCRC, int stride) {
			unsigned char* dt = (unsigned char*)list.data();
			auto sz = list.size();
			size_t i;

			unsigned int* h = Hash.data();
			size_t hsz = Hash.size();
			for (i = 0; i < hsz; i++) {
				if ((h[i] == itemCRC) && (!memcmp(item, dt + stride*i, stride)))
					break;
			}
			if (i == hsz) {
				Hash.push_back(itemCRC);
				list.resize(sz + stride);
				memcpy(&list[i*stride], item, stride);
			}
			return int(i);
		}

		InterleavedArrayObject ObjectBuilder::Build() {
			using uchar = unsigned char;
			InterleavedArrayObject ret{};
			ret.Mode = AddLookup[Mode].Mode;
			ret.Stride =
				sizeof(Vector3D) +
				sizeof(Vector3D) * !!(Flags&HasNormal) +
				sizeof(Vector2D) * !!(Flags&HasTex) +
				sizeof(Vector3D) * !!(Flags&HasTex3D) +
				sizeof(Vector4D) * !!(Flags&HasColor);
			size_t sz = Data.Vertex.size();
			size_t dsz = sz * ret.Stride;
			std::vector<unsigned char> tmp(dsz);

			ret.Attributes.push_back({ false, "Position", 3, AttribType::FLOAT, 0, 0, "vec3" });

			if (ret.Stride == sizeof(Vector3D)) {
				memcpy(&tmp[0], &Data.Vertex[0], dsz);
			} else {
				unsigned char* dt = &tmp[0];
				PVX::Interleave(dt, ret.Stride, Data.Vertex.data(), sizeof(Vector3D), sz);
				int off = sizeof(Vector3D);

				if (Flags&HasNormal) {
					ret.Attributes.push_back({ false, "Normal", 3, AttribType::FLOAT, 0, off, "vec3" });
					ret.NormalOffset = off;
					PVX::Interleave(dt+off, ret.Stride, Data.Normal.data(), sizeof(Vector3D), sz);
					off += sizeof(Vector3D);
				}
				if (Flags&HasTex) {
					ret.Attributes.push_back({ false, "UV", 2, AttribType::FLOAT, 0, off, "vec2" });
					ret.TexCoordOffset = off;
					PVX::Interleave(dt+off, ret.Stride, Data.TexCoord.data(), sizeof(Vector2D), sz);
					off += sizeof(Vector2D);
				}
				if (Flags&HasTex3D) {
					ret.Attributes.push_back({ false, "UVW", 2, AttribType::FLOAT, 0, off, "vec3" });
					ret.TexCoordOffset3D = off;
					PVX::Interleave(dt+off, ret.Stride, Data.TexCoord3D.data(), sizeof(Vector3D), sz);
					off += sizeof(Vector2D);
				}
				if (Flags&HasColor) {
					ret.Attributes.push_back({ false, "Color", 4, AttribType::FLOAT, 0, off, "vec4" });
					ret.ColorOffset = off;
					PVX::Interleave(dt+off, ret.Stride, Data.Color.data(), sizeof(Vector4D), sz);
				}
			}
			{
				ret.Index.reserve(Data.Index.size());
				ret.Data.reserve(tmp.size());

				std::map<unsigned int, std::vector<int>> HashMap;

				int Count = 0;
				for (auto i : Data.Index) {
					unsigned char* dt = tmp.data() + ret.Stride * i;

					auto& similar = HashMap[PVX::Encrypt::CRC32_Algorithm().Update(dt, ret.Stride).Get()];

					for (auto s : similar) {
						if (!memcmp(dt, ret.Data.data() + ret.Stride * s, ret.Stride)) {
							ret.Index.push_back(s);
							goto cont;
						}
					}

					PVX::Append(ret.Data, dt, ret.Stride);
					ret.Index.push_back(Count);
					similar.push_back(Count++);
				cont:
					continue;
				}
				ret.Data.shrink_to_fit();
				return ret;
			}
			{
				std::vector<unsigned int> InHash(sz);
				std::map<unsigned int, int> HashMap;
				unsigned char* dt = tmp.data();
				for (auto i = 0; i < sz; i++) { InHash[i] = PVX::Encrypt::CRC32_Algorithm().Update(dt, ret.Stride).Get(); dt += ret.Stride; }
				//for (auto i = 0; i < sz; i++) HashMap[InHash[i]] = 0;
				//for (auto i = 0; i < sz; i++) HashMap[InHash[i]]++;

				//for(auto i : Data.Index) HashMap[InHash[i]] = 0;

				//for(auto i : Data.Index) HashMap[InHash[i]]++;

				std::map<unsigned int, std::vector<unsigned int>> MoreHash;

				std::vector<unsigned int> OutHash;
				OutHash.reserve(Data.Index.size());
				ret.Index.reserve(Data.Index.size());
				ret.Data.reserve(tmp.size());
				for (auto i : Data.Index) {
					unsigned int ih = InHash[i];
					if (HashMap[ih]++) {
						OutHash.push_back(ih);
						auto oldSize = ret.Data.size();
						ret.Index.push_back(ret.Data.size() / ret.Stride);
						ret.Data.resize(oldSize + ret.Stride);
						memcpy(&ret.Data[oldSize], &tmp[i * ret.Stride], ret.Stride);
						MoreHash[ih].push_back(ret.Index.back());
					} else {
						ret.Index.push_back(InsertUnique(ret.Data, OutHash, &tmp[i * ret.Stride], ih, ret.Stride));
					}
				}
				ret.Data.shrink_to_fit();
				return ret;
			}
		}

		//static void Interleave(void* dst, size_t dstStride, void* src, size_t srcStride, size_t Count) {
		//	float min = srcStride < dstStride ? srcStride : dstStride;

		//	unsigned char* Dst = (unsigned char*)dst;
		//	unsigned char* Src = (unsigned char*)src;
		//	for (int i = 0; i < Count; i++) {
		//		memcpy(Dst, Src, min);
		//		Dst += dstStride;
		//		Src += srcStride;
		//	}
		//}

		InterleavedArrayObject::InterleavedArrayObject(GLenum Mode, int VertexCount, Vector3D* Position, int IndexCount, int* Index, Vector3D* Normal, Vector3D* UV, Vector3D* Color) {
			this->Mode = Mode;
			Stride = sizeof(Vector3D);
			Attributes.push_back({ false, "Position", 3, AttribType::FLOAT, false, 0, "vec3" });
			NormalOffset = TexCoordOffset = ColorOffset = TangentOffset = 0;
			if (Normal) {
				Attributes.push_back({ false, "Normal", 3, AttribType::FLOAT, false, Stride, "vec3" });
				NormalOffset = Stride;
				Stride += sizeof(Vector3D);
			}
			if (UV) {
				Attributes.push_back({ false, "UV", 2, AttribType::FLOAT, false, Stride, "vec2" });
				TexCoordOffset = Stride;
				Stride += sizeof(Vector2D);
			}
			if (Color) {
				Attributes.push_back({ false, "Color", 4, AttribType::FLOAT, false, Stride, "vec4" });
				ColorOffset = Stride;
				Stride += sizeof(Vector4D);
			}
			Data.resize(Stride * VertexCount);
			Interleave(&Data[0], Stride, Position, sizeof(Vector3D), VertexCount);

			if (Normal) Interleave(&Data[NormalOffset], Stride, Normal, sizeof(Vector3D), VertexCount);
			if (UV) Interleave(&Data[TexCoordOffset], Stride, UV, sizeof(Vector2D), VertexCount);
			if (Color) Interleave(&Data[ColorOffset], Stride, Color, sizeof(Vector4D), VertexCount);

			this->Index.resize(IndexCount);
			memcpy(&this->Index[0], Index, 4 * IndexCount);
		}

		void InterleavedArrayObject::Bind() {
			GL_CHECK(glEnableClientState(GL_VERTEX_ARRAY));
			GL_CHECK(glVertexPointer(3, GL_FLOAT, Stride, (GLvoid*)&Data[0]));
			if (NormalOffset) {
				GL_CHECK(glEnableClientState(GL_NORMAL_ARRAY));
				GL_CHECK(glNormalPointer(GL_FLOAT, Stride, (GLvoid*)&Data[NormalOffset]));
			}
			if (TexCoordOffset) {
				GL_CHECK(glEnableClientState(GL_TEXTURE_COORD_ARRAY));
				GL_CHECK(glTexCoordPointer(2, GL_FLOAT, Stride, (GLvoid*)&Data[TexCoordOffset]));
			}
			if (TexCoordOffset3D) {
				GL_CHECK(glEnableClientState(GL_TEXTURE_COORD_ARRAY));
				GL_CHECK(glTexCoordPointer(3, GL_FLOAT, Stride, (GLvoid*)&Data[TexCoordOffset3D]));
			}
			if (ColorOffset) {
				glEnableClientState(GL_COLOR_ARRAY);
				glColorPointer(4, GL_FLOAT, Stride, (GLvoid*)&Data[ColorOffset]);
			}
		}
		void InterleavedArrayObject::Draw() {
			glDrawElements(Mode, Index.size(), GL_UNSIGNED_INT, &Index[0]);
		}
		void InterleavedArrayObject::Unbind() {
			GL_CHECK(glDisableClientState(GL_VERTEX_ARRAY));
			if (NormalOffset) GL_CHECK(glDisableClientState(GL_NORMAL_ARRAY));
			if (TexCoordOffset || TexCoordOffset3D)
				GL_CHECK(glDisableClientState(GL_TEXTURE_COORD_ARRAY));
			if (ColorOffset) glDisableClientState(GL_COLOR_ARRAY);
		}
		void InterleavedArrayObject::BindDrawUnbind() {
			Bind(); Draw(); Unbind();
		}

		InterleavedArrayObject& InterleavedArrayObject::MakeTangents() {
			if (!(NormalOffset&&TexCoordOffset))return *this;
			std::vector<Vector3D> Vertex;
			std::vector<Vector2D> UV;
			std::vector<Vector3D> Normal;
			std::vector<Vector4D> Color;
			std::vector<Vector4D> Tangent;
			std::vector<Vector3D> BiTangent;
			int vCount = Data.size() / Stride;

			Vertex.resize(vCount);
			UV.resize(vCount);
			Normal.resize(vCount);
			Tangent.resize(vCount);
			BiTangent.resize(vCount);
			memset(&Tangent[0], 0, sizeof(Vector4D) * vCount);
			memset(&BiTangent[0], 0, sizeof(Vector3D) * vCount);


			Interleave(&Vertex[0], sizeof(Vector3D), &Data[0], Stride, vCount);
			Interleave(&UV[0], sizeof(Vector2D), &Data[TexCoordOffset], Stride, vCount);
			Interleave(&Normal[0], sizeof(Vector3D), &Data[NormalOffset], Stride, vCount);
			if (ColorOffset) {
				Color.resize(vCount);
				Interleave(&Color[0], sizeof(Vector4D), &Data[ColorOffset], Stride, vCount);
			}

			Triangle* tris = (Triangle*)&Index[0];
			int TriCount = Index.size() / 3;

			for (int i = 0; i < TriCount; i++) {
				long i1 = tris[i].Index[0];
				long i2 = tris[i].Index[1];
				long i3 = tris[i].Index[2];

				const Vector3D& v1 = Vertex[i1];
				const Vector3D& v2 = Vertex[i2];
				const Vector3D& v3 = Vertex[i3];

				const Vector2D& w1 = UV[i1];
				const Vector2D& w2 = UV[i2];
				const Vector2D& w3 = UV[i3];

				float x1 = v2.x - v1.x;
				float y1 = v2.y - v1.y;
				float z1 = v2.z - v1.z;

				float x2 = v3.x - v1.x;
				float y2 = v3.y - v1.y;
				float z2 = v3.z - v1.z;

				float s1 = w2.x - w1.x;
				float t1 = w2.y - w1.y;

				float s2 = w3.x - w1.x;
				float t2 = w3.y - w1.y;

				float r = 1.0F / (s1 * t2 - s2 * t1);
				Vector3D sdir{
					(t2 * x1 - t1 * x2) * r,
					(t2 * y1 - t1 * y2) * r,
					(t2 * z1 - t1 * z2) * r
				};
				Vector3D tdir{
					(s1 * x2 - s2 * x1) * r,
					(s1 * y2 - s2 * y1) * r,
					(s1 * z2 - s2 * z1) * r
				};

				Tangent[i1].Vec3 += sdir;
				Tangent[i2].Vec3 += sdir;
				Tangent[i3].Vec3 += sdir;

				BiTangent[i1] += tdir;
				BiTangent[i2] += tdir;
				BiTangent[i3] += tdir;
			}

			for (int i = 0; i < vCount; i++) {
				const Vector3D& n = Normal[i];
				const Vector3D& t = Tangent[i].Vec3;

				// Gram-Schmidt orthogonalize
				Tangent[i].Vec3 = (t - n * n.Dot(t)).Normalized();

				// Calculate handedness
				Tangent[i].w = (n.Cross(t).Dot(BiTangent[i]) < 0.0F) ? -1.0f : 1.0f;
			}

			Data.resize((Stride + sizeof(Vector4D)) * vCount);
			Attributes.clear();

			Attributes.push_back({ false, "Position", 3, AttribType::FLOAT, false, 0, "vec3" });
			Stride = sizeof(Vector3D);

			Attributes.push_back({ false, "Normal", 3, AttribType::FLOAT, false, Stride, "vec3" });
			NormalOffset = Stride;
			Stride += sizeof(Vector3D);

			Attributes.push_back({ false, "UV", 2, AttribType::FLOAT, false, Stride, "vec2" });
			TexCoordOffset = Stride;
			Stride += sizeof(Vector2D);

			if (ColorOffset) {
				Attributes.push_back({ false, "Color", 4, AttribType::FLOAT, false, Stride, "vec4" });
				ColorOffset = Stride;
				Stride += sizeof(Vector4D);
			}

			Attributes.push_back({ false, "Tangent", 4, AttribType::FLOAT, false, Stride, "vec4" });
			TangentOffset = Stride;
			Stride += sizeof(Vector4D);

			Interleave(&Data[0], Stride, &Vertex[0], sizeof(Vector3D), vCount);
			Interleave(&Data[TexCoordOffset], Stride, &UV[0], sizeof(Vector2D), vCount);
			Interleave(&Data[NormalOffset], Stride, &Normal[0], sizeof(Vector3D), vCount);
			Interleave(&Data[TangentOffset], Stride, &Tangent[0], sizeof(Vector4D), vCount);
			if (ColorOffset)
				Interleave(&Data[ColorOffset], Stride, &Color[0], sizeof(Vector4D), vCount);
			return *this;
		}

		InterleavedArrayObject::operator Geometry() {
			return {
				PVX::OpenGL::PrimitiveType(Mode),
				int(Index.size()),
				Index,
				{
					{
						Data,
						Attributes,
						Stride
					}
				}
			};
		}

		BufferObject::operator Geometry() {
			return {
				PVX::OpenGL::PrimitiveType(Mode),
				IndexCount,
				Indices,
				{
					{
						Vertices,
						Attributes,
						Stride
					}
				}
			};
		}

		BufferObject::BufferObject(const InterleavedArrayObject& ao) :
			Vertices{ ao.Data.data(), ao.Data.size() },
			Indices{ ao.Index.data(), ao.Index.size() } {
			Mode = ao.Mode;
			Stride = ao.Stride;
			NormalOffset = ao.NormalOffset;
			ColorOffset = ao.ColorOffset;
			TexCoordOffset = ao.TexCoordOffset;
			TexCoordOffset3D = ao.TexCoordOffset3D;
			TangentOffset = ao.TangentOffset;
			IndexCount = ao.Index.size();
			for (auto& i : ao.Attributes)
				Attributes.push_back(i);

			Material = ao.Material;
		}

		void BufferObject::Bind() {
			GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, Vertices.Get()));
			GL_CHECK(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, Indices.Get()));
			GL_CHECK(glEnableClientState(GL_VERTEX_ARRAY));
			GL_CHECK(glVertexPointer(3, GL_FLOAT, Stride, (GLvoid*)0));
			if (NormalOffset) {
				GL_CHECK(glEnableClientState(GL_NORMAL_ARRAY));
				GL_CHECK(glNormalPointer(GL_FLOAT, Stride, (GLvoid*)NormalOffset));
			}
			if (TexCoordOffset) {
				GL_CHECK(glEnableClientState(GL_TEXTURE_COORD_ARRAY));
				GL_CHECK(glTexCoordPointer(2, GL_FLOAT, Stride, (GLvoid*)TexCoordOffset));
			}
			if (TexCoordOffset3D) {
				GL_CHECK(glEnableClientState(GL_TEXTURE_COORD_ARRAY));
				GL_CHECK(glTexCoordPointer(3, GL_FLOAT, Stride, (GLvoid*)TexCoordOffset3D));
			}
			if (ColorOffset) {
				GL_CHECK(glEnableClientState(GL_COLOR_ARRAY));
				GL_CHECK(glColorPointer(4, GL_FLOAT, Stride, (GLvoid*)ColorOffset));
			}
		}
		void BufferObject::Draw() {
			GL_CHECK(glDrawElements(Mode, IndexCount, GL_UNSIGNED_INT, 0));
		}
		void BufferObject::Unbind() {
			GL_CHECK(glDisableClientState(GL_VERTEX_ARRAY));
			if (NormalOffset)
				GL_CHECK(glDisableClientState(GL_NORMAL_ARRAY));

			if (TexCoordOffset | TexCoordOffset3D)
				GL_CHECK(glDisableClientState(GL_TEXTURE_COORD_ARRAY));

			if (ColorOffset)
				GL_CHECK(glDisableClientState(GL_COLOR_ARRAY));
			GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, 0));
			GL_CHECK(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));
		}
		void BufferObject::BindDrawUnbind() {
			if (IndexCount) {
				Bind(); Draw(); Unbind();
			}
		}

		void BufferObject::BindAttributes() {
			GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, Vertices.Get()));
			GL_CHECK(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, Indices.Get()));
			int i = 0;
			for (auto& a : Attributes) {
				glEnableVertexAttribArray(i);
				glVertexAttribPointer(i++, a.Size, GLenum(a.Type), a.Normalized, Stride, (void*)a.Offset);
			}
		}

		void BufferObject::UnbindAttributes() {
			int i = 0;
			for (auto& a : Attributes)
				glDisableVertexAttribArray(i++);
			GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, 0));
			GL_CHECK(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));
		}

		void BufferObject::BindAttributesDrawUnbind() {
			if (IndexCount) {
				BindAttributes();
				Draw();
				UnbindAttributes();
			}
		}
		void BufferObject::BindAttributesDrawUnbind2() {
			glBindVertexArray(VBO);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, Indices.Get());
			glDrawElements(Mode, IndexCount, GL_UNSIGNED_INT, 0);
			glBindVertexArray(0);
		}


		void BufferObject::MakeVBO() {
			glGenVertexArrays(1, &VBO);
			glBindVertexArray(VBO);
			glBindBuffer(GL_ARRAY_BUFFER, Vertices.Get());
			int i = 0;
			for (auto& a : Attributes) {
				glEnableVertexAttribArray(i);
				glVertexAttribPointer(i++, a.Size, GLenum(a.Type), a.Normalized, Stride, (void*)a.Offset);
			}
			glBindVertexArray(0);
		}

		InterleavedArrayObject MakeAxis(float size) {
			PVX::OpenGL::ObjectBuilder ob;
			ob.Begin(GL_LINES);

			ob.Color(1.0f, 0, 0);
			ob.Vertex(0, 0, 0);
			ob.Vertex(size, 0, 0);

			ob.Color(0, 0, 1.0);
			ob.Vertex(0, 0, 0);
			ob.Vertex(0, 0, size);

			ob.Color(1.0, 1.0f, 0);
			ob.Vertex(0, 0, 0);
			ob.Vertex(0, size, 0);

			ob.End();
			return ob.Build();
		}

		InterleavedArrayObject MakeGrid(int size, float dist) {
			PVX::OpenGL::ObjectBuilder ob;
			float maxsz = dist * size;
			float cur = -maxsz;
			ob.Begin(GL_LINES);
			for (int i = -size; i < size; i++) {
				if (i) ob.Color(0.5f, 0.5f, 0.5f, 1.0f);
				else ob.Color(1.0f, 1.0f, 1.0f, 1.0f);

				ob.Vertex(maxsz, 0, cur);
				ob.Vertex(-maxsz, 0, cur);

				ob.Vertex(cur, 0, maxsz);
				ob.Vertex(cur, 0, -maxsz);

				cur += dist;
			}
			ob.End();
			return ob.Build();
		}

		InterleavedArrayObject MakeCube() {
			PVX::OpenGL::ObjectBuilder ob;
			ob.Begin(GL_QUADS); {
				ob.Normal({ 0, 0, 1.0f });
				ob.Vertex({ -1.0f,  1.0f,  1.0f });
				ob.Vertex({ -1.0f, -1.0f,  1.0f });
				ob.Vertex({ 1.0f, -1.0f,  1.0f });
				ob.Vertex({ 1.0f,  1.0f,  1.0f });

				ob.Normal({ 0, 0, -1.0f });
				ob.Vertex({ 1.0f,  1.0f,  -1.0f });
				ob.Vertex({ 1.0f, -1.0f,  -1.0f });
				ob.Vertex({ -1.0f, -1.0f, -1.0f });
				ob.Vertex({ -1.0f,  1.0f, -1.0f });

				ob.Normal({ 0.0f,  1.0f,  0.0f });
				ob.Vertex({ 1.0f,  1.0f,  1.0f });
				ob.Vertex({ 1.0f,  1.0f, -1.0f });
				ob.Vertex({ -1.0f,  1.0f, -1.0f });
				ob.Vertex({ -1.0f,  1.0f,  1.0f });

				ob.Normal({ 0.0f,  -1.0f,  0.0f });
				ob.Vertex({ -1.0f, -1.0f,  1.0f });
				ob.Vertex({ -1.0f, -1.0f, -1.0f });
				ob.Vertex({ 1.0f,  -1.0f, -1.0f });
				ob.Vertex({ 1.0f,  -1.0f,  1.0f });

				ob.Normal({ 1.0f,  0.0f,  0.0f });
				ob.Vertex({ 1.0f,  1.0f,  1.0f });
				ob.Vertex({ 1.0f, -1.0f,  1.0f });
				ob.Vertex({ 1.0f, -1.0f, -1.0f });
				ob.Vertex({ 1.0f,  1.0f, -1.0f });

				ob.Normal({ -1.0f,  0.0f,  0.0f });
				ob.Vertex({ -1.0f,  1.0f, -1.0f });
				ob.Vertex({ -1.0f, -1.0f, -1.0f });
				ob.Vertex({ -1.0f, -1.0f,  1.0f });
				ob.Vertex({ -1.0f,  1.0f,  1.0f });
			}ob.End();

			return ob.Build();
		}

		InterleavedArrayObject MakeSphere(int segH, int segV) {

			std::vector<PVX::Vector3D> Verts((segV - 1) * (segH + 1) + 2);
			Verts[0] = { 0, 1.0f, 0 };
			Verts[1] = { 0, -1.0f, 0 };

			float step1 = PI / segV;
			float step2 = 2.0 * PI / segH;

			float curV = step1;
			float curU = 0;
			int k = 2;
			for (int i = 1; i < segV; i++) {
				curU = 0;
				float r = sinf(curV);
				float y = cosf(curV);
				float v = i * (1.0f / (segV));
				for (int j = 0; j < segH; j++) {
					Verts[k] = { r * cosf(curU), y, r * sinf(curU) };
					curU += step2;
					k++;
				}
				Verts[k] = { r, y, 0 };
				curV += step1;
				k++;
			}
			ObjectBuilder gl;
			gl.Begin(GL_TRIANGLES);
			for (int i = 0; i < segH; i++) {
				auto& v1 = Verts[0];
				auto& v2 = Verts[i + 2];
				auto& v3 = Verts[i + 3];

				auto& v4 = Verts[i + 2 + (segH + 1) * (segV - 2)];
				auto& v5 = Verts[i + 3 + (segH + 1) * (segV - 2)];
				auto& v6 = Verts[1];

				gl.Normal(v1);
				gl.Vertex(v1);

				gl.Normal(v2);
				gl.Vertex(v2);

				gl.Normal(v3);
				gl.Vertex(v3);


				gl.Normal(v4);
				gl.Vertex(v4);

				gl.Normal(v5);
				gl.Vertex(v5);

				gl.Normal(v6);
				gl.Vertex(v6);
			}

			for (int i = 0; i < segV - 2; i++) {
				for (int j = 0; j < segH; j++) {
					auto& v1 = Verts[2 + i * (segH + 1) + j];
					auto& v2 = Verts[2 + i * (segH + 1) + j + 1];
					auto& v3 = Verts[2 + i * (segH + 1) + j + (segH + 1)];
					auto& v4 = Verts[2 + i * (segH + 1) + j + (segH + 1)];
					auto& v5 = Verts[2 + i * (segH + 1) + j + 1];
					auto& v6 = Verts[2 + i * (segH + 1) + j + 1 + (segH + 1)];

					gl.Normal(v1);
					gl.Vertex(v1);

					gl.Normal(v2);
					gl.Vertex(v2);

					gl.Normal(v3);
					gl.Vertex(v3);


					gl.Normal(v4);
					gl.Vertex(v4);

					gl.Normal(v5);
					gl.Vertex(v5);

					gl.Normal(v6);
					gl.Vertex(v6);
				}
			}
			gl.End();
			return gl.Build();
		}


		InterleavedArrayObject MakeSphereUV(int segH, int segV) {
			struct FullVert { Vector3D pos; Vector2D UV; };
			std::vector<FullVert> Verts((segV - 1)* (segH + 1) + 2);

			Verts[0].pos = { 0, 1.0f, 0 };
			Verts[0].UV = { 0.5f, 0.0f };
			Verts[1].pos = { 0, -1.0f, 0 };
			Verts[1].UV = { 0.5f, 1.0f };

			float step1 = PI / segV;
			float step2 = 2.0 * PI / segH;

			float curV = step1;
			float curU = 0;
			int k = 2;
			for (int i = 1; i < segV; i++) {
				curU = 0;
				float r = sinf(curV);
				float y = cosf(curV);
				float v = i * (1.0f / (segV));
				for (int j = 0; j < segH; j++) {
					Verts[k].pos = { r * cosf(curU), y, r * sinf(curU) };
					Verts[k].UV = { j / (float)(segH), v };
					curU += step2;
					k++;
				}
				Verts[k].pos = { r, y, 0 };
				Verts[k].UV = { 1.0f, v };
				curV += step1;
				k++;
			}
			ObjectBuilder gl;
			gl.Begin(GL_TRIANGLES);
			for (int i = 0; i < segH; i++) {
				auto& v1 = Verts[0];
				auto& v2 = Verts[i + 2];
				auto& v3 = Verts[i + 3];

				auto& v4 = Verts[i + 2 + (segH + 1) * (segV - 2)];
				auto& v5 = Verts[i + 3 + (segH + 1) * (segV - 2)];
				auto& v6 = Verts[1];

				gl.TexCoord({ 0.5f / segH + i / (float)segH, 0 });
				//gl.TexCoord(v1.UV);
				gl.Normal(v1.pos);
				gl.Vertex(v1.pos);

				gl.TexCoord(v2.UV);
				gl.Normal(v2.pos);
				gl.Vertex(v2.pos);

				gl.TexCoord(v3.UV);
				gl.Normal(v3.pos);
				gl.Vertex(v3.pos);



				gl.TexCoord(v4.UV);
				gl.Normal(v4.pos);
				gl.Vertex(v4.pos);

				gl.TexCoord(v5.UV);
				gl.Normal(v5.pos);
				gl.Vertex(v5.pos);

				gl.TexCoord({ 0.5f / segH + i / (float)segH, 1.0f });
				//gl.TexCoord(v6.UV);
				gl.Normal(v6.pos);
				gl.Vertex(v6.pos);
			}

			for (int i = 0; i < segV - 2; i++) {
				for (int j = 0; j < segH; j++) {
					auto& v1 = Verts[2 + i * (segH + 1) + j];
					auto& v2 = Verts[2 + i * (segH + 1) + j + 1];
					auto& v3 = Verts[2 + i * (segH + 1) + j + (segH + 1)];
					auto& v4 = Verts[2 + i * (segH + 1) + j + (segH + 1)];
					auto& v5 = Verts[2 + i * (segH + 1) + j + 1];
					auto& v6 = Verts[2 + i * (segH + 1) + j + 1 + (segH + 1)];



					gl.TexCoord(v1.UV);
					gl.Normal(v1.pos);
					gl.Vertex(v1.pos);

					gl.TexCoord(v2.UV);
					gl.Normal(v2.pos);
					gl.Vertex(v2.pos);

					gl.TexCoord(v3.UV);
					gl.Normal(v3.pos);
					gl.Vertex(v3.pos);



					gl.TexCoord(v4.UV);
					gl.Normal(v4.pos);
					gl.Vertex(v4.pos);

					gl.TexCoord(v5.UV);
					gl.Normal(v5.pos);
					gl.Vertex(v5.pos);

					gl.TexCoord(v6.UV);
					gl.Normal(v6.pos);
					gl.Vertex(v6.pos);
				}
			}
			gl.End();
			return gl.Build();
		}

		InterleavedArrayObject MakeCubeWithUV() {
			PVX::OpenGL::ObjectBuilder ob;
			ob.Begin(GL_QUADS); {
				Vector2D Base = { 0, 0 };
				ob.Normal({ 0, 0, 1.0f });
				ob.TexCoord(Base + Vector2D{ 1.0f / 3.0f, 0.0f });
				ob.Vertex({ -1.0f,  1.0f,  1.0f });
				ob.TexCoord(Base + Vector2D{ 1.0f / 3.0f, 0.5f });
				ob.Vertex({ -1.0f, -1.0f,  1.0f });
				ob.TexCoord(Base + Vector2D{ 0.0f / 3.0f, 0.5f });
				ob.Vertex({ 1.0f, -1.0f,  1.0f });
				ob.TexCoord(Base + Vector2D{ 0.0f / 3.0f, 0.0f });
				ob.Vertex({ 1.0f,  1.0f,  1.0f });

				Base = { 1.0f / 3.0f, 0 };
				ob.Normal({ -1.0f,  0.0f,  0.0f });
				ob.TexCoord(Base + Vector2D{ 1.0f / 3.0f, 0.0f });
				ob.Vertex({ -1.0f,  1.0f, -1.0f });
				ob.TexCoord(Base + Vector2D{ 1.0f / 3.0f, 0.5f });
				ob.Vertex({ -1.0f, -1.0f, -1.0f });
				ob.TexCoord(Base + Vector2D{ 0.0f / 3.0f, 0.5f });
				ob.Vertex({ -1.0f, -1.0f,  1.0f });
				ob.TexCoord(Base + Vector2D{ 0.0f / 3.0f, 0.0f });
				ob.Vertex({ -1.0f,  1.0f,  1.0f });

				Base = { 2.0f / 3.0f, 0 };
				ob.Normal({ 0, 0, -1.0f });
				ob.TexCoord(Base + Vector2D{ 1.0f / 3.0f, 0.0f });
				ob.Vertex({ 1.0f,  1.0f,  -1.0f });
				ob.TexCoord(Base + Vector2D{ 1.0f / 3.0f, 0.5f });
				ob.Vertex({ 1.0f, -1.0f,  -1.0f });
				ob.TexCoord(Base + Vector2D{ 0.0f / 3.0f, 0.5f });
				ob.Vertex({ -1.0f, -1.0f, -1.0f });
				ob.TexCoord(Base + Vector2D{ 0.0f / 3.0f, 0.0f });
				ob.Vertex({ -1.0f,  1.0f, -1.0f });

				Base = { 0, 0.5f };
				ob.Normal({ 1.0f,  0.0f,  0.0f });
				ob.TexCoord(Base + Vector2D{ 1.0f / 3.0f, 0.0f });
				ob.Vertex({ 1.0f,  1.0f,  1.0f });
				ob.TexCoord(Base + Vector2D{ 1.0f / 3.0f, 0.5f });
				ob.Vertex({ 1.0f, -1.0f,  1.0f });
				ob.TexCoord(Base + Vector2D{ 0.0f / 3.0f, 0.5f });
				ob.Vertex({ 1.0f, -1.0f, -1.0f });
				ob.TexCoord(Base + Vector2D{ 0.0f / 3.0f, 0.0f });
				ob.Vertex({ 1.0f,  1.0f, -1.0f });

				Base = { 1.0f / 3.0f, 0.5f };
				ob.Normal({ 0.0f,  -1.0f,  0.0f });
				ob.TexCoord(Base + Vector2D{ 1.0f / 3.0f, 0.0f });
				ob.Vertex({ -1.0f, -1.0f,  1.0f });
				ob.TexCoord(Base + Vector2D{ 1.0f / 3.0f, 0.5f });
				ob.Vertex({ -1.0f, -1.0f, -1.0f });
				ob.TexCoord(Base + Vector2D{ 0.0f / 3.0f, 0.5f });
				ob.Vertex({ 1.0f,  -1.0f, -1.0f });
				ob.TexCoord(Base + Vector2D{ 0.0f / 3.0f, 0.0f });
				ob.Vertex({ 1.0f,  -1.0f,  1.0f });

				Base = { 2.0f / 3.0f, 0.5f };
				ob.Normal({ 0.0f,  1.0f,  0.0f });
				ob.TexCoord(Base + Vector2D{ 0.0f / 3.0f, 0.5f });
				ob.Vertex({ 1.0f,  1.0f,  1.0f });
				ob.TexCoord(Base + Vector2D{ 0.0f / 3.0f, 0.0f });
				ob.Vertex({ 1.0f,  1.0f, -1.0f });
				ob.TexCoord(Base + Vector2D{ 1.0f / 3.0f, 0.0f });
				ob.Vertex({ -1.0f,  1.0f, -1.0f });
				ob.TexCoord(Base + Vector2D{ 1.0f / 3.0f, 0.5f });
				ob.Vertex({ -1.0f,  1.0f,  1.0f });
			}ob.End();

			return ob.Build();
		}

		InterleavedArrayObject MakeCubeWithUVW() {
			PVX::OpenGL::ObjectBuilder ob;
			ob.Begin(GL_QUADS); {
				ob.TexCoord3D({ -1.0f,  1.0f,  1.0f }); ob.Vertex({ -1.0f,  1.0f,  1.0f });
				ob.TexCoord3D({ -1.0f, -1.0f,  1.0f }); ob.Vertex({ -1.0f, -1.0f,  1.0f });
				ob.TexCoord3D({ 1.0f, -1.0f,  1.0f }); ob.Vertex({ 1.0f, -1.0f,  1.0f });
				ob.TexCoord3D({ 1.0f,  1.0f,  1.0f }); ob.Vertex({ 1.0f,  1.0f,  1.0f });

				ob.TexCoord3D({ -1.0f,  1.0f, -1.0f }); ob.Vertex({ -1.0f,  1.0f, -1.0f });
				ob.TexCoord3D({ -1.0f, -1.0f, -1.0f }); ob.Vertex({ -1.0f, -1.0f, -1.0f });
				ob.TexCoord3D({ -1.0f, -1.0f,  1.0f }); ob.Vertex({ -1.0f, -1.0f,  1.0f });
				ob.TexCoord3D({ -1.0f,  1.0f,  1.0f }); ob.Vertex({ -1.0f,  1.0f,  1.0f });

				ob.TexCoord3D({ 1.0f,  1.0f, -1.0f }); ob.Vertex({ 1.0f,  1.0f, -1.0f });
				ob.TexCoord3D({ 1.0f, -1.0f, -1.0f }); ob.Vertex({ 1.0f, -1.0f, -1.0f });
				ob.TexCoord3D({ -1.0f, -1.0f, -1.0f }); ob.Vertex({ -1.0f, -1.0f, -1.0f });
				ob.TexCoord3D({ -1.0f,  1.0f, -1.0f }); ob.Vertex({ -1.0f,  1.0f, -1.0f });

				ob.TexCoord3D({ 1.0f,  1.0f,  1.0f }); ob.Vertex({ 1.0f,  1.0f,  1.0f });
				ob.TexCoord3D({ 1.0f, -1.0f,  1.0f }); ob.Vertex({ 1.0f, -1.0f,  1.0f });
				ob.TexCoord3D({ 1.0f, -1.0f, -1.0f }); ob.Vertex({ 1.0f, -1.0f, -1.0f });
				ob.TexCoord3D({ 1.0f,  1.0f, -1.0f }); ob.Vertex({ 1.0f,  1.0f, -1.0f });

				ob.TexCoord3D({ -1.0f, -1.0f,  1.0f }); ob.Vertex({ -1.0f, -1.0f,  1.0f });
				ob.TexCoord3D({ -1.0f, -1.0f, -1.0f }); ob.Vertex({ -1.0f, -1.0f, -1.0f });
				ob.TexCoord3D({ 1.0f, -1.0f, -1.0f }); ob.Vertex({ 1.0f, -1.0f, -1.0f });
				ob.TexCoord3D({ 1.0f, -1.0f,  1.0f }); ob.Vertex({ 1.0f, -1.0f,  1.0f });

				ob.TexCoord3D({ 1.0f,  1.0f,  1.0f }); ob.Vertex({ 1.0f,  1.0f,  1.0f });
				ob.TexCoord3D({ 1.0f,  1.0f, -1.0f }); ob.Vertex({ 1.0f,  1.0f, -1.0f });
				ob.TexCoord3D({ -1.0f,  1.0f, -1.0f }); ob.Vertex({ -1.0f,  1.0f, -1.0f });
				ob.TexCoord3D({ -1.0f,  1.0f,  1.0f }); ob.Vertex({ -1.0f,  1.0f,  1.0f });
			}ob.End();

			return ob.Build();
		}

		InterleavedArrayObject MakeCubeWithUVWandNormals() {
			PVX::OpenGL::ObjectBuilder ob;
			ob.Begin(GL_QUADS); {
				ob.Normal({ 0, 0, 1.0f });
				ob.TexCoord3D({ -1.0f,  1.0f,  1.0f }); ob.Vertex({ -1.0f,  1.0f,  1.0f });
				ob.TexCoord3D({ -1.0f, -1.0f,  1.0f }); ob.Vertex({ -1.0f, -1.0f,  1.0f });
				ob.TexCoord3D({ 1.0f, -1.0f,  1.0f }); ob.Vertex({ 1.0f, -1.0f,  1.0f });
				ob.TexCoord3D({ 1.0f,  1.0f,  1.0f }); ob.Vertex({ 1.0f,  1.0f,  1.0f });

				ob.Normal({ -1.0f,  0.0f,  0.0f });
				ob.TexCoord3D({ -1.0f,  1.0f, -1.0f }); ob.Vertex({ -1.0f,  1.0f, -1.0f });
				ob.TexCoord3D({ -1.0f, -1.0f, -1.0f }); ob.Vertex({ -1.0f, -1.0f, -1.0f });
				ob.TexCoord3D({ -1.0f, -1.0f,  1.0f }); ob.Vertex({ -1.0f, -1.0f,  1.0f });
				ob.TexCoord3D({ -1.0f,  1.0f,  1.0f }); ob.Vertex({ -1.0f,  1.0f,  1.0f });

				ob.Normal({ 0, 0, -1.0f });
				ob.TexCoord3D({ 1.0f,  1.0f, -1.0f }); ob.Vertex({ 1.0f,  1.0f, -1.0f });
				ob.TexCoord3D({ 1.0f, -1.0f, -1.0f }); ob.Vertex({ 1.0f, -1.0f, -1.0f });
				ob.TexCoord3D({ -1.0f, -1.0f, -1.0f }); ob.Vertex({ -1.0f, -1.0f, -1.0f });
				ob.TexCoord3D({ -1.0f,  1.0f, -1.0f }); ob.Vertex({ -1.0f,  1.0f, -1.0f });

				ob.Normal({ 1.0f,  0.0f,  0.0f });
				ob.TexCoord3D({ 1.0f,  1.0f,  1.0f }); ob.Vertex({ 1.0f,  1.0f,  1.0f });
				ob.TexCoord3D({ 1.0f, -1.0f,  1.0f }); ob.Vertex({ 1.0f, -1.0f,  1.0f });
				ob.TexCoord3D({ 1.0f, -1.0f, -1.0f }); ob.Vertex({ 1.0f, -1.0f, -1.0f });
				ob.TexCoord3D({ 1.0f,  1.0f, -1.0f }); ob.Vertex({ 1.0f,  1.0f, -1.0f });

				ob.Normal({ 0.0f,  -1.0f,  0.0f });
				ob.TexCoord3D({ -1.0f, -1.0f,  1.0f }); ob.Vertex({ -1.0f, -1.0f,  1.0f });
				ob.TexCoord3D({ -1.0f, -1.0f, -1.0f }); ob.Vertex({ -1.0f, -1.0f, -1.0f });
				ob.TexCoord3D({ 1.0f, -1.0f, -1.0f }); ob.Vertex({ 1.0f, -1.0f, -1.0f });
				ob.TexCoord3D({ 1.0f, -1.0f,  1.0f }); ob.Vertex({ 1.0f, -1.0f,  1.0f });

				ob.Normal({ 0.0f,  1.0f,  0.0f });
				ob.TexCoord3D({ 1.0f,  1.0f,  1.0f }); ob.Vertex({ 1.0f,  1.0f,  1.0f });
				ob.TexCoord3D({ 1.0f,  1.0f, -1.0f }); ob.Vertex({ 1.0f,  1.0f, -1.0f });
				ob.TexCoord3D({ -1.0f,  1.0f, -1.0f }); ob.Vertex({ -1.0f,  1.0f, -1.0f });
				ob.TexCoord3D({ -1.0f,  1.0f,  1.0f }); ob.Vertex({ -1.0f,  1.0f,  1.0f });
			}ob.End();

			return ob.Build();
		}

		InterleavedArrayObject MakeCubeWithUV(int Width, int Height) {
			if (!Height) Height = Width;
			float pixelWidth = 1.0f / (Width * 2);
			float pixelHeight = 1.0f / (Height * 2);

			PVX::OpenGL::ObjectBuilder ob;
			ob.Begin(GL_QUADS); {
				Vector2D Base = { 0, 0 };
				ob.Normal({ 0, 0, 1.0f });
				ob.TexCoord(Base + Vector2D{ 1.0f / 3.0f - pixelWidth, 0.0f + pixelHeight });
				ob.Vertex({ -1.0f,  1.0f,  1.0f });
				ob.TexCoord(Base + Vector2D{ 1.0f / 3.0f - pixelWidth, 0.5f - pixelHeight });
				ob.Vertex({ -1.0f, -1.0f,  1.0f });
				ob.TexCoord(Base + Vector2D{ 0.0f / 3.0f + pixelWidth, 0.5f - pixelHeight });
				ob.Vertex({ 1.0f, -1.0f,  1.0f });
				ob.TexCoord(Base + Vector2D{ 0.0f / 3.0f + pixelWidth, 0.0f + pixelHeight });
				ob.Vertex({ 1.0f,  1.0f,  1.0f });

				Base = { 1.0f / 3.0f, 0 };
				ob.Normal({ -1.0f,  0.0f,  0.0f });
				ob.TexCoord(Base + Vector2D{ 1.0f / 3.0f - pixelWidth, 0.0f + pixelHeight });
				ob.Vertex({ -1.0f,  1.0f, -1.0f });
				ob.TexCoord(Base + Vector2D{ 1.0f / 3.0f - pixelWidth, 0.5f - pixelHeight });
				ob.Vertex({ -1.0f, -1.0f, -1.0f });
				ob.TexCoord(Base + Vector2D{ 0.0f / 3.0f + pixelWidth, 0.5f - pixelHeight });
				ob.Vertex({ -1.0f, -1.0f,  1.0f });
				ob.TexCoord(Base + Vector2D{ 0.0f / 3.0f + pixelWidth, 0.0f + pixelHeight });
				ob.Vertex({ -1.0f,  1.0f,  1.0f });

				Base = { 2.0f / 3.0f, 0 };
				ob.Normal({ 0, 0, -1.0f });
				ob.TexCoord(Base + Vector2D{ 1.0f / 3.0f - pixelWidth, 0.0f + pixelHeight });
				ob.Vertex({ 1.0f,  1.0f,  -1.0f });
				ob.TexCoord(Base + Vector2D{ 1.0f / 3.0f - pixelWidth, 0.5f - pixelHeight });
				ob.Vertex({ 1.0f, -1.0f,  -1.0f });
				ob.TexCoord(Base + Vector2D{ 0.0f / 3.0f + pixelWidth, 0.5f - pixelHeight });
				ob.Vertex({ -1.0f, -1.0f, -1.0f });
				ob.TexCoord(Base + Vector2D{ 0.0f / 3.0f + pixelWidth, 0.0f + pixelHeight });
				ob.Vertex({ -1.0f,  1.0f, -1.0f });

				Base = { 0, 0.5f };
				ob.Normal({ 1.0f,  0.0f,  0.0f });
				ob.TexCoord(Base + Vector2D{ 1.0f / 3.0f - pixelWidth, 0.0f + pixelHeight });
				ob.Vertex({ 1.0f,  1.0f,  1.0f });
				ob.TexCoord(Base + Vector2D{ 1.0f / 3.0f - pixelWidth, 0.5f - pixelHeight });
				ob.Vertex({ 1.0f, -1.0f,  1.0f });
				ob.TexCoord(Base + Vector2D{ 0.0f / 3.0f + pixelWidth, 0.5f - pixelHeight });
				ob.Vertex({ 1.0f, -1.0f, -1.0f });
				ob.TexCoord(Base + Vector2D{ 0.0f / 3.0f + pixelWidth, 0.0f + pixelHeight });
				ob.Vertex({ 1.0f,  1.0f, -1.0f });

				Base = { 1.0f / 3.0f, 0.5f };
				ob.Normal({ 0.0f,  -1.0f,  0.0f });
				ob.TexCoord(Base + Vector2D{ 1.0f / 3.0f - pixelWidth, 0.0f + pixelHeight });
				ob.Vertex({ -1.0f, -1.0f,  1.0f });
				ob.TexCoord(Base + Vector2D{ 1.0f / 3.0f - pixelWidth, 0.5f - pixelHeight });
				ob.Vertex({ -1.0f, -1.0f, -1.0f });
				ob.TexCoord(Base + Vector2D{ 0.0f / 3.0f + pixelWidth, 0.5f - pixelHeight });
				ob.Vertex({ 1.0f,  -1.0f, -1.0f });
				ob.TexCoord(Base + Vector2D{ 0.0f / 3.0f + pixelWidth, 0.0f + pixelHeight });
				ob.Vertex({ 1.0f,  -1.0f,  1.0f });

				Base = { 2.0f / 3.0f, 0.5f };
				ob.Normal({ 0.0f,  1.0f,  0.0f });
				ob.TexCoord(Base + Vector2D{ 0.0f / 3.0f + pixelWidth, 0.5f - pixelHeight });
				ob.Vertex({ 1.0f,  1.0f,  1.0f });
				ob.TexCoord(Base + Vector2D{ 0.0f / 3.0f + pixelWidth, 0.0f + pixelHeight });
				ob.Vertex({ 1.0f,  1.0f, -1.0f });
				ob.TexCoord(Base + Vector2D{ 1.0f / 3.0f - pixelWidth, 0.0f + pixelHeight });
				ob.Vertex({ -1.0f,  1.0f, -1.0f });
				ob.TexCoord(Base + Vector2D{ 1.0f / 3.0f - pixelWidth, 0.5f - pixelHeight });
				ob.Vertex({ -1.0f,  1.0f,  1.0f });
			}ob.End();

			return ob.Build();
		}


		InterleavedArrayObject MakeSquareWithUV_Flipped() {
			PVX::OpenGL::ObjectBuilder gl;
			gl.Begin(GL_QUADS); {
				gl.TexCoord({ 0.0f, 0.0f }); gl.Vertex({ -1.0f, 1.0f, 0 });
				gl.TexCoord({ 1.0f, 0.0f }); gl.Vertex({ 1.0f, 1.0f, 0 });
				gl.TexCoord({ 1.0f, 1.0f }); gl.Vertex({ 1.0f, -1.0f, 0 });
				gl.TexCoord({ 0.0f, 1.0f }); gl.Vertex({ -1.0f, -1.0f, 0 });
			}gl.End();
			return gl.Build();
		}

		InterleavedArrayObject MakeSquareWithUV() {
			PVX::OpenGL::ObjectBuilder gl;
			gl.Begin(GL_QUADS); {
				gl.TexCoord({ 0.0f, 0.0f }); gl.Vertex({ -1.0f, -1.0f, 0 });
				gl.TexCoord({ 1.0f, 0.0f }); gl.Vertex({ 1.0f, -1.0f, 0 });
				gl.TexCoord({ 1.0f, 1.0f }); gl.Vertex({ 1.0f, 1.0f, 0 });
				gl.TexCoord({ 0.0f, 1.0f }); gl.Vertex({ -1.0f, 1.0f, 0 });
			}gl.End();
			return gl.Build();
		}

		void MarxhingCubes() {
			const int Sizes[]{ 0, 3, 3, 6, 3, 6, 6, 12, 3, 6, 6, 12, 6, 12, 12, 6, 3, 6, 6, 12, 6, 12, 12, 18, 6, 12, 12, 18, 12, 18, 18, 12, 3, 6, 6, 12, 6, 12, 12, 18, 6, 12, 12, 18, 12, 18, 18, 12, 6, 12, 12, 6, 12, 18, 18, 12, 12, 18, 18, 12, 18, 21, 21, 6, 3, 6, 6, 12, 6, 12, 12, 18, 6, 12, 12, 18, 12, 18, 18, 12, 6, 12, 12, 18, 12, 18, 18, 21, 12, 18, 18, 21, 18, 21, 21, 18, 6, 12, 12, 18, 12, 18, 6, 12, 12, 18, 18, 21, 18, 21, 12, 6, 12, 18, 18, 12, 18, 21, 12, 6, 18, 21, 21, 18, 21, 6, 18, 3, 3, 6, 6, 12, 6, 12, 12, 18, 6, 12, 12, 18, 12, 18, 18, 12, 6, 12, 12, 18, 12, 18, 18, 21, 12, 6, 18, 12, 18, 12, 21, 6, 6, 12, 12, 18, 12, 18, 18, 21, 12, 18, 18, 21, 18, 21, 21, 18, 12, 18, 18, 12, 18, 21, 21, 18, 18, 12, 21, 6, 21, 18, 6, 3, 6, 12, 12, 18, 12, 18, 18, 21, 12, 18, 18, 21, 6, 12, 12, 6, 12, 18, 18, 21, 18, 21, 21, 6, 18, 12, 21, 18, 12, 6, 18, 3, 12, 18, 18, 21, 18, 21, 12, 18, 18, 21, 21, 6, 12, 18, 6, 3, 6, 12, 12, 6, 12, 18, 6, 3, 12, 6, 18, 3, 6, 3, 3, 0 };
			const int Offsets[]{ 0, 0, 3, 6, 12, 15, 21, 27, 39, 42, 48, 54, 66, 72, 84, 96, 102, 105, 111, 117, 129, 135, 147, 159, 177, 183, 195, 207, 225, 237, 255, 273, 285, 288, 294, 300, 312, 318, 330, 342, 360, 366, 378, 390, 408, 420, 438, 456, 468, 474, 486, 498, 504, 516, 534, 552, 564, 576, 594, 612, 624, 642, 663, 684, 690, 693, 699, 705, 717, 723, 735, 747, 765, 771, 783, 795, 813, 825, 843, 861, 873, 879, 891, 903, 921, 933, 951, 969, 990, 1002, 1020, 1038, 1059, 1077, 1098, 1119, 1137, 1143, 1155, 1167, 1185, 1197, 1215, 1221, 1233, 1245, 1263, 1281, 1302, 1320, 1341, 1353, 1359, 1371, 1389, 1407, 1419, 1437, 1458, 1470, 1476, 1494, 1515, 1536, 1554, 1575, 1581, 1599, 1602, 1605, 1611, 1617, 1629, 1635, 1647, 1659, 1677, 1683, 1695, 1707, 1725, 1737, 1755, 1773, 1785, 1791, 1803, 1815, 1833, 1845, 1863, 1881, 1902, 1914, 1920, 1938, 1950, 1968, 1980, 2001, 2007, 2013, 2025, 2037, 2055, 2067, 2085, 2103, 2124, 2136, 2154, 2172, 2193, 2211, 2232, 2253, 2271, 2283, 2301, 2319, 2331, 2349, 2370, 2391, 2409, 2427, 2439, 2460, 2466, 2487, 2505, 2511, 2514, 2520, 2532, 2544, 2562, 2574, 2592, 2610, 2631, 2643, 2661, 2679, 2700, 2706, 2718, 2730, 2736, 2748, 2766, 2784, 2805, 2823, 2844, 2865, 2871, 2889, 2901, 2922, 2940, 2952, 2958, 2976, 2979, 2991, 3009, 3027, 3048, 3066, 3087, 3099, 3117, 3135, 3156, 3177, 3183, 3195, 3213, 3219, 3222, 3228, 3240, 3252, 3258, 3270, 3288, 3294, 3297, 3309, 3315, 3333, 3336, 3342, 3345, 3348 };

			const int Indices[]{ 3, 8, 0, 9, 1, 0, 3, 8, 1, 9, 3, 1, 10, 2, 1, 3, 8, 0, 1, 3, 0, 10, 2, 9, 0, 10, 9, 3, 8, 2, 2, 3, 2, 10, 8, 10, 9, 10, 10, 2, 11, 3, 2, 11, 0, 8, 2, 0, 0, 9, 1, 2, 0, 1, 2, 11, 1, 1, 2, 1, 9, 11, 9, 8, 9, 9, 1, 10, 3, 11, 1, 3, 1, 10, 0, 0, 1, 0, 8, 10, 8, 11, 8, 8, 0, 9, 3, 3, 0, 3, 11, 9, 11, 10, 11, 11, 10, 8, 9, 10, 10, 9, 8, 7, 4, 0, 3, 4, 7, 0, 4, 9, 1, 0, 8, 9, 0, 9, 1, 4, 4, 9, 4, 7, 1, 7, 3, 7, 7, 10, 2, 1, 8, 10, 1, 7, 4, 3, 3, 7, 3, 1, 4, 0, 2, 1, 0, 10, 2, 9, 9, 10, 9, 8, 2, 0, 4, 8, 0, 9, 10, 2, 2, 9, 2, 2, 7, 9, 7, 2, 9, 9, 7, 3, 4, 9, 3, 7, 4, 8, 3, 7, 8, 7, 4, 11, 11, 7, 11, 2, 4, 2, 0, 2, 2, 1, 0, 9, 8, 1, 9, 2, 7, 4, 3, 2, 4, 11, 7, 4, 9, 11, 4, 9, 11, 4, 11, 9, 4, 2, 9, 2, 1, 2, 2, 1, 10, 3, 3, 1, 3, 7, 10, 11, 8, 7, 11, 10, 11, 1, 1, 10, 1, 1, 11, 4, 0, 1, 4, 11, 7, 4, 4, 11, 4, 8, 7, 4, 9, 8, 4, 9, 11, 0, 11, 9, 0, 0, 11, 10, 3, 0, 10, 11, 7, 4, 4, 11, 4, 9, 9, 11, 11, 9, 11, 4, 5, 9, 4, 5, 9, 0, 4, 9, 4, 5, 0, 1, 4, 0, 4, 5, 8, 8, 4, 8, 3, 5, 3, 1, 3, 3, 10, 2, 1, 9, 10, 1, 8, 0, 3, 1, 8, 3, 4, 10, 2, 9, 4, 2, 10, 2, 5, 5, 10, 5, 4, 2, 4, 0, 4, 4, 5, 10, 2, 3, 5, 2, 3, 5, 2, 5, 3, 2, 4, 3, 4, 8, 4, 4, 4, 5, 9, 2, 4, 9, 2, 11, 0, 0, 2, 0, 4, 11, 8, 9, 4, 8, 4, 5, 0, 0, 4, 0, 2, 5, 1, 3, 2, 1, 5, 1, 2, 2, 5, 2, 2, 8, 5, 8, 2, 5, 8, 4, 11, 5, 8, 11, 11, 3, 10, 10, 11, 10, 9, 3, 1, 5, 9, 1, 5, 9, 4, 0, 5, 4, 8, 1, 8, 10, 8, 8, 11, 8, 1, 10, 11, 1, 0, 4, 5, 5, 0, 5, 5, 11, 0, 11, 5, 0, 0, 11, 10, 3, 0, 10, 8, 4, 5, 5, 8, 5, 10, 10, 8, 8, 10, 8, 8, 7, 9, 5, 8, 9, 0, 3, 9, 9, 0, 9, 5, 3, 5, 7, 5, 5, 8, 7, 0, 0, 8, 0, 1, 7, 1, 5, 1, 1, 3, 5, 1, 3, 3, 1, 8, 7, 9, 9, 8, 9, 10, 7, 5, 1, 10, 5, 2, 1, 10, 9, 2, 10, 5, 0, 5, 3, 5, 5, 7, 5, 0, 3, 7, 0, 2, 0, 8, 8, 2, 8, 8, 5, 2, 5, 8, 2, 5, 10, 7, 2, 5, 7, 5, 10, 2, 2, 5, 2, 3, 3, 5, 5, 3, 5, 5, 9, 7, 7, 5, 7, 3, 9, 8, 11, 3, 8, 7, 5, 9, 9, 7, 9, 9, 2, 7, 2, 9, 7, 7, 2, 0, 11, 7, 0, 11, 3, 2, 0, 11, 2, 1, 8, 1, 7, 1, 1, 5, 1, 8, 7, 5, 8, 1, 2, 11, 11, 1, 11, 7, 7, 1, 1, 7, 1, 8, 5, 9, 8, 8, 9, 10, 7, 5, 1, 10, 5, 3, 10, 3, 11, 3, 3, 0, 7, 5, 5, 0, 5, 7, 9, 0, 11, 7, 0, 0, 1, 0, 10, 0, 0, 0, 10, 11, 0, 10, 11, 11, 0, 11, 10, 3, 0, 5, 10, 0, 0, 8, 0, 7, 0, 0, 0, 7, 5, 5, 10, 11, 7, 5, 11, 5, 6, 10, 3, 8, 0, 5, 3, 0, 1, 0, 9, 5, 1, 9, 3, 8, 1, 1, 3, 1, 5, 8, 9, 10, 5, 9, 5, 6, 1, 2, 5, 1, 5, 6, 1, 1, 5, 1, 3, 6, 2, 0, 3, 2, 5, 6, 9, 9, 5, 9, 0, 6, 0, 2, 0, 0, 8, 9, 5, 5, 8, 5, 5, 2, 8, 2, 5, 8, 2, 3, 6, 8, 2, 6, 11, 3, 2, 10, 11, 2, 8, 0, 11, 11, 8, 11, 10, 0, 2, 6, 10, 2, 9, 1, 0, 2, 9, 0, 5, 11, 3, 10, 5, 3, 6, 10, 5, 1, 6, 5, 9, 2, 9, 11, 9, 9, 8, 9, 2, 11, 8, 2, 11, 3, 6, 6, 11, 6, 5, 3, 5, 1, 5, 5, 11, 8, 0, 0, 11, 0, 0, 5, 11, 5, 0, 11, 11, 5, 1, 6, 11, 1, 6, 11, 3, 0, 6, 3, 0, 6, 3, 6, 0, 3, 5, 0, 5, 9, 5, 5, 9, 5, 6, 6, 9, 6, 11, 11, 9, 9, 11, 9, 6, 10, 5, 4, 6, 5, 0, 3, 4, 4, 0, 4, 6, 3, 7, 5, 6, 7, 0, 9, 1, 5, 0, 1, 8, 6, 10, 4, 8, 10, 5, 6, 10, 1, 5, 10, 1, 7, 9, 7, 1, 9, 9, 7, 3, 4, 9, 3, 2, 1, 6, 6, 2, 6, 4, 1, 5, 7, 4, 5, 5, 2, 1, 5, 5, 1, 3, 6, 2, 0, 3, 2, 4, 3, 4, 7, 4, 4, 7, 4, 8, 9, 7, 8, 0, 5, 0, 6, 0, 0, 2, 0, 5, 6, 2, 5, 9, 3, 7, 7, 9, 7, 3, 4, 9, 2, 3, 9, 9, 5, 9, 6, 9, 9, 9, 6, 2, 2, 11, 3, 7, 2, 3, 10, 4, 8, 6, 10, 8, 6, 10, 5, 4, 6, 5, 4, 2, 7, 2, 4, 7, 7, 2, 0, 11, 7, 0, 9, 1, 0, 4, 9, 0, 2, 8, 7, 3, 2, 7, 10, 5, 11, 6, 10, 11, 1, 2, 9, 9, 1, 9, 9, 2, 11, 4, 9, 11, 11, 7, 11, 4, 11, 11, 6, 10, 5, 7, 4, 8, 3, 7, 8, 3, 5, 11, 5, 3, 11, 11, 5, 1, 6, 11, 1, 11, 1, 5, 5, 11, 5, 1, 6, 11, 0, 1, 11, 11, 7, 11, 4, 11, 11, 11, 4, 0, 9, 5, 0, 0, 9, 0, 0, 5, 6, 3, 0, 6, 6, 11, 6, 3, 6, 6, 7, 4, 8, 9, 5, 6, 6, 9, 6, 4, 11, 9, 7, 4, 9, 11, 7, 9, 9, 11, 9, 9, 4, 10, 6, 9, 10, 6, 10, 4, 4, 6, 4, 0, 10, 9, 8, 0, 9, 1, 0, 10, 10, 1, 10, 6, 0, 6, 4, 6, 6, 1, 3, 8, 8, 1, 8, 8, 6, 1, 6, 8, 1, 1, 6, 4, 10, 1, 4, 9, 4, 1, 1, 9, 1, 2, 4, 2, 6, 2, 2, 8, 0, 3, 1, 8, 3, 2, 9, 2, 4, 2, 2, 6, 2, 9, 4, 6, 9, 4, 2, 0, 4, 4, 0, 2, 3, 8, 8, 2, 8, 4, 4, 2, 2, 4, 2, 9, 4, 10, 10, 9, 10, 11, 4, 6, 2, 11, 6, 2, 8, 0, 2, 2, 0, 4, 11, 8, 9, 4, 8, 10, 4, 10, 6, 10, 10, 2, 11, 3, 0, 2, 3, 0, 6, 1, 6, 0, 1, 1, 6, 4, 10, 1, 4, 1, 4, 6, 6, 1, 6, 4, 10, 1, 8, 4, 1, 1, 2, 1, 11, 1, 1, 1, 11, 8, 4, 6, 9, 9, 4, 9, 9, 6, 3, 1, 9, 3, 6, 11, 3, 3, 6, 3, 1, 11, 8, 8, 1, 8, 11, 0, 1, 6, 11, 1, 1, 9, 1, 4, 1, 1, 1, 4, 6, 6, 11, 3, 3, 6, 3, 0, 0, 6, 6, 0, 6, 8, 4, 6, 11, 8, 6, 6, 10, 7, 7, 6, 7, 8, 10, 8, 9, 8, 8, 3, 7, 0, 0, 3, 0, 0, 7, 10, 9, 0, 10, 7, 6, 10, 10, 7, 10, 7, 6, 10, 1, 7, 10, 1, 7, 10, 7, 1, 10, 8, 1, 8, 0, 8, 8, 7, 6, 10, 10, 7, 10, 1, 1, 7, 7, 1, 7, 6, 2, 1, 1, 6, 1, 1, 8, 6, 8, 1, 6, 6, 8, 9, 7, 6, 9, 9, 6, 2, 2, 9, 2, 6, 1, 9, 7, 6, 9, 9, 0, 9, 3, 9, 9, 9, 3, 7, 0, 8, 7, 7, 0, 7, 6, 6, 0, 0, 6, 0, 2, 3, 7, 6, 2, 7, 11, 3, 2, 10, 11, 2, 10, 8, 6, 8, 10, 6, 6, 8, 9, 7, 6, 9, 7, 0, 2, 2, 7, 2, 0, 11, 7, 9, 0, 7, 7, 6, 7, 10, 7, 7, 7, 10, 9, 0, 8, 1, 1, 0, 1, 1, 8, 7, 10, 1, 7, 7, 6, 7, 10, 7, 7, 11, 3, 2, 1, 2, 11, 11, 1, 11, 10, 7, 1, 6, 10, 1, 7, 6, 1, 1, 7, 1, 6, 9, 8, 8, 6, 8, 9, 7, 6, 1, 9, 6, 6, 11, 6, 3, 6, 6, 6, 3, 1, 1, 9, 0, 11, 1, 0, 0, 8, 7, 7, 0, 7, 3, 6, 0, 11, 3, 0, 6, 11, 0, 0, 6, 0, 6, 11, 7, 11, 6, 7, 8, 0, 3, 11, 8, 3, 9, 1, 0, 11, 9, 0, 9, 1, 8, 8, 9, 8, 11, 1, 3, 7, 11, 3, 2, 1, 10, 6, 2, 10, 10, 2, 1, 3, 10, 1, 6, 8, 0, 11, 6, 0, 0, 9, 2, 2, 0, 2, 6, 9, 10, 11, 6, 10, 7, 11, 6, 2, 7, 6, 10, 3, 10, 8, 10, 10, 9, 10, 3, 8, 9, 3, 3, 2, 7, 6, 3, 7, 8, 0, 7, 7, 8, 7, 6, 0, 6, 2, 6, 6, 6, 7, 2, 2, 6, 2, 0, 7, 3, 1, 0, 3, 2, 6, 1, 1, 2, 1, 1, 6, 8, 9, 1, 8, 7, 8, 8, 6, 7, 8, 6, 7, 10, 10, 6, 10, 1, 7, 1, 3, 1, 1, 6, 7, 10, 1, 6, 10, 1, 10, 7, 8, 1, 7, 0, 1, 7, 8, 0, 7, 7, 3, 0, 0, 7, 0, 0, 10, 7, 10, 0, 7, 10, 6, 9, 7, 10, 9, 10, 6, 7, 7, 10, 7, 8, 8, 10, 10, 8, 10, 4, 8, 6, 11, 4, 6, 11, 6, 3, 3, 11, 3, 0, 6, 0, 4, 0, 0, 11, 6, 8, 8, 11, 8, 9, 6, 4, 0, 9, 4, 6, 4, 9, 9, 6, 9, 9, 3, 6, 3, 9, 6, 3, 11, 1, 6, 3, 1, 4, 8, 6, 6, 4, 6, 2, 8, 11, 10, 2, 11, 10, 2, 1, 3, 10, 1, 0, 11, 0, 6, 0, 0, 4, 0, 11, 6, 4, 11, 8, 11, 4, 4, 8, 4, 0, 11, 6, 2, 0, 6, 10, 2, 9, 9, 10, 9, 3, 9, 10, 10, 3, 10, 9, 2, 3, 4, 9, 3, 3, 11, 3, 6, 3, 3, 3, 6, 4, 3, 2, 8, 8, 3, 8, 4, 2, 4, 6, 4, 4, 2, 4, 0, 4, 2, 0, 0, 9, 1, 2, 0, 1, 2, 4, 3, 4, 2, 3, 3, 4, 6, 8, 3, 6, 4, 9, 1, 1, 4, 1, 2, 2, 4, 4, 2, 4, 3, 1, 8, 8, 3, 8, 8, 1, 6, 4, 8, 6, 10, 6, 6, 1, 10, 6, 0, 1, 10, 10, 0, 10, 6, 6, 0, 0, 6, 0, 3, 6, 4, 4, 3, 4, 6, 8, 3, 10, 6, 3, 3, 0, 3, 9, 3, 3, 3, 9, 10, 4, 9, 10, 6, 4, 10, 5, 9, 4, 7, 5, 4, 3, 8, 0, 4, 3, 0, 11, 5, 9, 7, 11, 9, 1, 0, 5, 5, 1, 5, 7, 0, 4, 6, 7, 4, 6, 7, 11, 8, 6, 11, 3, 4, 3, 5, 3, 3, 1, 3, 4, 5, 1, 4, 4, 5, 9, 10, 4, 9, 7, 2, 1, 6, 7, 1, 7, 11, 6, 1, 7, 6, 0, 10, 2, 8, 0, 2, 9, 4, 3, 5, 9, 3, 11, 6, 7, 5, 11, 7, 4, 10, 4, 2, 4, 4, 0, 4, 10, 2, 0, 10, 8, 4, 3, 3, 8, 3, 3, 4, 5, 2, 3, 5, 5, 10, 5, 2, 5, 5, 6, 7, 11, 3, 2, 7, 7, 3, 7, 5, 2, 6, 4, 5, 6, 4, 5, 9, 0, 4, 9, 0, 6, 8, 6, 0, 8, 8, 6, 2, 7, 8, 2, 2, 6, 3, 3, 2, 3, 1, 6, 7, 5, 1, 7, 4, 5, 0, 0, 4, 0, 8, 2, 6, 6, 8, 6, 2, 7, 8, 1, 2, 8, 8, 4, 8, 5, 8, 8, 8, 5, 1, 4, 5, 9, 10, 4, 9, 1, 6, 1, 7, 1, 1, 3, 1, 6, 7, 3, 6, 10, 6, 1, 1, 10, 1, 1, 6, 7, 0, 1, 7, 7, 8, 7, 0, 7, 7, 4, 5, 9, 10, 0, 4, 4, 10, 4, 0, 5, 10, 3, 0, 10, 10, 6, 10, 7, 10, 10, 10, 7, 3, 10, 6, 7, 7, 10, 7, 5, 8, 10, 4, 5, 10, 8, 4, 10, 10, 8, 10, 5, 9, 6, 6, 5, 6, 11, 9, 11, 8, 11, 11, 11, 6, 3, 0, 11, 3, 0, 3, 6, 5, 0, 6, 9, 0, 6, 5, 9, 6, 8, 11, 0, 0, 8, 0, 0, 11, 5, 1, 0, 5, 6, 5, 5, 11, 6, 5, 3, 11, 6, 6, 3, 6, 5, 5, 3, 3, 5, 3, 10, 2, 1, 9, 10, 1, 9, 11, 5, 11, 9, 5, 5, 11, 8, 6, 5, 8, 3, 11, 0, 0, 3, 0, 0, 11, 6, 9, 0, 6, 6, 5, 6, 9, 6, 6, 10, 2, 1, 5, 8, 11, 11, 5, 11, 8, 6, 5, 0, 8, 5, 5, 10, 5, 2, 5, 5, 5, 2, 0, 3, 11, 6, 6, 3, 6, 2, 5, 3, 10, 2, 3, 5, 10, 3, 3, 5, 3, 9, 8, 5, 5, 9, 5, 5, 8, 2, 6, 5, 2, 8, 3, 2, 2, 8, 2, 6, 5, 9, 9, 6, 9, 0, 0, 6, 6, 0, 6, 8, 5, 1, 1, 8, 1, 5, 0, 8, 6, 5, 8, 8, 3, 8, 2, 8, 8, 8, 2, 6, 6, 5, 1, 2, 6, 1, 6, 3, 1, 1, 6, 1, 3, 10, 6, 8, 3, 6, 6, 5, 6, 9, 6, 6, 6, 9, 8, 0, 1, 10, 10, 0, 10, 9, 6, 0, 5, 9, 0, 6, 5, 0, 0, 6, 0, 8, 3, 0, 5, 8, 0, 6, 5, 10, 10, 5, 11, 7, 10, 11, 10, 5, 11, 11, 10, 11, 8, 5, 7, 3, 8, 7, 7, 11, 5, 5, 7, 5, 1, 11, 10, 9, 1, 10, 5, 7, 10, 10, 5, 10, 9, 7, 11, 8, 9, 11, 3, 8, 1, 1, 3, 1, 2, 1, 11, 11, 2, 11, 7, 1, 7, 5, 7, 7, 3, 8, 0, 1, 3, 0, 1, 7, 2, 7, 1, 2, 2, 7, 5, 11, 2, 5, 5, 7, 9, 9, 5, 9, 9, 7, 2, 0, 9, 2, 11, 2, 2, 7, 11, 2, 2, 5, 7, 7, 2, 7, 5, 11, 2, 9, 5, 2, 2, 3, 2, 8, 2, 2, 2, 8, 9, 10, 5, 2, 2, 10, 2, 3, 5, 3, 7, 3, 3, 0, 2, 8, 8, 0, 8, 8, 2, 5, 7, 8, 5, 2, 10, 5, 5, 2, 5, 1, 0, 9, 5, 1, 9, 5, 3, 10, 3, 5, 10, 10, 3, 7, 2, 10, 7, 2, 8, 9, 9, 2, 9, 8, 1, 2, 7, 8, 2, 2, 10, 2, 5, 2, 2, 2, 5, 7, 5, 3, 1, 3, 5, 1, 7, 8, 0, 0, 7, 0, 1, 1, 7, 7, 1, 7, 3, 0, 9, 9, 3, 9, 5, 5, 3, 3, 5, 3, 7, 8, 9, 5, 7, 9, 4, 8, 5, 5, 4, 5, 10, 8, 10, 11, 10, 10, 4, 0, 5, 5, 4, 5, 5, 0, 11, 10, 5, 11, 3, 11, 11, 0, 3, 11, 9, 1, 0, 8, 9, 0, 8, 10, 4, 10, 8, 4, 4, 10, 11, 5, 4, 11, 4, 11, 10, 10, 4, 10, 11, 5, 4, 3, 11, 4, 4, 9, 4, 1, 4, 4, 4, 1, 3, 1, 5, 2, 2, 1, 2, 2, 5, 8, 11, 2, 8, 5, 4, 8, 8, 5, 8, 11, 4, 0, 0, 11, 0, 4, 3, 11, 5, 4, 11, 11, 2, 11, 1, 11, 11, 11, 1, 5, 5, 2, 0, 0, 5, 0, 2, 9, 5, 11, 2, 5, 5, 4, 5, 8, 5, 5, 5, 8, 11, 5, 4, 9, 2, 5, 9, 10, 5, 2, 3, 10, 2, 3, 2, 5, 4, 3, 5, 8, 3, 5, 4, 8, 5, 2, 10, 5, 5, 2, 5, 4, 4, 2, 2, 4, 2, 2, 10, 3, 3, 2, 3, 3, 10, 5, 8, 3, 5, 5, 4, 5, 8, 5, 5, 9, 1, 0, 2, 10, 5, 5, 2, 5, 1, 4, 2, 9, 1, 2, 4, 9, 2, 2, 4, 2, 5, 4, 8, 8, 5, 8, 3, 3, 5, 5, 3, 5, 5, 4, 0, 1, 5, 0, 5, 4, 8, 8, 5, 8, 9, 3, 5, 0, 9, 5, 3, 0, 5, 5, 3, 5, 5, 4, 9, 7, 11, 4, 4, 7, 4, 9, 11, 9, 10, 9, 9, 3, 8, 0, 4, 3, 0, 9, 7, 9, 11, 9, 9, 10, 9, 7, 11, 10, 7, 11, 10, 1, 1, 11, 1, 1, 4, 11, 4, 1, 11, 4, 7, 0, 11, 4, 0, 4, 1, 3, 3, 4, 3, 1, 8, 4, 10, 1, 4, 4, 7, 4, 11, 4, 4, 4, 11, 10, 7, 11, 4, 9, 7, 4, 9, 4, 11, 2, 9, 11, 1, 9, 11, 2, 1, 11, 4, 7, 9, 9, 4, 9, 9, 7, 11, 1, 9, 11, 11, 2, 11, 1, 11, 11, 3, 8, 0, 4, 7, 11, 11, 4, 11, 2, 2, 4, 4, 2, 4, 4, 7, 11, 11, 4, 11, 8, 2, 4, 3, 8, 4, 2, 3, 4, 4, 2, 4, 10, 9, 2, 2, 10, 2, 2, 9, 7, 3, 2, 7, 4, 7, 7, 9, 4, 7, 7, 10, 9, 9, 7, 9, 10, 4, 7, 2, 10, 7, 7, 8, 7, 0, 7, 7, 7, 0, 2, 10, 7, 3, 3, 10, 3, 7, 2, 10, 4, 7, 10, 10, 1, 10, 0, 10, 10, 10, 0, 4, 2, 10, 1, 8, 2, 1, 1, 9, 4, 4, 1, 4, 7, 7, 1, 1, 7, 1, 1, 9, 4, 4, 1, 4, 0, 7, 1, 8, 0, 1, 7, 8, 1, 1, 7, 1, 3, 0, 4, 7, 3, 4, 7, 8, 4, 8, 10, 9, 10, 8, 9, 9, 0, 3, 3, 9, 3, 11, 11, 9, 9, 11, 9, 10, 1, 0, 0, 10, 0, 8, 8, 10, 10, 8, 10, 10, 1, 3, 11, 10, 3, 11, 2, 1, 1, 11, 1, 9, 9, 11, 11, 9, 11, 9, 0, 3, 3, 9, 3, 1, 11, 9, 2, 1, 9, 11, 2, 9, 9, 11, 9, 11, 2, 0, 8, 11, 0, 11, 2, 3, 8, 3, 2, 2, 8, 2, 10, 10, 8, 8, 10, 8, 2, 10, 9, 0, 2, 9, 8, 3, 2, 2, 8, 2, 0, 10, 8, 1, 0, 8, 10, 1, 8, 8, 10, 8, 2, 10, 1, 8, 3, 1, 9, 8, 1, 1, 9, 0, 8, 3, 0 };
			const PVX::Vector3D Vertices[12]{
				{  0.0f, -1.0f,  1.0f },
				{  1.0f, -1.0f,  0.0f },
				{  0.0f, -1.0f, -1.0f },
				{ -1.0f, -1.0f,  0.0f },
				{  0.0f,  1.0f,  1.0f },
				{  1.0f,  1.0f,  0.0f },
				{  0.0f,  1.0f, -1.0f },
				{ -1.0f,  1.0f,  0.0f },
				{ -1.0f,  0.0f,  1.0f },
				{  1.0f,  0.0f,  1.0f },
				{  1.0f,  0.0f, -1.0f },
				{ -1.0f,  0.0f, -1.0f },
			};
		}
	}
}
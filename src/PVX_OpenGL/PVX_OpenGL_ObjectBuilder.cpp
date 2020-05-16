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
			size_t i = data.Vertex.size() - 4;
			data.Index.push_back(i + 0);
			data.Index.push_back(i + 1);
			data.Index.push_back(i + 2);

			data.Index.push_back(i + 0);
			data.Index.push_back(i + 2);
			data.Index.push_back(i + 3);
		}
		static void AddQuad2(ObjectData& data, int Start) {
			size_t i = data.Vertex.size() - 4;
			data.Index.push_back(i + 1);
			data.Index.push_back(i + 0);
			data.Index.push_back(i + 2);

			data.Index.push_back(i + 1);
			data.Index.push_back(i + 2);
			data.Index.push_back(i + 3);
		}

		static void AddTriangleFan(ObjectData& data, int Start) {
			size_t i = data.Vertex.size() - 2;
			data.Index.push_back(Start);
			data.Index.push_back(i + 0);
			data.Index.push_back(i + 1);
		}

		static void AddTriangle(ObjectData& data, int Start) {
			size_t i = data.Vertex.size() - 1;
			data.Index.push_back(i - 2);
			data.Index.push_back(i - 1);
			data.Index.push_back(i - 0);
		}
		static void AddTriangleStrip1(ObjectData& data, int Start) {
			size_t i = data.Vertex.size() - 1;
			data.Index.push_back(i - 3);
			data.Index.push_back(i - 1);
			data.Index.push_back(i - 0);
		}
		static void AddTriangleStrip2(ObjectData& data, int Start) {
			size_t i = data.Vertex.size() - 1;
			data.Index.push_back(i - 1);
			data.Index.push_back(i - 2);
			data.Index.push_back(i - 0);
		}
		static void AddPoint(ObjectData& data, int Start) {
			size_t i = data.Vertex.size() - 1;
			data.Index.push_back(i);
		}
		static void AddLine(ObjectData& data, int Start) {
			size_t i = data.Vertex.size() - 1;
			data.Index.push_back(i - 1);
			data.Index.push_back(i);
		}
		static void AddLineStrip(ObjectData& data, int Start) {
			size_t i = data.Vertex.size() - 1;
			data.Index.push_back(data.Index.back());
			data.Index.push_back(i);
		}
		static void AddLineLoop(ObjectData& data, int Start) {
			size_t i = data.Vertex.size() - 2;
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
			Start = Data.Vertex.size();
		}
		void ObjectBuilder::End() {
			if (Max != 0x7fffffff && Count != Min) {
				Count = Data.Vertex.size() - Count + Min;
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
			return i / stride;
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
			return i;
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
			return i;
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

			ret.Attributes.push_back({ "Position", 3, GL_FLOAT, 0, 0, "vec3" });

			if (ret.Stride == sizeof(Vector3D)) {
				memcpy(&tmp[0], &Data.Vertex[0], dsz);
			} else {
				unsigned char* dt = &tmp[0];
				PVX::Interleave(dt, ret.Stride, Data.Vertex.data(), sizeof(Vector3D), sz);
				int off = sizeof(Vector3D);

				if (Flags&HasNormal) {
					ret.Attributes.push_back({ "Normal", 3, GL_FLOAT, 0, off, "vec3" });
					ret.NormalOffset = off;
					PVX::Interleave(dt+off, ret.Stride, Data.Normal.data(), sizeof(Vector3D), sz);
					off += sizeof(Vector3D);
				}
				if (Flags&HasTex) {
					ret.Attributes.push_back({ "UV", 2, GL_FLOAT, 0, off, "vec2" });
					ret.TexCoordOffset = off;
					PVX::Interleave(dt+off, ret.Stride, Data.TexCoord.data(), sizeof(Vector2D), sz);
					off += sizeof(Vector2D);
				}
				if (Flags&HasTex3D) {
					ret.Attributes.push_back({ "UVW", 2, GL_FLOAT, 0, off, "vec3" });
					ret.TexCoordOffset3D = off;
					PVX::Interleave(dt+off, ret.Stride, Data.TexCoord3D.data(), sizeof(Vector3D), sz);
					off += sizeof(Vector2D);
				}
				if (Flags&HasColor) {
					ret.Attributes.push_back({ "Color", 4, GL_FLOAT, 0, off, "vec4" });
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

		static void Interleave(void* dst, int dstStride, void* src, int srcStride, int Count) {
			float min = srcStride < dstStride ? srcStride : dstStride;

			unsigned char* Dst = (unsigned char*)dst;
			unsigned char* Src = (unsigned char*)src;
			for (int i = 0; i < Count; i++) {
				memcpy(Dst, Src, min);
				Dst += dstStride;
				Src += srcStride;
			}
		}

		InterleavedArrayObject::InterleavedArrayObject(GLenum Mode, int VertexCount, Vector3D* Position, int IndexCount, int* Index, Vector3D* Normal, Vector3D* UV, Vector3D* Color) {
			this->Mode = Mode;
			Stride = sizeof(Vector3D);
			Attributes.push_back({ "Position", 3, GL_FLOAT, false, 0, "vec3" });
			NormalOffset = TexCoordOffset = ColorOffset = TangentOffset = 0;
			if (Normal) {
				Attributes.push_back({ "Normal", 3, GL_FLOAT, false, Stride, "vec3" });
				NormalOffset = Stride;
				Stride += sizeof(Vector3D);
			}
			if (UV) {
				Attributes.push_back({ "UV", 2, GL_FLOAT, false, Stride, "vec2" });
				TexCoordOffset = Stride;
				Stride += sizeof(Vector2D);
			}
			if (Color) {
				Attributes.push_back({ "Color", 4, GL_FLOAT, false, Stride, "vec4" });
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

			//NormalOffset = TexCoordOffset = ColorOffset = TangentOffset = 0;

			Attributes.push_back({ "Position", 3, GL_FLOAT, false, 0, "vec3" });
			Stride = sizeof(Vector3D);

			Attributes.push_back({ "Normal", 3, GL_FLOAT, false, Stride, "vec3" });
			NormalOffset = Stride;
			Stride += sizeof(Vector3D);

			Attributes.push_back({ "UV", 2, GL_FLOAT, false, Stride, "vec2" });
			TexCoordOffset = Stride;
			Stride += sizeof(Vector2D);

			if (ColorOffset) {
				Attributes.push_back({ "Color", 4, GL_FLOAT, false, Stride, "vec4" });
				ColorOffset = Stride;
				Stride += sizeof(Vector4D);
			}

			Attributes.push_back({ "Tangent", 4, GL_FLOAT, false, Stride, "vec4" });
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
				glVertexAttribPointer(i++, a.Size, a.Type, a.Normalized, Stride, (void*)a.Offset);
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
			struct FullVert {
				Vector3D pos;
				Vector2D UV;
			};
			FullVert* Verts = new FullVert[(segV - 1) * (segH + 1) + 2];
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

		//static Texture * MyLoadTexture(std::map<std::string, Texture*> & Textures, const std::string & fn) {
		//	if(Textures.find(fn) != Textures.end()) 
		//		return Textures[fn];
		//	PVX::OpenGL::TextureLoader txtr;
		//	txtr.Load(fn.c_str());
		//	auto * t = new Texture(txtr.MakeTexture2D());
		//	Textures[fn] = t;
		//	return t;
		//}

		//std::vector<InterleavedArrayObject> LoadObj(const char * fn) {
		//	union Index {
		//		struct {
		//			int Vertex, UV, Normal;
		//		};
		//		int Array[3];
		//	};

		//	auto lines = PVX::String::Split_No_Empties(PVX::IO::ReadText(fn), "\n");
		//	std::vector<InterleavedArrayObject> ret;
		//	if(!lines.size()) return ret;

		//	std::map<std::string, PVX::OpenGL::SimpleMatrial> Materials;
		//	std::map<std::string, Texture*> Textures;

		//	auto mltLines = PVX::String::Split_No_Empties(PVX::IO::ReadText(PVX::IO::ReplaceExtension(fn, "mtl").c_str()), "\n");
		//	if(mltLines.size()) {
		//		SimpleMatrial * mat = nullptr;
		//		for(auto i = 0; i < mltLines.size(); i++) {
		//			auto tokens = PVX::String::Split_No_Empties(mltLines[i], " ");
		//			if(tokens[0] == "newmtl") {
		//				Materials[tokens[1]] = SimpleMatrial{ 0 };
		//				mat = &Materials[tokens[1]];
		//			} else if(tokens[0] == "Ka") {
		//				mat->Ambient = { (float)atof(tokens[1].c_str()),(float)atof(tokens[2].c_str()),(float)atof(tokens[3].c_str()), 1.0f };
		//			} else if(tokens[0] == "Kd") {
		//				mat->Diffuse = { (float)atof(tokens[1].c_str()),(float)atof(tokens[2].c_str()),(float)atof(tokens[3].c_str()), 1.0f };
		//			} else if(tokens[0] == "Ks") {
		//				mat->Specular = { (float)atof(tokens[1].c_str()),(float)atof(tokens[2].c_str()),(float)atof(tokens[3].c_str()), 1.0f };
		//			} else if(tokens[0] == "Ke") {
		//				mat->Emission = { (float)atof(tokens[1].c_str()),(float)atof(tokens[2].c_str()),(float)atof(tokens[3].c_str()), 1.0f };
		//			} else if(tokens[0] == "d") {
		//				mat->Transparency = 1.0f - atof(tokens[1].c_str());
		//			} else if(tokens[0] == "Ns") {
		//				mat->SpeculatPower = atof(tokens[1].c_str());
		//			} else if(tokens[0] == "map_Kd") {
		//				std::string Name = tokens[1];
		//				mat->Textures.Diffuse = MyLoadTexture(Textures, Name);
		//				Textures[Name] = mat->Textures.Diffuse;
		//			} else if(tokens[0] == "map_Ke") {
		//				std::string Name = tokens[1];
		//				mat->Textures.Emission = MyLoadTexture(Textures, Name);
		//				Textures[Name] = mat->Textures.Emission;
		//			} else if(tokens[0] == "map_Bump") {
		//				std::string Name = tokens[3];
		//				mat->Textures.Bump = MyLoadTexture(Textures, Name);
		//				Textures[Name] = mat->Textures.Bump;
		//			} else if(tokens[0] == "map_Ks") {
		//				std::string Name = tokens[1];
		//				mat->Textures.Specular = MyLoadTexture(Textures, Name);
		//				Textures[Name] = mat->Textures.Specular;
		//			} else if(tokens[0] == "map_Ka") {
		//				std::string Name = tokens[1];
		//				mat->Textures.Ambient = MyLoadTexture(Textures, Name);
		//				Textures[Name] = mat->Textures.Ambient;
		//			}
		//		}
		//	}

		//	std::vector<Vector3D> Vertices;
		//	std::vector<Vector2D> UVs;
		//	std::vector<Vector3D> Normals;
		//	std::vector<Index> Indices;

		//	SimpleMatrial CurrentMat;

		//	for(auto lineIndex = 0; lineIndex < lines.size(); lineIndex++) {
		//		for(; lineIndex < lines.size(); lineIndex++) {
		//			auto & l = lines[lineIndex];
		//			auto tokens = PVX::String::Split_No_Empties(l, " ");
		//			if(tokens[0] == "v") {
		//				Vertices.push_back({ (float)atof(tokens[1].c_str()), (float)atof(tokens[2].c_str()), (float)atof(tokens[3].c_str()) });
		//			} else if(tokens[0] == "vn") {
		//				Normals.push_back({ (float)atof(tokens[1].c_str()), (float)atof(tokens[2].c_str()), (float)atof(tokens[3].c_str()) });
		//			} else if(tokens[0] == "vt") {
		//				UVs.push_back({ (float)atof(tokens[1].c_str()), 1.0f - (float)atof(tokens[2].c_str()) });
		//			} else if(tokens[0] == "f") {
		//				int first = Indices.size();
		//				std::vector<Index> face;
		//				for(int i = 1; i < tokens.size(); i++) {
		//					Index tmp{ -1, -1, -1 };
		//					auto idx = PVX::String::Split(tokens[i], "/");
		//					for(int i = 0; i < idx.size(); i++) {
		//						tmp.Array[i] = atoi(idx[i].c_str()) - 1;
		//					}
		//					face.push_back(tmp);
		//				}
		//				for(int i = 2; i < face.size(); i++) {
		//					Indices.push_back(face[0]);
		//					Indices.push_back(face[i - 1]);
		//					Indices.push_back(face[i]);
		//				}
		//			} else if(tokens[0] == "o") {
		//				break;
		//			} else if(tokens[0] == "usemtl") { 
		//				CurrentMat = Materials[tokens[1]];
		//			}
		//		}
		//		if(Indices.size()) {
		//			ObjectBuilder gl;
		//			gl.Begin(GL_TRIANGLES);
		//			for(auto & f : Indices) {
		//				if(f.Normal != -1) gl.Normal(Normals[f.Normal]);
		//				if(f.UV != -1) gl.TexCoord(UVs[f.UV]);
		//				gl.Vertex(Vertices[f.Vertex]);
		//			}
		//			gl.End();
		//			ret.push_back(gl.Build());
		//			ret.back().Material = CurrentMat;
		//		}
		//		//Vertices.clear();
		//		//Normals.clear();
		//		//UVs.clear();
		//		Indices.clear();
		//	}
		//	return ret;
		//}
	}
}
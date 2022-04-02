#include <fbxsdk.h>
#include <PVX_Object3D.h>
#include <PVX_Animation.h>
#include <PVX.inl>
#include <sstream>
#include <map>
#include <PVX_Encrypt.h>

//using namespace fbxsdk;

namespace PVX::Object3D {


	size_t Reindex(std::vector<unsigned char>& VertexData, std::vector<int>& Index, int Stride) {
		size_t VertexCount = 0;
		std::vector<unsigned char> out;
		out.reserve(VertexData.size());
		std::vector<int> IndexOut;
		IndexOut.reserve(Index.size());

		std::map<unsigned int, std::vector<int>> HashMap;
		for (auto i : Index) {
			unsigned char* vec = VertexData.data() + size_t(i) * Stride;
			auto& similar = HashMap[PVX::Encrypt::CRC32_Algorithm().Update(vec, Stride).Get()];
			for (auto idx: similar) {
				if (!memcmp(out.data() + size_t(idx) * Stride, vec, Stride)) {
					IndexOut.push_back(idx);
					goto Found;
				}
			}
			memcpy(&out[PVX::Extend(out, Stride)], vec, Stride);
			similar.push_back(VertexCount);
			IndexOut.push_back(VertexCount++);
		Found:
			continue;
		}
		out.shrink_to_fit();
		VertexData = std::move(out);
		Index = std::move(IndexOut);
		return VertexCount;
	}

	//struct FbxLoad {
	//	std::string Name;
	//	FbxMesh* Mesh;
	//	FbxSkeleton* Skeleton;
	//	FbxNode* Node;
	//	Transform TransformItem;
	//	std::vector<FbxLoad> Child;
	//};

	typedef struct Influence {
		int Index;
		float Weight;
	} Influence;

	enum class VertexItemType_Primitive {
		VertexItemType_Bool,
		VertexItemType_Int,
		VertexItemType_UInt,
		VertexItemType_Float,
		VertexItemType_Double,
		VertexItemType_DWORD = 0xffffffff
	};

	typedef struct VertexItemType {
		VertexItemType_Primitive Type;
		int ItemSize;
		int ItemCount;
		int ItemStride;
		unsigned int GLSL, HLSL;
	} VertexItemType;

	typedef struct VertexItem {
		VertexItem(const std::string& Type, const std::string& Name, ItemUsage Usage, int Count = 1);
		std::string Type;
		std::string Name;
		union {
			int Index;
			int Count;
		};
		ItemUsage Usage;
		int Offset;
		VertexItemType* pType;
	protected:
		VertexItem() {};
		friend struct DeinterleavedItem;
	} VertexItem;

	static std::map<std::string, VertexItemType> Types{
		{ "int", { VertexItemType_Primitive::VertexItemType_Int, 4, 1, 4, 0, 0 } },
		{ "uint", { VertexItemType_Primitive::VertexItemType_UInt, 4, 1, 4, 0, 0 } },

		{ "int4", { VertexItemType_Primitive::VertexItemType_Int, 4, 4, 16, 0, 0 } },
		{ "uint4", { VertexItemType_Primitive::VertexItemType_UInt, 4, 4, 16, 0, 0 } },

		{ "float", { VertexItemType_Primitive::VertexItemType_Float, 4, 1, 4, 0, 0 } },
		{ "float2", { VertexItemType_Primitive::VertexItemType_Float, 4, 2, 8, 0, 0 } },
		{ "float3", { VertexItemType_Primitive::VertexItemType_Float, 4, 3, 12, 0, 0 } },
		{ "float4", { VertexItemType_Primitive::VertexItemType_Float, 4, 4, 16, 0, 0 } },

		{ "mat3", { VertexItemType_Primitive::VertexItemType_Float, 4, 9, 4 * 9, 0, 0 } },
		{ "mat3x4", { VertexItemType_Primitive::VertexItemType_Float, 4, 3 * 4, 3 * 4 * 4, 0, 0 } },
		{ "mat4", { VertexItemType_Primitive::VertexItemType_Float, 4, 4 * 4, 4 * 4 * 4, 0, 0 } },
		{ "byte4", { VertexItemType_Primitive::VertexItemType_Int, 1, 4, 4, 0, 0 } },
	};


	static std::map<std::string, VertexAttribute> myTypes {
		{ "byte4", { VertexAttribute::_Type::Byte, 4, 0, ItemUsage::ItemUsage_General, "" } },
		{ "float2", { VertexAttribute::_Type::Float, 2, 0, ItemUsage::ItemUsage_General, "" } },
		{ "float3", { VertexAttribute::_Type::Float, 3, 0, ItemUsage::ItemUsage_General, "" } },
		{ "float4", { VertexAttribute::_Type::Float, 4, 0, ItemUsage::ItemUsage_General, "" } },
		{ "int4", { VertexAttribute::_Type::Int, 4, 0, ItemUsage::ItemUsage_General, "" } },
	};

	struct DeinterleavedItem {
		VertexItem Description;
		std::vector<unsigned char> Data;

		template<typename T>
		T* Ptr() { return (T*)&Data[0]; }

		template<typename T>
		DeinterleavedItem(const std::string& Name, const std::string& Type, ItemUsage Usage, const std::vector<T>& Data, int Index = 0) : Description{} {
			Description.Name = Name;
			Description.Type = Type;
			Description.Usage = Usage;
			Description.Count = Index;
			InitType();
			Set(Data);
		}
		DeinterleavedItem(const std::string& Name, const std::string& Type, ItemUsage Usage, int Count = 1) : Description{} {
			Description.Name = Name;
			Description.Type = Type;
			Description.Usage = Usage;
			Description.Count = Count;
			InitType();
		}

		template<typename T>
		void Set(const std::vector<T>& data) {
			Data.resize(sizeof(T) * data.size());
			memcpy(&Data[0], &data[0], Data.size());
		}

		template<typename T>
		void Add(const T& item) {
			auto sz = Data.size();
			Data.resize(sz + sizeof(T));
			memcpy(&Data[sz], &item, sizeof(T));
		}

		template<typename T>
		std::vector<T> Get() {
			std::vector<T> data(Data.size() / sizeof(T));
			memcpy(&data[0], &Data[0], Data.size());
			return data;
		}

		DeinterleavedItem() {};
		void InitType() {
			Description.pType = &Types[Description.Type];
		}
		friend class Part;
	};


	inline PVX::Vector4D ToVec4(const FbxVector4& v) {
		return { (float)v[0], (float)v[1], (float)v[2], (float)v[3] };
	}
	inline PVX::Vector4D ToVec4_1(const FbxVector4& v) {
		return { (float)v[0], (float)v[1], (float)v[2], 1.0f };
	}
	inline PVX::Vector3D ToVec3(const FbxVector4& v) {
		return { (float)v[0], (float)v[1], (float)v[2] };
	}
	inline PVX::Vector4D ToVec4(const FbxVector2& v) {
		return { (float)v[0], (float)v[1], 0, 0 };
	}
	inline PVX::Vector2D ToVec2(const FbxVector2& v) {
		return { (float)v[0], (float)v[1] };
	}
	inline PVX::Matrix4x4 ToMat(const FbxMatrix& m) {
		return {
			(float)m[0][0], (float)m[0][1], (float)m[0][2], (float)m[0][3],
			(float)m[1][0], (float)m[1][1], (float)m[1][2], (float)m[1][3],
			(float)m[2][0], (float)m[2][1], (float)m[2][2], (float)m[2][3],
			(float)m[3][0], (float)m[3][1], (float)m[3][2], (float)m[3][3]
		};
	}

	static std::vector<DeinterleavedItem> LoadShape(FbxShape* Shape, int* Index, int PolyVertexCount) {
		std::vector<DeinterleavedItem> Items;

		// Position
		{
			auto* Vertices = Shape->GetControlPoints();
			Items.push_back(DeinterleavedItem("Position", "float3", ItemUsage::ItemUsage_Position, PVX::Map(PolyVertexCount, [&](size_t i) { return ToVec3(Vertices[Index[i]]); })));
		}

		// Normal
		{
			FbxLayerElementArrayTemplate<FbxVector4>* Normals;
			Shape->GetNormals(&Normals);
			if (Normals) {
				Items.push_back(DeinterleavedItem("Normal", "float3", ItemUsage::ItemUsage_Normal, [&] {
					if (Normals->GetCount() == PolyVertexCount)
						return PVX::Map(PolyVertexCount, [Normals](size_t i) { return ToVec3(Normals->GetAt(int(i))); });
					else
						return PVX::Map(PolyVertexCount, [Normals, Index](size_t i) { return ToVec3(Normals->GetAt(Index[i])); });
				}()));
			}
		}

		// TexCoords
		{
			FbxStringList UV_Names;
			Shape->GetUVSetNames(UV_Names);
			int UV_Count = UV_Names.GetCount();

			for (auto i = 1; i <= UV_Count; i++) {
				std::vector<PVX::Vector2D> UVs;

				std::stringstream uvName;
				uvName << "TexCoord";
				if (i > 1)uvName << i;

				auto* uv = Shape->GetElementUV(UV_Names[i - 1]);
				auto& Direct = uv->GetDirectArray();
				switch (uv->GetMappingMode()) {
					case FbxLayerElement::eByPolygonVertex:
					{
						auto& Map = uv->GetIndexArray();
						UVs = PVX::Map(PolyVertexCount, [&](size_t j) { return ToVec2(Direct[Map[int(j)]]); });
					}
					break;
					case FbxLayerElement::eByControlPoint:
						UVs = PVX::Map(PolyVertexCount, [&](size_t j) { return ToVec2(Direct[Index[j]]); });
						break;
				}

				Items.push_back(DeinterleavedItem(uvName.str(), "float2", ItemUsage::ItemUsage_UV, UVs));

				// Tangents
				{
					FbxLayerElementArrayTemplate<FbxVector4>* Tangents, *Binormals;
					if (Shape->GetTangents(&Tangents, i - 1) && Shape->GetBinormals(&Binormals, i - 1)) {
						//auto Data = [&] {
						//	if (Tangents->GetCount() == PolyVertexCount)
						//		return PVX::Map(PolyVertexCount, [Tangents](size_t i) { return ToVec4(Tangents->GetAt(int(i))); });
						//	else
						//		return PVX::Map(PolyVertexCount, [Tangents, Index](size_t i) { return ToVec4(Tangents->GetAt(Index[i])); });
						//}();
						//auto DataB = [&] {
						//	if (Binormals->GetCount() == PolyVertexCount)
						//		return PVX::Map(PolyVertexCount, [Binormals](size_t i) { return ToVec3(Binormals->GetAt(int(i))); });
						//	else
						//		return PVX::Map(PolyVertexCount, [Binormals, Index](size_t i) { return ToVec3(Binormals->GetAt(Index[i])); });
						//}();
						//for (auto i = 0; i<Data.size(); i++) {
						//	Data[i].w = DataB[i].Dot(NormalData[i].Cross(Data[i].Vec3))>0 ? 1.0f : -1.0f;
						//}
						Items.push_back(DeinterleavedItem("Tangent", "float4", ItemUsage::ItemUsage_Tangent, [&] {
							if (Tangents->GetCount() == PolyVertexCount)
								return PVX::Map(PolyVertexCount, [Tangents](size_t i) { return ToVec4(Tangents->GetAt(int(i))); });
							else
								return PVX::Map(PolyVertexCount, [Tangents, Index](size_t i) { return ToVec4(Tangents->GetAt(Index[i])); });
						}()));
					}
				}
			}

			return Items;
		}


	}

	static std::vector<DeinterleavedItem> LoadMorphShape(FbxShape* Shape, int* Index, int PolyVertexCount) {
		std::vector<DeinterleavedItem> Items;

		// Position
		{
			auto* Vertices = Shape->GetControlPoints();
			auto dt = PVX::Map(PolyVertexCount, [&](size_t i) { return ToVec3(Vertices[Index[i]]); });
			Items.push_back(DeinterleavedItem("Position", "float3", ItemUsage::ItemUsage_MorphPosition, dt));
		}

		// Normal
		{
			FbxLayerElementArrayTemplate<FbxVector4>* Normals;
			Shape->GetNormals(&Normals);
			if (Normals) {
				auto dt = [&] {
					if (Normals->GetCount() == PolyVertexCount)
						return PVX::Map(PolyVertexCount, [Normals](size_t i) { return ToVec3(Normals->GetAt(int(i))); });
					else
						return PVX::Map(PolyVertexCount, [Normals, Index](size_t i) { return ToVec3(Normals->GetAt(Index[i])); });
				}();
				Items.push_back(DeinterleavedItem("Normal", "float3", ItemUsage::ItemUsage_MorphNormal, dt));
			}
		}

		// TexCoords
		{
			FbxStringList UV_Names;
			Shape->GetUVSetNames(UV_Names);
			int UV_Count = UV_Names.GetCount();

			for (auto i = 1; i <= UV_Count; i++) {
				std::vector<PVX::Vector2D> UVs;

				std::stringstream uvName;
				uvName << "TexCoord";
				if (i > 1)uvName << i;

				auto* uv = Shape->GetElementUV(UV_Names[i - 1]);
				auto& Direct = uv->GetDirectArray();
				switch (uv->GetMappingMode()) {
					case FbxLayerElement::eByPolygonVertex:
					{
						auto& Map = uv->GetIndexArray();
						UVs = PVX::Map(PolyVertexCount, [&](size_t j) { return ToVec2(Direct[Map[int(j)]]); });
					}
					break;
					case FbxLayerElement::eByControlPoint:
						UVs = PVX::Map(PolyVertexCount, [&](size_t j) { return ToVec2(Direct[Index[j]]); });
						break;
				}

				Items.push_back(DeinterleavedItem(uvName.str(), "float2", ItemUsage::ItemUsage_MorphUV, UVs));

				// Tangents
				{
					FbxLayerElementArrayTemplate<FbxVector4>* Tangents, * Binormals;
					if (Shape->GetTangents(&Tangents, i - 1) && Shape->GetBinormals(&Binormals, i - 1)) {
						Items.push_back(DeinterleavedItem("Tangent", "float4", ItemUsage::ItemUsage_MorphTangent, [&] {
							if (Tangents->GetCount() == PolyVertexCount)
								return PVX::Map(PolyVertexCount, [Tangents](size_t i) { return ToVec4(Tangents->GetAt(int(i))); });
							else
								return PVX::Map(PolyVertexCount, [Tangents, Index](size_t i) { return ToVec4(Tangents->GetAt(Index[i])); });
						}()));
					}
				}
			}

			return Items;
		}


	}

	void MakeWeights(std::vector<PVX::Vector4D>& Weights, std::vector<PVX::iVector4D>& WeightIndices, size_t VertexCount, const std::vector<std::vector<Influence>>& Influences) {
		Weights.resize(VertexCount);
		WeightIndices.resize(VertexCount);
		memset(&Weights[0], 0, sizeof(PVX::Vector4D) * VertexCount);
		memset(&WeightIndices[0], 0, sizeof(decltype(WeightIndices[0])) * VertexCount);

		std::vector<std::vector<Influence>> infs(VertexCount);

		int nextBone = 0;
		int step;
		for (auto& b : Influences) {
			step = 0;
			for (auto& inf: b) {
				if (inf.Weight > 0.00001f) {
					infs[inf.Index].push_back({ nextBone, inf.Weight });
					step = 1;
				}
			}
			nextBone += step;
		}
		for (auto i = 0; i < VertexCount; i++) {
			auto& inf = infs[i];
			auto& weight = Weights[i].Array;
			auto& windex = WeightIndices[i].Array;

			PVX::SortInplace(inf, [](const Influence& a, const Influence& b) {
				return a.Weight > b.Weight;
			});
			float sum = 0;
			for (int j = 0; j<4&&j<inf.size(); j++) {
				sum += (weight[j] = inf[j].Weight);
				windex[j] = inf[j].Index;
			}
			Weights[i] /= sum;
		}
	}


	//void MakeWeights(std::vector<PVX::Vector4D>& Weights, std::vector<PVX::iVector4D>& WeightIndices, size_t VertexCount, const std::vector<std::vector<Influence>>& Influences) {
	//	Weights.resize(VertexCount);
	//	WeightIndices.resize(VertexCount);
	//	std::vector<int> Counts(VertexCount);
	//	memset(&Weights[0], 0, sizeof(PVX::Vector4D) * VertexCount);
	//	memset(&WeightIndices[0], 0, sizeof(PVX::ucVector4D) * VertexCount);
	//	memset(&Counts[0], 0, sizeof(int) * VertexCount);

	//	std::vector<Influence> Test;

	//	int nextBone = 0;
	//	for (auto i = 0; i < Influences.size(); i++) {
	//		for (auto& f : Influences[i]) {
	//			if (f.Weight > 0.00001f && Counts[f.Index] < 4) {
	//				WeightIndices[f.Index].Array[Counts[f.Index]] = nextBone;
	//				Weights[f.Index].Array[Counts[f.Index]] = f.Weight;
	//				Counts[f.Index]++;
	//			} else {
	//				Test.push_back(f);
	//			}
	//		}
	//		if (Influences[i].size())nextBone++;
	//	}
	//	for (auto& w : Weights) {
	//		if (float sum = w.x + w.y + w.z + w.w) {
	//			w /= sum;
	//		}
	//	}
	//}

	FbxAMatrix GetGeometry(FbxNode* pNode) {
		const FbxVector4 lT = pNode->GetGeometricTranslation(FbxNode::eSourcePivot);
		const FbxVector4 lR = pNode->GetGeometricRotation(FbxNode::eSourcePivot);
		const FbxVector4 lS = pNode->GetGeometricScaling(FbxNode::eSourcePivot);
		return FbxAMatrix(lT, lR, lS);
	}

	static bool LoadSkin(FbxMesh* Mesh, std::vector<DeinterleavedItem>& Parts, std::vector<std::string>& Bones, std::vector<PVX::Matrix4x4>& PostTransform) {
		int* Index = Mesh->GetPolygonVertices();
		int PolyVertexCount = Mesh->GetPolygonVertexCount();
		FbxSkin* skin = (FbxSkin*)Mesh->GetDeformer(0, FbxDeformer::eSkin);
		if (skin) {
			FbxAMatrix NodeTran = GetGeometry(Mesh->GetNode());
			std::vector<PVX::Vector4D> Weights;
			std::vector<PVX::iVector4D> WeightIndices;

			MakeWeights(Weights, WeightIndices, Mesh->GetControlPointsCount(), PVX::Map(skin->GetClusterCount(), [&](size_t i) {
				FbxCluster* cl = skin->GetCluster(int(i));

				FbxAMatrix transformMatrix, transformLinkMatrix, BoneMat;
				cl->GetTransformMatrix(transformMatrix);
				cl->GetTransformLinkMatrix(transformLinkMatrix);
				BoneMat = transformLinkMatrix.Inverse() * transformMatrix * NodeTran;

				int* cp = cl->GetControlPointIndices();
				double* cpw = cl->GetControlPointWeights();

				auto inflCount = cl->GetControlPointIndicesCount();
				if (inflCount && PVX::Any(inflCount, [&](auto i) { return float(cpw[i]) > 0.00001f; })) {
					Bones.push_back(cl->GetLink()->GetName());
					PostTransform.push_back(ToMat(BoneMat));
				}

				return PVX::Map(inflCount, [&](size_t j) { return Influence{ cp[j], (float)cpw[j] }; });
			}));

			auto WeightData = PVX::Map(PolyVertexCount, [&](size_t i) { return Weights[Index[i]]; });
			auto WeightIndexData = PVX::Map(PolyVertexCount, [&](size_t i) { return WeightIndices[Index[i]]; });
			Parts.push_back(DeinterleavedItem("Weights", "float4", ItemUsage::ItemUsage_Weight, WeightData));
			Parts.push_back(DeinterleavedItem("WeightIndices", "int4", ItemUsage::ItemUsage_WeightIndex, WeightIndexData));
			return true;
		}
		return false;
	}

	std::vector<std::vector<std::vector<DeinterleavedItem>>> GetBlendShapes(FbxMesh* Mesh) {
		int* Index = Mesh->GetPolygonVertices();
		int PolyVertexCount = Mesh->GetPolygonVertexCount();
		std::vector<std::vector<std::vector<DeinterleavedItem>>> Items;
		int BlendCount = Mesh->GetDeformerCount(FbxDeformer::eBlendShape);
		for (int i = 0; i < BlendCount; i++) {
			auto* Blend = (FbxBlendShape*)Mesh->GetDeformer(i, FbxDeformer::eBlendShape);
			int cnlCount = Blend->GetBlendShapeChannelCount();
			std::vector<std::vector<DeinterleavedItem>> Channel;
			for (int j = 0; j < cnlCount; j++) {
				FbxBlendShapeChannel* cnl = Blend->GetBlendShapeChannel(j);
				int bsCount = cnl->GetTargetShapeCount();
				double* full = cnl->GetTargetShapeFullWeights();

				for (int k = 0; k < bsCount; k++) {
					cnl->DeformPercent = full[k];
					Channel.push_back(LoadMorphShape(cnl->GetTargetShape(k), Index, PolyVertexCount));
				}
				cnl->DeformPercent = 0;
			}
			Items.push_back(Channel);
		}
		return Items;
	}

	std::vector<std::vector<DeinterleavedItem>> GetBlendShapes(FbxMesh* Mesh, std::vector<std::string>& Morphs) {
		Morphs.clear();
		int* Index = Mesh->GetPolygonVertices();
		int PolyVertexCount = Mesh->GetPolygonVertexCount();
		std::vector<std::vector<DeinterleavedItem>> Items;
		std::vector<PVX::Animation::Blender::Channel> Channels;

		int BlendCount = Mesh->GetDeformerCount(FbxDeformer::eBlendShape);
		for (int i = 0; i < BlendCount; i++) {
			auto* Blend = (FbxBlendShape*)Mesh->GetDeformer(i, FbxDeformer::eBlendShape);
			int cnlCount = Blend->GetBlendShapeChannelCount();
			for (int j = 0; j < cnlCount; j++) {
				FbxBlendShapeChannel* cnl = Blend->GetBlendShapeChannel(j);
				int bsCount = cnl->GetTargetShapeCount();
				double* full = cnl->GetTargetShapeFullWeights();
				
				for (int k = 0; k < bsCount; k++) {
					cnl->DeformPercent = full[k];
					Items.push_back(LoadMorphShape(cnl->GetTargetShape(k), Index, PolyVertexCount));
					Morphs.push_back(cnl->GetTargetShape(k)->GetName());
				}
				cnl->DeformPercent = 0;
			}
		}
		//Blender = Channels;
		return Items;
	}

	ObjectSubPart InterleaveAttributes(const std::vector<DeinterleavedItem>& Attributes) {
		ObjectSubPart ret;
		ret.Stride = std::reduce(Attributes.begin(), Attributes.end(), 0, [](int acc, const DeinterleavedItem& it) {
			return acc + it.Description.pType->ItemStride;
		});
		ret.VertexCount = int(Attributes[0].Data.size()) / (Attributes[0].Description.pType->ItemStride);
		ret.VertexData.resize(ret.Stride * ret.VertexCount);
		int Offset = 0;
		for (auto& it : Attributes) {
			ret.Attributes.push_back([&] {
				VertexAttribute ret = myTypes.at(it.Description.Type);
				ret.Name = it.Description.Name;
				ret.Offset = Offset;
				ret.Usage = it.Description.Usage;
				return ret;
			}());
			PVX::Interleave(ret.VertexData.data() + Offset, ret.Stride, it.Data.data(), it.Description.pType->ItemStride, ret.VertexCount);
			Offset += it.Description.pType->ItemStride;
		}
		return ret;
	}

	std::string MakeTypeName(const VertexAttribute& attr) {
		std::string ret;
		switch (attr.Type) {
			case VertexAttribute::_Type::Byte:
				ret = "byte"; break;
			case VertexAttribute::_Type::Int:
				ret = "int"; break;
			case VertexAttribute::_Type::Float:
				ret = "float"; break;
		}
		ret.push_back('0' + attr.Count);
		return ret;
	}

	std::vector<DeinterleavedItem> DeinterleaveAttributes(const ObjectSubPart& part) {
		std::vector<DeinterleavedItem> ret;
		ret.reserve(part.Attributes.size());
		for (auto& attr : part.Attributes) {
			auto& Item = ret.emplace_back();
			Item.Description.Name = attr.Name;
			Item.Description.Type = MakeTypeName(attr);
			Item.Description.pType = &Types.at(Item.Description.Type);
			Item.Description.Usage = attr.Usage;
			auto stride = Item.Description.pType->ItemStride;
			Item.Data.resize(stride * part.VertexCount);
			PVX::Interleave(Item.Data.data(), stride, part.VertexData.data() + attr.Offset, part.Stride, part.VertexCount);
		}
		return ret;
	}

	static std::vector<int> LoadPolygonIndices(FbxMesh* Mesh) {
		int Base = 0;
		auto Polygons = PVX::Map(Mesh->GetPolygonCount(), [&](size_t i) {
			return PVX::IndexArrayRef(Mesh->GetPolygonSize(int(i)), Base);
		});
		std::vector<int> Index;
		for (auto& p : Polygons) {
			for (int i = 2; i < p.size(); i++) {
				Index.push_back(p[0]);
				Index.push_back(p[i - 1]);
				Index.push_back(p[i]);
			}
		}
		return Index;
	}

	std::vector<int> GetTriangleMaterialMap(FbxMesh* Mesh) {
		std::vector<int> ret;
		int PolygonCount = Mesh->GetPolygonCount();
		auto Layer = Mesh->GetLayer(0);
		fbxsdk::FbxLayerElementMaterial* pLayerMaterials = Layer->GetMaterials();
		if (pLayerMaterials) {
			FbxLayerElementArrayTemplate<int>& indexArray = pLayerMaterials->GetIndexArray();

			if (pLayerMaterials->GetMappingMode() == FbxLayerElement::eAllSame) {
				int m = indexArray[0];
				for (int i = 0; i < PolygonCount; i++) {
					int count = Mesh->GetPolygonSize(i) - 2;
					for (int j = 0; j < count; j++) {
						ret.push_back(m);
					}
				}
			} else {
				for (int i = 0; i < PolygonCount; i++) {
					int count = Mesh->GetPolygonSize(i) - 2;
					for (int j = 0; j < count; j++) {
						ret.push_back(indexArray[i]);
					}
				}
			}
		}
		return ret;
	}

	std::vector<ObjectSubPart> Split(const ObjectSubPart& part, const std::vector<int> PolyIndex) {
		int Max = -1;
		for (auto p : PolyIndex) if (Max < p) Max = p;

		if (Max==-1)
			return {};

		std::vector<ObjectSubPart> ret(Max + 1);
		for (auto& r : ret) {
			r.VertexCount = 0;
			r.Stride = part.Stride;
			r.Attributes = part.Attributes;
			r.VertexData.reserve(part.VertexData.size());
		}
		PVX::Triangle* tri = (PVX::Triangle*)&part.Index[0];
		auto triCount = part.Index.size() / 3;

		for (auto i = 0; i < triCount; i++) {

			auto& p = ret[PolyIndex[i]];

			for (int t = 0; t < 3; t++) {
				auto sz = p.VertexData.size();
				p.VertexData.resize(sz + part.Stride);
				memcpy(&p.VertexData[sz], &part.VertexData[tri[i].Index[t] * size_t(part.Stride)], part.Stride);
				p.Index.push_back(p.VertexCount++);
			}
		}
		for (auto& r : ret) {
			r.VertexData.shrink_to_fit();
			Reindex(r.VertexData, r.Index, r.Stride);
			r.VertexCount = int(r.VertexData.size()) / r.Stride;
		}

		//if (ShadowIndex.size()) for (auto& r : ret) r.MakeShadowIndex();
		return ret;
	}

	std::string ToMatName(const char* Name) {
		std::string nm = Name;
		for (int j = 0; j < nm.size(); j++)
			if (nm[j] == ':')nm[j] = '_';
		return std::move(nm);
	}

	std::string TextureFilename(FbxTexture* Tex) {
		return FbxCast<FbxFileTexture>(Tex)->GetFileName();
	}

	PVX::Vector3D GetProperty3D(FbxSurfaceMaterial* mat, const char* propName) {
		const auto prop = mat->FindProperty(propName);
		if (prop.IsValid()) 
			return ToVec3(prop.Get<FbxVector4>());
		return {};
	}
	PVX::Vector2D GetProperty2D(FbxSurfaceMaterial* mat, const char* propName) {
		const auto prop = mat->FindProperty(propName);
		if (prop.IsValid())
			return ToVec2(prop.Get<FbxVector2>());
		return {};
	}
	float GetPropertyFloat(FbxSurfaceMaterial* mat, const char* propName) {
		const auto prop = mat->FindProperty(propName);
		if (prop.IsValid())
			return prop.Get<double>();
		return {};
	}
	std::string GetPropertyTexture(FbxSurfaceMaterial* mat, const char* propName) {
		const auto prop = mat->FindProperty(propName);
		if (prop.IsValid())
			if (FbxTexture* Tex = prop.GetSrcObject<FbxTexture>())
				return FbxCast<FbxFileTexture>(Tex)->GetFileName();
		return {};
	}

	int ReadMaterials(PVX::Object3D::Object& Scene, FbxScene* fbxScene) {
		int mCount = fbxScene->GetMaterialCount();
		for (int i = 0; i < mCount; i++) {
			FbxSurfaceMaterial* mat = fbxScene->GetMaterial(i);			

			auto Name = ToMatName(mat->GetName());

			PVX::Object3D::PlaneMaterial& stdMat = Scene.Material[Name];

			stdMat.Diffuse = stdMat.Color = GetProperty3D(mat, FbxSurfaceMaterial::sDiffuse);
			stdMat.DiffuseFactor = GetPropertyFloat(mat, FbxSurfaceMaterial::sDiffuseFactor);

			stdMat.Ambient = GetProperty3D(mat, FbxSurfaceMaterial::sAmbient);
			stdMat.AmbientFactor = GetPropertyFloat(mat, FbxSurfaceMaterial::sAmbientFactor);

			stdMat.Emissive = GetProperty3D(mat, FbxSurfaceMaterial::sEmissive);
			stdMat.EmissiveFactor = GetPropertyFloat(mat, FbxSurfaceMaterial::sEmissiveFactor);

			stdMat.Bump = GetPropertyFloat(mat, FbxSurfaceMaterial::sBumpFactor);

			stdMat.Specular = GetProperty3D(mat, FbxSurfaceMaterial::sSpecular);
			stdMat.SpecularFactor = GetPropertyFloat(mat, FbxSurfaceMaterial::sSpecularFactor);
			stdMat.SpecularPower = GetPropertyFloat(mat, FbxSurfaceMaterial::sShininess);

			PVX::Vector3D tmpVec3;
			tmpVec3 = GetProperty3D(mat, FbxSurfaceMaterial::sTransparentColor);
			tmpVec3 *= GetProperty3D(mat, FbxSurfaceMaterial::sTransparencyFactor);
			stdMat.Alpha = 1.0f - (tmpVec3.x + tmpVec3.y + tmpVec3.z) / 3.0f;

			stdMat.Textures.Diffuse = GetPropertyTexture(mat, FbxSurfaceMaterial::sDiffuse);
			stdMat.Textures.Ambient = GetPropertyTexture(mat, FbxSurfaceMaterial::sAmbient);
			stdMat.Textures.Emissive = GetPropertyTexture(mat, FbxSurfaceMaterial::sEmissive);
			stdMat.Textures.Specular = GetPropertyTexture(mat, FbxSurfaceMaterial::sSpecular);

			if (auto pbr = mat->FindProperty("Metalness"); pbr.IsValid()) {
				stdMat.Metallic = pbr.Get<double>();
			} else {
				stdMat.Metallic = 0;
			}

			if (auto pbr = mat->FindProperty("Roughness"); pbr.IsValid()) {
				stdMat.Roughness = pbr.Get<double>();
			} else {
				stdMat.Roughness = 1.0f - stdMat.SpecularFactor /(stdMat.AmbientFactor + stdMat.DiffuseFactor + stdMat.SpecularFactor + 1e-9);
			}

			//FbxSurfaceLambert* Lambert = (FbxSurfaceLambert*)mat;

			//stdMat.Color = ToVec3(Lambert->Diffuse.Get());
			////stdMat.Color *= (float)Lambert->ColorFactor.Get();
			//stdMat.Diffuse = ToVec3(Lambert->Diffuse.Get());
			//stdMat.DiffuseFactor = (float)Lambert->DiffuseFactor.Get();
			//stdMat.Ambient = ToVec3(Lambert->Ambient.Get());
			//stdMat.AmbientFactor = (float)Lambert->AmbientFactor.Get();
			//stdMat.Emissive = ToVec3(Lambert->Emissive.Get());
			//stdMat.EmissiveFactor = (float)Lambert->EmissiveFactor.Get();
			//stdMat.Bump = (float)Lambert->BumpFactor.Get();

			//PVX::Vector3D tmpVec;
			//tmpVec = ToVec3(Lambert->TransparentColor.Get());
			//tmpVec *= (float)Lambert->TransparencyFactor.Get();
			//stdMat.Alpha = 1.0f - (tmpVec.x + tmpVec.y + tmpVec.z) / 3.0f;


			//if (FbxTexture* Tex = Lambert->Diffuse.GetSrcObject<FbxTexture>())
			//	stdMat.Textures.Diffuse = FbxCast<FbxFileTexture>(Tex)->GetFileName();

			//if (FbxTexture* Tex = Lambert->Ambient.GetSrcObject<FbxTexture>())
			//	stdMat.Textures.Ambient = FbxCast<FbxFileTexture>(Tex)->GetFileName();

			//if (FbxTexture* Tex = Lambert->Emissive.GetSrcObject<FbxTexture>())
			//	stdMat.Textures.Emissive = FbxCast<FbxFileTexture>(Tex)->GetFileName();

			////int tmp = 0;

			////auto nrm = Lambert->NormalMap;
			////auto nrmc = nrm.GetSrcObjectCount();
			////for (auto i = 0; i< nrmc; i++) {
			////	auto test = nrm.GetSrcObject(i);
			////	tmp++;
			////}
			//////auto nrmt = Lambert->NormalMap.GetSrcObject<FbxTexture>();

			////auto bmp = Lambert->NormalMap;
			////auto bmpc = nrm.GetSrcObjectCount();
			////for (auto i = 0; i< bmpc; i++) {
			////	auto test = bmp.GetSrcObject(i);
			////	tmp++;
			////}
			//////auto bmpt = Lambert->NormalMap.GetSrcObject<FbxTexture>();

			//if (FbxTexture* Tex = Lambert->Bump.GetSrcObject<FbxTexture>())
			//	stdMat.Textures.Bump = FbxCast<FbxFileTexture>(Tex)->GetFileName();

			//if (FbxTexture* Tex = Lambert->NormalMap.GetSrcObject<FbxTexture>())
			//	stdMat.Textures.Bump = FbxCast<FbxFileTexture>(Tex)->GetFileName();


			//if (FbxTexture* Tex = Lambert->TransparencyFactor.GetSrcObject<FbxTexture>())
			//	stdMat.Textures.Transparency = FbxCast<FbxFileTexture>(Tex)->GetFileName();

			//if (mat->GetClassId().Is(FbxSurfacePhong::ClassId)) {
			//	FbxSurfacePhong* Phong = (FbxSurfacePhong*)mat;

			//	stdMat.Specular = ToVec3(Phong->Specular.Get());
			//	stdMat.SpecularFactor = (float)Phong->SpecularFactor.Get();
			//	stdMat.SpecularPower = (float)Phong->Shininess.Get();

			//	if (FbxTexture* Tex = Phong->Specular.GetSrcObject<FbxTexture>())
			//		stdMat.Textures.Specular = FbxCast<FbxFileTexture>(Tex)->GetFileName();
			//}


			
			//if (auto pbr = mat->FindProperty("Metallic"); pbr.IsValid()) {
			//	stdMat.Metallic = pbr.Get<double>();
			//} else {
			//	stdMat.Metallic = 0;
			//}
			//
			//if (auto pbr = mat->FindProperty("Roughness"); pbr.IsValid()) {
			//	stdMat.Roughness = pbr.Get<double>();
			//} else {
			//	stdMat.Roughness = 1.0f - stdMat.SpecularFactor /(stdMat.AmbientFactor + stdMat.DiffuseFactor + stdMat.SpecularFactor);
			//}
		}
		return 0;
	}

	//Transform* FindNode(FbxNode* node, FbxLoad& Heir) {
	//	if (Heir.Node == node) return &Heir.TransformItem;
	//	for (auto& ch : Heir.Child) {
	//		if (auto h = FindNode(node, ch)) return h;
	//	}
	//	return nullptr;
	//}

	//bool GetSkin(FbxLoad& hier, ObjectPart& mesh, std::vector<PVX::Weight>& Weights) {
	//	struct TempWeight {
	//		PVX::Weight Skin;
	//		int Count;
	//	};
	//	using uchar = unsigned char;

	//	FbxMesh* Node = hier.Mesh;
	//	int PolyVertexCount = Node->GetPolygonVertexCount();
	//	int VertexCount = Node->GetControlPointsCount();
	//	int* Index = Node->GetPolygonVertices();
	//	FbxSkin* skin = (FbxSkin*)Node->GetDeformer(0, FbxDeformer::eSkin);

	//	int ClusterCount = skin->GetClusterCount();

	//	FbxAMatrix NodeTran = GetGeometry(Node->GetNode());

	//	std::vector<TempWeight> tmpWeights(VertexCount);

	//	size_t BoneResetSize = mesh.BoneNodes.size();
	//	size_t ptReset = mesh.BonePostTransform.size();

	//	for (int i = 0; i < ClusterCount; i++) {
	//		FbxCluster* cl = skin->GetCluster(i);
	//		if (auto* Bone = FindNode(cl->GetLink(), hier)) {
	//			size_t BoneIndex = mesh.BoneNodes.size();
	//			int cpIndex = cl->GetControlPointIndicesCount();
	//			int* cp = cl->GetControlPointIndices();
	//			double* cpw = cl->GetControlPointWeights();

	//			for (int j = 0; j < cpIndex; j++) {
	//				TempWeight& w = tmpWeights[cp[j]];
	//				if (w.Count < 4) {
	//					w.Skin.W[w.Count] = (float)cpw[j];
	//					w.Skin.I[w.Count] = (uchar)BoneIndex;
	//					w.Count++;
	//					continue;
	//				}
	//				mesh.BoneNodes.resize(BoneResetSize);
	//				mesh.BonePostTransform.resize(ptReset);
	//				return 0;
	//			}

	//			mesh.BoneNodes.push_back(Bone->Name);
	//			FbxAMatrix transformMatrix, transformLinkMatrix, BoneMat;
	//			cl->GetTransformMatrix(transformMatrix);
	//			cl->GetTransformLinkMatrix(transformLinkMatrix);
	//			BoneMat = transformLinkMatrix.Inverse() * transformMatrix * NodeTran;
	//			mesh.BonePostTransform.push_back(ToMat(BoneMat));
	//		}
	//	}
	//	if (size_t cnt = mesh.BoneNodes.size()) {
	//		Weights.resize(PolyVertexCount);
	//		PVX::Weight* weight = Weights.data();

	//		for (int i = 0; i < PolyVertexCount; i++)
	//			weight[i] = tmpWeights[Index[i]].Skin;
	//	}
	//	return mesh.BoneNodes.size() > BoneResetSize;
	//}

	DeinterleavedItem* FindDeinterleavedItem(std::vector<DeinterleavedItem>& List, const std::string& Name) {
		for (auto& i : List) {
			if (i.Description.Name == Name) {
				return &i;
			}
		}
		return 0;
	}

	static void LoadMesh(FbxMesh* Mesh, Object& scene) {
		int* Index = Mesh->GetPolygonVertices();
		int PolyVertexCount = Mesh->GetPolygonVertexCount();

		auto MainParts = LoadShape((FbxShape*)Mesh, Index, PolyVertexCount);

		auto& part = scene.Parts.emplace_back();
		part.TransformNode = Mesh->GetNode()->GetName();
		LoadSkin(Mesh, MainParts, part.BoneNodes, part.BonePostTransform);
		size_t MainPartsSize = MainParts.size();

		auto BlendShapes = GetBlendShapes(Mesh, part.BlendShapes);

		for (auto& vi : BlendShapes) {
			for (auto& i : vi) {
				if (auto& m = *FindDeinterleavedItem(MainParts, i.Description.Name); &m) {
					switch (i.Description.Usage) {
						case ItemUsage::ItemUsage_MorphPosition:
						case ItemUsage::ItemUsage_MorphNormal: {
							PVX::Vector3D* morph = (PVX::Vector3D*)i.Data.data();
							PVX::Vector3D* original = (PVX::Vector3D*)m.Data.data();
							i = DeinterleavedItem(i.Description.Name, "float4", i.Description.Usage, PVX::Map(i.Data.size() / sizeof(PVX::Vector3D), [&](size_t c) -> PVX::Vector4D {
								return { morph[c] - original[c] ,1.0f };
							}));
							break;
						}
						case ItemUsage::ItemUsage_MorphUV: {
							PVX::Vector2D* morph = (PVX::Vector2D*)i.Data.data();
							PVX::Vector2D* original = (PVX::Vector2D*)m.Data.data();
							i = DeinterleavedItem(i.Description.Name, "float4", i.Description.Usage, PVX::Map(i.Data.size() / sizeof(PVX::Vector2D), [&](size_t c) -> PVX::Vector4D {
								return { morph[c].x - original[c].x, morph[c].y - original[c].y, 0, 1.0f };
							}));
							break;
						}
						case ItemUsage::ItemUsage_MorphTangent: {
							PVX::Vector4D* morph = (PVX::Vector4D*)i.Data.data();
							PVX::Vector4D* original = (PVX::Vector4D*)m.Data.data();
							i = DeinterleavedItem(i.Description.Name, "float4", i.Description.Usage, PVX::Map(i.Data.size() / sizeof(PVX::Vector4D), [&](size_t c) -> PVX::Vector4D {
								return { (morph[c].Vec3 - original[c].Vec3) , original[c].w };
							}));
							break;
						}
					}
				}
			}
		}

		for (auto& b : BlendShapes) {
			for (auto& s : b) {
				MainParts.push_back(s);
			}
		}

		ObjectSubPart fullMesh = InterleaveAttributes(MainParts);

		fullMesh.Index = LoadPolygonIndices(Mesh);
		MainParts.clear();
		auto Splited = Split(fullMesh, GetTriangleMaterialMap(Mesh));


		int MatIndex = 0;
		for (auto& s : Splited) {
			if (s.Index.size()) {
				auto tmp = DeinterleaveAttributes(s);
				for (auto i = 0; i < MainPartsSize; i++) MainParts.push_back(std::move(tmp[i]));
				auto name = ToMatName(Mesh->GetNode()->GetMaterial(MatIndex++)->GetName());
				auto& pp = part.Parts[name];
				pp = InterleaveAttributes(MainParts);
				pp.Index = s.Index;

				if (tmp.size() > MainPartsSize) {
					MainParts.clear();
					for (auto i = MainPartsSize; i < tmp.size(); i++)
						MainParts.push_back(std::move(tmp[i]));
					auto morph = InterleaveAttributes(MainParts);

					pp.BlendAttributes = std::move(morph.Attributes);
					pp.BlendShapeData = std::move(morph.VertexData);
				}
				MainParts.clear();
			}
		}
	}

	PVX::Matrix4x4& (*_RotationOrder[])(PVX::Matrix4x4&, PVX::Vector3D&) = {
		PVX::RotateXYZ,
		PVX::RotateXZY,
		PVX::RotateYXZ,
		PVX::RotateYZX,
		PVX::RotateZXY,
		PVX::RotateZYX,
	};

	void GetTransform(FbxNode* node, Transform& hier) {

		FbxAMatrix Roff, Rp, Rpre, Rpost, iRp, Soff, Sp, iSp;
		FbxAMatrix PostTranslate, PostRotate, PostScale;

		Roff.SetIdentity(); Rp.SetIdentity(); Rpre.SetIdentity(), Rpost.SetIdentity(), Soff.SetIdentity(), Sp.SetIdentity();

		Roff.SetTOnly(node->GetRotationOffset(FbxNode::EPivotSet::eSourcePivot));
		Rp.SetTOnly(node->GetRotationPivot(FbxNode::EPivotSet::eSourcePivot));
		Rpre.SetROnly(node->GetPreRotation(FbxNode::EPivotSet::eSourcePivot));
		Rpost.SetROnly(node->GetPostRotation(FbxNode::EPivotSet::eSourcePivot));
		Soff.SetTOnly(node->GetScalingOffset(FbxNode::EPivotSet::eSourcePivot));
		Sp.SetTOnly(node->GetScalingPivot(FbxNode::EPivotSet::eSourcePivot));
		iRp = Rp.Inverse();
		iSp = Sp.Inverse();

		PostTranslate = Roff * Rp * Rpre;
		PostRotate = Rpost * iRp * Soff*Sp;
		PostScale = iSp;

		hier.PostTranslate = ToMat(PostTranslate);
		hier.PostRotate = ToMat(PostRotate);
		hier.PostScale = ToMat(PostScale);
	}

	void ReadAnimation(FbxNode* Node, Transform& hier) {
		FbxAnimStack* anim = Node->GetScene()->GetCurrentAnimationStack();
		if (!anim) return;
		FbxTimeSpan tSpan = anim->GetLocalTimeSpan();

		hier.Animation.FrameRate = 24.0f;
		double rate = 1.0f / hier.Animation.FrameRate;
		double start = tSpan.GetStart().GetSecondDouble();
		size_t FrameCount = (int)ceil(tSpan.GetDuration().GetSecondDouble()*hier.Animation.FrameRate);

		//hier->Transform->Animation = new PVX::Game::AnimationFrame[hier->Transform->FrameCount];

		hier.Animation.Position.reserve(FrameCount);
		hier.Animation.Rotation.reserve(FrameCount);
		hier.Animation.Scale.reserve(FrameCount);

		for (auto i = 0; i < FrameCount; i++) {
			FbxTime fbxCur = 0;
			fbxCur.SetSecondDouble(start + i * rate);
			auto pos = Node->EvaluateLocalTranslation(fbxCur);
			hier.Animation.Position.emplace_back((float)pos[0], (float)pos[1], (float)pos[2]);

			FbxVector4 aRot = Node->EvaluateLocalRotation(fbxCur);
			hier.Animation.Rotation.emplace_back(PVX::ToRAD(aRot[0]), PVX::ToRAD(aRot[1]), PVX::ToRAD(aRot[2]));

			auto scl = Node->EvaluateLocalScaling(fbxCur).Buffer();
			hier.Animation.Scale.emplace_back((float)scl[0], (float)scl[1], (float)scl[2]);
		}
	}

	Transform ReadTransform(FbxNode* Node) {
		Transform hier;
		hier.Name = Node->GetName();
		hier.RotationOrder = Node->GetTransform().GetRotationOrder().GetOrder();
		FbxDouble3 pos = Node->LclTranslation.Get();
		FbxDouble3 rot = Node->LclRotation.Get();
		FbxDouble3 scl = Node->LclScaling.Get();

		hier.Position.x = (float)pos[0];
		hier.Position.y = (float)pos[1];
		hier.Position.z = (float)pos[2];
		hier.Rotation.x = (float)PVX::ToRAD(rot[0]);
		hier.Rotation.y = (float)PVX::ToRAD(rot[1]);
		hier.Rotation.z = (float)PVX::ToRAD(rot[2]);
		hier.Scale.x = (float)scl[0];
		hier.Scale.y = (float)scl[1];
		hier.Scale.z = (float)scl[2];

		GetTransform(Node, hier);
		ReadAnimation(Node, hier);
		return hier;
	}

	static void LoadNode(FbxNode* Node, Object& scene, int64_t ParentIndex) {
		auto h = ReadTransform(Node);
		h.ParentIndex = ParentIndex;
		ParentIndex = scene.Heirarchy.size();
		h.Name = Node->GetName();
		scene.Heirarchy.push_back(h);
		FbxNodeAttribute* attr = Node->GetNodeAttribute();
		if (attr) {
			if (auto tp = attr->GetAttributeType(); tp == FbxNodeAttribute::EType::eMesh) {
				LoadMesh((FbxMesh*)attr, scene);
			}
		}
		int cCount = Node->GetChildCount();
		for (int i = 0; i < cCount; i++) {
			LoadNode(Node->GetChild(i), scene, ParentIndex);
			//ret.Child.push_back();
		}
	}

	//void MakeTransform(FbxLoad& tree, std::vector<Transform>& Hier, int64_t ParentIndex) {
	//	tree.TransformItem.ParentIndex = ParentIndex;
	//	tree.TransformItem.Name = tree.Name;
	//	ParentIndex = Hier.size();
	//	Hier.push_back(tree.TransformItem);
	//	for (auto& t : tree.Child)
	//		MakeTransform(t, Hier, ParentIndex);
	//}

	Object LoadFbx(const std::string& Filename) {
		FbxManager* Manager = FbxManager::Create();
		FbxImporter* Importer = FbxImporter::Create(Manager, "");
		Importer->Initialize(Filename.c_str());
		FbxScene* fbxScene = FbxScene::Create(Manager, Filename.c_str());
		int ret = Importer->Import(fbxScene);
		Importer->Destroy(1);

		Object obj;

		ReadMaterials(obj, fbxScene);

		LoadNode(fbxScene->GetRootNode(), obj, -1);
		//MakeTransform(Tree, obj.Heirarchy, -1);

		if (obj.Heirarchy[0].Name=="RootNode")
			obj.Heirarchy[0].Name = Filename;

		Manager->Destroy();

		return obj;
	}

	void Object::Save(const std::string& Filename) {
		BinSaver bin(Filename.c_str(), "OBJ3");
		Save(bin);
	}
	//void Standart_Phong::Save(BinSaver& bin, const std::string& Name) {
	//	bin.Begin("MTRL"); {
	//		bin.Write("NAME", Name);

	//		bin.Write("COLR", Color);
	//		bin.Write("CAMB", Ambient);
	//		bin.Write("CDIF", Diffuse);
	//		bin.Write("CSPC", Specular);
	//		bin.Write("CEMT", Emissive);
	//		bin.Write("CSPC", SpecularPower);
	//		bin.Write("CTRN", Transparency);
	//		bin.Write("CBMP", Bump);

	//		bin.Write("TAMB", Textures.Ambient);
	//		bin.Write("TDIF", Textures.Diffuse);
	//		bin.Write("TSPC", Textures.Specular);
	//		bin.Write("TEMT", Textures.Emissive);
	//		bin.Write("TBMP", Textures.Bump);
	//		bin.Write("TTRN", Textures.Transparency);
	//	} bin.End();
	//}

	void PlaneMaterial::Save(BinSaver& bin, const std::string& Name) {
		bin.Begin("MTRL"); {
			bin.Write("NAME", Name);

			bin.Write("COLR", Color);

			bin.Write("CAMB", Ambient);
			bin.Write("CDIF", Diffuse);
			bin.Write("CSPC", Specular);
			bin.Write("CEMT", Emissive);

			bin.Write("FAMB", AmbientFactor);
			bin.Write("FDIF", DiffuseFactor);
			bin.Write("FSPC", SpecularFactor);
			bin.Write("FEMT", EmissiveFactor);

			bin.Write("FMTL", Metallic);
			bin.Write("FRGH", Roughness);

			bin.Write("CSPP", SpecularPower);
			bin.Write("CTRN", Alpha);
			bin.Write("CBMP", Bump);

			bin.Write("TAMB", Textures.Ambient);
			bin.Write("TDIF", Textures.Diffuse);
			bin.Write("TSPC", Textures.Specular);
			bin.Write("TEMT", Textures.Emissive);
			bin.Write("TBMP", Textures.Bump);
			bin.Write("TTRN", Textures.Transparency);
		} bin.End();
	}

	void ObjectPart::Save(BinSaver& bin) {
		bin.Begin("PART"); {
			bin.Write("NAME", TransformNode);
			for (auto& [Material, Part] : Parts) {
				bin.Begin("PART"); {
					bin.Write("MTRL", Material);
					Part.Save(bin);
				}bin.End();
			}
			for (auto i = 0; i<BonePostTransform.size(); i++) {
				bin.Begin("BONE"); {
					bin.Write("NAME", BoneNodes[i]);
					bin.Write("BPTF", BonePostTransform[i]);
				}bin.End();
			}
		} bin.End();
	}
	void ObjectSubPart::Save(BinSaver& bin) {
		bin.Begin("SPRT"); {
			bin.Write("VRTS", VertexData);
			bin.Write("INDX", Index);
			bin.Write("STRD", Stride);
			bin.Write("VCNT", VertexCount);
			for (auto& a : Attributes) {
				bin.Begin("ATTR"); {
					bin.Write("NAME", a.Name);
					bin.Write("TYPE", a.Type);
					bin.Write("TCNT", a.Count);
					bin.Write("OFST", a.Offset);
					bin.Write("USAG", a.Usage);
				}bin.End();
			}
			if (BlendShapeData.size()) {
				for (auto& a : BlendAttributes) {
					bin.Begin("BATR"); {
						bin.Write("NAME", a.Name);
						bin.Write("TYPE", a.Type);
						bin.Write("TCNT", a.Count);
						bin.Write("OFST", a.Offset);
						bin.Write("USAG", a.Usage);
					}bin.End();
				}
				bin.Write("BVRT", BlendShapeData);
			}
		} bin.End();
	}
	ObjectSubPart ObjectSubPart::FilterAttributes(const std::vector<std::string>& Attrs) const {
		auto dint = DeinterleaveAttributes(*this);
		std::vector<DeinterleavedItem> items;
		items.reserve(Attrs.size() + BlendAttributes.size());

		for (auto& d : dint) for (auto& f : Attrs) if (f==d.Description.Name) items.push_back(std::move(d));

		if (!BlendAttributes.size()) {
			auto ret = InterleaveAttributes(items);
			ret.Index = Index;
			Reindex(ret.VertexData, ret.Index, ret.Stride);
			ret.VertexCount = ret.VertexData.size() / ret.Stride;
			return ret;
		} else {
			auto MainSize = items.size();
			int bStride = BlendShapeData.size() / VertexCount;
			auto tmpPart = ObjectSubPart{ std::move(BlendShapeData), {}, std::move(BlendAttributes), {}, {}, bStride, VertexCount };
			auto sMorph = DeinterleaveAttributes(tmpPart);
			for (auto& m : sMorph) {
				for (auto& f : Attrs) {
					if (m.Description.Name == f) {
						items.push_back(std::move(m));
						goto Found;
					}
				}
			Found:;
			}

			auto tmpRet = InterleaveAttributes(items);
			tmpRet.Index = Index;
			Reindex(tmpRet.VertexData, tmpRet.Index, tmpRet.Stride);

			dint = DeinterleaveAttributes(tmpRet);
			items.clear();
			for (auto i = 0; i<MainSize; i++) items.push_back(std::move(dint[i]));
			auto ret = InterleaveAttributes(items);
			ret.Index = std::move(tmpRet.Index);
			ret.VertexCount = ret.VertexData.size() / ret.Stride;

			items.clear();
			for (auto i = MainSize; i<dint.size(); i++) items.push_back(std::move(dint[i]));
			tmpRet = InterleaveAttributes(items);
			ret.BlendAttributes = std::move(tmpRet.Attributes);
			ret.BlendShapeData = std::move(tmpRet.VertexData);


			return ret;
		}
	}

	void Transform::Save(BinSaver& bin) {
		bin.Begin("NODE"); {
			bin.Write("NAME", Name);
			bin.Write("PRNT", ParentIndex);
			bin.Write("RORD", RotationOrder);
			bin.Write("PTRN", PostTranslate);
			bin.Write("PROT", PostRotate);
			bin.Write("PSCL", PostScale);
			bin.Write("SPOS", Position);
			bin.Write("SROT", Rotation);
			bin.Write("SSCL", Scale);
			bin.Write("RATE", Animation.FrameRate);
			bin.Write("APOS", Animation.Position);
			bin.Write("AROT", Animation.Rotation);
			bin.Write("ASCL", Animation.Scale);
		} bin.End();
	}
	void Object::Save(BinSaver& bin) {
		for (auto& [Name, Mat] : Material) {
			Mat.Save(bin, Name);
		}
		for (auto& Part : Parts) {
			Part.Save(bin);
		}
		for (auto& t : Heirarchy) {
			t.Save(bin);
		}
	}
	Object Object::Load(const std::string& Filename) {
		PVX::BinLoader bin(Filename.c_str(), "OBJ3");
		Object ret;

		bin.Process("PART", [&](BinLoader& bin) {
			auto& part = ret.Parts.emplace_back();
			bin.Process("NAME", part.TransformNode);
			bin.Process("PART", [&](BinLoader& bin) {
				std::string MatName;
				bin.Process("MTRL", MatName);
				bin.Process("SPRT", [&](BinLoader& bin) {
					auto& sPart = part.Parts[MatName];
					bin.Process("VRTS", sPart.VertexData);
					bin.Process("INDX", sPart.Index);
					bin.Process("STRD", sPart.Stride);
					bin.Process("VCNT", sPart.VertexCount);
					bin.Process("ATTR", [&](BinLoader& bin) {
						auto& a = sPart.Attributes.emplace_back();
						bin.Process("NAME", a.Name);
						bin.Process("TYPE", a.Type);
						bin.Process("TCNT", a.Count);
						bin.Process("OFST", a.Offset);
						bin.Process("USAG", a.Usage);
						bin.Execute();
					});
					bin.Process("BATR", [&](BinLoader& bin) {
						auto& a = sPart.Attributes.emplace_back();
						bin.Process("NAME", a.Name);
						bin.Process("TYPE", a.Type);
						bin.Process("TCNT", a.Count);
						bin.Process("OFST", a.Offset);
						bin.Process("USAG", a.Usage);
						bin.Execute();
					});
					bin.Execute();
				});
				bin.Execute();
			});
			bin.Process("BONE", [&](BinLoader& bin) {
				bin.Process("NAME", [&](BinLoader& bin) {
					part.BoneNodes.push_back(bin.RemainingAsString());
				});
				bin.Process("BPTF", [&](BinLoader& bin) {
					PVX::Matrix4x4& Mat = part.BonePostTransform.emplace_back();
					bin.Read(Mat);
				});
				bin.Execute();
			});
			bin.Execute();
		});

		bin.Process("NODE", [&](BinLoader& bin) {
			auto& h = ret.Heirarchy.emplace_back();
			bin.Process("NAME", h.Name);

			bin.Process("PRNT", h.ParentIndex);
			bin.Process("RORD", h.RotationOrder);
			bin.Process("PTRN", h.PostTranslate);
			bin.Process("PROT", h.PostRotate);
			bin.Process("PSCL", h.PostScale);

			bin.Process("SPOS", h.Position);
			bin.Process("SROT", h.Rotation);
			bin.Process("SSCL", h.Scale);

			bin.Process("RATE", h.Animation.FrameRate);
			bin.Process("APOS", h.Animation.Position);
			bin.Process("AROT", h.Animation.Rotation);
			bin.Process("ASCL", h.Animation.Scale);
			bin.Execute();
		});

		bin.Process("MTRL", [&](PVX::BinLoader& bin) {
			PlaneMaterial mat{};
			std::string Name;

			bin.Process("NAME", Name);

			bin.Process("COLR", mat.Color);

			bin.Process("CAMB", mat.Ambient);
			bin.Process("CDIF", mat.Diffuse);
			bin.Process("CSPC", mat.Specular);
			bin.Process("CEMT", mat.Emissive);

			bin.Process("FAMB", mat.AmbientFactor);
			bin.Process("FDIF", mat.DiffuseFactor);
			bin.Process("FSPC", mat.SpecularFactor);
			bin.Process("FEMT", mat.EmissiveFactor);

			bin.Process("FMTL", mat.Metallic);
			bin.Process("FRGH", mat.Roughness);

			bin.Process("CSPP", mat.SpecularPower);
			bin.Process("CTRN", mat.Alpha);
			bin.Process("CBMP", mat.Bump);

			bin.Process("TAMB", mat.Textures.Ambient);
			bin.Process("TDIF", mat.Textures.Diffuse);
			bin.Process("TSPC", mat.Textures.Specular);
			bin.Process("TEMT", mat.Textures.Emissive);
			bin.Process("TBMP", mat.Textures.Bump);
			bin.Process("TTRN", mat.Textures.Transparency);
			bin.Execute();
			ret.Material[Name] = mat;
		});
		bin.Execute();
		return ret;
	}
}
#pragma once
#include <map>
#include <vector>
#include <string>
#include <numeric>
#include <type_traits>
#include "PVX_Math3D.h"
#include "PVX_String.h"
#include <charconv>
#include <unordered_map>

namespace PVX::Reflect {
	struct Primitive {
		enum class _Type : unsigned char {
			Struct,
			Byte,
			Int,
			Float,
			UInt,
		};

		_Type Type{_Type::Struct};
		unsigned char TypeSize{};
		unsigned short Count{};

		inline int Size() const { return Count * TypeSize; }
		
		template<typename T>
		constexpr static Primitive Get() {
			if constexpr (std::is_same<T, int>::value) return { _Type::Int, 4, 1 };
			else if constexpr (std::is_same<T, PVX::iVector2D>::value) return { _Type::Int, 4, 2 };
			else if constexpr (std::is_same<T, PVX::iVector3D>::value) return { _Type::Int, 4, 3 };
			else if constexpr (std::is_same<T, PVX::iVector4D>::value) return { _Type::Int, 4, 4 };

			else if constexpr (std::is_same<T, unsigned int>::value) return { _Type::UInt, 4, 1 };
			else if constexpr (std::is_same<T, PVX::uVector2D>::value) return { _Type::UInt, 4, 2 };
			else if constexpr (std::is_same<T, PVX::uVector3D>::value) return { _Type::UInt, 4, 3 };
			else if constexpr (std::is_same<T, PVX::uVector4D>::value) return { _Type::UInt, 4, 4 };

			else if constexpr (std::is_same<T, unsigned char>::value) return { _Type::Byte, 1, 1 };
			else if constexpr (std::is_same<T, PVX::ucVector2D>::value) return { _Type::Byte, 1, 2 };
			else if constexpr (std::is_same<T, PVX::ucVector3D>::value) return { _Type::Byte, 1, 3 };
			else if constexpr (std::is_same<T, PVX::ucVector4D>::value) return { _Type::Byte, 1, 4 };

			else if constexpr (std::is_same<T, float>::value) return { _Type::Float, 4, 1 };
			else if constexpr (std::is_same<T, PVX::Vector2D>::value) return { _Type::Float, 4, 2 };
			else if constexpr (std::is_same<T, PVX::Vector3D>::value) return { _Type::Float, 4, 3 };
			else if constexpr (std::is_same<T, PVX::Vector4D>::value) return { _Type::Float, 4, 4 };


			else if constexpr (std::is_same<T, PVX::Matrix3x3>::value) return { _Type::Float, 4, 9 };
			else if constexpr (std::is_same<T, PVX::Matrix3x4>::value) return { _Type::Float, 4, 12 };
			else if constexpr (std::is_same<T, PVX::Matrix4x4>::value) return { _Type::Float, 4, 16 };
			
			else return { _Type::Struct, 0, 0 };
		}

		constexpr uint32_t GetTypeId() const { 
			return (uint32_t(Type) << 24) | (uint32_t(TypeSize) << 16) | (uint32_t(Count));
		}

		template<typename T>
		constexpr static uint32_t GetTypeId() {
			constexpr auto ret = Get<T>();
			return (uint32_t(ret.Type) << 24) | (uint32_t(ret.TypeSize) << 16) | (uint32_t(ret.Count));
		}
	};
	struct Descriptor {
	private:
		enum class tType {
			None, Struct, Float, Int, Int2, Int3, Int4, UInt, UInt2, UInt3, UInt4, Byte, Byte2, Byte3, Byte4, Vec2D, Vec3D, Vec4D, Num, NameArray, Name, CloseSquare, CloseCurly, Semicolon,
		};
		template<typename T> int HandlePrimitive(std::pair<tType, std::string>*& tokens) {
			if (tokens[1].first == tType::Name && tokens[2].first == tType::Semicolon) {
				Add<T>(tokens[1].second);
				tokens += 3;
			} else if (tokens[1].first == tType::NameArray && tokens[2].first == tType::Num && tokens[3].first == tType::CloseSquare && tokens[4].first == tType::Semicolon) {
				Add<T>(tokens[1].second, atoi(tokens[2].second.c_str()));
				tokens += 5;
			} else {
				return 1;
			}
			return 0; 
		}

		void ProcessTokens(std::pair<tType, std::string>*& tokens) {
			while (tokens->first!= tType::None) {
				switch (tokens->first) {
					case tType::Int: if (HandlePrimitive<int>(tokens)) return; break;
					case tType::Int2: if (HandlePrimitive<iVector2D>(tokens)) return; break;
					case tType::Int3: if (HandlePrimitive<iVector3D>(tokens)) return; break;
					case tType::Int4: if (HandlePrimitive<iVector4D>(tokens)) return; break;
					case tType::Byte: if (HandlePrimitive<unsigned char>(tokens)) return; break;
					case tType::Byte2: if (HandlePrimitive<ucVector2D>(tokens)) return; break;
					case tType::Byte3: if (HandlePrimitive<ucVector3D>(tokens)) return; break;
					case tType::Byte4: if (HandlePrimitive<ucVector4D>(tokens)) return; break;
					case tType::Float: if (HandlePrimitive<float>(tokens)) return; break;
					case tType::Vec2D: if (HandlePrimitive<Vector2D>(tokens)) return; break;
					case tType::Vec3D: if (HandlePrimitive<Vector3D>(tokens)) return; break;
					case tType::Vec4D: if (HandlePrimitive<Vector4D>(tokens)) return; break;
					case tType::UInt: if (HandlePrimitive<unsigned int>(tokens)) return; break;
					case tType::UInt2: if (HandlePrimitive<uVector2D>(tokens)) return; break;
					case tType::UInt3: if (HandlePrimitive<uVector3D>(tokens)) return; break;
					case tType::UInt4: if (HandlePrimitive<uVector4D>(tokens)) return; break;

					case tType::Struct: {
						tokens++;
						Descriptor ch;
						ch.ProcessTokens(tokens);

						if (tokens[1].first == tType::Name && tokens[2].first == tType::Semicolon) {
							AddStruct(ch, tokens[1].second);
							tokens += 3;
						} else if (tokens[1].first == tType::NameArray && tokens[2].first == tType::Num && tokens[3].first == tType::CloseSquare && tokens[4].first == tType::Semicolon) {
							AddStruct(ch, tokens[1].second, atoi(tokens[2].second.c_str()));
							tokens += 5;
						} else {
							return;
						}
						break;
					}
					case tType::CloseCurly:
						return;
				}
			}
		}

	public:
		Primitive Type;
		size_t Offset{};
		size_t Count{ 1 };
		std::map<std::string, size_t> ChildrenNames;
		std::vector<Descriptor> Children;

		Descriptor() = default;
		Descriptor(Primitive Type, size_t Offset, size_t Count) : Type{ Type }, Offset{ Offset }, Count{ Count } {}
		Descriptor(const std::string_view& Struct) {
			auto tokens = PVX::StringView::Tokenize<tType>(Struct, {
				{ tType::Struct, R"regex([\s\r\n]*(struct[\s\r\n]*\{)[\s\r\n]*)regex", 1 },

				{ tType::Vec2D, R"regex([\s\r\n]*(vec2|float2|Vector2D|PVX\:\:Vector2D)[\s\r\n]*)regex", 1 },
				{ tType::Vec3D, R"regex([\s\r\n]*(vec3|float3|Vector3D|PVX\:\:Vector3D)[\s\r\n]*)regex", 1 },
				{ tType::Vec4D, R"regex([\s\r\n]*(vec4|float4|Vector4D|PVX\:\:Vector4D)[\s\r\n]*)regex", 1 },
				{ tType::Float, R"regex([\s\r\n]*(float)[\s\r\n]*)regex", 1 },

				{ tType::Byte2, R"regex([\s\r\n]*(byte2|uchar2)[\s\r\n]*)regex", 1 },
				{ tType::Byte3, R"regex([\s\r\n]*(byte3|uchar3)[\s\r\n]*)regex", 1 },
				{ tType::Byte4, R"regex([\s\r\n]*(byte4|uchar4)[\s\r\n]*)regex", 1 },
				{ tType::Byte, R"regex([\s\r\n]*(byte|uchar)[\s\r\n]*)regex", 1 },

				{ tType::Int2, R"regex([\s\r\n]*(ivec2|int2|iVector2D|PVX\:\:iVector2D)[\s\r\n]*)regex", 1 },
				{ tType::Int3, R"regex([\s\r\n]*(ivec3|int3|iVector3D|PVX\:\:iVector3D)[\s\r\n]*)regex", 1 },
				{ tType::Int4, R"regex([\s\r\n]*(ivec4|int4|iVector4D|PVX\:\:iVector4D)[\s\r\n]*)regex", 1 },
				{ tType::Int, R"regex([\s\r\n]*(int)[\s\r\n]*)regex", 1 },

				{ tType::UInt2, R"regex([\s\r\n]*(uint2)[\s\r\n]*)regex", 1 },
				{ tType::UInt3, R"regex([\s\r\n]*(uint3)[\s\r\n]*)regex", 1 },
				{ tType::UInt4, R"regex([\s\r\n]*(uint4)[\s\r\n]*)regex", 1 },
				{ tType::UInt, R"regex([\s\r\n]*(uint)[\s\r\n]*)regex", 1 },

				{ tType::Num, R"regex([\s\r\n]*([+-]?[0-9]+)[\s\r\n]*)regex", 1 },
				{ tType::NameArray, R"regex([\s\r\n]*(([_a-zA-Z][_a-zA-Z0-9]*[\s\r\n]*)\[)[\s\r\n]*)regex", 2 },
				{ tType::Name, R"regex([\s\r\n]*([_a-zA-Z][_a-zA-Z0-9]*)[\s\r\n]*)regex", 1 },
				{ tType::CloseSquare, R"regex([\s\r\n]*(\])[\s\r\n]*)regex", 1 },
				{ tType::CloseCurly, R"regex([\s\r\n]*(\})[\s\r\n]*)regex", 1 },
				{ tType::Semicolon, R"regex([\s\r\n]*(;)[\s\r\n]*)regex", 1 },
			});
			tokens.push_back({ tType::None, "" });
			if (tokens[0].first==tType::Struct) {
				auto* tk = &tokens[1];
				ProcessTokens(tk);
			}
		}

		struct Path {
			std::string_view Name;
			size_t Index{};
		};

		inline size_t Stride() const {
			if (Children.size()) {
				//return std::reduce(Children.begin(), Children.end(), 0, [](size_t acc, const Descriptor& d) { 
				//	return acc + d.Size(); 
				//});
				return Children.back().Offset + Children.back().Size();
			}
			else
				return Type.Size();
		}
		inline size_t Size() const {
			return Count * Stride();
		}
		template<typename T>
		void Add(const std::string_view& Name, size_t Count = 1) {
			ChildrenNames[std::string(Name)] = Children.size();
			Children.push_back({ Primitive::Get<T>(), Size(), Count });
		}
		void AddStruct(const Descriptor& str, const std::string_view& Name, size_t Count = 1) {
			auto off = Size();
			ChildrenNames[std::string(Name)] = Children.size();
			Children.push_back(str);
			Children.back().Offset = off;
			Children.back().Count = Count;
		}
	private:
		static std::vector<Path> GetPath(const std::string_view& path) {
			std::vector<Path> ret;
			ret.reserve(PVX::StringView::Count(path, ".") + 1);
			PVX::StringView::OnSplit(path, ".", [&ret](const std::string_view& w) {
				auto sp = PVX::StringView::Split(w, "[");
				if (sp.size()>1) {
					size_t idx;
					std::from_chars(sp[1].data(), sp[1].data() + sp[1].size()-1, idx);
					ret.push_back({ sp[0], idx });
				} else {
					ret.push_back({ sp[0], 0 });
				}
			});
			return std::move(ret);
		}
	public:
		inline size_t GetOffset(const std::string_view& path) const { return GetOffset(GetPath(path), 0); }

	private:
		inline size_t GetOffset(const std::vector<Path>& path, size_t Index) const {
			if (Index < path.size()) {
				const auto& p = path[Index];
				const auto& Child = Children[ChildrenNames.at(std::string(p.Name))];
				if (p.Index)
					return Child.Offset + p.Index * Child.Stride() + Child.GetOffset(path, Index + 1);
				else
					return Child.Offset + Child.GetOffset(path, Index + 1);
			} else {
				return 0;
			}
		}
		friend class Reflector;
	};

	class Reflector {
		Descriptor Descr;
	public:
		std::vector<unsigned char> Data;
		Reflector() {}
		Reflector(const Descriptor& descr) : Descr{ descr } {
			Data.resize(Descr.Size());
		}
		Reflector(const std::string_view& Struct) : Descr(Struct) { Data.resize(Descr.Size()); }
		const Descriptor& GetDescriptor() const { return Descr; }
		template<typename T>
		T& Add(const std::string_view& Name, size_t Count = 1) {
			auto offset = Data.size();
			Descr.Add<T>(Name, Count);
			return *(T*)&Data[offset];
		}
		template<typename T> T& Get(const std::string_view& Path) { return *(T*)&Data[Descr.GetOffset(Path)]; }
		template<typename T> const T& Get(const std::string_view& Path) const { return *(T*)&Data[Descr.GetOffset(Path)]; }
		template<typename T> T& Get() { return *(T*)Data.data(); }
		inline const Primitive& GetPrimitive(const std::string_view& Path) const {
			const Descriptor* cur = &Descr;
			for (auto& p : Descriptor::GetPath(Path)) cur = &cur->Children[cur->ChildrenNames.at(std::string(p.Name))];
			return cur->Type;
		}
	};
}
#pragma once
#include <map>
#include <vector>
#include <string>
#include <numeric>
#include <type_traits>
#include <PVX_Math3D.h>
#include <PVX_String.h>
#include <charconv>

namespace PVX::Reflect {
	struct Primitive {
		enum class _Type {
			Struct,
			Byte,
			Int,
			Float,
		};
		_Type Type{ _Type::Struct };
		int TypeSize{};
		int Count{};
		inline int Size() const { return Count * TypeSize; }

		template<typename T>
		constexpr static Primitive Get() {
			if constexpr (std::is_same<T, int>::value)
				return { _Type::Int, 4, 1 };
			else if constexpr (std::is_same<T, int>::value)
				return { _Type::Float, 4, 1 };
			else if constexpr (std::is_same<T, float>::value)
				return { _Type::Float, 4, 1 };
			else if constexpr (std::is_same<T, unsigned char>::value)
				return { _Type::Byte, 1, 1 };
			else if constexpr (std::is_same<T, PVX::Vector2D>::value)
				return { _Type::Float, 4, 2 };
			else if constexpr (std::is_same<T, PVX::Vector3D>::value)
				return { _Type::Float, 4, 3 };
			else if constexpr (std::is_same<T, PVX::Vector4D>::value)
				return { _Type::Float, 4, 4 };
			else if constexpr (std::is_same<T, PVX::Matrix3x3>::value)
				return { _Type::Float, 4, 9 };
			else if constexpr (std::is_same<T, PVX::Matrix3x4>::value)
				return { _Type::Float, 4, 12 };
			else if constexpr (std::is_same<T, PVX::Matrix4x4>::value)
				return { _Type::Float, 4, 16 };
			else
				return { _Type::Struct, 0, 0 };
		}
	};
	struct Descriptor {
	private:
		enum class tType {
			None, Struct, Float, Int, Byte, Byte2, Byte3, Byte4, Vec2D, Vec3D, Vec4D, Num, NameArray, Name, CloseSquare, CloseCurly, Semicolon,
		};
		void ProcessTokens(std::pair<tType, std::string>*& tokens) {
			while (tokens->first!= tType::None) {
				switch (tokens->first) {
					case tType::Float:
						if (tokens[1].first == tType::Name && tokens[2].first == tType::Semicolon) {
							Add<float>(tokens[1].second);
							tokens += 3;
						} else if (tokens[1].first == tType::NameArray && tokens[2].first == tType::Num && tokens[3].first == tType::CloseSquare && tokens[4].first == tType::Semicolon) {
							Add<float>(tokens[1].second, atoi(tokens[2].second.c_str()));
							tokens += 5;
						} else {
							return;
						}
						break;
					case tType::Int:
						if (tokens[1].first == tType::Name && tokens[2].first == tType::Semicolon) {
							Add<int>(tokens[1].second);
							tokens += 3;
						} else if (tokens[1].first == tType::NameArray && tokens[2].first == tType::Num && tokens[3].first == tType::CloseSquare && tokens[4].first == tType::Semicolon) {
							Add<int>(tokens[1].second, atoi(tokens[2].second.c_str()));
							tokens += 5;
						} else {
							return;
						}
						break;
					case tType::Byte:
						if (tokens[1].first == tType::Name && tokens[2].first == tType::Semicolon) {
							Add<unsigned char>(tokens[1].second);
							tokens += 3;
						} else if (tokens[1].first == tType::NameArray && tokens[2].first == tType::Num && tokens[3].first == tType::CloseSquare && tokens[4].first == tType::Semicolon) {
							Add<unsigned char>(tokens[1].second, atoi(tokens[2].second.c_str()));
							tokens += 5;
						} else {
							return;
						}
						break;
					case tType::Vec2D:
						if (tokens[1].first == tType::Name && tokens[2].first == tType::Semicolon) {
							Add<Vector2D>(tokens[1].second);
							tokens += 3;
						} else if (tokens[1].first == tType::NameArray && tokens[2].first == tType::Num && tokens[3].first == tType::CloseSquare && tokens[4].first == tType::Semicolon) {
							Add<Vector2D>(tokens[1].second, atoi(tokens[2].second.c_str()));
							tokens += 5;
						} else {
							return;
						}
						break;
					case tType::Vec3D:
						if (tokens[1].first == tType::Name && tokens[2].first == tType::Semicolon) {
							Add<Vector3D>(tokens[1].second);
							tokens += 3;
						} else if (tokens[1].first == tType::NameArray && tokens[2].first == tType::Num && tokens[3].first == tType::CloseSquare && tokens[4].first == tType::Semicolon) {
							Add<Vector3D>(tokens[1].second, atoi(tokens[2].second.c_str()));
							tokens += 5;
						} else {
							return;
						}
						break;
					case tType::Vec4D:
						if (tokens[1].first == tType::Name && tokens[2].first == tType::Semicolon) {
							Add<Vector4D>(tokens[1].second);
							tokens += 3;
						} else if (tokens[1].first == tType::NameArray && tokens[2].first == tType::Num && tokens[3].first == tType::CloseSquare && tokens[4].first == tType::Semicolon) {
							Add<Vector4D>(tokens[1].second, atoi(tokens[2].second.c_str()));
							tokens += 5;
						} else {
							return;
						}
						break;
					case tType::Byte2:
						if (tokens[1].first == tType::Name && tokens[2].first == tType::Semicolon) {
							Add<ucVector2D>(tokens[1].second);
							tokens += 3;
						} else if (tokens[1].first == tType::NameArray && tokens[2].first == tType::Num && tokens[3].first == tType::CloseSquare && tokens[4].first == tType::Semicolon) {
							Add<ucVector2D>(tokens[1].second, atoi(tokens[2].second.c_str()));
							tokens += 5;
						} else {
							return;
						}
						break;
					case tType::Byte3:
						if (tokens[1].first == tType::Name && tokens[2].first == tType::Semicolon) {
							Add<ucVector3D>(tokens[1].second);
							tokens += 3;
						} else if (tokens[1].first == tType::NameArray && tokens[2].first == tType::Num && tokens[3].first == tType::CloseSquare && tokens[4].first == tType::Semicolon) {
							Add<ucVector3D>(tokens[1].second, atoi(tokens[2].second.c_str()));
							tokens += 5;
						} else {
							return;
						}
						break;
					case tType::Byte4:
						if (tokens[1].first == tType::Name && tokens[2].first == tType::Semicolon) {
							Add<ucVector4D>(tokens[1].second);
							tokens += 3;
						} else if (tokens[1].first == tType::NameArray && tokens[2].first == tType::Num && tokens[3].first == tType::CloseSquare && tokens[4].first == tType::Semicolon) {
							Add<ucVector4D>(tokens[1].second, atoi(tokens[2].second.c_str()));
							tokens += 5;
						} else {
							return;
						}
						break;
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
				{ tType::Vec2D, R"regex([\s\r\n]*(float2)[\s\r\n]*)regex", 1 },
				{ tType::Vec3D, R"regex([\s\r\n]*(float3)[\s\r\n]*)regex", 1 },
				{ tType::Vec4D, R"regex([\s\r\n]*(float4)[\s\r\n]*)regex", 1 },
				{ tType::Float, R"regex([\s\r\n]*(float)[\s\r\n]*)regex", 1 },
				{ tType::Vec2D, R"regex([\s\r\n]*(Vector2D)[\s\r\n]*)regex", 1 },
				{ tType::Vec3D, R"regex([\s\r\n]*(Vector3D)[\s\r\n]*)regex", 1 },
				{ tType::Vec4D, R"regex([\s\r\n]*(Vector4D)[\s\r\n]*)regex", 1 },
				{ tType::Vec2D, R"regex([\s\r\n]*(PVX\:\:Vector2D)[\s\r\n]*)regex", 1 },
				{ tType::Vec3D, R"regex([\s\r\n]*(PVX\:\:Vector3D)[\s\r\n]*)regex", 1 },
				{ tType::Vec4D, R"regex([\s\r\n]*(PVX\:\:Vector4D)[\s\r\n]*)regex", 1 },
				{ tType::Byte2, R"regex([\s\r\n]*(byte2)[\s\r\n]*)regex", 1 },
				{ tType::Byte3, R"regex([\s\r\n]*(byte3)[\s\r\n]*)regex", 1 },
				{ tType::Byte4, R"regex([\s\r\n]*(byte4)[\s\r\n]*)regex", 1 },
				{ tType::Int, R"regex([\s\r\n]*(int)[\s\r\n]*)regex", 1 },
				{ tType::Byte, R"regex([\s\r\n]*(byte)[\s\r\n]*)regex", 1 },
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

		inline size_t GetOffset(const std::string_view& path) {
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
			return GetOffset(ret, 0);
		}

	private:
		inline size_t GetOffset(const std::vector<Path>& path, size_t Index) const {
			if (Index < path.size()) {
				const auto& p = path[Index];
				const auto& Child = Children[ChildrenNames.at(std::string(p.Name))];
				if (p.Index)
					return p.Index * Child.Size() + Child.GetOffset(path, Index + 1);
				else
					return Child.GetOffset(path, Index + 1);
			} else {
				return Offset;
			}
		}
	};

	class Reflector {
		std::vector<unsigned char> Data;
		Descriptor Descr;
	public:
		Reflector() {}
		Reflector(const Descriptor& descr) : Descr{ descr } {
			Data.resize(Descr.Size());
		}
		Reflector(const std::string_view& Struct) : Descr(Struct) {
			Data.resize(Descr.Size());
		}
		template<typename T>
		T& Add(const std::string_view& Name, size_t Count = 1) {
			auto offset = Data.size();
			Descr.Add<T>(Name, Count);
			return *(T*)&Data[offset];
		}
		template<typename T>
		T& Get(const std::string_view& Path) {
			return *(T*)&Data[Descr.GetOffset(Path)];
		}
	};
}
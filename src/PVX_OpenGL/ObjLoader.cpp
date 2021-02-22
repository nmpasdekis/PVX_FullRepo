#include <PVX_OpenGL_Object.h>
#include <PVX_String.h>
#include <PVX_File.h>
#include <vector>
#include <future>
#include <PVX_Threading.h>
#include <fstream>
#include <iostream>

namespace PVX::OpenGL {
	Texture2D MyLoadTexture(std::map<std::string, Texture2D>& Textures, const std::string& Filename) {
		return Texture2D();
	}

	union Index {
		struct {
			int Vertex, UV, Normal;
		};
		int Array[3];
	};

	std::map<std::string, PVX::OpenGL::SimpleMatrial> LoadMaterial(const std::wstring& fn) {
		std::map<std::string, PVX::OpenGL::SimpleMatrial> Materials;
		std::map<std::string, PVX::OpenGL::Texture2D> Textures;
		std::vector<std::vector<std::string>> allMatTokens;

		PVX::IO::ReadLines(fn, [&](const std::string& line) {
			auto tokens = PVX::String::Split_No_Empties_Trimed(line, " ");
			if (tokens.size())
				allMatTokens.push_back(tokens);
		});

		if (allMatTokens.size()) {
			SimpleMatrial* mat = nullptr;
			for (auto& tokens: allMatTokens) {
				if (tokens[0] == "newmtl") {
					Materials[tokens[1]] = SimpleMatrial{};
					mat = &Materials[tokens[1]];
				} else if (tokens[0] == "Ka") {
					mat->Ambient = { (float)atof(tokens[1].c_str()),(float)atoi(tokens[2].c_str()),(float)atof(tokens[3].c_str()), 1.0f };
				} else if (tokens[0] == "Kd") {
					mat->Diffuse = { (float)atof(tokens[1].c_str()),(float)atof(tokens[2].c_str()),(float)atof(tokens[3].c_str()), 1.0f };
				} else if (tokens[0] == "Ks") {
					mat->Specular = { (float)atof(tokens[1].c_str()),(float)atof(tokens[2].c_str()),(float)atof(tokens[3].c_str()), 1.0f };
				} else if (tokens[0] == "Ke") {
					mat->Emission = { (float)atof(tokens[1].c_str()),(float)atof(tokens[2].c_str()),(float)atof(tokens[3].c_str()), 1.0f };
				} else if (tokens[0] == "d") {
					mat->Transparency = 1.0f - float(atof(tokens[1].c_str()));
				} else if (tokens[0] == "Ns") {
					mat->SpecularPower = float(atof(tokens[1].c_str()));
				} else if (tokens[0] == "map_Kd") {
					std::string Name = tokens[1];
					mat->Textures.Diffuse = MyLoadTexture(Textures, Name);
					Textures[Name] = mat->Textures.Diffuse;
				} else if (tokens[0] == "map_Ke") {
					std::string Name = tokens[1];
					mat->Textures.Emission = MyLoadTexture(Textures, Name);
					Textures[Name] = mat->Textures.Emission;
				} else if (tokens[0] == "map_Bump") {
					std::string Name = tokens[3];
					mat->Textures.Bump = MyLoadTexture(Textures, Name);
					Textures[Name] = mat->Textures.Bump;
				} else if (tokens[0] == "map_Ks") {
					std::string Name = tokens[1];
					mat->Textures.Specular = MyLoadTexture(Textures, Name);
					Textures[Name] = mat->Textures.Specular;
				} else if (tokens[0] == "map_Ka") {
					std::string Name = tokens[1];
					mat->Textures.Ambient = MyLoadTexture(Textures, Name);
					Textures[Name] = mat->Textures.Ambient;
				}
			}
		}
		return Materials;
	}

	std::vector<InterleavedArrayObject> LoadObj2(const wchar_t* fn) {
		std::ifstream fin(fn);
		if (fin.fail())return {};

		std::map<std::string, PVX::OpenGL::SimpleMatrial> Materials = LoadMaterial(PVX::IO::ReplaceExtension(fn, L"mtl"));

		struct matData {
			std::string MaterialName;
			std::vector<std::vector<Index>> Faces;
		};

		std::vector<matData> MaterialData;

		std::vector<PVX::Vector3D> Vertices;
		std::vector<PVX::Vector2D> UVs;
		std::vector<PVX::Vector3D> Normals;
		std::vector<std::vector<std::string>> lines;
		
		PVX::IO::ReadLines(fn, [&](const std::string& line) {
			if (auto tokens = PVX::String::Split_No_Empties_Trimed(line, " "); tokens.size()) {
				if (tokens[0] == "v") {
					Vertices.push_back({ (float)atof(tokens[1].c_str()), (float)atof(tokens[2].c_str()), (float)atof(tokens[3].c_str()) });
				} else if (tokens[0] == "vn") {
					Normals.push_back({ (float)atof(tokens[1].c_str()), (float)atof(tokens[2].c_str()), (float)atof(tokens[3].c_str()) });
				} else if (tokens[0] == "vt") {
					UVs.push_back({ (float)atof(tokens[1].c_str()), 1.0f - (float)atof(tokens[2].c_str()) });
				} else if (tokens[0] == "f") {
					lines.push_back(tokens);
				} else if (tokens[0] == "o") {
					lines.push_back(tokens);
				} else if (tokens[0] == "usemtl") {
					lines.push_back(tokens);
				}
			}
		});

		{
			std::map<std::string, std::map<std::string, std::vector<std::vector<Index>>>> Objects;
			std::string ObjectName = "Object", MaterialName;

			std::vector<std::vector<Index>>* Faces = nullptr;

			for (auto i = 0; i<lines.size(); i++) {
				matData mat;
				for (; i<lines.size(); i++) {
					auto& tokens = lines[i];
					if (tokens[0] == "f") {
						if (Faces==nullptr) Faces = &Objects[ObjectName][MaterialName];
						std::vector<Index> face;
						for (int i = 1; i<tokens.size(); i++) {
							Index tmp{ -1, -1, -1 };
							auto idx = PVX::String::Split(tokens[i], "/");
							for (int i = 0; i < idx.size(); i++) {
								tmp.Array[i] = atoi(idx[i].c_str()) - 1;
							}
							face.push_back(tmp);
						}
						Faces->push_back(face);
					} else if (tokens[0] == "usemtl") {
						MaterialName = tokens[1];
						Faces = nullptr;
					} else if (tokens[0] == "o") {
						ObjectName = tokens[1];
						Faces = nullptr;
					}
					tokens = {};
				}
			}
			lines = {};
		}
	}

	std::vector<InterleavedArrayObject> LoadObj(const wchar_t* fn) {
		std::map<std::string, PVX::OpenGL::SimpleMatrial> Materials = LoadMaterial(PVX::IO::ReplaceExtension(fn, L"mtl"));

		std::vector<PVX::Vector3D> Vertices;
		std::vector<PVX::Vector2D> UVs;
		std::vector<PVX::Vector3D> Normals;
		std::vector<std::vector<std::string>> lines;


		PVX::Threading::TaskPump Tasks;

		{
			std::mutex vMutex, vnMutex, vtMutex, linesMutex;
			//std::map<size_t, std::vector<std::string>> v, vn, vt, lines2;
			std::map<size_t, std::vector<std::string>> lines2;
			std::map<size_t, Vector3D> v;
			std::map<size_t, Vector3D> vn;
			std::map<size_t, Vector2D> vt;

			size_t Index = 0;
			PVX::IO::ReadLines(fn, [&](const std::string& line) {
				auto i = Index++;

				if (line.size()) {
					Tasks.Enqueue([&lines2, line, i, &vMutex, &vnMutex, &vtMutex, &linesMutex, &v, &vn, &vt]() {
						auto tokens = PVX::String::Split_No_Empties_Trimed(PVX::String::Trim(line), " ");
						if (tokens.size()) {
							if (tokens[0] == "v") {
								Vector3D vv{ (float)atof(tokens[1].c_str()), (float)atof(tokens[2].c_str()), (float)atof(tokens[3].c_str()) };
								{
									std::lock_guard<std::mutex> lock{ vMutex };
									v[i] = std::move(vv);
								}
							} else if (tokens[0] == "vn") {
								Vector3D vv{ (float)atof(tokens[1].c_str()), (float)atof(tokens[2].c_str()), (float)atof(tokens[3].c_str()) };
								{
									std::lock_guard<std::mutex> lock{ vnMutex };
									vn[i] = std::move(vv);
								}
							} else if (tokens[0] == "vt") {
								Vector2D vv{ (float)atof(tokens[1].c_str()), (float)atof(tokens[2].c_str()) };
								{
									std::lock_guard<std::mutex> lock{ vtMutex };
									vt[i] = std::move(vv);
								}
							} else if (tokens[0] == "f" || tokens[0] == "usemtl") {
								std::lock_guard<std::mutex> lock{ linesMutex };
								lines2[i] = std::move(tokens);
							}
						}
					});
				}
			});
			Tasks.Wait();

			std::thread t1([&] {
				for (auto& [n, tokens] : vt) {
					UVs.push_back(std::move(tokens));
				}
			});
			std::thread t2([&] {
				for (auto& [n, tokens] : vn) {
					Normals.push_back(std::move(tokens));
				}
			});
			std::thread t3([&] {
				for (auto& [n, tokens] : lines2) {
					lines.push_back(std::move(tokens));
				}
			});
			for (auto& [n, tokens] : v) {
				Vertices.push_back(std::move(tokens));
			}


			t1.join();
			t2.join();
			t3.join();
		};


		//{
		//	std::mutex vMutex, vnMutex, vtMutex, linesMutex;
		//	std::map<size_t, std::vector<std::string>> v, vn, vt, lines2;

		//	size_t Index = 0;
		//	PVX::IO::ReadLines(fn, [&](const std::string& line) {
		//		auto i = Index++;
		//		
		//		if (line.size()) {
		//			Tasks.Enqueue([&lines2, line, i, &vMutex, &vnMutex, &vtMutex, &linesMutex, &v, &vn, &vt]() {
		//				auto tokens = PVX::String::Split_No_Empties_Trimed(PVX::String::Trim(line), " ");
		//				if (tokens.size()) {
		//					if (tokens[0] == "v") {
		//						std::lock_guard<std::mutex> lock{ vMutex };
		//						v[i] = std::move(tokens);
		//					} else if (tokens[0] == "vn") {
		//						std::lock_guard<std::mutex> lock{ vnMutex };
		//						vn[i] = std::move(tokens);
		//					} else if (tokens[0] == "vt") {
		//						std::lock_guard<std::mutex> lock{ vtMutex };
		//						vt[i] = std::move(tokens);
		//					} else if (tokens[0] == "f" || tokens[0] == "usemtl") {
		//						std::lock_guard<std::mutex> lock{ linesMutex };
		//						lines2[i] = std::move(tokens);
		//					}
		//				}
		//			});
		//		}
		//	});
		//	Tasks.Wait();

		//	std::thread tv([&] {
		//		for (auto& [n, tokens] : v) {
		//			Vertices.emplace_back((float)atof(tokens[1].c_str()), (float)atof(tokens[2].c_str()), (float)atof(tokens[3].c_str()));
		//		}
		//		v = {};
		//	});
		//	std::thread tvn([&] {
		//		for (auto& [n, tokens] : vn) {
		//			Normals.emplace_back((float)atof(tokens[1].c_str()), (float)atof(tokens[2].c_str()), (float)atof(tokens[3].c_str()));
		//		}
		//		vn = {};
		//	});
		//	std::thread tlines([&] {
		//		for (auto& [n, tokens] : lines2) {
		//			lines.push_back(std::move(tokens));
		//		}
		//		lines2 = {};
		//	});
		//	for (auto& [n, tokens] : vt) {
		//		UVs.emplace_back((float)atof(tokens[1].c_str()), 1.0f - (float)atof(tokens[2].c_str()));
		//	}

		//	vt = {};
		//	tv.join();
		//	tvn.join();
		//	tlines.join();
		//};

		std::map<std::string, Texture2D> Textures;

		struct matData {
			std::string MaterialName;
			std::vector<std::vector<std::string>> Lines;
		};
		std::vector<matData> MaterialData;

		{
			std::map<std::string, matData> mt;
			matData* curMat = nullptr;

			for (auto i = 0; i<lines.size(); i++) {
				auto& tokens = lines[i];
				if (curMat && tokens[0] == "f") {
					curMat->Lines.push_back(std::move(tokens));
				} else if (tokens[0] == "usemtl") {
					curMat = &mt[tokens[1]];
				}
			}
			for (auto& [name, mat]: mt) {
				MaterialData.push_back(std::move(mat));
				MaterialData.back().MaterialName = name;
			}
			lines = {};
		}

		std::vector<PVX::OpenGL::InterleavedArrayObject> Ret(MaterialData.size());
		for (auto ItemIndex = 0; ItemIndex<MaterialData.size(); ItemIndex++) {
			Tasks.Enqueue([&Vertices, &Normals, &UVs, &Materials, ItemIndex, &Ret, &MaterialData] {
				auto& mat = MaterialData[ItemIndex];
				std::vector<Index> Indices;
				for (auto& tokens: mat.Lines) {
					std::vector<Index> face;
					for (int i = 1; i<tokens.size(); i++) {
						Index tmp{ -1, -1, -1 };
						auto idx = PVX::String::Split(tokens[i], "/");
						for (int i = 0; i < idx.size(); i++) {
							tmp.Array[i] = atoi(idx[i].c_str()) - 1;
						}
						face.push_back(tmp);
					}
					for (int i = 2; i < face.size(); i++) {
						Indices.push_back(face[0]);
						Indices.push_back(face[i - 1]);
						Indices.push_back(face[i]);
					}
				}
				{
					ObjectBuilder gl;
					gl.Begin(GL_TRIANGLES);
					for (auto& f : Indices) {
						if (f.Normal != -1) gl.Normal(Normals[f.Normal]);
						if (f.UV != -1) gl.TexCoord(UVs[f.UV]);
						gl.Vertex(Vertices[f.Vertex]);
					}
					gl.End();
					Ret[ItemIndex] = std::move(gl.Build());
					Ret[ItemIndex].Material = Materials[mat.MaterialName];
				}
			});
		}
		Tasks.Wait();
		return std::move(Ret);
	}
}
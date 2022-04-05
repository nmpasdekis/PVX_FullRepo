#ifndef __BIN_SAVER_H__
#define __BIN_SAVER_H__

#include<vector>
#include<string>
//#include<stdio.h>
#include<map>
#include<functional>
#include<fstream>
#include<PVX_Math3D.h>

namespace PVX {
	class BinSaver {
		std::vector<std::streampos> SizePos;
		//FILE* fout;
		std::ofstream fout;
	public:
		BinSaver(const char* Filename, const char* head);
		BinSaver(const wchar_t* Filename, const char* head);
		BinSaver(const std::string& Filename);
		BinSaver(const char* Filename);
		~BinSaver();
		void Begin(const char* Name);
		void End();
		int Save();
		size_t write(const void* buffer, size_t size, size_t count);
		size_t write(const std::vector<unsigned char>& Bytes);

		template<typename T>
		size_t write(const T& val, size_t Count = 1) {
			if (Count > 0) return write(&val, sizeof(T), Count);
			return 0;
		}

		int OK();

		template<typename T>
		inline size_t Write(const char* Name, const T& v) {
			size_t ret;
			Begin(Name); {
				ret = write(&v, sizeof(T), 1);
			} End();
			return ret;
		}

		template<typename T>
		inline size_t Write(const char* Name, const T* Array, size_t Count) {
			size_t ret = 0;
			if (Count) {
				Begin(Name); {
					ret = write(Array, sizeof(T), Count);
				} End();
			}
			return ret;
		}

		template<typename T>
		inline size_t Write(const char* Name, const std::vector<T>& v) {
			size_t ret = 0;
			if (v.size()) {
				Begin(Name); {
					ret = write(&v[0], sizeof(T), v.size());
				} End();
			}
			return ret;
		}

		inline size_t Write(const char* Name, const std::string& v) {
			size_t ret = 0;
			if (v.size()) {
				Begin(Name); {
					ret = write(&v[0], 1, v.size());
				} End();
			}
			return ret;
		}
	};

	typedef struct BinHeader {
		union {
			unsigned int iName;
			char sName[4];
		};
		unsigned int Size;
	}BinHeader;

#define TAG(x,y,z,w) (w<<24|z<<16|y<<8|x)

	class BinLoader {
	private:
		//FILE* fin = nullptr;
		std::ifstream __fin;
		std::ifstream& fin;
		size_t cur = 0;
		size_t Size = 0;
		BinLoader* Parent = nullptr;
		std::map<unsigned int, std::function<void(BinLoader& bin)>> Loader;
		std::function<void(BinLoader& bin, const char*)> AnyLoader = nullptr;
		BinLoader(std::ifstream& fin, size_t Size, BinLoader* Parent);
	public:
		BinLoader(const char* fn, const char* header);
		BinLoader(const wchar_t* fn, const char* header);
		BinLoader(const std::string& fn);
		BinLoader(const char* fn);
		~BinLoader();
		void Process(const char* header, std::function<void(BinLoader&)> Loader);
		void ProcessAny(std::function<void(BinLoader&, const char*)> Loader);

		template<typename T>
		void Process(const char* header, T& Value) {
			Process(header, [&Value](BinLoader& bin) {
				bin.Read<T>(Value);
			});
		}
		template<typename T>
		void Process(const char* header, std::vector<T>& Value) {
			Process(header, [&Value](BinLoader& bin) {
				size_t cnt = bin.Remaining(sizeof(T));
				Value.resize(cnt);
				if(cnt) bin.ReadAll(&Value[0]);
			});
		}
		void Process(const char* header, std::string& Value) {
			Process(header, [&Value](BinLoader& bin) {
				Value = bin.RemainingAsString();
			});
		}

		void Read(void* Data, size_t sz);
		size_t ReadAll(void* Data);
		std::vector<unsigned char> ReadAll();
		std::string RemainingAsString();
		std::wstring RemainingAsWideString();
		//short Short();
		//unsigned short UShort();
		//unsigned int UInt();
		//int Int();
		//float Float();
		//double Double();
		//uint64_t QuadWord();
		//PVX::Vector2D Vec2();
		//PVX::Vector3D Vec3();
		//PVX::Vector4D Vec4();
		//PVX::Matrix2x2 Mat2();
		//PVX::Matrix3x3 Mat3();
		//PVX::Matrix4x4 Mat4();
		//PVX::Matrix3x4 Mat3x4();
		void Execute();
		int OK();
		size_t Remaining(int ItemSize = 1);

		void ReadBytes(std::vector<unsigned char>& Bytes, size_t size = 0);

		template<typename T>
		T read() {
			T ret;
			Read(&ret, sizeof(T));
			return ret;
		}

		template<typename T>
		inline void Read(T& val) { Read(&val, sizeof(T)); }

		template<typename T>
		inline void Read(const char* Name, T& val) {
			Process(Name, [&val](BinLoader& b) {
				b.ReadAll(&val);
			});
		}

		template<typename T>
		inline void Read(const char* Name, std::vector<T>& val) {
			Process(Name, [&val](BinLoader& b) {
				size_t sz = b.Remaining(sizeof(T));
				val.resize(sz);
				b.ReadAll(&val[0]);
			});
		}

		inline void Read(const char* Name, std::string& val) {
			Process(Name, [&val](BinLoader& b) {
				size_t sz = b.Remaining();
				val.resize(sz);
				b.ReadAll(&val[0]);
			});
		}
	};
}
#endif
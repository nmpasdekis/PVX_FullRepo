#include <array>
#include <vector>
#include <functional>
#include <type_traits>

namespace PVX::Encrypt {
	class CRC32_Algorithm {
		unsigned int CRC32 = 0;
	public:
		enum{ BlockSize = 4, OutputSize = 4, Cycles = 1 };
		CRC32_Algorithm& Update(const void* Message, size_t MessageSize);
		std::array<unsigned char, 4> operator()();
		unsigned int Get() { return CRC32; }
		template<typename T>
		CRC32_Algorithm& Update(const T& Message) {
			return Update(Message.data(), Message.size());
		}
	};

	class SHA1_Algorithm {
	private:
		unsigned int h0 = 0x67452301;
		unsigned int h1 = 0xEFCDAB89;
		unsigned int h2 = 0x98BADCFE;
		unsigned int h3 = 0x10325476;
		unsigned int h4 = 0xC3D2E1F0;
		unsigned long long BitCount = 0;
		unsigned long long More = 0;
		unsigned char tmp[128]{ 0 };
	protected:
		void ProcessBlock(void* Block);
	public:
		enum { BlockSize = 64, OutputSize = 20, Cycles = 80 };
		SHA1_Algorithm& Update(const void* Message, size_t MessageSize);
		std::array<unsigned char, 20> operator()();
		template<typename T>
		SHA1_Algorithm& Update(const T& Message) {
			return Update(Message.data(), Message.size());
		}
	};
	class SHA256_Algorithm{
	private:
		unsigned int h0 = 0x6a09e667;
		unsigned int h1 = 0xbb67ae85;
		unsigned int h2 = 0x3c6ef372;
		unsigned int h3 = 0xa54ff53a;
		unsigned int h4 = 0x510e527f;
		unsigned int h5 = 0x9b05688c;
		unsigned int h6 = 0x1f83d9ab;
		unsigned int h7 = 0x5be0cd19;
		unsigned long long BitCount = 0;
		unsigned long long More = 0;
		unsigned char tmp[128]{ 0 };
	protected:
		void ProcessBlock(void* Block);
	public:
		enum { BlockSize = 64, OutputSize = 32, Cycles = 64 };
		SHA256_Algorithm& Update(const void* Message, size_t MessageSize);
		std::array<unsigned char, 32> operator()();
		template<typename T>
		SHA256_Algorithm& Update(const T& Message) {
			return Update(Message.data(), Message.size());
		}
	};
	class SHA512_Algorithm {
	private:
		unsigned long long h0 = 0x6a09e667f3bcc908;
		unsigned long long h1 = 0xbb67ae8584caa73b;
		unsigned long long h2 = 0x3c6ef372fe94f82b;
		unsigned long long h3 = 0xa54ff53a5f1d36f1;
		unsigned long long h4 = 0x510e527fade682d1;
		unsigned long long h5 = 0x9b05688c2b3e6c1f;
		unsigned long long h6 = 0x1f83d9abfb41bd6b;
		unsigned long long h7 = 0x5be0cd19137e2179;
		unsigned long long BitCount = 0;
		unsigned long long More = 0;
		unsigned char tmp[256]{ 0 };
	protected:
		void ProcessBlock(void* Block);
	public:
		enum { BlockSize = 128, OutputSize = 64, Cycles = 80 };
		SHA512_Algorithm& Update(const void* Message, size_t MessageSize);
		std::array<unsigned char, OutputSize> operator()();
		template<typename T>
		SHA512_Algorithm& Update(const T& Message) { return Update(Message.data(), Message.size()); }
	};

	template<typename Hash>
	decltype(Hash()()) HMAC(const void* Key, size_t KeySize, const void* Message, size_t MessageSize) {
		using Block = std::array<unsigned char, Hash::BlockSize>;
		using OutBlock = decltype(Hash()());

		Block key = [](const unsigned char* k, size_t sz) {
			if (sz > Hash::BlockSize) {
				OutBlock tmpKey = Hash().Update(k, sz)();
				if constexpr (Hash::OutputSize < Hash::BlockSize) {
					Block ret{ 0 };
					memcpy(&ret[0], &tmpKey[0], Hash::OutputSize);
					return ret;
				} else {
					Block ret{ 0 };
					memcpy(&ret[0], &tmpKey[0], Hash::BlockSize);
					return ret;
				}
			} else {
				Block ret{ 0 };
				memcpy(&ret[0], k, sz);
				return ret;
			}
		}((unsigned char*)Key, KeySize);

		Block opad;
		Block ipad;
		for (int i = 0; i<Hash::BlockSize; i++) {
			opad[i] = 0x5c ^ key[i];
			ipad[i] = 0x36 ^ key[i];
		}
		auto Inner = Hash().Update(ipad).Update(Message, MessageSize)();
		return Hash().Update(opad).Update(Inner)();
	}
	template<typename Hash, typename T1, typename T2>
	decltype(Hash()()) HMAC(const T1& Key, const T2& Message) {
		return HMAC<Hash>(Key.data(), Key.size(), Message.data(), Message.size());
	}
}
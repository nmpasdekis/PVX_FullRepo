#include <array>
#include <vector>
#include <string>
#include <functional>
#include <type_traits>
#include <PVX_Encode.h>

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
		uint64_t BitCount = 0;
		uint64_t More = 0;
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
		uint64_t BitCount = 0;
		uint64_t More = 0;
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
		uint64_t h0 = 0x6a09e667f3bcc908;
		uint64_t h1 = 0xbb67ae8584caa73b;
		uint64_t h2 = 0x3c6ef372fe94f82b;
		uint64_t h3 = 0xa54ff53a5f1d36f1;
		uint64_t h4 = 0x510e527fade682d1;
		uint64_t h5 = 0x9b05688c2b3e6c1f;
		uint64_t h6 = 0x1f83d9abfb41bd6b;
		uint64_t h7 = 0x5be0cd19137e2179;
		uint64_t BitCount = 0;
		uint64_t More = 0;
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

	template<typename Hash, int Iterations, int saltSize, typename T1, typename T2>
	decltype(Hash()()) PBKDF2_F(const T1& password, T2& salt, int i) {
		salt[saltSize + 0] = (i >> 24)&0xff;
		salt[saltSize + 1] = (i >> 16)&0xff;
		salt[saltSize + 2] = (i >>  8)&0xff;
		salt[saltSize + 3] = (i >>  0)&0xff;
		auto U = HMAC<Hash>(password, salt);
		auto U0 = U;
		
		for (auto j = 1; j < Iterations; j++) {
			U = HMAC<Hash>(password, U);
			for (auto k = 0; k<U.size(); k++)
				U0[k] ^= U[k];
		}
		return U0;
	}

	template<typename Hash, int KeyLen, int saltSize, int Iterations, typename saltType>
	std::array<unsigned char, KeyLen / 8> PBKDF2(const std::string& password, const saltType& salt) {
		std::array<unsigned char, saltSize / 8 + 4> Salt;
		memcpy(&Salt[0], salt, saltSize / 8);

		constexpr int iters = (KeyLen / 8 + Hash::OutputSize - 1) / Hash::OutputSize;
		std::array<unsigned char, KeyLen / 8> ret;

		int curSize = 0;
		int i = 1;
		if constexpr ((KeyLen / 8) >= Hash::OutputSize) {
			for (; i<iters; i++, curSize += Hash::OutputSize) {
				auto U = PBKDF2_F<Hash, Iterations, saltSize / 8>(password, Salt, i);
				memcpy(ret.data() + curSize, U.data(), Hash::OutputSize);
			}
		}
		if constexpr ((KeyLen / 8) % Hash::OutputSize) {
			auto U = PBKDF2_F<Hash, Iterations, saltSize / 8>(password, Salt, i);
			memcpy(ret.data() + curSize, U.data(), (KeyLen / 8) % Hash::OutputSize);
		}

		return ret;
	}


	template<typename Hash, int KeyLen, int saltSize, int Iterations>
	std::array<unsigned char, KeyLen / 8> PBKDF2(
		const unsigned char* Password, int PasswordSize, 
		const unsigned char* salt) {

		std::string password;
		password.resize(PasswordSize);
		memcpy(password.data(), Password, PasswordSize);
		std::array<unsigned char, saltSize / 8 + 4> Salt;
		memcpy(Salt.data(), salt, saltSize / 8);

		constexpr int iters = (KeyLen / 8 + Hash::OutputSize - 1) / Hash::OutputSize;
		std::array<unsigned char, KeyLen / 8> ret{};

		int curSize = 0;
		int i = 1;
		if constexpr ((KeyLen / 8) >= Hash::OutputSize) {
			for (; i<iters; i++, curSize += Hash::OutputSize) {
				auto U = PBKDF2_F<Hash, Iterations, saltSize / 8>(password, Salt, i);
				memcpy(ret.data() + curSize, U.data(), Hash::OutputSize);
			}
		}
		if constexpr (((KeyLen / 8) % Hash::OutputSize) != 0) {
			auto U = PBKDF2_F<Hash, Iterations, saltSize / 8>(password, Salt, i);
			memcpy(ret.data() + curSize, U.data(), (KeyLen / 8) % Hash::OutputSize);
		}
		return ret;
	}

	template<typename Algorithm, int SaltSize, int HashLength, int Iterations>
	bool IdentityPasswordVerifier(const std::wstring& Password, const std::wstring& PasswordHash) {
		using namespace PVX::Encrypt;
		auto pass = PVX::Encode::UTF(Password);
		auto Hashed = PVX::Decode::Base64(PasswordHash);
		auto NewHash = PBKDF2<Algorithm, HashLength, SaltSize, Iterations>(pass.data(), pass.size(), Hashed.data() + 1);
		return !memcmp(NewHash.data(), Hashed.data() + 1 + SaltSize / 8, HashLength / 8);
	}
}
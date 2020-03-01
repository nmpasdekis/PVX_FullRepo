#include <PVX_Encrypt.h>

namespace PVX::Encrypt {
	using u32 = unsigned int;
	using u64 = unsigned long long;

	inline u32 leftrotate(u32 v, int i) {
		return (v << i) | (v >> (32-i));
	}
	inline u64 leftrotate(u64 v, int i) {
		return (v << i) | (v >> (64-i));
	}
	inline u32 rightrotate(u32 v, int i) {
		return (v << (32 - i)) | (v >> i);
	}
	inline u64 rightrotate(u64 v, int i) {
		return (v << (64 - i)) | (v >> i);
	}
	inline u32 endian(u32 v) {
		return ((leftrotate(v, 24) & 0xff00ff00) | (leftrotate(v, 8) & 0x00ff00ff));
	}
	inline u64 endian(u64 v) {
		u32 c1 = (v >> 32) & 0x00ffffffff; c1 = endian(c1);
		u32 c0 = (v) & 0x00ffffffff; c0 = endian(c0);
		return c1 | (((u64)c0) << 32);
	}

//#define Make_Choise(b, c, d) ((b & c) ^ ((~b) & d))
#define Make_Choise(b, c, d) ((b & c) | ((~b) & d))
//#define Make_Choise(b, c, d) (d ^ (b & (c & d)))
//#define Make_Choise(b, c, d) ((b & c) + ((~b) & d))

//#define Majority(b, c, d) ((b & c) ^ (b & d) ^ (c & d))
#define Majority(b, c, d) ((b & c) | (b & d) | (c & d))
//#define Majority(b, c, d) ((b & c) | (d & (b | c)))
//#define Majority(b, c, d) ((b & c) | (d & (b ^ c)))
//#define Majority(b, c, d) ((b & c) ^ (d & (b ^ c)))
//#define Majority(b, c, d) ((b & c) + (d & (b ^ c)))
//#define Majority(b, c, d) ((b & c) ^ (b & d) ^ (c & d))


#define do_all(i, nk) \
	k = (nk);\
	u32 temp = leftrotate(a, 5) + f + e + k + w[i]; \
    e = d; \
	d = c; \
	c = leftrotate(b, 30); \
	b = a; \
	a = temp;

#define do_00_19(i) { f = (b & c) | ((~b) & d); do_all((i), 0x5A827999);}
#define do_20_39(i) { f = b ^ c ^ d; do_all((i), 0x6ED9EBA1);}
#define do_40_59(i) { f = (b & c) | (b & d) | (c & d); do_all((i), 0x8F1BBCDC);}
#define do_60_79(i) { f = b ^ c ^ d; do_all((i), 0xCA62C1D6);}

	void SHA1_Algorithm::ProcessBlock(void* Block) {
		u32* block = (u32*)Block;
		u32 w[80], f, k;
		u32 a = h0;
		u32 b = h1;
		u32 c = h2;
		u32 d = h3;
		u32 e = h4;

		for (int i = 0; i<16; i++) w[i] = endian(block[i]);
		for (int i = 16; i < 80; i++) w[i] = leftrotate(w[i-3] ^ w[i-8] ^ w[i-14] ^ w[i-16], 1);

		for (int i = 0; i < 20; i++) do_00_19(i);
		for (int i = 20; i < 40; i++) do_20_39(i);
		for (int i = 40; i < 60; i++) do_40_59(i);
		for (int i = 60; i < 80; i++) do_60_79(i);

		h0 += a;
		h1 += b;
		h2 += c;
		h3 += d;
		h4 += e;
	}
	SHA1_Algorithm& SHA1_Algorithm::Update(const void* Message, size_t MessageSize) {
		unsigned char* Msg = (unsigned char*)Message;
		BitCount += MessageSize * 8;

		if (More) {
			int less = BlockSize - More;
			if (MessageSize<less)less = MessageSize;
			memcpy(tmp+More, Message, less);
			More += less;
			MessageSize -= less;
			Msg += less;
			if (More < BlockSize) {
				return *this;
			}
			ProcessBlock(tmp);
			More = 0;
			memset(tmp, 0, BlockSize*2);
		}

		while (MessageSize >= BlockSize) {
			ProcessBlock(Msg);
			Msg += BlockSize;
			MessageSize -= BlockSize;
		}

		memcpy(tmp, Msg, MessageSize);
		More = MessageSize;

		return *this;
	}
	std::array<unsigned char, 20> SHA1_Algorithm::operator()() {
		u64 ml = endian(BitCount);

		u64* tmp64 = ((u64*)tmp);
		tmp64[15] = ml;

		tmp[More] = 0x80;
		if (More < 56) {
			tmp64[7] = ml;
			ProcessBlock(tmp);
		} else {
			ProcessBlock(tmp);
			ProcessBlock((tmp+64));
		}

		std::array<unsigned char, 20> ret;
		u32* retDword = (u32*)&ret[0];
		retDword[0] = endian(h0);
		retDword[1] = endian(h1);
		retDword[2] = endian(h2);
		retDword[3] = endian(h3);
		retDword[4] = endian(h4);

		h0 = 0x67452301;
		h1 = 0xEFCDAB89;
		h2 = 0x98BADCFE;
		h3 = 0x10325476;
		h4 = 0xC3D2E1F0;

		BitCount = 0;
		More = 0;
		memset(tmp, 0, 128);
		return ret;
	}



	void SHA256_Algorithm::ProcessBlock(void* Block) {
		constexpr u32 k[]{
			0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
			0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
			0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
			0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
			0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
			0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
			0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
			0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
		};
		u32* block = (u32*)Block;
		u32 w[Cycles];

		u32 a = h0;
		u32 b = h1;
		u32 c = h2;
		u32 d = h3;
		u32 e = h4;
		u32 f = h5;
		u32 g = h6;
		u32 h = h7;

		for (int i = 0; i<16; i++) w[i] = endian(block[i]);
		for (int i = 16; i < Cycles; i++) {
			u32 s1 = (rightrotate(w[i - 15], 7) ^ rightrotate(w[i - 15], 18) ^ (w[i - 15] >> 3));
			u32 s2 = (rightrotate(w[i - 2], 17) ^ rightrotate(w[i - 2], 19) ^ (w[i - 2] >> 10));
			w[i] = w[i - 16] + s1 +	w[i - 7] + s2;
		}

		for (int i = 0; i < Cycles; i++) {
			u32 S1 = rightrotate(e, 6) ^ rightrotate(e, 11) ^ rightrotate(e, 25);
			u32 ch = Make_Choise(e, f, g);
			u32 temp1 = h + S1 + ch + k[i] + w[i];
			u32 S0 = rightrotate(a, 2) ^ rightrotate(a, 13) ^ rightrotate(a, 22);
			u32 maj = Majority(a, b, c);
			u32 temp2 = S0 + maj;

			h = g;
			g = f;
			f = e;
			e = d + temp1;
			d = c;
			c = b;
			b = a;
			a = temp1 + temp2;
		}

		h0 += a;
		h1 += b;
		h2 += c;
		h3 += d;
		h4 += e;
		h5 += f;
		h6 += g;
		h7 += h;
	}
	SHA256_Algorithm& SHA256_Algorithm::Update(const void* Message, size_t MessageSize) {
		unsigned char* Msg = (unsigned char*)Message;
		BitCount += MessageSize * 8;

		if (More) {
			int less = BlockSize - More;
			if (MessageSize<less)less = MessageSize;
			memcpy(tmp+More, Message, less);
			More += less;
			MessageSize -= less;
			Msg += less;
			if (More<BlockSize) {
				return *this;
			}
			ProcessBlock(tmp);
			More = 0;
			memset(tmp, 0, BlockSize*2);
		}

		while (MessageSize>=BlockSize) {
			ProcessBlock(Msg);
			Msg += BlockSize;
			MessageSize -= BlockSize;
		}

		memcpy(tmp, Msg, MessageSize);
		More = MessageSize;

		return *this;
	}
	std::array<unsigned char, 32> SHA256_Algorithm::operator()() {
		u64 ml = endian(BitCount);

		u64* tmp64 = ((u64*)tmp);
		tmp64[(BlockSize/4)-1] = ml;

		tmp[More] = 0x80;
		if (More < (BlockSize - 8)) {
			tmp64[(BlockSize/8)-1] = ml;
			ProcessBlock(tmp);
		} else {
			ProcessBlock(tmp);
			ProcessBlock((tmp+64));
		}

		std::array<unsigned char, OutputSize> ret;
		u32* retDword = (u32*)&ret[0];
		retDword[0] = endian(h0);
		retDword[1] = endian(h1);
		retDword[2] = endian(h2);
		retDword[3] = endian(h3);
		retDword[4] = endian(h4);
		retDword[5] = endian(h5);
		retDword[6] = endian(h6);
		retDword[7] = endian(h7);

		h0 = 0x6a09e667;
		h1 = 0xbb67ae85;
		h2 = 0x3c6ef372;
		h3 = 0xa54ff53a;
		h4 = 0x510e527f;
		h5 = 0x9b05688c;
		h6 = 0x1f83d9ab;
		h7 = 0x5be0cd19;

		BitCount = 0;
		More = 0;
		memset(tmp, 0, BlockSize * 2);
		return ret;
	}



	void SHA512_Algorithm::ProcessBlock(void* Block) {
		constexpr u64 k[]{
			0x428a2f98d728ae22, 0x7137449123ef65cd, 0xb5c0fbcfec4d3b2f, 0xe9b5dba58189dbbc, 0x3956c25bf348b538,
			0x59f111f1b605d019, 0x923f82a4af194f9b, 0xab1c5ed5da6d8118, 0xd807aa98a3030242, 0x12835b0145706fbe,
			0x243185be4ee4b28c, 0x550c7dc3d5ffb4e2, 0x72be5d74f27b896f, 0x80deb1fe3b1696b1, 0x9bdc06a725c71235,
			0xc19bf174cf692694, 0xe49b69c19ef14ad2, 0xefbe4786384f25e3, 0x0fc19dc68b8cd5b5, 0x240ca1cc77ac9c65,
			0x2de92c6f592b0275, 0x4a7484aa6ea6e483, 0x5cb0a9dcbd41fbd4, 0x76f988da831153b5, 0x983e5152ee66dfab,
			0xa831c66d2db43210, 0xb00327c898fb213f, 0xbf597fc7beef0ee4, 0xc6e00bf33da88fc2, 0xd5a79147930aa725,
			0x06ca6351e003826f, 0x142929670a0e6e70, 0x27b70a8546d22ffc, 0x2e1b21385c26c926, 0x4d2c6dfc5ac42aed,
			0x53380d139d95b3df, 0x650a73548baf63de, 0x766a0abb3c77b2a8, 0x81c2c92e47edaee6, 0x92722c851482353b,
			0xa2bfe8a14cf10364, 0xa81a664bbc423001, 0xc24b8b70d0f89791, 0xc76c51a30654be30, 0xd192e819d6ef5218,
			0xd69906245565a910, 0xf40e35855771202a, 0x106aa07032bbd1b8, 0x19a4c116b8d2d0c8, 0x1e376c085141ab53,
			0x2748774cdf8eeb99, 0x34b0bcb5e19b48a8, 0x391c0cb3c5c95a63, 0x4ed8aa4ae3418acb, 0x5b9cca4f7763e373,
			0x682e6ff3d6b2b8a3, 0x748f82ee5defb2fc, 0x78a5636f43172f60, 0x84c87814a1f0ab72, 0x8cc702081a6439ec,
			0x90befffa23631e28, 0xa4506cebde82bde9, 0xbef9a3f7b2c67915, 0xc67178f2e372532b, 0xca273eceea26619c,
			0xd186b8c721c0c207, 0xeada7dd6cde0eb1e, 0xf57d4f7fee6ed178, 0x06f067aa72176fba, 0x0a637dc5a2c898a6,
			0x113f9804bef90dae, 0x1b710b35131c471b, 0x28db77f523047d84, 0x32caab7b40c72493, 0x3c9ebe0a15c9bebc,
			0x431d67c49c100d4c, 0x4cc5d4becb3e42b6, 0x597f299cfc657e2a, 0x5fcb6fab3ad6faec, 0x6c44198c4a475817
		};
		u64* block = (u64*)Block;
		u64 w[Cycles];
		u64 a = h0;
		u64 b = h1;
		u64 c = h2;
		u64 d = h3;
		u64 e = h4;
		u64 f = h5;
		u64 g = h6;
		u64 h = h7;

		for (int i = 0; i<16; i++) w[i] = endian(block[i]);
		for (int i = 16; i < Cycles; i++) {
			u64 s1 = (rightrotate(w[i-15], 1) ^ rightrotate(w[i-15], 8) ^ (w[i-15] >> 7));
			u64 s2 = (rightrotate(w[i-2], 19) ^ rightrotate(w[i-2], 61) ^ (w[i-2] >> 6));
			w[i] = w[i - 16] + s1 +	w[i - 7] + s2;
		}

		for (int i = 0; i < Cycles; i++) {
			u64 S1 = rightrotate(e, 14) ^ rightrotate(e, 18) ^ rightrotate(e, 41);
			u64 ch = Make_Choise(e, f, g);
			u64 temp1 = h + S1 + ch + k[i] + w[i];
			u64 S0 = rightrotate(a, 28) ^ rightrotate(a, 34) ^ rightrotate(a, 39);
			u64 maj = Majority(a, b, c);
			u64 temp2 = S0 + maj;

			h = g;
			g = f;
			f = e;
			e = d + temp1;
			d = c;
			c = b;
			b = a;
			a = temp1 + temp2;
		}

		h0 += a;
		h1 += b;
		h2 += c;
		h3 += d;
		h4 += e;
		h5 += f;
		h6 += g;
		h7 += h;
	}
	SHA512_Algorithm& SHA512_Algorithm::Update(const void* Message, size_t MessageSize) {
		unsigned char* Msg = (unsigned char*)Message;
		BitCount += MessageSize * 8;

		if (More) {
			int less = BlockSize - More;
			if (MessageSize<less)less = MessageSize;
			memcpy(tmp+More, Message, less);
			More += less;
			MessageSize -= less;
			Msg += less;
			if (More<BlockSize) {
				return *this;
			}
			ProcessBlock(tmp);
			More = 0;
			memset(tmp, 0, BlockSize*2);
		}

		while (MessageSize>=BlockSize) {
			ProcessBlock(Msg);
			Msg += BlockSize;
			MessageSize -= BlockSize;
		}

		memcpy(tmp, Msg, MessageSize);
		More = MessageSize;

		return *this;
	}
	std::array<unsigned char, 64> SHA512_Algorithm::operator()() {
		u64 ml = endian(BitCount);

		u64* tmp64 = ((u64*)tmp);
		tmp64[(BlockSize/4)-1] = ml;

		tmp[More] = 0x80;
		if (More < (BlockSize - 16)) {
			tmp64[(BlockSize/8)-1] = ml;
			ProcessBlock(tmp);
		} else {
			ProcessBlock(tmp);
			ProcessBlock((tmp+64));
		}

		std::array<unsigned char, OutputSize> ret;
		u64* retDword = (u64*)&ret[0];
		retDword[0] = endian(h0);
		retDword[1] = endian(h1);
		retDword[2] = endian(h2);
		retDword[3] = endian(h3);
		retDword[4] = endian(h4);
		retDword[5] = endian(h5);
		retDword[6] = endian(h6);
		retDword[7] = endian(h7);

		h0 = 0x6a09e667f3bcc908;
		h1 = 0xbb67ae8584caa73b;
		h2 = 0x3c6ef372fe94f82b;
		h3 = 0xa54ff53a5f1d36f1;
		h4 = 0x510e527fade682d1;
		h5 = 0x9b05688c2b3e6c1f;
		h6 = 0x1f83d9abfb41bd6b;
		h7 = 0x5be0cd19137e2179;

		BitCount = 0;
		More = 0;
		memset(tmp, 0, BlockSize * 2);
		return ret;
	}
	namespace {
		constexpr unsigned int PVX_CRCLookUp32[256] = {
		0x00000000,0xb71dc104,0x6e3b8209,0xd926430d,0xdc760413,0x6b6bc517,0xb24d861a,0x0550471e,
		0xb8ed0826,0x0ff0c922,0xd6d68a2f,0x61cb4b2b,0x649b0c35,0xd386cd31,0x0aa08e3c,0xbdbd4f38,
		0x70db114c,0xc7c6d048,0x1ee09345,0xa9fd5241,0xacad155f,0x1bb0d45b,0xc2969756,0x758b5652,
		0xc836196a,0x7f2bd86e,0xa60d9b63,0x11105a67,0x14401d79,0xa35ddc7d,0x7a7b9f70,0xcd665e74,
		0xe0b62398,0x57abe29c,0x8e8da191,0x39906095,0x3cc0278b,0x8bdde68f,0x52fba582,0xe5e66486,
		0x585b2bbe,0xef46eaba,0x3660a9b7,0x817d68b3,0x842d2fad,0x3330eea9,0xea16ada4,0x5d0b6ca0,
		0x906d32d4,0x2770f3d0,0xfe56b0dd,0x494b71d9,0x4c1b36c7,0xfb06f7c3,0x2220b4ce,0x953d75ca,
		0x28803af2,0x9f9dfbf6,0x46bbb8fb,0xf1a679ff,0xf4f63ee1,0x43ebffe5,0x9acdbce8,0x2dd07dec,
		0x77708634,0xc06d4730,0x194b043d,0xae56c539,0xab068227,0x1c1b4323,0xc53d002e,0x7220c12a,
		0xcf9d8e12,0x78804f16,0xa1a60c1b,0x16bbcd1f,0x13eb8a01,0xa4f64b05,0x7dd00808,0xcacdc90c,
		0x07ab9778,0xb0b6567c,0x69901571,0xde8dd475,0xdbdd936b,0x6cc0526f,0xb5e61162,0x02fbd066,
		0xbf469f5e,0x085b5e5a,0xd17d1d57,0x6660dc53,0x63309b4d,0xd42d5a49,0x0d0b1944,0xba16d840,
		0x97c6a5ac,0x20db64a8,0xf9fd27a5,0x4ee0e6a1,0x4bb0a1bf,0xfcad60bb,0x258b23b6,0x9296e2b2,
		0x2f2bad8a,0x98366c8e,0x41102f83,0xf60dee87,0xf35da999,0x4440689d,0x9d662b90,0x2a7bea94,
		0xe71db4e0,0x500075e4,0x892636e9,0x3e3bf7ed,0x3b6bb0f3,0x8c7671f7,0x555032fa,0xe24df3fe,
		0x5ff0bcc6,0xe8ed7dc2,0x31cb3ecf,0x86d6ffcb,0x8386b8d5,0x349b79d1,0xedbd3adc,0x5aa0fbd8,
		0xeee00c69,0x59fdcd6d,0x80db8e60,0x37c64f64,0x3296087a,0x858bc97e,0x5cad8a73,0xebb04b77,
		0x560d044f,0xe110c54b,0x38368646,0x8f2b4742,0x8a7b005c,0x3d66c158,0xe4408255,0x535d4351,
		0x9e3b1d25,0x2926dc21,0xf0009f2c,0x471d5e28,0x424d1936,0xf550d832,0x2c769b3f,0x9b6b5a3b,
		0x26d61503,0x91cbd407,0x48ed970a,0xfff0560e,0xfaa01110,0x4dbdd014,0x949b9319,0x2386521d,
		0x0e562ff1,0xb94beef5,0x606dadf8,0xd7706cfc,0xd2202be2,0x653deae6,0xbc1ba9eb,0x0b0668ef,
		0xb6bb27d7,0x01a6e6d3,0xd880a5de,0x6f9d64da,0x6acd23c4,0xddd0e2c0,0x04f6a1cd,0xb3eb60c9,
		0x7e8d3ebd,0xc990ffb9,0x10b6bcb4,0xa7ab7db0,0xa2fb3aae,0x15e6fbaa,0xccc0b8a7,0x7bdd79a3,
		0xc660369b,0x717df79f,0xa85bb492,0x1f467596,0x1a163288,0xad0bf38c,0x742db081,0xc3307185,
		0x99908a5d,0x2e8d4b59,0xf7ab0854,0x40b6c950,0x45e68e4e,0xf2fb4f4a,0x2bdd0c47,0x9cc0cd43,
		0x217d827b,0x9660437f,0x4f460072,0xf85bc176,0xfd0b8668,0x4a16476c,0x93300461,0x242dc565,
		0xe94b9b11,0x5e565a15,0x87701918,0x306dd81c,0x353d9f02,0x82205e06,0x5b061d0b,0xec1bdc0f,
		0x51a69337,0xe6bb5233,0x3f9d113e,0x8880d03a,0x8dd09724,0x3acd5620,0xe3eb152d,0x54f6d429,
		0x7926a9c5,0xce3b68c1,0x171d2bcc,0xa000eac8,0xa550add6,0x124d6cd2,0xcb6b2fdf,0x7c76eedb,
		0xc1cba1e3,0x76d660e7,0xaff023ea,0x18ede2ee,0x1dbda5f0,0xaaa064f4,0x738627f9,0xc49be6fd,
		0x09fdb889,0xbee0798d,0x67c63a80,0xd0dbfb84,0xd58bbc9a,0x62967d9e,0xbbb03e93,0x0cadff97,
		0xb110b0af,0x060d71ab,0xdf2b32a6,0x6836f3a2,0x6d66b4bc,0xda7b75b8,0x035d36b5,0xb440f7b1
		};
	}
	CRC32_Algorithm& CRC32_Algorithm::Update(const void* Message, size_t MessageSize) {
		unsigned char* _byte = (unsigned char*)Message;
		while (MessageSize--) {
			CRC32 = (CRC32>>8)^(PVX_CRCLookUp32[(*_byte++)^(CRC32&0xff)]);
		}
		return *this;
	}

	std::array<unsigned char, 4> CRC32_Algorithm::operator()() {
		std::array<unsigned char, 4> ret;
		*(unsigned int*)& ret[0] = CRC32;
		return ret;
	}
}
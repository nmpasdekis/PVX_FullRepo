#include <vector>

#define Z_NO_COMPRESSION         0
#define Z_BEST_SPEED             1
#define Z_BEST_COMPRESSION       9
#define Z_DEFAULT_COMPRESSION  (-1)

namespace PVX {
	namespace Compress {
		enum class DeflateLevel {
			NoCompression	= 0,
			BestSpeed		= 1,
			BestCompression	= 9,
			Default			= -1
		};

		int Deflate(std::vector<unsigned char>& dest, const unsigned char* source, int sSize, DeflateLevel level = DeflateLevel::BestCompression);
		std::vector<unsigned char> Deflate(const std::vector<unsigned char> & data, DeflateLevel level = DeflateLevel::BestCompression);

		int Inflate(std::vector<unsigned char> & dest, const unsigned char *source, int sSize);
		std::vector<unsigned char> Inflate(const std::vector<unsigned char> & data);
	}
}
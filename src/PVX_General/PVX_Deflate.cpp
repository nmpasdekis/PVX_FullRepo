#include <assert.h>
#include "zlib.h"
#include <vector>
#include <PVX_Deflate.h>

/*
#define Z_NO_COMPRESSION         0
#define Z_BEST_SPEED             1
#define Z_BEST_COMPRESSION       9
#define Z_DEFAULT_COMPRESSION  (-1)
*/
namespace PVX {
	namespace Compress {

#define CHUNK 0x4000

		int Deflate(std::vector<unsigned char> & dest, const unsigned char *source, int sSize, DeflateLevel level) {
			int ret, flush;
			unsigned have;
			z_stream strm;
			unsigned char out[CHUNK];
			int cur = 0;

			/* allocate deflate state */
			strm.zalloc = Z_NULL;
			strm.zfree = Z_NULL;
			strm.opaque = Z_NULL;
			
			if ((ret = deflateInit(&strm, (int)level)) != Z_OK) return ret;


			/* compress until end of file */
			do {
				int more = sSize - cur;
				if (more > CHUNK) more = CHUNK;

				strm.next_in = (unsigned char*)(source + cur);
				strm.avail_in = more;
				cur += more;

				flush = more ? Z_NO_FLUSH : Z_FINISH;
				do {
					strm.avail_out = CHUNK;
					strm.next_out = out;

					ret = deflate(&strm, flush);    /* no bad return value */
					assert(ret != Z_STREAM_ERROR);  /* state not clobbered */

					if (have = CHUNK - strm.avail_out) {
						auto sz = dest.size();
						dest.resize(sz + have);
						memcpy(&dest[sz], out, have);
					}
				} while (strm.avail_out == 0);
				assert(strm.avail_in == 0);     /* all input will be used */

				/* done when last data in file processed */
			} while (flush != Z_FINISH);
			assert(ret == Z_STREAM_END);        /* stream will be complete */

			/* clean up and return */
			(void)deflateEnd(&strm);
			return Z_OK;
		}
		std::vector<unsigned char> Deflate(const std::vector<unsigned char> & data, DeflateLevel Level) {
			std::vector<unsigned char> ret;
			if (Deflate(ret, data.data(), data.size(), Level)) ret.clear();
			return ret;
		}

		int Inflate(std::vector<unsigned char> & dest, const unsigned char *source, int sSize) {
			int ret;
			unsigned have;
			z_stream strm;
			unsigned char out[CHUNK];
			int cur = 0;

			/* allocate inflate state */
			strm.zalloc = Z_NULL;
			strm.zfree = Z_NULL;
			strm.opaque = Z_NULL;
			strm.avail_in = 0;
			strm.next_in = Z_NULL;

			if ((ret = inflateInit(&strm)) != Z_OK) return ret;

			/* decompress until deflate stream ends or end of file */
			do {
				int more = sSize - cur;
				if (more < 0) more = 0;
				else if (more > CHUNK) more = CHUNK;

				strm.avail_in = more;

				if (strm.avail_in == 0)
					break;
				strm.next_in = (unsigned char*)(source + cur);
				cur += more;

				/* run inflate() on input until output buffer not full */
				do {
					strm.avail_out = CHUNK;
					strm.next_out = out;

					ret = inflate(&strm, Z_NO_FLUSH);
					assert(ret != Z_STREAM_ERROR);  /* state not clobbered */
					switch (ret) {
					case Z_NEED_DICT:
						ret = Z_DATA_ERROR;     /* and fall through */
					case Z_DATA_ERROR:
					case Z_MEM_ERROR:
						inflateEnd(&strm);
						return ret;
					}

					if (have = CHUNK - strm.avail_out) {
						auto sz = dest.size();
						dest.resize(sz + have);
						memcpy(&dest[sz], out, have);
					}
				} while (strm.avail_out == 0);

				/* done when inflate() says it's done */
			} while (ret != Z_STREAM_END);

			/* clean up and return */
			inflateEnd(&strm);
			return ret == Z_STREAM_END ? Z_OK : Z_DATA_ERROR;
		}
		std::vector<unsigned char> Inflate(const std::vector<unsigned char> & data) {
			std::vector<unsigned char> ret;
			if (Inflate(ret, data.data(), data.size())) ret.clear();
			return ret;
		}
	}
}
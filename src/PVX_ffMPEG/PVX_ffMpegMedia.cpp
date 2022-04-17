#include <PVX_ffMPEG.h>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
}

namespace {
	class _ffMedia {
	public:
		inline _ffMedia(const char* fn) {

		}

	};
}

namespace PVX::ffMPEG {
	ffMedia::ffMedia(const char* filename) :
		Data{ (void*)new _ffMedia(filename), [](void* x) { delete ((_ffMedia*)x);  } }
	{}
}
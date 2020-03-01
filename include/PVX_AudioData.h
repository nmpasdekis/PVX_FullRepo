#ifndef __PVX_AUDIO_DATA_H__
#define __PVX_AUDIO_DATA_H__

namespace PVX {

	template<typename T = unsigned char>
	struct AudioData {
		unsigned int Channels;
		unsigned int BitsPerSample;
		unsigned int SampleRate;
		std::vector<T> Data;
	};
}

#endif
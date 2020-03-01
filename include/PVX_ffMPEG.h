#ifndef __PVX_AUDIO_VIDEO_H__
#define __PVX_AUDIO_VIDEO_H__
#include <vector>
#include <functional>
#include "PVX_AudioData.h"

namespace PVX {
	namespace AudioVideo {
		class Media {
		public:
			Media();
			Media(const Media & m);
			~Media();

			int ReadAudioStream(std::vector<unsigned char> & Data);
			int ReadAudioStream(std::vector<short> & Data);
			int ReadAudioStream(std::vector<float> & Data);

			int ReadAudioStream(std::vector<std::vector<short>> & Channel);
			int ReadAudioStream(std::vector<std::vector<float>> & Channel);

			void ReadAllAudio(std::vector<unsigned char> & Data);
			void ReadAllAudio(std::vector<short> & Data);
			void ReadAllAudio(std::vector<float>& Data);
			void ReadAllAudio(std::vector<std::vector<float>> & Data);

			template<typename T>
			PVX::AudioData<T> ReadAllAudio() {
				std::vector<T> ret;
				ret.reserve((192000.0 / sizeof(T) * 2.1) * EstimatedDuration());
				ReadAllAudio(ret);
				ret.shrink_to_fit();

				//int cnt = 0;
				//for (auto& t : ret)
				//	cnt += !!t;
				if constexpr (sizeof(T) == 4) {
					return {
						(unsigned int)AudioChannels(),
						32,
						(unsigned int)AudioSampleRate(),
						std::move(ret)
					};
				}
				else{
					return {
						(unsigned int)AudioChannels(),
						(unsigned int)(AudioBitsPerSmple()<16 ? 8 : 16),
						(unsigned int)AudioSampleRate(),
						std::move(ret)
					};
				}
			}

			double EstimatedDuration();
			int ReadVideoFrame(std::vector<unsigned char> & Pixels);

			int AudioSampleRate();
			int AudioBitsPerSmple();
			int AudioChannels();

			void SetAudioEvent(std::function<void(const std::vector<short> & Data)> Event);
			void SetVideoEvent(std::function<void(const std::vector<unsigned char> & Data)> Event);
			int DoEvents();

			int VideoWidth();
			int VideoHeight();
			float VideoFramerate();
		private:
			void * Data;
		};

		Media LoadFile(const char * Filename);
	}
}

#endif
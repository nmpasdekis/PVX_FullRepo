#include <PVX_ffMPEG.h>
#include <functional>
extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
}

namespace PVX {
	namespace AudioVideo {
		typedef struct _Media {
			_Media() {
				RefCount = 1;
				Context = 0;
				Audio = 0;
				Video = 0;
				ScaleContext = 0;
				av_init_packet(&ReadPacket);
			}
			~_Media() {
				if(Audio) {
					av_frame_free(&AudioFrame);
					avcodec_close(Audio);
				}
				avformat_close_input(&Context);
			}

			int FillBuffers() {
				int err, err2;
				if (ReadPacket.stream_index == AudioStreamIndex) {
					while ((err = avcodec_send_packet(Audio, &ReadPacket)) != -EAGAIN) {
						av_packet_unref(&ReadPacket);
						if ((err2 = av_read_frame(Context, &ReadPacket)))
							return err2;
					}
					return 0;
				} else if (ReadPacket.stream_index == VideoStreamIndex) {
					while ((err = avcodec_send_packet(Video, &ReadPacket)) != -EAGAIN) {
						av_packet_unref(&ReadPacket);
						if ((err2 = av_read_frame(Context, &ReadPacket)))
							return err2;
					}
					return 0;
				}
				return 1;
			}
			std::vector<short> AudioBuffer;
			std::vector<unsigned char> VideoBuffer;
			int Read() {
				int err;
				if ((err = av_read_frame(Context, &ReadPacket))) 
					return err;

				if (ReadPacket.stream_index == AudioStreamIndex) {
					if (AudioEvent != nullptr && !avcodec_send_packet(Audio, &ReadPacket)) {
						while(!ReadAudioFrame(AudioBuffer))
							AudioEvent(AudioBuffer);
					}
				} else if (ReadPacket.stream_index == VideoStreamIndex) {
					if (VideoEvent != nullptr && !avcodec_send_packet(Video, &ReadPacket))
						while(!ReadVideoFrame(VideoBuffer)) {
							VideoEvent(VideoBuffer);
					}
				}
				av_packet_unref(&ReadPacket);
				return err;
			}

			int ReadAudioFrame(std::vector<unsigned char>& data) {
				int err;
				if(!(err = avcodec_receive_frame(Audio, AudioFrame))) {
					if(AudioBytesPerSample != 4) {
						data.resize(size_t(AudioBytesPerSample) * Audio->channels * AudioFrame->nb_samples);
						auto o = &data[0];

						int pos = 0;
						for(int i = 0; i < AudioFrame->nb_samples; i++) {
							for(int j = 0; j < Audio->channels; j++) {
								memcpy(o, AudioFrame->data[j] + pos, AudioBytesPerSample);
								o += AudioBytesPerSample;
							}
							pos += AudioBytesPerSample;
						}
					} else {
						data.resize(2ull * Audio->channels * AudioFrame->nb_samples);
						short * o = (short*)&data[0];

						for(int i = 0; i < AudioFrame->nb_samples; i++) {
							for(int j = 0; j < Audio->channels; j++) {
								*o = short(((float*)AudioFrame->data[j])[i] * 0x7800);
								o++;
							}
						}
					}
				}
				return -err;
			}
			int ReadAudioFrame(std::vector<short> & data) {
				int err;
				if(!(err = avcodec_receive_frame(Audio, AudioFrame))) {
					data.resize(Audio->channels * AudioFrame->nb_samples);
					short * o = &data[0];
					if(AudioBytesPerSample == 2) {
						for(int i = 0; i < AudioFrame->nb_samples; i++) {
							for(int j = 0; j < Audio->channels; j++) {
								*o = ((short*)AudioFrame->data[j])[i];
								o++;
							}
						}
					} else if(AudioBytesPerSample == 4) {
						for(int i = 0; i < AudioFrame->nb_samples; i++) {
							for(int j = 0; j < Audio->channels; j++) {
								*o = short(((float*)AudioFrame->data[j])[i] * 0x7800);
								o++;
							}
						}
					}
				}
				return -err;
			}
			int ReadAudioFrame(std::vector<float> & data) {
				int err;
				if(!(err = avcodec_receive_frame(Audio, AudioFrame))) {
					data.resize(Audio->channels * AudioFrame->nb_samples);
					if(AudioBytesPerSample == 2) {
						auto o = &data[0];

						const float inv = 1.0f / 0x7fff;

						for(int i = 0; i < AudioFrame->nb_samples; i++) {
							for(int j = 0; j < Audio->channels; j++) {
								*o = ((short*)AudioFrame->data[j])[i] * inv;
								o++;
							}
						}
					} else if(AudioBytesPerSample == 4) {
						float * o = (float*)&data[0];

						for(int i = 0; i < AudioFrame->nb_samples; i++) {
							for(int j = 0; j < Audio->channels; j++) {
								*o = ((float*)AudioFrame->data[j])[i];
								o++;
							}
						}
					}
				}
				return -err;
			}
			int ReadAudioFrame(std::vector<std::vector<float>> & data) {
				int err;
				data.clear();
				if(!(err = avcodec_receive_frame(Audio, AudioFrame))) {
					if(AudioBytesPerSample == 2) {
						const float inv = 1.0f / 0x7fff;

						for(int j = 0; j < Audio->channels; j++) {
							std::vector<float> ch(AudioFrame->nb_samples);
							float * o = &ch[0];
							for(int i = 0; i < AudioFrame->nb_samples; i++) {
								*o = ((short*)AudioFrame->data[j])[i] * inv;
								o++;
							}
							data.push_back(ch);
						}
					} else if(AudioBytesPerSample == 4) {
						for(int j = 0; j < Audio->channels; j++) {
							std::vector<float> ch(AudioFrame->nb_samples);
							memcpy(&ch[0], AudioFrame->data[j], AudioFrame->nb_samples * sizeof(float));
							data.push_back(ch);
						}
					}
				}
				return -err;
			}
			int ReadAudioFrame(std::vector<std::vector<short>> & data) {
				int err;
				data.clear();
				if(!(err = avcodec_receive_frame(Audio, AudioFrame))) {
					if(AudioBytesPerSample == 4) {
						for(int j = 0; j < Audio->channels; j++) {
							std::vector<short> ch(AudioFrame->nb_samples);
							auto * o = &ch[0];
							for(int i = 0; i < AudioFrame->nb_samples; i++) {
								*o = short(((float*)AudioFrame->data[j])[i] * 0x7fff);
								o++;
							}
							data.push_back(ch);
						}
					} else if(AudioBytesPerSample == 2) {
						for(int j = 0; j < Audio->channels; j++) {
							std::vector<short> ch(AudioFrame->nb_samples);
							memcpy(&ch[0], AudioFrame->data[j], AudioFrame->nb_samples * sizeof(short));
							data.push_back(ch);
						}
					}
				}
				return -err;
			}

			int ReadVideoFrame(std::vector<unsigned char> & Pixels) {
				int err;
				if (!(err = avcodec_receive_frame(Video, VideoFrame))) {
					int sz = 3 * Width * Height; // (((3 * Width) + 3)& ~3)
					Pixels.resize(sz);
					if (VideoFrame->format == AVPixelFormat::AV_PIX_FMT_RGB24) {
						memcpy(Pixels.data(), VideoFrame->data, sz);
					}
					else {
						if (!ScaleContext) {
							DecodedVideoFrame = av_frame_alloc();
							av_image_fill_arrays(
								DecodedVideoFrame->data,
								DecodedVideoFrame->linesize,
								VideoBuffer.data(),
								AVPixelFormat::AV_PIX_FMT_RGB24,
								VideoFrame->width,
								VideoFrame->height,
								1);
							ScaleContext = sws_getCachedContext(0,
								Width, Height, (AVPixelFormat)VideoFrame->format,
								Width, Height, AVPixelFormat::AV_PIX_FMT_RGB24,
								SWS_BICUBIC, nullptr, nullptr, nullptr);


							//DecodedVideoFrame = av_frame_alloc();
							//avpicture_fill(reinterpret_cast<AVPicture*>(DecodedVideoFrame), VideoBuffer.data(), AVPixelFormat::AV_PIX_FMT_RGB24, VideoFrame->width, VideoFrame->height);
							//ScaleContext = sws_getCachedContext(0,
							//	Width, Height, (AVPixelFormat)VideoFrame->format,
							//	Width, Height, AVPixelFormat::AV_PIX_FMT_RGB24,
							//	SWS_BICUBIC, nullptr, nullptr, nullptr);
						}
						sws_scale(ScaleContext, VideoFrame->data, VideoFrame->linesize, 0, VideoFrame->height, DecodedVideoFrame->data, DecodedVideoFrame->linesize);
					}
				}
				return -err;
			}

			int RefCount;
			AVFormatContext * Context;
			AVPacket ReadPacket;


			int AudioBytesPerSample;
			
			int AudioStreamIndex;
			AVCodecContext * Audio;
			AVFrame * AudioFrame;

			int VideoStreamIndex;
			AVCodecContext * Video;
			AVFrame * VideoFrame, *DecodedVideoFrame;
			SwsContext* ScaleContext;
			int Width, Height;
			float Fps;

			std::function<void(const std::vector<unsigned char> & Data)> VideoEvent;
			std::function<void(const std::vector<short> & Data)> AudioEvent;
		} _Media;

		Media::Media() {
			Data = new _Media();
		}
		Media::Media(const Media & m) {
			Data = m.Data;
			((_Media*)m.Data)->RefCount++;
		}
		Media::~Media() {
			if(!--((_Media*)Data)->RefCount)
				delete ((_Media*)Data);
		}

		double Media::EstimatedDuration() {
			return ((*(_Media*)Data).Context->duration * 1.0) / AV_TIME_BASE;
		}

		int Media::ReadVideoFrame(std::vector<unsigned char>& data) {
			_Media & m = *(_Media*)Data;
			int err;
			int more = 1;
			while (more) {
				if (!(err = m.ReadVideoFrame(data)))
					return 1;
				if (err == 11)
					more = !m.FillBuffers();
			}
			return 0;
		}

		void Media::SetAudioEvent(std::function<void(const std::vector<short> & Data)> Event) {
			((_Media*)Data)->AudioEvent = Event;
		}
		void Media::SetVideoEvent(std::function<void(const std::vector<unsigned char> & Data)> Event) {
			((_Media*)Data)->VideoEvent = Event;
		}

		int Media::DoEvents() {
			return ((_Media*)Data)->Read();
		}

		int Media::VideoWidth() {
			return ((_Media*)Data)->Width;
		}

		int Media::VideoHeight() {
			return ((_Media*)Data)->Height;
		}

		float Media::VideoFramerate() {
			return ((_Media*)Data)->Fps;
		}

		int Media::ReadAudioStream(std::vector<unsigned char>& data) {
			_Media & m = *(_Media*)Data;
			int err;
			int more = 1;
			while(more) {
				if(!(err = m.ReadAudioFrame(data))) return 1;
				if(err == 11) more = !m.FillBuffers();
			}
			return 0;
		}
		int Media::ReadAudioStream(std::vector<short>& data) {
			_Media & m = *(_Media*)Data;
			int err;
			int more = 1;
			while(more) {
				if(!(err = m.ReadAudioFrame(data)))
					return 1;
				if(err == 11)
					more = !m.FillBuffers();
			}
			return 0;
		}
		int Media::ReadAudioStream(std::vector<float>& data) {
			_Media & m = *(_Media*)Data;
			int err;
			int more = 1;
			while(more) {
				if(!(err = m.ReadAudioFrame(data)))
					return 1;
				if(err == 11)
					more = !m.FillBuffers();
			}
			return 0;
		}

		int Media::ReadAudioStream(std::vector<std::vector<short>>& data) {
			_Media & m = *(_Media*)Data;
			int err;
			int more = 1;
			while(more) {
				if(!(err = m.ReadAudioFrame(data)))
					return 1;
				if(err == 11)
					more = !m.FillBuffers();
			}
			return 0;
		}

		int Media::ReadAudioStream(std::vector<std::vector<float>>& data) {
			_Media & m = *(_Media*)Data;
			int err;
			int more = 1;
			while(more) {
				if(!(err = m.ReadAudioFrame(data)))
					return 1;
				if(err == 11)
					more = !m.FillBuffers();
			}
			return 0;
		}
		void Media::ReadAllAudio(std::vector<unsigned char>& data) {
			_Media & m = *(_Media*)Data;
			auto sz = data.size();
			std::vector<unsigned char> tmp;
			while (ReadAudioStream(tmp)) {
				data.resize(sz + tmp.size());
				memcpy(&data[sz], &tmp[0], tmp.size());
				sz = data.size();
			}
		}
		void Media::ReadAllAudio(std::vector<short>& data) {
			//_Media & m = *(_Media*)Data;
			//auto sz = data.size();
			//std::vector<short> tmp;
			//while(ReadAudioStream(tmp)) {
			//	data.resize(sz + tmp.size());
			//	memcpy(&data[sz], &tmp[0], tmp.size() * sizeof(short));
			//	sz = data.size();
			//}

			_Media& m = *(_Media*)Data;
			size_t sz = 0;
			std::vector<float> tmp, Unormalized;
			while (ReadAudioStream(tmp)) {
				Unormalized.resize(sz + tmp.size());
				memcpy(&Unormalized[sz], &tmp[0], tmp.size() * sizeof(float));
				sz = Unormalized.size();
			}
			float mx = 0, abs;
			for (auto& v : Unormalized) {
				abs = fabsf(v);
				if (abs > mx) mx = abs;
			}
			mx = 0x7ff0 / mx;
			data.resize(Unormalized.size());
			for (auto i = 0; i < Unormalized.size(); i++) {
				data[i] = short(Unormalized[i] * mx);
			}
		}
		void Media::ReadAllAudio(std::vector<float>& data) {
			_Media& m = *(_Media*)Data;
			auto sz = data.size();
			std::vector<float> tmp;
			while (ReadAudioStream(tmp)) {
				data.resize(sz + tmp.size());
				memcpy(&data[sz], &tmp[0], tmp.size() * sizeof(float));
				sz = data.size();
			}
		}
		void Media::ReadAllAudio(std::vector<std::vector<float>>& data) {
			_Media& m = *(_Media*)Data;
			data.clear();
			data.resize(m.Audio->channels);
			std::vector<std::vector<float>> tmp;
			while (ReadAudioStream(tmp)) {
				for (int i = 0; i<tmp.size(); i++) {
					auto& Out = data[i];
					auto& In = tmp[i];

					auto sz = Out.size();
					Out.resize(sz + In.size());
					memcpy(&Out[sz], &In[0], In.size() * sizeof(float));
					sz = Out.size();
				}
			}
		}

		int Media::AudioSampleRate() {
			return (*(_Media*)Data).Audio->sample_rate;
		}
		int Media::AudioBitsPerSmple() {
			return (*(_Media*)Data).AudioBytesPerSample << 3;
		}
		int Media::AudioChannels() {
			return (*(_Media*)Data).Audio->channels;
		}

		Media LoadFile(const char * Filename) {
			Media ret;
			_Media & m = **(_Media**)&ret;


			void* opaque = nullptr;
			while(av_demuxer_iterate(&opaque));

			int res = avformat_open_input(&m.Context, Filename, NULL, NULL);
			avformat_find_stream_info(m.Context, 0);
			for(unsigned int i = 0; i < m.Context->nb_streams; i++) {
				auto stream = m.Context->streams[i];

				auto codec = avcodec_find_decoder(stream->codecpar->codec_id);
				if (codec->type == AVMediaType::AVMEDIA_TYPE_AUDIO && !m.Audio) {
					auto cc = avcodec_alloc_context3(codec);
					//cc->codec = codec;

					if (stream->codecpar->extradata_size) {
						cc->extradata = stream->codecpar->extradata;
						cc->extradata_size = stream->codecpar->extradata_size;
					}

					int rr = avcodec_open2(cc, codec, 0);
					m.AudioStreamIndex = i;
					m.Audio = cc;
					m.AudioFrame = av_frame_alloc();
					m.AudioBytesPerSample = av_get_bytes_per_sample(cc->sample_fmt);
				} else if (codec->type == AVMediaType::AVMEDIA_TYPE_VIDEO && !m.Video) {
					auto cc = avcodec_alloc_context3(codec);
					//cc->codec = codec;
					m.VideoStreamIndex = i;
					m.Video = cc;
					m.Width = cc->width;
					m.Height = cc->height;
					m.Fps = float(av_q2d(cc->framerate));
					int rr = avcodec_open2(cc, codec, 0);
					m.VideoFrame = av_frame_alloc();
				}


				//auto cc = stream->codec;
				//if(cc->codec_type == AVMediaType::AVMEDIA_TYPE_AUDIO && !m.Audio) {
				//	auto codec = avcodec_find_decoder(cc->codec_id);
				//	cc->codec = codec;
				//	int rr = avcodec_open2(cc, codec, 0);
				//	m.AudioStreamIndex = i;
				//	m.Audio = cc;
				//	m.AudioFrame = av_frame_alloc();
				//	m.AudioBytesPerSample = av_get_bytes_per_sample(cc->sample_fmt);
				//}else if (cc->codec_type == AVMediaType::AVMEDIA_TYPE_VIDEO && !m.Video) {
				//	auto codec = avcodec_find_decoder(cc->codec_id);
				//	cc->codec = codec;
				//	m.VideoStreamIndex = i;
				//	m.Video = cc;
				//	m.Width = cc->width;
				//	m.Height = cc->height;
				//	m.Fps = av_q2d(cc->framerate);
				//	int rr = avcodec_open2(cc, codec, 0);
				//	m.VideoFrame = av_frame_alloc();
				//}
			}
			//avformat_find_stream_info(m.Context, nullptr);
			//av_find_best_stream(m.Context, AVMediaType::AVMEDIA_TYPE_AUDIO, -1, -1, &m.Audio, 0);

			av_read_frame(m.Context, &m.ReadPacket);
			//while(!m.FillBuffers());
			
			//if (m.Video) {
			//	m.ScaleContext = sws_getCachedContext(0,
			//		m.Width, m.Height, *m.Video->codec->pix_fmts,
			//		m.Width, m.Height, AVPixelFormat::AV_PIX_FMT_RGB24,
			//		SWS_BICUBIC, nullptr, nullptr, nullptr);
			//}

			return ret;

			//AVInputFormat * fmt = 0;
			//for(;;) {
			//	fmt = av_iformat_next(fmt);
			//	if(!fmt) break;
			//	printf("%s\n", fmt->long_name);
			//} 
			//return ret;
		}
	}
}
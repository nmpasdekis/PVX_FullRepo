#include <PVX_ffMPEG.h>
#include <functional>
#include <mutex>
extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
}

namespace PVX {
	namespace AudioVideo {
		enum class AVErrors {
			OK = 0,
			BSF_NOT_FOUND = AVERROR_BSF_NOT_FOUND,
			BUG = AVERROR_BUG,
			BUFFER_TOO_SMALL = AVERROR_BUFFER_TOO_SMALL,
			DECODER_NOT_FOUND = AVERROR_DECODER_NOT_FOUND,
			DEMUXER_NOT_FOUND = AVERROR_DEMUXER_NOT_FOUND,
			ENCODER_NOT_FOUND = AVERROR_ENCODER_NOT_FOUND,
			EOFile = AVERROR_EOF,
			EXIT = AVERROR_EXIT,
			EXTERNAL = AVERROR_EXTERNAL,
			FILTER_NOT_FOUND = AVERROR_FILTER_NOT_FOUND,
			INVALIDDATA = AVERROR_INVALIDDATA,
			MUXER_NOT_FOUND = AVERROR_MUXER_NOT_FOUND,
			OPTION_NOT_FOUND = AVERROR_OPTION_NOT_FOUND,
			PATCHWELCOME = AVERROR_PATCHWELCOME,
			PROTOCOL_NOT_FOUND = AVERROR_PROTOCOL_NOT_FOUND,
			STREAM_NOT_FOUND = AVERROR_STREAM_NOT_FOUND,
			BUG2 = AVERROR_BUG2,
			UNKNOWN = AVERROR_UNKNOWN,
			EXPERIMENTAL = AVERROR_EXPERIMENTAL,
			INPUT_CHANGED = AVERROR_INPUT_CHANGED,
			OUTPUT_CHANGED = AVERROR_OUTPUT_CHANGED,
			HTTP_BAD_REQUEST = AVERROR_HTTP_BAD_REQUEST,
			HTTP_UNAUTHORIZED = AVERROR_HTTP_UNAUTHORIZED,
			HTTP_FORBIDDEN = AVERROR_HTTP_FORBIDDEN,
			HTTP_NOT_FOUND = AVERROR_HTTP_NOT_FOUND,
			HTTP_OTHER_4XX = AVERROR_HTTP_OTHER_4XX,
			HTTP_SERVER_ERROR = AVERROR_HTTP_SERVER_ERROR
		};


		typedef struct _Media {
			_Media() {
				RefCount = 1;
				Context = 0;
				Audio = 0;
				Video = 0;
				ScaleContext = 0;
				ReadPacket = *av_packet_alloc();
				//av_init_packet(&ReadPacket);
			}
			~_Media() {
				if(Audio) {
					av_frame_free(&AudioFrame);
					avcodec_close(Audio);
				}
				if (Video) {
					av_frame_free(&VideoFrame);
					avcodec_close(Video);
				}
				avformat_close_input(&Context);
			}

			int FillBuffers() {
				int err, err2 = 1;
				if (ReadPacket.stream_index == AudioStreamIndex) {
					err2 = 0;
					while ((err = avcodec_send_packet(Audio, &ReadPacket)) != -EAGAIN) {
						av_packet_unref(&ReadPacket);
						if ((err2 = av_read_frame(Context, &ReadPacket))) break;
					}
					curAudioTimestamp = ReadPacket.pts;
				} else if (ReadPacket.stream_index == VideoStreamIndex) {
					err2 = 0;
					while ((err = avcodec_send_packet(Video, &ReadPacket)) != -EAGAIN) {
						av_packet_unref(&ReadPacket);
						if ((err2 = av_read_frame(Context, &ReadPacket))) break;
					}
				}
				return err2;
			}
			void Seek(int64_t Frame) {
				std::lock_guard<std::mutex> lock{ Mutex };
				//if(!Frame)
				//	avio_seek(Context->pb, 0, SEEK_SET);
				av_seek_frame(Context, -1, Frame, 0);
			}
			std::mutex Mutex;
			std::vector<short> AudioBuffer;
			std::vector<unsigned char> VideoBuffer;

			int Read() {
				std::unique_lock<std::mutex> lock{ Mutex };
				AVErrors err = AVErrors::OK;
				if ((err = (AVErrors)av_read_frame(Context, &ReadPacket)) != AVErrors::OK)
					return (int)err;

				if (ReadPacket.stream_index == AudioStreamIndex) {
					curAudioTimestamp = ReadPacket.pts;
					if (AudioEvent != nullptr && !avcodec_send_packet(Audio, &ReadPacket)) {
						while (!ReadAudioFrame(AudioBuffer)) {
							lock.unlock();
							AudioEvent(AudioBuffer);
							lock.lock();
						}
					}
				} else if (ReadPacket.stream_index == VideoStreamIndex) {
					if (VideoEvent != nullptr && !avcodec_send_packet(Video, &ReadPacket)) {
						while (!ReadVideoFrame(VideoBuffer)) {
							lock.unlock();
							VideoEvent(VideoBuffer);
							lock.lock();
						}
					}
				}
				av_packet_unref(&ReadPacket);
				return 0;
			}

			//int Read() {
			//	auto err = av_read_frame(Context, &ReadPacket);
			//	if (ReadPacket.stream_index == AudioStreamIndex) {
			//		if (AudioEvent != nullptr && !avcodec_send_packet(Audio, &ReadPacket)) {
			//			while (!ReadAudioFrame(AudioBuffer))
			//				AudioEvent(AudioBuffer);
			//		}
			//	} else if (ReadPacket.stream_index == VideoStreamIndex) {
			//		if (VideoEvent != nullptr && !avcodec_send_packet(Video, &ReadPacket))
			//			while (!ReadVideoFrame(VideoBuffer)) {
			//				VideoEvent(VideoBuffer);
			//			}
			//	}
			//	av_packet_unref(&ReadPacket);
			//	return err;
			//}

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
			int32_t AudioChannels;
			int32_t AudioSampleRate;
			int64_t AudioDuration;
			int64_t curAudioTimestamp;
			double AudioDurationSec;

			int VideoStreamIndex;
			AVCodecContext * Video;
			AVFrame * VideoFrame, *DecodedVideoFrame;
			double VideoDuration;
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

		void Media::Seek(int64_t tm) {
			((_Media*)Data)->Seek(tm);
		}

		double Media::EstimatedDuration() {
			auto ctx = (*(_Media*)Data).Context;
			return (ctx->duration * 1.0) / AV_TIME_BASE;
		}

		double Media::CurrentTimeSecs() const {
			auto& m = *(_Media*)Data;
			return (m.AudioDurationSec * m.curAudioTimestamp) / m.AudioDuration;
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
				if(!(err = m.ReadAudioFrame(data))) 
					return 1;
				if(err == 11) 
					more = 0==m.FillBuffers();
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
					more = 0==m.FillBuffers();
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
					more = 0==m.FillBuffers();
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
				if (err == 11)
					more = 0==m.FillBuffers();
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
				if (err == 11)
					more = 0==m.FillBuffers();
			}
			return 0;
		}
		void Media::ReadAllAudio(std::vector<unsigned char>& data) {
			auto sz = data.size();
			std::vector<unsigned char> tmp;
			while (ReadAudioStream(tmp)) {
				data.resize(sz + tmp.size());
				memcpy(&data[sz], &tmp[0], tmp.size());
				sz = data.size();
			}
		}
		void Media::ReadAllAudio(std::vector<short>& data) {
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
			return (*(_Media*)Data).AudioSampleRate;
		}
		int Media::AudioBitsPerSample() {
			return ((*(_Media*)Data).AudioBytesPerSample << 3) /
				(*(_Media*)Data).AudioChannels;
		}
		int Media::AudioChannels() {
			return (*(_Media*)Data).AudioChannels;
		}

		Media LoadFile(const char * Filename) {
			Media ret;
			_Media & m = **(_Media**)&ret;

			avformat_open_input(&m.Context, Filename, NULL, NULL);
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

					avcodec_open2(cc, codec, 0);
					m.AudioStreamIndex = i;
					m.AudioDurationSec = stream->duration * (1.0 * stream->time_base.num) / stream->time_base.den;
					m.AudioDuration = stream->duration;
					m.AudioChannels = stream->codecpar->channels;
					m.AudioSampleRate = stream->codecpar->sample_rate;
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
					avcodec_open2(cc, codec, 0);
					m.VideoFrame = av_frame_alloc();
				}
			}
			av_read_frame(m.Context, &m.ReadPacket);
			//m.FillBuffers();

			//if (m.Video) {
			//	m.ScaleContext = sws_getCachedContext(0,
			//		m.Width, m.Height, *m.Video->codec->pix_fmts,
			//		m.Width, m.Height, AVPixelFormat::AV_PIX_FMT_RGB24,
			//		SWS_BICUBIC, nullptr, nullptr, nullptr);
			//}

			return ret;
		}
	}
}
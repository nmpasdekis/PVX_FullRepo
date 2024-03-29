#ifndef __PVX_OPEN_AL_H__
#define __PVX_OPEN_AL_H__

#include <PVX_Math3D.h>
#include <vector>
#include <queue>
#include <functional>
#include <string>
#include <PVX_AudioData.h>
#include <PVX.inl>

namespace PVX {
	namespace Audio {
		enum class AL_Enum {
			al_NONE = 0,
			al_FALSE = 0,
			al_TRUE = 1,
			al_SOURCE_RELATIVE = 0x202,
			al_CONE_INNER_ANGLE = 0x1001,
			al_CONE_OUTER_ANGLE = 0x1002,
			al_PITCH = 0x1003,
			al_POSITION = 0x1004,
			al_DIRECTION = 0x1005,
			al_VELOCITY = 0x1006,
			al_LOOPING = 0x1007,
			al_BUFFER = 0x1009,
			al_GAIN = 0x100A,
			al_MIN_GAIN = 0x100D,
			al_MAX_GAIN = 0x100E,
			al_ORIENTATION = 0x100F,
			al_CHANNEL_MASK = 0x3000,
			al_SOURCE_STATE = 0x1010,
			al_INITIAL = 0x1011,
			al_PLAYING = 0x1012,
			al_PAUSED = 0x1013,
			al_STOPPED = 0x1014,
			al_BUFFERS_QUEUED = 0x1015,
			al_BUFFERS_PROCESSED = 0x1016,
			al_SEC_OFFSET = 0x1024,
			al_SAMPLE_OFFSET = 0x1025,
			al_BYTE_OFFSET = 0x1026,
			al_SOURCE_TYPE = 0x1027,
			al_STATIC = 0x1028,
			al_STREAMING = 0x1029,
			al_UNDETERMINED = 0x1030,
			al_FORMAT_MONO8 = 0x1100,
			al_FORMAT_MONO16 = 0x1101,
			al_FORMAT_STEREO8 = 0x1102,
			al_FORMAT_STEREO16 = 0x1103,
			al_REFERENCE_DISTANCE = 0x1020,
			al_ROLLOFF_FACTOR = 0x1021,
			al_CONE_OUTER_GAIN = 0x1022,
			al_MAX_DISTANCE = 0x1023,
			al_FREQUENCY = 0x2001,
			al_BITS = 0x2002,
			al_CHANNELS = 0x2003,
			al_SIZE = 0x2004,
			al_UNUSED = 0x2010,
			al_PENDING = 0x2011,
			al_PROCESSED = 0x2012,
			al_NO_ERROR = 0,
			al_INVALID_NAME = 0xA001,
			al_ILLEGAL_ENUM = 0xA002,
			al_INVALID_ENUM = 0xA002,
			al_INVALID_VALUE = 0xA003,
			al_ILLEGAL_COMMAND = 0xA004,
			al_INVALID_OPERATION = 0xA004,
			al_OUT_OF_MEMORY = 0xA005,
			al_VENDOR = 0xB001,
			al_VERSION = 0xB002,
			al_RENDERER = 0xB003,
			al_EXTENSIONS = 0xB004,
			al_DOPPLER_FACTOR = 0xC000,
			al_DOPPLER_VELOCITY = 0xC001,
			al_SPEED_OF_SOUND = 0xC003,
			al_DISTANCE_MODEL = 0xD000,
			al_INVERSE_DISTANCE = 0xD001,
			al_INVERSE_DISTANCE_CLAMPED = 0xD002,
			al_LINEAR_DISTANCE = 0xD003,
			al_LINEAR_DISTANCE_CLAMPED = 0xD004,
			al_EXPONENT_DISTANCE = 0xD005,
			al_EXPONENT_DISTANCE_CLAMPED = 0xD006
		};

		class Buffer {
		public:
			Buffer();
			~Buffer();
			Buffer(int Channnels, int BitsPerSamples, int SampleRate, void * Data = 0, int SampleCount = 0);
			Buffer(const AudioData<unsigned char>& Data);
			Buffer(const AudioData<short>& Data);
			Buffer(const AudioData<float>& Data);
			Buffer(int SampleRate, const std::vector<short>& Samples);
			
			void SetData(const void * Data, int Size);
			
			template<typename T>
			void SetData(const std::vector<T>& Data) {
				SetData(Data.data(), Data.size() * sizeof(T));
			}
			void SetData(const AudioData<unsigned char>& Data);
			void SetData(const AudioData<short>& Data);
			void SetData(const AudioData<float>& Data);

			uint32_t Get() const;

			void Get(int Type, float & Value) const;
			void Get(int Type, int & Value) const;
			void Get(int Type, PVX::Vector3D & Value) const;
			void Get(int Type, float * Value) const;
			void Get(int Type, int * Value) const;

			void Set(int Type, const float & Value);
			void Set(int Type, const int & Value);
			void Set(int Type, const PVX::Vector3D & Value);
			void Set(int Type, const float * Value);
			void Set(int Type, const int * Value);
		private:
			int SampleRate;
			int BitsPerSamples;
			int Channels;
			int Size;
			int Format;
			unsigned int Id;
			PVX::RefCounter Ref;
			friend class Source;
		};

		class Source {
		public:
			Source();
			~Source();

			void SetPosition(const PVX::Vector3D & Value);
			void SetVelocity(const PVX::Vector3D & Value);
			void Move(const PVX::Vector3D & Value, float dt);
			void Play();
			void Stop();
			void Pause();
			void Loop(int Enable);
			void SetBuffer(const Buffer& buffer);
			void UnsetBuffer();
			void Volume(float Value);
			float Volume();
			void Pitch(float Value);
			float Pitch();
			int IsPlaying();

			void SetOffset(int Sample);
			int GetSampleOffset();

			void SetOffset(float Seconds);
			float GetSecOffset();

			uint32_t Get();

			void Get(int Type, float & Value);
			void Get(int Type, int & Value);
			void Get(int Type, PVX::Vector3D & Value);
			void Get(int Type, float * Value);
			void Get(int Type, int * Value);

			void Set(int Type, const float & Value);
			void Set(int Type, const int & Value);
			void Set(int Type, const PVX::Vector3D & Value);
			void Set(int Type, const float * Value);
			void Set(int Type, const int * Value);
		private:
			unsigned int Id;
			unsigned int Flags;
			PVX::Vector3D Position{}, Velocity{};
			float gain, pitch;
			PVX::RefCounter Ref;
		};

		class StreamOut {
		public:
			StreamOut(int Channels = 2, int BitsPerSample = 16, int SampleRate = 44100, const int bufferCount = 3);
			~StreamOut();
			StreamOut(const StreamOut&) = delete;
			StreamOut(StreamOut&&) = delete;
			void Stream(const void * Data, int Size);
			template<typename T>
			void Stream(const std::vector<T>& Data) {
				Stream(Data.data(), Data.size() * sizeof(T));
			}
			void StreamSamples(const void * Data, int Count);
			void Start();
			void Stop();
			void Flush();
			Source & Get();
			int AvailableBuffers();
			void SetProperties(int Channels, int BitsPerSample, int SampleRate);
			inline bool IsPlaying() { return Playing!=0; }
		private:
			int Channels;
			int BitsPerSample;
			int SampleRate;
			int Format;
			int availableBuffers;
			int Playing;
			Source Player;
			std::vector<uint32_t> Buffers;
			std::queue<uint32_t> BufferQueue;
		};

		typedef struct StereoSample32 {
			float Left, Right;
		} StereoSample32;

		typedef struct StereoSample16 {
			short Left, Right;
		} StereoSample;

		inline std::function<void(const unsigned char* Data, int Size)> CaptureMono16(std::function<void(const short* Samples, int SampleCount)> fnc) {
			return [fnc](const unsigned char* Data, int Size) {
				fnc((const short*)Data, Size >> 1);
			};
		}

		inline std::function<void(const unsigned char* Data, int Size)> CaptureStereo16(std::function<void(const StereoSample16* Samples, int SampleCount)> fnc) {
			return [fnc](const unsigned char* Data, int Size) {
				fnc((const StereoSample16*)Data, Size >> 2);
			};
		}

#define PVX_AUDIO_CAPTURE_STEPS 3

		class Engine {
		public:
			Engine(const char * Name = 0);
			~Engine();

			void InitCapture(int Channnels, int BitsPerSamples, int SampleRate, int BufferSizeInSamples = -5, const char * DeviceName = 0);
			void SetCapture(int WindowSizeInSamples, std::function<void(const unsigned char * Data, int ByteCount)> Function, int WindowStepInSamples = -1);
			
			void SetCapture(int WindowSizeInSamples, std::function<void(const short* Samples, int SampleCount)> fnc, int WindowStepInSamples = -1) {
				SetCapture(WindowSizeInSamples, CaptureMono16(fnc), WindowStepInSamples);
			}
			void SetCapture(int WindowSizeInSamples, std::function<void(const StereoSample16* Samples, int SampleCount)> fnc, int WindowStepInSamples = -1) {
				SetCapture(WindowSizeInSamples, CaptureStereo16(fnc), WindowStepInSamples);
			}
			
			void CaptureStart();
			void CaptureStop();
			void Capture();
			static std::vector<std::string> Devices();
			static std::vector<std::string> CaptureDevices();
		private:
			void * _Device;
			void * _Context;
			
			void * _CaptureDevice;
			void ReadCapturedSamplesBlock();
			int ReadCapturedSamples();
			int WindowSize;
			int WindowSizeBytes;
			int WindowStepBytes;
			int BytesPerSample;
			std::function<void(const unsigned char*Data, int ByteCount)> Function;
			std::vector<unsigned char> CapBuffer;
			unsigned char * CurCapBuffer[PVX_AUDIO_CAPTURE_STEPS];
			unsigned char * ReadBuffer;
			int WriteBufferSelector;
			int ReadBufferSelector;
			int CapBufferWritePos;
			int CapBufferReadPos;

			friend class Buffer;
			friend class Source;
		};

		template<typename T>
		class BufferedOutput {
			std::vector<T> Buffer;
			std::function<void(T*)> OnOutput;
			int Position;
			int OutSize;
		public:
			BufferedOutput(int OutSize, std::function<void(T*)> Output): 
				Buffer(OutSize * 2), OnOutput{ Output }, Position{ 0 }, OutSize{ OutSize } {}

			void Input(T * Data, int Count) {
				if(Buffer.size() < Position + Count)
					Buffer.resize(Position + Count * 2);
				memcpy(&Buffer[Position], Data, Count * sizeof(T));
				Position += Count;
				while(Position >= OutSize) {
					OnOutput(&Buffer[0]);
					memcpy(&Buffer[0], &Buffer[OutSize], (Position - OutSize) * sizeof(T));
					Position -= OutSize;
				}
			}
		};


		typedef std::vector<float> FloatVector;

		PVX::AudioData<unsigned char> LoadWaveFile(const char * Filename);
		std::vector<unsigned char> LoadWaveFile(const char * Filename, uint32_t& Channels, uint32_t BitsPerSample, uint32_t& SampleRate);
		//std::vector<Complex> dft(const float * Samples, int SampleCount);
		//std::vector<Complex> dft(const float * Samples, int SampleCount, int);
		//FloatVector idft(const Complex * Coefficient, int SampleCount);

		template<typename T>
		std::vector<T> LeftChannel(const std::vector<T> && samples) {
			std::vector<T> ret(samples.size() >> 1);
			const T * samps = &samples[0];
			for(T & s : ret) {
				s = *samps;
				samps += 2;
			}
			return ret;
		}
		template<typename T>
		std::vector<T> RightChannel(const std::vector<T> && samples) {
			std::vector<T> ret(samples.size() >> 1);
			const T * samps = &samples[1];
			for(T & s : ret) {
				s = *samps;
				samps += 2;
			}
			return ret;
		}


		FloatVector ToFloatSamples(int BitsPerSample, const void * Data, int Size);
		std::vector<StereoSample32> ToFloatSamplesStereo(int BitsPerSample, const void * Data, int Size);
		std::vector<unsigned char> SamplesToData(int BitsPerSample, const float * Data, int Count);


		void Sleep(int ms);

		std::vector<float> Add(const FloatVector & a, const FloatVector & b, int Offset = 0);

		typedef struct StereoAudioSamples {
			std::vector<float> Left, Right;
		} StereoAudioSamples;

		template<typename T>
		std::vector<T> InterleaveChannels(const std::vector<std::vector<T>> & ch) {
			auto cCount = ch.size();
			auto sCount = ch[0].size();
			std::vector<T> ret(cCount * sCount);
			for(auto i = 0; i < cCount; i++) {
				T * inp = &ch[i][0];
				T * o = &ret[i];
				for(auto j = 0; j < sCount; j++) {
					*o = inp[j];
					o++;
				}
			}
			return ret;
		}

	}
}

#endif
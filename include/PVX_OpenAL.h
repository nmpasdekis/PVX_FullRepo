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
			NONE = 0,
			FALSE = 0,
			TRUE = 1,
			SOURCE_RELATIVE = 0x202,
			CONE_INNER_ANGLE = 0x1001,
			CONE_OUTER_ANGLE = 0x1002,
			PITCH = 0x1003,
			POSITION = 0x1004,
			DIRECTION = 0x1005,
			VELOCITY = 0x1006,
			LOOPING = 0x1007,
			BUFFER = 0x1009,
			GAIN = 0x100A,
			MIN_GAIN = 0x100D,
			MAX_GAIN = 0x100E,
			ORIENTATION = 0x100F,
			CHANNEL_MASK = 0x3000,
			SOURCE_STATE = 0x1010,
			INITIAL = 0x1011,
			PLAYING = 0x1012,
			PAUSED = 0x1013,
			STOPPED = 0x1014,
			BUFFERS_QUEUED = 0x1015,
			BUFFERS_PROCESSED = 0x1016,
			SEC_OFFSET = 0x1024,
			SAMPLE_OFFSET = 0x1025,
			BYTE_OFFSET = 0x1026,
			SOURCE_TYPE = 0x1027,
			STATIC = 0x1028,
			STREAMING = 0x1029,
			UNDETERMINED = 0x1030,
			FORMAT_MONO8 = 0x1100,
			FORMAT_MONO16 = 0x1101,
			FORMAT_STEREO8 = 0x1102,
			FORMAT_STEREO16 = 0x1103,
			REFERENCE_DISTANCE = 0x1020,
			ROLLOFF_FACTOR = 0x1021,
			CONE_OUTER_GAIN = 0x1022,
			MAX_DISTANCE = 0x1023,
			FREQUENCY = 0x2001,
			BITS = 0x2002,
			CHANNELS = 0x2003,
			SIZE = 0x2004,
			UNUSED = 0x2010,
			PENDING = 0x2011,
			PROCESSED = 0x2012,
			NO_ERROR = 0,
			INVALID_NAME = 0xA001,
			ILLEGAL_ENUM = 0xA002,
			INVALID_ENUM = 0xA002,
			INVALID_VALUE = 0xA003,
			ILLEGAL_COMMAND = 0xA004,
			INVALID_OPERATION = 0xA004,
			OUT_OF_MEMORY = 0xA005,
			VENDOR = 0xB001,
			VERSION = 0xB002,
			RENDERER = 0xB003,
			EXTENSIONS = 0xB004,
			DOPPLER_FACTOR = 0xC000,
			DOPPLER_VELOCITY = 0xC001,
			SPEED_OF_SOUND = 0xC003,
			DISTANCE_MODEL = 0xD000,
			INVERSE_DISTANCE = 0xD001,
			INVERSE_DISTANCE_CLAMPED = 0xD002,
			LINEAR_DISTANCE = 0xD003,
			LINEAR_DISTANCE_CLAMPED = 0xD004,
			EXPONENT_DISTANCE = 0xD005,
			EXPONENT_DISTANCE_CLAMPED = 0xD006
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
			PVX::Vector3D Position, Velocity;
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
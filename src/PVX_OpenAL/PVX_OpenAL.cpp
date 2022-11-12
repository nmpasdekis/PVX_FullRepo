#include <PVX_OpenAL.h>
#include <AL/alc.h>
#include <AL/al.h>
#include <AL/alext.h>
#include <stdio.h>
#include <thread>
#include <map>

#define FLAGS(Set, Reset) Flags = ((Flags& (~(Reset))) | (Set))
#define PLAYING		1
#define PAUSED		2
#define LOOPING		4


namespace PVX {
	namespace Audio {
		Engine * Current;

		Engine::Engine(const char * Name) {
			_Device = (void*)alcOpenDevice(Name);
			_Context = (void*)alcCreateContext((ALCdevice*)_Device, 0);
			alcMakeContextCurrent((ALCcontext*)_Context);
			_CaptureDevice = NULL;
			Current = this;
		}
		Engine::~Engine() {
			alcMakeContextCurrent(0);
			alcDestroyContext((ALCcontext*)_Context);
			alcCloseDevice((ALCdevice*)_Device);
		}
		Source::Source() {
			gain = 1.0f;
			pitch = 1.0f;
			alGenSources(1, &Id);
		}
		Source::~Source() {
			if(Id&&!Ref)
				alDeleteSources(1, &Id);
		}
		void Source::SetPosition(const PVX::Vector3D & Value) {
			Position = Value;
			alSourcefv(Id, AL_POSITION, Position.Array);
		}
		void Source::SetVelocity(const PVX::Vector3D & Value) {
			Velocity = Value;
			alSourcefv(Id, AL_VELOCITY, Velocity.Array);
		}
		void Source::Move(const PVX::Vector3D & Value, float dt) {
			dt = 1.0f / dt;
			Velocity.x = (Value.x - Position.x) * dt;
			Velocity.y = (Value.y - Position.y) * dt;
			Velocity.z = (Value.z - Position.z) * dt;
			Position = Value;

			alSourcefv(Id, AL_POSITION, Position.Array);
			alSourcefv(Id, AL_VELOCITY, Velocity.Array);
		}

		void Source::Play() {
			int p;
			alGetSourcei(Id, AL_SOURCE_STATE, &p);
			if(p!=AL_PLAYING)
				alSourcePlay(Id);
			FLAGS(PLAYING, PAUSED);
		}
		void Source::Stop() {
			int p;
			alGetSourcei(Id, AL_SOURCE_STATE, &p);
			if(p != AL_STOPPED)
				alSourcePlay(Id);
			alSourceStop(Id);
			FLAGS(0, PLAYING | PAUSED);
		}
		void Source::Pause() {
			if(Flags&PAUSED) {
				alSourcePlay(Id);
				FLAGS(PLAYING, PAUSED);
			} else {
				alSourcePause(Id);
				FLAGS(PAUSED, 0);
			}
		}
		void Source::Loop(int Enable) {
			Enable = !!Enable;
			alSourcei(Id, AL_LOOPING, Enable);
			FLAGS(LOOPING&-Enable, LOOPING);
		}
		void Source::SetBuffer(const Buffer & buffer) {
			alSourcei(Id, AL_BUFFER, buffer.Id);
		}

		void Source::UnsetBuffer() {
			alSourcei(Id, AL_BUFFER, 0);
		}

		void Source::Volume(float Value) {
			gain = Value;
			alSourcef(Id, AL_GAIN, Value);
		}

		float Source::Volume() {
			return gain;
		}

		void Source::Pitch(float Value) {
			pitch = Value;
			alSourcef(Id, AL_PITCH, Value);
		}

		float Source::Pitch() {
			return pitch;
		}

		int Source::IsPlaying() {
			int p;
			alGetSourcei(Id, AL_SOURCE_STATE, &p);
			return (p == AL_PLAYING);
		}

		//template<AL_Enum Function, typename RetType>
		//auto Get(int Id) {
		//	switch constexpr(Function) {
		//		case AL_Enum::AL_SEC_OFFSET: {
		//			float ret;
		//			alGetSourcef(Id, (int)Function, &ret);
		//			return ret;
		//		}
		//		case AL_Enum::AL_SAMPLE_OFFSET: {
		//			int ret;
		//			alGetSourcef(Id, (int)Function, &ret);
		//			return ret;
		//		}
		//	}
		//}

		void Source::SetOffset(int pos) {
			alSourcei(Id, AL_SAMPLE_OFFSET, pos);
		}
		int Source::GetSampleOffset() {
			int pos;
			alGetSourcei(Id, AL_SAMPLE_OFFSET, &pos);
			return pos;
		}

		void Source::SetOffset(float pos) {
			alSourcef(Id, AL_SEC_OFFSET, pos);
		}
		float Source::GetSecOffset() {
			float pos;
			alGetSourcef(Id, AL_SEC_OFFSET, &pos);
			return pos;
		}

		uint32_t Source::Get() { return Id; }

		void Source::Get(int Type, int & Value) { alGetSourcei(Id, Type, &Value); }
		void Source::Get(int Type, float & Value) { alGetSourcef(Id, Type, &Value); }
		void Source::Get(int Type, PVX::Vector3D & Value) { alGetSource3f(Id, Type, &Value.x, &Value.y, &Value.z); }
		void Source::Get(int Type, float * Value) { alGetSourcefv(Id, Type, Value); }
		void Source::Get(int Type, int * Value) { alGetSourceiv(Id, Type, Value); }

		void Source::Set(int Type, const float & Value) { alSourcef(Id, Type, Value); }
		void Source::Set(int Type, const int & Value) { alSourcei(Id, Type, Value); }
		void Source::Set(int Type, const PVX::Vector3D & Value) { alSource3f(Id, Type, Value.x, Value.y, Value.z); }
		void Source::Set(int Type, const float * Value) { alSourcefv(Id, Type, Value); }
		void Source::Set(int Type, const int * Value) { alSourceiv(Id, Type, Value); }

		constexpr int SetFormat(int Channels, int BitsPerSamples) {
			if(Channels == 1) {
				if (BitsPerSamples == 8) return AL_FORMAT_MONO8;
				else if (BitsPerSamples == 16) return AL_FORMAT_MONO16;
				else {
					return AL_FORMAT_MONO_FLOAT32;
				}
			} else {
				if(BitsPerSamples == 8) return AL_FORMAT_STEREO8;
				else if(BitsPerSamples == 16) return AL_FORMAT_STEREO16;
				else {
					return AL_FORMAT_STEREO_FLOAT32;
				}
			}
		}
		Buffer::Buffer():
			SampleRate{ 0 },
			BitsPerSamples{ 0 },
			Channels{ 0 },
			Size{ 0 },
			Format{ 0 } {
			alGenBuffers(1, &Id);
		}

		Buffer::Buffer(int Channels, int BitsPerSamples, int SampleRate, void * Data, int Size):
			SampleRate{ SampleRate },
			BitsPerSamples{ BitsPerSamples },
			Channels{ Channels },
			Size{ Size },
			Format{ SetFormat(Channels, BitsPerSamples) } {
			alGenBuffers(1, &Id);
			if(Data)
				SetData(Data, Size);
		}
		Buffer::Buffer(const AudioData<unsigned char>& Data) {
			alGenBuffers(1, &Id);
			SetData(Data);
		}
		Buffer::Buffer(const AudioData<short>& Data) {
			alGenBuffers(1, &Id);
			SetData(Data);
		}
		Buffer::Buffer(const AudioData<float>& Data) {
			alGenBuffers(1, &Id);
			SetData(Data);
		}
		Buffer::Buffer(int SampleRate, const std::vector<short>& Samples):
			SampleRate{ SampleRate },
			BitsPerSamples{ 16 },
			Channels{ 2 },
			Format{ SetFormat(2, 16) } {
			alGenBuffers(1, &Id);
			SetData(Samples);
		}
		Buffer::~Buffer() {
			if(Id&&!Ref)
				alDeleteBuffers(1, &Id);
		}
		void Buffer::SetData(const void * Data, int Size) {
			this->Size = Size;
			alBufferData(Id, Format, Data, Size, SampleRate);
		}

		void Buffer::SetData(const AudioData<unsigned char>& Data) {
			SampleRate = Data.SampleRate;
			BitsPerSamples = Data.BitsPerSample;
			Channels = Data.Channels;
			Format = SetFormat(Channels, BitsPerSamples);
			SetData(Data.Data);
		}
		void Buffer::SetData(const AudioData<short>& Data) {
			SampleRate = Data.SampleRate;
			BitsPerSamples = Data.BitsPerSample;
			Channels = Data.Channels;
			Format = SetFormat(Channels, BitsPerSamples);
			SetData(Data.Data);
		}
		void Buffer::SetData(const AudioData<float>& Data) {
			SampleRate = Data.SampleRate;
			BitsPerSamples = Data.BitsPerSample;
			Channels = Data.Channels;
			Format = SetFormat(Channels, BitsPerSamples);
			SetData(Data.Data);
		}

		uint32_t Buffer::Get() const { return Id; }

		void Buffer::Get(int Type, float & Value) const { alGetBufferf(Id, Type, &Value); }
		void Buffer::Get(int Type, int & Value) const { alGetBufferi(Id, Type, &Value); }
		void Buffer::Get(int Type, PVX::Vector3D & Value) const { alGetBuffer3f(Id, Type, &Value.x, &Value.y, &Value.z); }
		void Buffer::Get(int Type, float * Value) const { alGetBufferfv(Id, Type, Value); }
		void Buffer::Get(int Type, int * Value) const { alGetBufferiv(Id, Type, Value); }

		void Buffer::Set(int Type, const float & Value) { alBufferf(Id, Type, Value); }
		void Buffer::Set(int Type, const int & Value) { alBufferi(Id, Type, Value); }
		void Buffer::Set(int Type, const PVX::Vector3D & Value) { alBuffer3f(Id, Type, Value.x, Value.y, Value.z); }
		void Buffer::Set(int Type, const float * Value) { alBufferfv(Id, Type, Value); }
		void Buffer::Set(int Type, const int * Value) { alBufferiv(Id, Type, Value); }


		StreamOut::StreamOut(int Channels, int BitsPerSample, int SampleRate, const int bufferCount):
			Channels{ Channels },
			BitsPerSample{ BitsPerSample },
			SampleRate{ SampleRate },
			Format{ SetFormat(Channels, BitsPerSample) },
			Buffers{ std::vector<uint32_t>(bufferCount) } {

			alGenBuffers(bufferCount, Buffers.data());
			for(int i = 0; i < bufferCount; i++)
				BufferQueue.push(Buffers[i]);
			availableBuffers = bufferCount;
			Playing = 0;
		}
		void StreamOut::SetProperties(int Channels, int BitsPerSample, int SampleRate) {
			this->Channels = Channels;
			this->BitsPerSample = BitsPerSample;
			this->SampleRate = SampleRate;
			Format = SetFormat(Channels, BitsPerSample);
		}

		StreamOut::~StreamOut() {
			int n = 0, id = Player.Get();
			alSourceStop(id);

			alGetSourcei(id, AL_BUFFERS_QUEUED, &n);
			if(n) {
				alSourceUnqueueBuffers(id, n, Buffers.data());
				for(int i = 0; i < n; i++)
					BufferQueue.push(Buffers[i]);
			}
			while(BufferQueue.size()) {
				uint32_t b = BufferQueue.front(); BufferQueue.pop();
				alDeleteBuffers(1, &b);
			}
		}

		int StreamOut::AvailableBuffers() {
			uint32_t id = Player.Get();
			int more = 0;
			alGetSourcei(id, AL_BUFFERS_PROCESSED, &more);
			if(more) {
				alSourceUnqueueBuffers(id, more, Buffers.data());
				for(int i = 0; i < more; i++)
					BufferQueue.push(Buffers[i]);
				availableBuffers += more;
			}
			return availableBuffers;
		}

		void Engine::InitCapture(int Channnels, int BitsPerSamples, int SampleRate, int BufferSizeInSamples, const char * DeviceName) {
			if(BufferSizeInSamples < 0)
				BufferSizeInSamples *= -SampleRate;
			BytesPerSample = (BitsPerSamples >> 3) * Channnels;
			_CaptureDevice = (void*)alcCaptureOpenDevice(DeviceName, SampleRate, SetFormat(Channnels, BitsPerSamples), BytesPerSample * BufferSizeInSamples);
		}

		void Engine::SetCapture(int ws, std::function<void(const unsigned char*Data, int Size)> Function, int wss) {
			if(wss <= 0)wss = ws;
			WindowSize = ws;
			WindowSizeBytes = (ws * BytesPerSample);
			WindowStepBytes = (wss * BytesPerSample);
			this->Function = Function;
			CapBuffer.resize(WindowSizeBytes * (PVX_AUDIO_CAPTURE_STEPS + 1));
			for(int i = 0; i < PVX_AUDIO_CAPTURE_STEPS; i++)
				CurCapBuffer[i] = &CapBuffer[i * WindowSizeBytes];
			ReadBuffer = &CapBuffer[PVX_AUDIO_CAPTURE_STEPS * WindowSizeBytes];
			CapBufferWritePos = 0;
			CapBufferReadPos = 0;
			WriteBufferSelector = 1;
			ReadBufferSelector = 0;
		}

		void Engine::CaptureStart() {
			alcCaptureStart((ALCdevice*)_CaptureDevice);
			ReadCapturedSamplesBlock();
			//ReadCapturedSamplesBlock();
		}
		void Engine::CaptureStop() {
			alcCaptureStop((ALCdevice*)_CaptureDevice);
		}
		void Engine::Capture() {
			int ReadBufferSelector2;
			if(CapBufferReadPos) {
				ReadBufferSelector2 = (ReadBufferSelector + 1) % PVX_AUDIO_CAPTURE_STEPS;
				if(ReadBufferSelector != WriteBufferSelector && ReadBufferSelector2 != WriteBufferSelector)
					ReadCapturedSamples();
				
				int FirstBufferSize = WindowSizeBytes - CapBufferReadPos;
				memcpy(ReadBuffer, CurCapBuffer[ReadBufferSelector] + CapBufferReadPos, FirstBufferSize);
				memcpy(ReadBuffer + FirstBufferSize, CurCapBuffer[ReadBufferSelector2], CapBufferReadPos);
			} else {
				if(ReadBufferSelector != WriteBufferSelector)
					ReadCapturedSamples();

				memcpy(ReadBuffer, CurCapBuffer[ReadBufferSelector], WindowSizeBytes);
			}

			CapBufferReadPos += WindowStepBytes;
			if(CapBufferReadPos >= WindowSizeBytes) {
				CapBufferReadPos -= WindowSizeBytes;
				ReadBufferSelector = (ReadBufferSelector + 1) % PVX_AUDIO_CAPTURE_STEPS;
			}
			Function(ReadBuffer, WindowSizeBytes);
			
			ReadBufferSelector2 = (ReadBufferSelector + 1) % PVX_AUDIO_CAPTURE_STEPS;
			

			if(ReadBufferSelector2 == WriteBufferSelector) {
				ReadCapturedSamplesBlock();
			}
		}

		std::vector<std::string> Engine::Devices() {
			char * tmp = (char*)alcGetString(NULL, ALC_ALL_DEVICES_SPECIFIER);
			std::vector<std::string> ret;
			while(tmp[0]) {
				ret.push_back(tmp);
				tmp += strlen(tmp) + 1;
			}
			return ret;
		}

		std::vector<std::string> Engine::CaptureDevices() {
			char * tmp = (char*)alcGetString(NULL, ALC_CAPTURE_DEVICE_SPECIFIER);
			std::vector<std::string> ret;
			while(tmp[0]) {
				ret.push_back(tmp);
				tmp += strlen(tmp) + 1;
			}
			return ret;
		}

		void Engine::ReadCapturedSamplesBlock() {
			int samples = 0;
			while(samples < WindowSize) {
				alcGetIntegerv((ALCdevice*)_CaptureDevice, ALC_CAPTURE_SAMPLES, 1, &samples);
				std::this_thread::sleep_for(std::chrono::microseconds(1));
			}
			alcCaptureSamples((ALCdevice*)_CaptureDevice, CurCapBuffer[WriteBufferSelector], WindowSize);
			WriteBufferSelector = (WriteBufferSelector + 1) % PVX_AUDIO_CAPTURE_STEPS;
		}
		int Engine::ReadCapturedSamples() {
			int samples = 0;
			alcGetIntegerv((ALCdevice*)_CaptureDevice, ALC_CAPTURE_SAMPLES, 1, &samples);
			if(samples >= WindowSize) {
				alcCaptureSamples((ALCdevice*)_CaptureDevice, CurCapBuffer[WriteBufferSelector], WindowSize);
				WriteBufferSelector = (WriteBufferSelector + 1) % PVX_AUDIO_CAPTURE_STEPS;
				return 1;
			}
			return 0;
		}

		void Sleep(int ms) {
			std::this_thread::sleep_for(std::chrono::milliseconds(ms));
		}
		void StreamOut::Stream(const void * Data, int Size) {
			int more = 0;
			//uint32_t bufs[10];
			uint32_t id = Player.Get();
			if(!availableBuffers) {
				while(!more) {
					std::this_thread::sleep_for(std::chrono::milliseconds(1));
					alGetSourcei(id, AL_BUFFERS_PROCESSED, &more);
				}
				alSourceUnqueueBuffers(id, more, Buffers.data());
				for(int i = 0; i < more; i++)
					BufferQueue.push(Buffers[i]);
				availableBuffers += more;
			}
			uint32_t buf = BufferQueue.front(); BufferQueue.pop();
			availableBuffers--;
			alBufferData(buf, Format, Data, Size, SampleRate);
			alSourceQueueBuffers(id, 1, &buf);
			if(Playing) { // &&availableBuffers == 9
				Player.Play();
			}
		}
		void StreamOut::Flush() {
			Player.Play();
			int more = 0;
			uint32_t id = Player.Get();
			while(availableBuffers < Buffers.size()) {
				while(!more) {
					alGetSourcei(id, AL_BUFFERS_PROCESSED, &more);
					std::this_thread::sleep_for(std::chrono::microseconds(1));
				}
				alSourceUnqueueBuffers(id, more, Buffers.data());
				for(int i = 0; i < more; i++)
					BufferQueue.push(Buffers[i]);
				availableBuffers += more;
			}
		}
		void StreamOut::StreamSamples(const void * Data, int Count) {
			Stream(Data, Count * (BitsPerSample >> 3));
		}
		void StreamOut::Start() {
			Playing = 1;
			Player.Play();
		}
		void StreamOut::Stop() {
			Playing = 0;
			Player.Stop();
		}
		Source & StreamOut::Get() {
			return Player;
		}

		//static std::vector<std::vector<Complex>> Base;
		
		//static void MakeBase(int SampleCount) {
		//	if(LastSampleCount == SampleCount) return;
		//	Base.resize(SampleCount);
		//	for(int j = 0; j < SampleCount; j++) {
		//		Base[j].resize(SampleCount);
		//		Complex * Tone = &Base[j][0];
		//		memset(Tone, 0, sizeof(Complex) * SampleCount);
		//		double mul = (2.0 * PVX_PI * j) / SampleCount;
		//		for(int i = 0; i < SampleCount; i++) {
		//			Tone[i].Real = cos(mul * i);
		//			Tone[i].Imaginary = sin(mul * i);
		//		}
		//	}
		//	LastSampleCount == SampleCount;
		//}

		std::vector<float> ToFloatSamples(int BitsPerSample, const void * Data, int Size) {
			if(BitsPerSample == 16) {
				std::vector<float> ret(Size >> 1);
				short * Buffer = (short*)Data;
				float * r = &ret[0];
				float inv = 1.0f / 0x8000;
				for(int i = 0; i < ret.size(); i++) {
					r[i] = Buffer[i] * inv;
				}
				return ret;
			} else {
				std::vector<float> ret(Size);
				unsigned char * Buffer = (unsigned char*)Data;
				float * r = &ret[0];
				float inv = 1.0f / 127.5f;
				for(int i = 0; i < ret.size(); i++) {
					r[i] = (Buffer[i] - 127.5) * inv;
				}
				return ret;
			}
		}
		std::vector<StereoSample32> ToFloatSamplesStereo(int BitsPerSample, const void * Data, int Size) {
			if(BitsPerSample == 16) {
				std::vector<StereoSample32> ret(Size >> 2);
				short * Buffer = (short*)Data;
				float * r = &ret[0].Left;
				float inv = 1.0f / 0x8000;
				for(int i = 0; i < ret.size(); i++) {
					r[i] = Buffer[i] * inv;
				}
				return ret;
			} else {
				std::vector<StereoSample32> ret(Size >> 1);
				unsigned char * Buffer = (unsigned char*)Data;
				float * r = &ret[0].Left;
				float inv = 1.0f / 127.5f;
				for(int i = 0; i < ret.size(); i++) {
					r[i] = (Buffer[i] - 127.5) * inv;
				}
				return ret;
			}
		}
		std::vector<unsigned char> SamplesToData(int BitsPerSample, const float * Data, int Count) {
			if(BitsPerSample == 16) {
				std::vector<unsigned char> ret(Count * 2);
				short* r = (short*)&ret[0];
				for(int i = 0; i < Count; i++)
					r[i] = Data[i] * 0x7fff;
				return ret;
			} else {
				std::vector<unsigned char> ret(Count);
				unsigned char* r = &ret[0];
				for(int i = 0; i < Count; i++)
					r[i] = Data[i] * 127.5 + 127.5;
				return ret;
			}
		}

		StereoAudioSamples SplitChannels(const StereoSample * Data, int Count) {
			StereoAudioSamples ret;
			ret.Left.resize(Count);
			ret.Right.resize(Count);

			for(auto i = 0; i < Count; i++) {
				ret.Left[i] = Data[i].Left;
				ret.Right[i] = Data[i].Right;
			}
			return ret;
		}
		std::vector<StereoSample> CombineChannels(float * left, float * right, int Count) {
			std::vector<StereoSample> ret(Count);
			for(int i = 0; i < Count; i++) {
				ret[i].Left = left[i];
				ret[i].Right = right[i];
			}
			return ret;
		}


		std::vector<float> Add(const FloatVector & a, const FloatVector & b, int Offset) {
			FloatVector ret;
			return ret;
		}

		FloatVector Autocorrelation(float * a, int Count) {
			std::vector<float> ret(Count - 1);
			memset(&ret[0], 0, sizeof(float) * ret.size());
			for(auto i = 1; i < Count; i++) {
				float & x = ret[i - 1];
				for(int j = i; j < Count; j++)
					x += a[j] * a[j - i];
				x /= (Count - i);
			}
			return ret;
		}
		std::vector<unsigned char> LoadWaveFile(const char * Filename, uint32_t & Channels, uint32_t BitsPerSample, uint32_t & SampleRate) {
			std::vector<unsigned char> ret;
			union Endian {
				unsigned int uValue;
				float fValue;
				struct {
					unsigned char a, b, c, d;
				};
				char cValue[4];
			};
			struct WaveHeader {
				Endian	RIFF;
				uint32_t	FileSize;
				Endian	WAVE;
				Endian	Format;
				uint32_t	size;
				uint16_t	TAG;
				uint16_t	Channels;
				uint32_t	SampleRate;
				uint32_t	AverageByteRate;
				uint16_t	BlockAlign;
				uint16_t	BitsPerSample;
			} Header;

			Endian tmp1;

			FILE * fin;
			uint32_t DataSize;
			if(fopen_s(&fin, Filename, "rb")||!fin) return ret;
			if(fread_s(&Header, sizeof(WaveHeader), sizeof(WaveHeader), 1, fin)) {
				fseek(fin, Header.size - 16, SEEK_CUR);
				fread_s(&tmp1, sizeof(Endian), sizeof(Endian), 1, fin);
				fread_s(&DataSize, sizeof(uint32_t), sizeof(uint32_t), 1, fin);
				ret.resize(DataSize);
				fread_s(&ret[0], DataSize, 1, DataSize, fin);

				BitsPerSample = Header.BitsPerSample;
				Channels = Header.Channels;
				SampleRate = Header.SampleRate;
			}

			fclose(fin);
			return ret;
		}

		AudioData<unsigned char> LoadWaveFile(const char * Filename) {
			AudioData<unsigned char> ret{ 0 };
			ret.Data = LoadWaveFile(Filename, ret.Channels, ret.BitsPerSample, ret.SampleRate);
			return ret;
		}
	}
}
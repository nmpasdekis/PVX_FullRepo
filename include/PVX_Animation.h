#pragma once

#include<initializer_list>
#include<vector>
#include<string>
#include<PVX_Math3D.h>

namespace PVX {
	namespace Animation {

		template<typename T>
		struct MappingRange {
			int Next;
			float Value{};
		};

		template<typename T>
		struct MappingState {
			MappingState() = default;
			T Bias = {}, Linear = {}, Quadratic = {};
			float scrBias = {};
			int RangeCount;
			MappingRange<T> Ranges[3];
			MappingState(const T& bias, const T& linear, const T& quadratic, float src_bias, int UnderState, float min, float max, int OverState) :
				Bias{ bias }, Linear{ linear }, Quadratic{ quadratic }, scrBias{ src_bias } {
				RangeCount = 2;
				Ranges[0].Next = UnderState;
				Ranges[0].Value = min;
				Ranges[1].Next = -1;
				Ranges[1].Value = max;
				Ranges[2].Next = OverState;
			}
			MappingState(const T& bias, const T& linear, const T& quadratic, float src_bias, int UnderState, float value, int OverState) :
				Bias{ bias }, Linear{ linear }, Quadratic{ quadratic }, scrBias{ src_bias } {
				RangeCount = 1;
				Ranges[0].Next = UnderState;
				Ranges[0].Value = value;
				Ranges[1].Next = OverState;
			}
		};

		template<typename T>
		class Mapper {
		public:
			Mapper(float& Src, T& Dst, const std::initializer_list<MappingState<T>>& Mappers) : src{ Src }, dst{ Dst } {
				int i = 0;
				for (auto& m : Mappers) {
					States.push_back(m);
					auto& x = States.back();
					for (int j = 0; j <= x.RangeCount; j++)
						if (x.Ranges[j].Next == -1)
							x.Ranges[j].Next = i;
					i++;
				}
				Current = &States.front();
			}
			void Update() {
				int i;
				MappingState<T>* last = 0;
				do {
					last = Current;
					for (i = 0; i<Current->RangeCount && src > Current->Ranges[i].Value; i++);
					Current = &States[Current->Ranges[i].Next];
				} while (last != Current);
				MappingState<T>& s = *Current;
				float x = src + Current->scrBias;
				dst = s.Bias + x * s.Linear + x * x * s.Quadratic;
			}
		private:
			MappingState<T>* Current;
			float& src;
			T& dst;
			std::vector<MappingState<T>> States;
		};

		enum class AnimationFlags {
			Step = 0,
			Linear = 1,
			Smooth = 2,
			Mask = 3,
			Loop = 4,
			HasSpeed = 8,
			HasTrasition = 16,
			Dirty = 0x80000000
		};

		template<typename T>
		struct Frame {
			float Time;
			T Value;
			T Speed;
			T HalfAcceleration;
		};

		template<typename T>
		struct Key {
			Key() = default;
			Key(float Time, const T& Value, AnimationFlags Type = AnimationFlags::Smooth) : Time{ Time }, Value{ Value }, Speed{ Speed }, Flags{ (unsigned int)AnimationFlags::Smooth } {}
			Key(float Time, const T& Value, const T& Speed, AnimationFlags Type = AnimationFlags::Smooth) : Time{ Time }, Value{ Value }, Speed{ Speed }, Flags{ (unsigned int)AnimationFlags::Smooth | (unsigned int)AnimationFlags::HasSpeed } {}
			Key(float Time, const T& Value, const T& Speed, float Transition, AnimationFlags Type = AnimationFlags::Smooth) : Time{ Time }, Value{ Value }, Speed{ Speed }, Transition{ Transition }, Flags{ (unsigned int)AnimationFlags::Smooth | (unsigned int)AnimationFlags::HasSpeed | (unsigned int)AnimationFlags::HasTrasition } {}
			float Time{};
			T Value{};
			T Speed{};
			float Transition{ 0.5f };
			unsigned int Flags{ (unsigned int)AnimationFlags::Smooth };
		};

		template<typename T>
		class Animator {
		protected:
			std::vector<Frame<T>> Frames;
			float Dur{};
		public:
			T GetValue(float Time) {
				int min = 0;
				int max = Frames.size() - 1;

				while (min < max) {
					int mid = (max + min) >> 1;
					auto& m = Frames[mid];
					if (Time < m.Time) {
						max = mid;
						continue;
					}
					if (Time < Frames[mid + 1].Time) {
						float t = Time - m.Time;
						return m.Value + t * (m.Speed + m.HalfAcceleration * t);
					}
					min = mid + 1;
				}
				auto& m = Frames[max];
				float t = Time - m.Time;
				return m.Value + t * (m.Speed + m.HalfAcceleration * t);
			}
			T GetValueRepeat(float Time) {
				float d = Duration();
				return GetValue(Time - d * floor(Time / d));
			}
			float& Duration() { return Dur; };
		};

		template<typename T>
		class Maker : public Animator<T> {
		public:
			Maker() = default;
			Maker(const std::initializer_list<Key<T>>& K, float RepeatAfter = -1) {
				Flags = (unsigned int)AnimationFlags::Dirty;
				for (auto& k : K) {
					Keys.push_back(k);
				}
				Animator<T>::Duration() = Keys.back().Time;
				if (RepeatAfter > 0) {
					Flags |= (unsigned int)AnimationFlags::Loop;
					Animator<T>::Duration() += RepeatAfter;
				}
				Reconstruct();
			}
			int Insert(const Key<T>& k) {
				Flags |= AnimationFlags::Dirty;

				if (!Keys.size()) {
					Animator<T>::Duration() += k.Time;
					Keys.push_back(k);
					return 0;
				}
				if (k.Time > Keys.back().Time) {
					Animator<T>::Duration() += k.Time - Keys.back().Time;
					Keys.push_back(k);
					return Keys.size() - 1;
				}
				Keys.push_back(Keys.back());
				int i = Keys.size() - 3;
				for (; i >= 0 && Keys[i].Time > k.Time; i--) {
					Keys[i + 1] = Keys[i];
				}
				Keys[i + 1] = k;
				return i + 1;
			}
			int Insert(float Time) {
				return Insert({ Time, GetValueRepeat(Time) });
			}
			int Append(const Key<T>& k) {
				Key<T> K = k;
				if (Keys.size())
					K.Time += Keys.back().Time;
				else
					K.Time = 0;
				return Insert(K);
			}
			void SetRepeatDelay(float time) {
				Flags |= AnimationFlags::Dirty;
				if (time <= 0) {
					Flags &= ~AnimationFlags::Loop;
					time = 0;
				} else {
					Flags |= AnimationFlags::Loop;
				}
				if (Keys.size())
					Animator<T>::Duration() = Keys.back().Time + time;
				else
					Animator<T>::Duration() = time;
			}
			T GetValue(float Time) {
				if (Flags& AnimationFlags::Dirty)
					Reconstruct();
				return Animator<T>::GetValue(Time);
			}
			T GetValueRepeat(float Time) {
				if (Flags& AnimationFlags::Dirty)
					Reconstruct();
				return Animator::GetValueRepeat(Time);
			}
			Key<T>& GetKey(int Index) {
				Flags |= AnimationFlags::Dirty;
				return Keys[Index];
			}
			Key<T>& GetKey(float Time) {
				Flags |= AnimationFlags::Dirty;
				return Keys[GetKeyIndex(Time)];
			}
			int GetKeyIndex(float Time) {
				int min = 0;
				float minDist = fabsf(Time - Keys[0].Time);
				for (int i = 1; i < Keys.size(); i++) {
					float d = fabsf(Time - Keys[i].Time);
					if (d > minDist)return min;
					min = i;
					minDist = d;
				}
				return min;
			}
		protected:
			void ReComputeSpeeds() {
				int Repeat = Keys.back().Time < Animator<T>::Duration();
				if (Repeat) {
					std::vector<Key<T>> tmp(Keys.size() + 2);
					tmp[0] = Keys.back();
					tmp.back() = Keys[0];
					tmp[0].Time -= Animator<T>::Duration();
					tmp.back().Time = Animator<T>::Duration();

					memcpy(&tmp[1], &Keys[0], Keys.size() * sizeof(Key<T>));
					Keys = tmp;
				}
				for (int i = 1; i < (Keys.size() - 1); i++) {
					Key<T>& mid = Keys[i];
					Key<T>& start = Keys[i - 1];
					Key<T>& end = Keys[i + 1];

					if (mid.Flags&(unsigned int)AnimationFlags::Step) {
						mid.Speed = {};
						mid.Flags |= (unsigned int)AnimationFlags::HasSpeed;
						Flags |= (unsigned int)AnimationFlags::Dirty;
					}

					if (!(mid.Flags & (unsigned int)AnimationFlags::HasSpeed) && !(start.Flags&(unsigned int)AnimationFlags::Linear)) {
						mid.Speed = (end.Value - start.Value) / (end.Time - start.Time);
						Flags |= (unsigned int)AnimationFlags::Dirty;
					}
					if (mid.Flags&(unsigned int)AnimationFlags::Linear) {
						mid.Speed = (end.Value - mid.Value) / (end.Time - mid.Time);
						mid.Flags |= (unsigned int)AnimationFlags::HasSpeed;

						if (!(end.Flags & (unsigned int)AnimationFlags::HasSpeed))
							end.Speed = mid.Speed;

						Flags |= (unsigned int)AnimationFlags::Dirty;
					}
				}
				if (Repeat) {
					std::vector<Key<T>> tmp(Keys.size() - 2);
					memcpy(&tmp[0], &Keys[1], tmp.size() * sizeof(Key<T>));
					Keys = tmp;
				}
			}
			void Reconstruct() {
				ReComputeSpeeds();
				float Duration2 = Animator<T>::Duration();
				int Repeat = Keys.back().Time < Duration2;
				Animator<T>::Frames.resize(0);
				{
					Key<T>& f = Keys[0];
					Animator<T>::Frames.push_back({ f.Time, f.Value, f.Speed, {} });
				}
				for (auto i = 1; i < Keys.size(); i++) {
					auto& k = Keys[i];

					Animator<T>::Frames.push_back({
						Keys[i - 1].Time + (k.Time - Keys[i - 1].Time) * Keys[i - 1].Transition,
						{},
						{},
						{}
					});
					Animator<T>::Frames.push_back({
						k.Time,
						k.Value,
						k.Speed,
						{}
					});
				}
				if (Repeat) {
					Key<T>& f = Keys.back();
					Animator<T>::Frames.push_back({ f.Time + (Duration2 - f.Time) * f.Transition , f.Value, f.Speed });

					Animator<T>::Frames.push_back(Animator<T>::Frames[0]);
					Animator<T>::Frames.back().Time += Duration2;
				}
				for (auto i = 2; i < Animator<T>::Frames.size(); i += 2) {
					Frame<T>& fs = Animator<T>::Frames[i - 2];
					Frame<T>& fm = Animator<T>::Frames[i - 1];
					Frame<T>& fe = Animator<T>::Frames[i - 0];

					float dt = fe.Time - fs.Time;
					float t1 = fm.Time - fs.Time;
					float t2 = fe.Time - fm.Time;
					T dx = fe.Value - fs.Value;

					Key<T>& kk = Keys[(i - 2) >> 1];
					if (!(kk.Flags&(unsigned int)AnimationFlags::Smooth)) {
						fm.Speed = fs.Speed;
						fs.HalfAcceleration = {};
						fm.Value = fs.Value + fs.Speed * (fm.Time - fs.Time);
						fm.HalfAcceleration = {};
					} else {
						fm.Speed = (2.0f * dx - (fs.Speed * t1 + fe.Speed * t2)) / dt;
						fs.HalfAcceleration = 0.5f * (fm.Speed - fs.Speed) / t1;
						fm.Value = fs.Value + fs.Speed * t1 + fs.HalfAcceleration * t1 * t1;
						fm.HalfAcceleration = 0.5f * (fe.Speed - fm.Speed) / t2;
					}
				}

				Animator<T>::Frames.pop_back();

				Animator<T>::Duration() = Duration2;
				Flags &= ~(unsigned int)AnimationFlags::Dirty;
			}
			std::vector<Key<T>> Keys;
			unsigned int Flags;
		};

		class Blender {
		public:
			struct Channel {
				struct ShapeValues {
					float Start;
					float MaxAt;
					float End;

					float FirstRannge;
					float SecondRannge;
				};
				std::vector<ShapeValues> Shapes;
				std::string Name;
				float Max, Value;
				Channel(const std::string& Name, const std::vector<float>& MaxAt) : Name{ Name }, Max{ MaxAt.back() }, Value{ 0 } {
					Shapes.push_back({ 0, MaxAt[0], MaxAt[0], MaxAt[0], 0 });
					for (auto i = 1; i < MaxAt.size(); i++) {
						Shapes.back().End = MaxAt[i];
						Shapes.back().SecondRannge = MaxAt[i] - MaxAt[i - 1];
						Shapes.push_back({ MaxAt[i - 1], MaxAt[i], Max, MaxAt[i] - MaxAt[i - 1], 0 });
					}
				}
			};
			Blender(const std::vector<Channel>& Items) : LocalOutput{ 0 }, Channels{ Items } {
				OutCount = 0;
				for (auto& c : Channels) {
					OutCount += c.Shapes.size();
				}
			}
			Blender() {};
			float& ChannelValue(int Index) {
				return Channels[Index].Value;
			}
			void SetOutput(float* Out = 0) {
				if (LocalOutput) delete LocalOutput;
				if (!Out) Out = LocalOutput = new float[OutCount];
				Output = Out;
			}
			void Update() {
				float* o = Output;
				for (auto& c : Channels) {
					for (auto i = 0; i < c.Shapes.size(); i++) {
						auto& s = c.Shapes[i];
						*o = 0;
						if (c.Value > s.Start && c.Value <= s.MaxAt) {
							*o = (c.Value - s.Start) / s.FirstRannge;
						} else if (c.Value > s.MaxAt && c.Value < s.End) {
							*o = (s.End - c.Value) / s.SecondRannge;
						}
						o++;
					}
				}
			}
		protected:
			std::vector<Channel> Channels;
			int OutCount;
			float* LocalOutput;
			float* Output;
		};
	}
}
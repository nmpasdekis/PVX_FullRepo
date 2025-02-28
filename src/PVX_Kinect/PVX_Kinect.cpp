#include "PVX_Kinect.h"
#include <mutex>
#include <thread>

namespace PVX {
	namespace Kinect {
		const int BodyTracker::JointIndex[40] = {
			0, 1,
			1, 20,
			20, 2,
			2, 3,

			20, 4,
			4, 5,
			5, 6,
			6, 7,

			20, 8,
			8, 9,
			9, 10,
			10, 11,

			0, 12,
			12, 13,
			13, 14,
			14, 15,

			0, 16,
			16, 17,
			17, 18,
			18, 19
		};
		DepthSensor::DepthSensor() {
			auto res = GetDefaultKinectSensor(&KinectSensor);
			KinectSensor->get_CoordinateMapper(&Mapper);
			res = KinectSensor->get_DepthFrameSource(&depth);
			IFrameDescription* descr;
			depth->get_FrameDescription(&descr);
			descr->get_Height(&Height);
			descr->get_Width(&Width);
			Capacity = Width * Height;
			UVs.resize(Capacity);
			invColorWidth = 1.0f / Width;
			invColorHeight = 1.0f / Height;
			InternalBuffer3D.resize(Capacity);
			InternalBufferFloat.resize(Capacity);
			InternalBuffer = (unsigned short*)InternalBufferFloat.data();

			res = depth->OpenReader(&dreader);

			BOOLEAN IsOpen;
			KinectSensor->get_IsOpen(&IsOpen);
			if (!IsOpen)
				res = KinectSensor->Open();
		}
		int DepthSensor::GetFrame(unsigned short* Data) {
			IDepthFrame* frame;
			auto res = dreader->AcquireLatestFrame(&frame);
			if (res == S_OK) {
				frame->CopyFrameDataToArray(Capacity, Data);
				Mapper->MapDepthFrameToColorSpace(Capacity, Data, Capacity, (ColorSpacePoint*)UVs.data());
				frame->Release();
				return Capacity;
			}
			return 0;
		}
		int DepthSensor::GetFrameMeters(float* Data) {
			unsigned short* tmp = (unsigned short*)Data;
			int ret = GetFrame(tmp);
			if (ret)	for (int i = Capacity - 1; i >= 0; i--)
				Data[i] = tmp[i] * 0.001f;
			return ret;
		}
		int DepthSensor::GetPoints3D(PVX::Vector3D* Points) {
			if (GetFrame()) {
				Mapper->MapDepthFrameToCameraSpace(Capacity, InternalBuffer, Capacity, (CameraSpacePoint*)Points);
			}
			return 0;
		}
		int DepthSensor::GetPoints3D_FromDepth(float* Depth, PVX::Vector3D* Points) {
			for (int i = 0; i < Capacity; i++)
				InternalBuffer[i] = (unsigned short)(Depth[i] * 1000);

			Mapper->MapDepthFrameToCameraSpace(Capacity, InternalBuffer, Capacity, (CameraSpacePoint*)Points);
			return Capacity;
		}
		int DepthSensor::GetPoints3D_FromDepth(unsigned short* Depth, PVX::Vector3D* Points) {
			Mapper->MapDepthFrameToCameraSpace(Capacity, Depth, Capacity, (CameraSpacePoint*)Points);
			return Capacity;
		}
		int DepthSensor::GetUVs(PVX::Vector2D* UVs) {
			for (int i = 0; i < Capacity; i++) {
				UVs[i].u = this->UVs[i].u * invColorWidth;
				UVs[i].v = this->UVs[i].v * invColorHeight;
			}
			return 0;
		}
		void DepthSensor::SetColorSize(int Width, int Height) {
			invColorWidth = 1.0f / Width;
			invColorHeight = 1.0f / Height;
		}
		unsigned short* DepthSensor::GetFrame() {
			if (GetFrame(InternalBuffer)) return InternalBuffer;
			return 0;
		}
		float* DepthSensor::GetFrameMeters() {
			if (GetFrameMeters(InternalBufferFloat.data())) return InternalBufferFloat.data();
			return 0;
		}
		PVX::Vector3D* DepthSensor::GetPoints3D() {
			if (GetPoints3D(InternalBuffer3D.data())) return InternalBuffer3D.data();
			return 0;
		}


		BodyTracker::BodyTracker() {
			auto res = GetDefaultKinectSensor(&KinectSensor);
			res = KinectSensor->get_BodyFrameSource(&bodySource);

			res = bodySource->OpenReader(&bReader);

			BOOLEAN IsOpen;
			KinectSensor->get_IsOpen(&IsOpen);
			if (!IsOpen)
				res = KinectSensor->Open();
		}

		int BodyTracker::GetFrame() {
			IBodyFrame* f = 0;

			Joint j[JointType::JointType_Count];
			JointOrientation jo[JointType::JointType_Count];

			if (bReader->AcquireLatestFrame(&f) == S_OK) {
				f->GetAndRefreshBodyData(6, bodies);
				for (int i = 0; i < 6; i++) {
					auto& b = bodies[i];
					auto& B = Bodies[i];
					b->get_IsTracked(&B.Tracked);
					if (B.Tracked && b->GetJoints(JointType::JointType_Count, j) == S_OK) {
						b->GetJointOrientations(JointType::JointType_Count, jo);
						b->get_TrackingId(&B.Id);
						for (int k = 0; k < JointType::JointType_Count; k++) {
							auto& Pos = j[k];
							auto& Rot = jo[k];
							B.JointState[Pos.JointType] = Pos.TrackingState == TrackingState::TrackingState_Tracked;
							memcpy(&B.JointPositions[Pos.JointType], &Pos.Position, sizeof(PVX::Vector3D));
							memcpy(&B.JointRotations[Rot.JointType], &Rot.Orientation, sizeof(PVX::Quaternion));
						}
					}
				}
				f->Release();
			}
			return 0;
		}
		ColorSensor::ColorSensor(bool initF4Buffer) {
			auto res = GetDefaultKinectSensor(&KinectSensor);
			res = KinectSensor->get_ColorFrameSource(&color);
			res = color->OpenReader(&cReader);

			IFrameDescription* descr;
			color->get_FrameDescription(&descr);
			descr->get_Height(&Height);
			descr->get_Width(&Width);
			Capacity = Width * Height;

			BOOLEAN IsOpen = 0;;
			
			

			KinectSensor->get_IsOpen(&IsOpen);
			if (initF4Buffer) {
				while (!IsOpen) {
					std::this_thread::sleep_for(std::chrono::milliseconds(1));
					KinectSensor->get_IsOpen(&IsOpen);
				}
				InitInternalBufferF4();
			}

			
			if (!IsOpen)
				res = KinectSensor->Open();
		}
		int ColorSensor::GetFrame(PVX::Vector4D* Color) {
			int ret = GetFrame((PVX::ucVector4D*)Color);
			if (ret) {
				float* out = (float*)Color;
				unsigned char* in = (unsigned char*)Color;
				for (int i = (ret * 4) - 1; i >= 0; i--) {
					out[i] = in[i] * (1.0f / 255.0f);
				}
				return Capacity;
			}
			return 0;
		}
		int ColorSensor::GetFrame(PVX::ucVector4D* Color) {
			IColorFrame* f;
			if (cReader->AcquireLatestFrame(&f) == S_OK) {
				f->CopyConvertedFrameDataToArray(Capacity * sizeof(PVX::ucVector4D), (unsigned char*)Color, ColorImageFormat::ColorImageFormat_Rgba);
				f->Release();
				return Capacity;
			}
			return 0;
		}

		void ColorSensor::InitInternalBufferUC4() {
			InternalColor.resize(Capacity);
		}
		void ColorSensor::InitInternalBufferF4() {
			InternalColorFloat.resize(Capacity);
		}

		const PVX::ucVector4D* ColorSensor::GetFrameUC4() {
			if (GetFrame(InternalColor.data())) {
				return InternalColor.data();
			}
			return 0;
		}

		const PVX::Vector4D* ColorSensor::GetFrameF4() {
			if (GetFrame(InternalColorFloat.data())) {
				return InternalColorFloat.data();
			}
			return 0;
		}

	}
}
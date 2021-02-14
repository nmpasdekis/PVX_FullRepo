#ifndef __PVX_KINECT_H__
#define __PVX_KINECT_H__

#include <Kinect.h>
#include <PVX_Math3D.h>
#include <wrl.h>
#include <vector>
#include <array>

namespace PVX {
	namespace Kinect {
		class ColorSensor {
		public:
			ColorSensor();

			int GetFrame(PVX::Vector4D * Color);
			int GetFrame(PVX::ucVector4D * Color);
			const PVX::ucVector4D* GetFrameUC4();
			const PVX::Vector4D* GetFrameF4();

			int Width, Height, Capacity;

			void InitInternalBufferUC4();
			void InitInternalBufferF4();
		protected:
			std::vector<PVX::ucVector4D> InternalColor;
			std::vector<PVX::Vector4D> InternalColorFloat;
			Microsoft::WRL::ComPtr<IKinectSensor> KinectSensor;
			Microsoft::WRL::ComPtr<IColorFrameSource> color;
			Microsoft::WRL::ComPtr<IColorFrameReader> cReader;
		};

		class DepthSensor {
		public:
			DepthSensor();

			int GetFrame(unsigned short * Data);
			int GetFrameMeters(float * Data);
			int GetPoints3D(PVX::Vector3D * Points);
			int GetPoints3D_FromDepth(float * Depth, PVX::Vector3D * Points);
			int GetPoints3D_FromDepth(unsigned short * Depth, PVX::Vector3D * Points);
			int GetUVs(PVX::Vector2D * UVs);
			void SetColorSize(int Width, int Height);

			unsigned short * GetFrame();
			float * GetFrameMeters();
			PVX::Vector3D * GetPoints3D();

			int Width, Height, Capacity;
		protected:
			float invColorWidth, invColorHeight;
			unsigned short* InternalBuffer;
			std::vector<float> InternalBufferFloat;
			std::vector<PVX::Vector3D> InternalBuffer3D;
			std::vector<PVX::Vector2D> UVs;
			Microsoft::WRL::ComPtr<IKinectSensor> KinectSensor;
			Microsoft::WRL::ComPtr<IDepthFrameSource> depth;
			Microsoft::WRL::ComPtr<IDepthFrameReader> dreader;
			Microsoft::WRL::ComPtr<ICoordinateMapper> Mapper;
		};

		typedef struct Body {
			BOOLEAN Tracked;
			unsigned long long Id;
			PVX::Vector3D JointPositions[JointType::JointType_Count];
			PVX::Quaternion JointRotations[JointType::JointType_Count];
			int JointState[JointType::JointType_Count];
		} Body;

		class BodyTracker {
		public:
			static const int JointIndex[40];
			BodyTracker();
			int GetFrame();
			Body Bodies[6];
		protected:
			IBody* bodies[6];
			Microsoft::WRL::ComPtr<IKinectSensor> KinectSensor;
			Microsoft::WRL::ComPtr<IBodyFrameSource> bodySource;
			Microsoft::WRL::ComPtr<IBodyFrameReader> bReader;
		};
	}
}

#endif // !__PVX::KINECT_H__

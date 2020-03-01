#include <PVX_Math3D.h>
#include <vector>
#include <map>
#include <string>
#include <PVX_OpenGL.h>

namespace PVX::Engine {
	struct ObjectPart {

	};
	struct Transform {
		long long						ParentIndex;
		Matrix4x4						PostTranslate;
		Matrix4x4						PostRotate;
		Matrix4x4						PostScale;

		Matrix4x4						Result;

		Vector3D						Position;
		Vector3D						Rotation;
		Vector3D						Scale;

		Matrix4x4(*Rotate)(const Vector3D&);

		struct {
			float						FrameRate;
			std::vector<PVX::Vector3D>	Position;
			std::vector<PVX::Vector3D>	Rotation;
			std::vector<PVX::Vector3D>	Scale;
		} Animation;
	};
	class Object {
		std::map<std::string, PVX::OpenGL::ConstantBuffer> Materials;
		std::vector<ObjectPart> Parts;
	};
}
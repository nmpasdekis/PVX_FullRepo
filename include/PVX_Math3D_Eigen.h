#include <Eigen/dense>

namespace PVX {
	//using Vector2D = Eigen::Vector2f;
	//using Vector3D = Eigen::Vector3f;
	//using Vector4D = Eigen::Vector4f;
	//using Matrix4 = Eigen::Matrix4f;
	//using Matrix4x3 = Eigen::Matrix<float, 4, 3>;

	union Vector3D {
		Vector3D(float X, float Y, float Z) { x = X; y = Y; z = Z; w = 1.0f; }
		Vector3D(float X, float Y, float Z, float W) { x = X; y = Y; z = Z; w = W; }
		Vector3D(const Vector3D& v) = default;
		Vector3D(const Eigen::Vector4f& v) { Column = v; };
		Vector3D(const Eigen::RowVector4f& v) { Row = v; };
		struct {
			union {
				struct {
					float x, y, z;
				};
				Eigen::Vector3f Column3;
				Eigen::RowVector3f Row3;
			};
			float w;
		};
		Eigen::Vector4f Column;
		Eigen::RowVector4f Row;

		Eigen::Vector4f operator*(const Vector3D& v) { return Row.array() * v.Row.array(); };
	};
}
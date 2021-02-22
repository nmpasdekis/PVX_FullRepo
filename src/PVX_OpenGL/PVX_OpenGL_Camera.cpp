#include <PVX_OpenGL.h>


namespace PVX::OpenGL {
	Camera::Camera(PVX::Matrix4x4* view, PVX::Matrix4x4* projection) : Position({ 0,0,0 }), Rotation({ 0,0,0 }), OrbitCenter({ 0,0,0 }) {
		_View = view ? view : &Storage.View;
		_Perspective = projection ? projection : &Storage.Perspective;
		Matrix4x4& View = *_View;
		View.m00 = View.m11 = View.m22 = View.m33 = 1.0f;
		Width = 16.0f;
		Height = 9.0f;
		OrbitDistance = 10.0f;
		SetPerspective(30.0f, 0.1f, 100.0f);
	}
	Camera::Camera(int Width, int Height, float FovDeg, float Near, float Far, PVX::Matrix4x4* view, PVX::Matrix4x4* projection) : Width{ float(Width) }, Height{ float(Height) }, Storage{ 0 }, Position({ 0,0,0 }), Rotation({ 0,0,0 }), OrbitCenter({ 0,0,0 }) {
		_View = view? view: &Storage.View;
		_Perspective = projection ? projection: &Storage.Perspective;
		Matrix4x4& View = *_View;
		View.m00 = View.m11 = View.m22 = View.m33 = 1.0f;
		SetPerspective(FovDeg, Near, Far);
	}

	Matrix4x4& Camera::SetPerspective(float Fov, float Near, float Far) {
		_fov = Fov;
		_near = Near;
		_far = Far;
		Matrix4x4& Perspective = *_Perspective;
		Perspective.m11 = 1.0f / (float)tanf(ToRAD(Fov)/2.0f);
		Perspective.m00 = Perspective.m11 * Height / Width;

		Perspective.m22 = (Far + Near) / (Near - Far);
		Perspective.m32 = (2.0f * Far * Near) / (Near - Far);
		Perspective.m23 = -1.0f;

		return Perspective;
	}

	Vector3D& Camera::GetLookVector(Vector3D& Look) {
		Look.x = -_View->m02;
		Look.y = -_View->m12;
		Look.z = -_View->m22;
		return Look;
	}
	Vector3D& Camera::GetUpVector(Vector3D& Up) {
		Up.x = _View->m01;
		Up.y = _View->m11;
		Up.z = _View->m21;
		return Up;
	}
	Vector3D& Camera::GetRightVector(Vector3D& Right) {
		Right.x = _View->m00;
		Right.y = _View->m10;
		Right.z = _View->m20;
		return Right;
	}
	Vector3D& Camera::Move(float x, float y, float z) {
		Matrix4x4& View = *_View;
		Position.x += x * View.m00 + y * View.m01 + z * View.m02;
		Position.y += x * View.m10 + y * View.m11 + z * View.m12;
		Position.z += x * View.m20 + y * View.m21 + z * View.m22;
		return Position;
	}

	Vector3D& Camera::Move(const Vector3D& xyz) {
		Matrix4x4& View = *_View;
		Position.x += xyz.x * View.m00 + xyz.y * View.m01 + xyz.z * View.m02;
		Position.y += xyz.x * View.m10 + xyz.y * View.m11 + xyz.z * View.m12;
		Position.z += xyz.x * View.m20 + xyz.y * View.m21 + xyz.z * View.m22;
		return Position;
	}
	Vector3D& Camera::MoveLevel(const Vector3D& xyz) {
		Matrix4x4& View = *_View;
		auto v1 = Vector2D{ View.m00, View.m02 }.Normalized();
		auto v3 = Vector2D{ View.m20, View.m22 }.Normalized();
		Position.x += xyz.x * v1.x + xyz.z * v1.y;
		Position.y += xyz.y;
		Position.z += xyz.x * v3.x + xyz.z * v3.y;
		return Position;
	}
	Vector3D& Camera::MoveCenter(float x, float y, float z) {
		Matrix4x4& View = *_View;
		OrbitCenter.x += x * View.m00 + y * View.m01 + z * View.m02;
		OrbitCenter.y += x * View.m10 + y * View.m11 + z * View.m12;
		OrbitCenter.z += x * View.m20 + y * View.m21 + z * View.m22;
		Position.x = OrbitCenter.x + OrbitDistance * View.m02;
		Position.y = OrbitCenter.y + OrbitDistance * View.m12;
		Position.z = OrbitCenter.z + OrbitDistance * View.m22;
		return Position;
	}
	Vector3D& Camera::MoveCenter(const Vector3D& xyz) {
		Matrix4x4& View = *_View;
		OrbitCenter.x += xyz.x * View.m00 + xyz.y * View.m01 + xyz.z * View.m02;
		OrbitCenter.y += xyz.x * View.m10 + xyz.y * View.m11 + xyz.z * View.m12;
		OrbitCenter.z += xyz.x * View.m20 + xyz.y * View.m21 + xyz.z * View.m22;
		Position.x = OrbitCenter.x + OrbitDistance * View.m02;
		Position.y = OrbitCenter.y + OrbitDistance * View.m12;
		Position.z = OrbitCenter.z + OrbitDistance * View.m22;
		return OrbitCenter;
	}
	Vector3D& Camera::MoveCenterLevel(const Vector3D& xyz) {
		Matrix4x4& View = *_View;
		auto v1 = Vector2D{ View.m00, View.m02 }.Normalized();
		auto v3 = Vector2D{ View.m20, View.m22 }.Normalized();
		OrbitCenter.x += xyz.x * v1.x + xyz.z * v1.y;
		OrbitCenter.y += xyz.y;
		OrbitCenter.z += xyz.x * v3.x + xyz.z * v3.y;
		Position.x = OrbitCenter.x + OrbitDistance * View.m02;
		Position.y = OrbitCenter.y + OrbitDistance * View.m12;
		Position.z = OrbitCenter.z + OrbitDistance * View.m22;
		return OrbitCenter;
	}

	Matrix4x4& Camera::UpdateView() {
		Matrix4x4& View = *_View;
		RotateYawPitchRoll2(View, Rotation);
		View.TranslateBefore(Position);
		return View;
	}
	Matrix4x4& Camera::UpdateView_Orbit() {
		Matrix4x4& View = *_View;
		//Vector3D orb={ 0.0f, 0.0f, OrbitDistance };
		RotateYawPitchRoll2(View, Rotation);
		View.TranslateBefore(OrbitCenter);
		View.Vec3.z -= OrbitDistance;

		Position.x = OrbitCenter.x + OrbitDistance * View.m02;
		Position.y = OrbitCenter.y + OrbitDistance * View.m12;
		Position.z = OrbitCenter.z + OrbitDistance * View.m22;
		return View;
	}

	Ray& Camera::CastScreenRay(float x, float y, Ray& Ray) {
		Ray.Position = Position;
		Ray.Direction = -Mul3x3(*_View, {
			((-2.0f * x / Width)+1.0f) / _Perspective->m00,
			((2.0f * y / Height)-1.0f)/ _Perspective->m11,
			1.0f
		}).Normalized();
		return Ray;
	}

	Ray Camera::CastScreenRay(float x, float y) {
		return { Position, -Mul3x3(*_View, {
			((-2.0f * x / Width) + 1.0f) / _Perspective->m00,
			((2.0f * y / Height) - 1.0f) / _Perspective->m11,
			1.0f
		}).Normalized() };
	}

	Ray& Camera::CastRay(float x, float y, Ray& Ray) {
		Vector3D screen = { x/ _Perspective->m00, y/ _Perspective->m11, 1.0f };

		Ray.Position = Position;

		Ray.Direction = -Mul3x3(*_View, screen).Normalized();
		return Ray;
	}

	Ray Camera::CastRay(float x, float y) {
		Vector3D screen = { x / _Perspective->m00, y / _Perspective->m11, 1.0f };
		Ray Ray;

		Ray.Position = Position;

		Ray.Direction = -Mul3x3(*_View, screen).Normalized();
		return Ray;
	}

	void Camera::SetSize(int width, int height) {
		Width = (float)width; Height = (float)height;
	}

	Matrix4x4& Camera::SetSizePerspective(int width, int height) {
		Width = (float)width; Height = (float)height;
		return SetPerspective(_fov, _near, _far);
	}

	void Camera::SetProjectionMatrix(Matrix4x4& Mat) {
		_Perspective = &Mat;
	}

	void Camera::SetViewMatrix(Matrix4x4& Mat) {
		_View = &Mat;
	}

	Matrix4x4& Camera::GetViewMatrix() {
		return *_View;
	}

	Matrix4x4& Camera::GetProjectionMatrix() {
		return *_Perspective;
	}
	void Camera::OrbitRelative(float u, float v, float Distance) {
		Rotation.Yaw += u;
		Rotation.Pitch += v;
		OrbitDistance *= Distance;
	}
}
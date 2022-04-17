#pragma once

//#define PVX_NO_INTRINSICS

#ifndef PVX_NO_INTRINSICS
#include <xmmintrin.h>
#include <intrin.h>
#endif

#include <cmath>
#include <utility>
#include <functional>

namespace PVX {
	constexpr double PI = 3.14159265358979323846;
#pragma intrinsic(sinf, cosf)

	constexpr float ToRAD(float x) { return ((float)(x*PI/180.0)); }
	constexpr double ToRAD(double x) { return x*PI/180.0; }
	constexpr float ToDEGREES(float x) { return ((float)(x*180.0/PI)); }
	constexpr double ToDEGREES(double x) { return x*180.0/PI; }

	inline float Q_rsqrt(float y) {
		float x2 = y * 0.5f;
		constexpr float threeHalfs = 1.5f;
		uint32_t i = 0x5f3759df - ((*(uint32_t*)&y) >> 1);
		y = *(float*)&i;
		y *= threeHalfs - x2 * y * y;
		return y;
	}

	inline float Q_rsqrt2(float y) {
		float x2 = y * 0.5f;
		constexpr float threeHalfs = 1.5f;
		uint32_t i = 0x5f3759df - ((*(uint32_t*)&y) >> 1);
		y = *(float*)&i;
		y *= threeHalfs - x2 * y * y;
		y *= threeHalfs - x2 * y * y;
		return y;
	}
	inline float Q_rsqrt4(float y) {
		float x2 = y * 0.5f;
		constexpr float threeHalfs = 1.5f;
		uint32_t i = 0x5f3759df - ((*(uint32_t*)&y) >> 1);
		y = *(float*)&i;
		y *= threeHalfs - x2 * y * y;
		y *= threeHalfs - x2 * y * y;
		y *= threeHalfs - x2 * y * y;
		y *= threeHalfs - x2 * y * y;
		return y;
	}

	union ucVector2D {
		enum Traits {
			HasX = true,
			HasY = true,
			HasZ = false,
			HasW = false,
			IsFloat = false
		};
		struct {
			unsigned char x, y;
		};
		unsigned char Array[2];
		struct {
			unsigned char r, g;
		};
		struct {
			unsigned char u, v;
		};
		unsigned short Word;
	};
	union ucVector3D {
		enum Traits {
			HasX = true,
			HasY = true,
			HasZ = true,
			HasW = false,
			IsFloat = false
		};
		struct {
			unsigned char x, y, z;
		};
		unsigned char Array[3];
		struct {
			unsigned char r, g, b;
		};
		struct {
			unsigned char _b, _g, _r;
		};
	};
	union ucVector4D {
		enum Traits {
			HasX = true,
			HasY = true,
			HasZ = true,
			HasW = true,
			IsFloat = false
		};
		struct {
			unsigned char x, y, z, w;
		};
		unsigned char Array[4];
		struct {
			unsigned char r, g, b, a;
		};
		unsigned int dWord;
	};
	union cVector2D {
		enum Traits {
			HasX = true,
			HasY = true,
			HasZ = false,
			HasW = false,
			IsFloat = false
		};
		struct {
			char x, y;
		};
		char Array[2];
		struct {
			char r, g;
		};
		unsigned short Word;
	};
	union cVector3D {
		enum Traits {
			HasX = true,
			HasY = true,
			HasZ = true,
			HasW = false,
			IsFloat = false
		};
		struct {
			char x, y, z;
		};
		char Array[3];
		struct {
			char r, g, b;
		};
	};
	union cVector4D {
		enum Traits {
			HasX = true,
			HasY = true,
			HasZ = true,
			HasW = true,
			IsFloat = false
		};
		struct {
			char x, y, z, w;
		};
		char Array[4];
		struct {
			char r, g, b, a;
		};
		unsigned int dWord;
	};
	union Vector2D {
		enum Traits {
			HasX = true,
			HasY = true,
			HasZ = false,
			HasW = false,
			IsFloat = true
		};
		//Vector2D() :x{ 0 }, y{ 0 }{}
		Vector2D() = default;
		inline Vector2D(float x, float y) : x{ x }, y{ y } {}
		struct {
			float x, y;
		};
		float Array[2];
		struct {
			float u, v;
		};
		struct {
			float Width, Height;
		};
		inline Vector2D operator-() const { return Vector2D{ -x, -y }; }
		inline float Length() const { return sqrtf(x*x+y*y); }
		inline float Length2() const { return (x*x+y*y); }
		inline float Dot(const Vector2D& v) const { return x*v.x + y*v.y; }
		inline Vector2D& Normalize() { float w = 1.0f / Length(); x *= w; y *= w; return *this; }
		inline Vector2D Normalized() const { float w = 1.0f / Length(); return{ x * w, y * w }; }
		inline float Cross(const Vector2D& v2) { return x*v2.y - y*v2.x; }
	};
	union Vector3D {
		enum Traits {
			HasX = true,
			HasY = true,
			HasZ = true,
			HasW = false,
			IsFloat = true
		};
		Vector3D() = default;
		//Vector3D() : x{ 0 }, y{ 0 }, z{ 0 } {}
		inline Vector3D(float x, float y, float z) : x{ x }, y{ y }, z{ z } {}
		struct {
			float x, y, z;
		};
		float Array[3];
		struct {
			float r, g, b;
		};
		struct {
			float b, g, r;
		} BGR;
		struct {
			float Pitch, Yaw, Roll;
		};
		struct {
			float H, L, S;
		};
		struct {
			float Width, Height, Depth;
		};
		struct {
			Vector2D vec2;
			float unused1;
		};
		inline Vector3D operator-() const { return Vector3D{ -x, -y, -z }; }
		inline float Length() const { return sqrtf(x*x + y*y + z*z); }
		inline float Length2() const { return (x*x + y*y + z*z); }
		inline float Dot(const Vector3D& v) const { return x*v.x + y*v.y + z*v.z; }
		inline Vector3D& Normalize() { float w = 1.0f / Length(); x *= w; y *= w; z *= w; return *this; }
		inline Vector3D Normalized() const { float w = 1.0f / Length(); return{ x * w, y * w, z * w }; }
		inline Vector3D Cross(const Vector3D& v) const { return { y* v.z - z*v.y, v.x* z - v.z*x, x* v.y - y*v.x }; }
		inline static Vector3D Normal(float x, float y, float z) { return Vector3D{ x, y, z }.Normalized(); }
	};
	__declspec(align(16))
		union Vector4D {
		enum Traits {
			HasX = true,
			HasY = true,
			HasZ = true,
			HasW = true,
			IsFloat = true
		};
		Vector4D() = default;
		//Vector4D() : x{ 0 }, y{ 0 }, z{ 0 }, w{ 0 } {}
		//inline Vector4D(float x, float y, float z, float w) : x{ x }, y{ y }, z{ z }, w{ w } {}
		//inline Vector4D(const Vector3D& vec, float W = 1.0f): x{ vec.x }, y{ vec.y }, z{ vec.z }, w{ W } {}
#ifndef PVX_NO_INTRINSICS
		inline Vector4D(float X, float Y, float Z, float W) : xmm{ _mm_set_ps(W, Z, Y, X) } {}
		inline Vector4D(const Vector3D& vec, float W = 1.0f) : xmm{ _mm_set_ps(W, vec.z, vec.y, vec.x) } {}
		inline Vector4D(float v) : xmm{ _mm_set1_ps(v) } {}
		__m128 xmm;
		inline operator __m128() const {
			return _mm_load_ps(Array);
		}
		inline Vector4D(const __m128 v) {
			_mm_store_ps(Array, v);
		}
#else
		inline Vector4D(float X, float Y, float Z, float W) :x{ X }, y{ Y }, z{ Z }, w{ W }{}
		inline Vector4D(const Vector3D& vec, float W = 1.0f) :x{ vec.x }, y{ vec.y }, z{ vec.z }, w{ W }{}
#endif
		struct {
			float x, y, z, w;
		};
		float Array[4];
		struct {
			float r, g, b, a;
		};
		struct {
			Vector3D Vec3;
			float Scalar;
		};
		struct {
			float Width, Height, Depth, Imagination;
		};
		inline Vector4D operator-() const { return Vector4D{ -x, -y, -z, -w }; }
		inline Vector4D operator*(const Vector4D& v) const { return Vector4D{ v.x*x, v.y*y, v.z * z, v.w * w }; }
		inline float Length() const { return sqrtf(x*x+y*y+z*z+w*w); }
		inline float Length2() const { return (x*x+y*y+z*z+w*w); }
		inline float Dot(const Vector4D& v) const { return x*v.x + y*v.y+z*v.z+w*v.w; }
		inline Vector4D& Normalize() { float i = 1.0f / Length(); x *= i; y *= i; z *= i; w *= i; return *this; }
		inline const Vector4D Normalized() const { float i = 1.0f / Length(); return{ x * i, y * i, z * i, w * i }; }
	};

	inline Vector4D operator+(const Vector4D& v1, const Vector4D& v2) {
#ifndef PVX_NO_INTRINSICS
		return _mm_add_ps(v1, v2);
#else 
		return{
			v1.x + v2.x,
			v1.y + v2.y,
			v1.z + v2.z,
			v1.w + v2.w,
		};
#endif
	}
	inline Vector4D operator*(const Vector4D& c, float f) {
#ifndef PVX_NO_INTRINSICS
		Vector4D out;
		__m128 tf = _mm_load_ps1(&f);
		__m128 tc = _mm_loadu_ps(&c.r);
		__m128 to = _mm_mul_ps(tf, tc);
		_mm_storeu_ps(&out.r, to);
		return out;
#else
		return {
			c.x* f,
			c.y* f,
			c.z* f,
			c.w* f
		};
#endif
	}
	inline Vector4D& operator*=(Vector4D& v1, float f) {
		v1.x *= f;
		v1.y *= f;
		v1.z *= f;
		v1.w *= f;
		return v1;
	}
	inline Vector4D& operator/=(Vector4D& v1, float d) {
		float f = 1.0f / d;
		v1.x *= f;
		v1.y *= f;
		v1.z *= f;
		v1.w *= f;
		return v1;
	}
	inline Vector4D& operator-=(Vector4D& v1, const Vector4D& v2) {
		v1.x -= v2.x;
		v1.y -= v2.y;
		v1.z -= v2.z;
		v1.w -= v2.w;
		return v1;
	}

	inline bool operator==(const Vector4D& v1, const Vector4D& v2) {
		return((v1.x == v2.x) && (v1.y == v2.y) && (v1.z == v2.z) && (v1.w == v2.w));
	}

	inline bool operator!=(const Vector4D& v1, const Vector4D& v2) {
		return((v1.x != v2.x) || (v1.y != v2.y) || (v1.z != v2.z) || (v1.w != v2.w));
	}

	//////////////////////////////////////////////////////////////////////////////

	union iVector2D {
		enum Traits {
			HasX = true,
			HasY = true,
			HasZ = false,
			HasW = false,
			IsFloat = false
		};
		struct {
			int x, y;
		};
		int Array[2];
		struct {
			int u, v;
		};
		struct {
			int Width, Height;
		};
		inline iVector2D operator-() const { return iVector2D{ -x, -y }; }
	};
	union iVector3D {
		enum Traits {
			HasX = true,
			HasY = true,
			HasZ = true,
			HasW = false,
			IsFloat = false
		};
		struct {
			int x, y, z;
		};
		int Array[3];
		struct {
			int r, g, b;
		};
		struct {
			int Pitch, Yaw, Roll;
		};
		struct {
			int Width, Height, Depth;
		};
		inline iVector3D operator-() const { return iVector3D{ -x, -y, -z }; }
	};
	__declspec(align(16))
		union iVector4D {
		enum Traits {
			HasX = true,
			HasY = true,
			HasZ = true,
			HasW = true,
			IsFloat = false
		};
		struct {
			int x, y, z, w;
		};
		int Array[4];
		struct {
			int r, g, b, a;
		};
		struct {
			Vector3D Vec3;
			int Scalar;
		};
		struct {
			int Width, Height, Depth, Imagination;
		};
		inline iVector4D operator-() const { return iVector4D{ -x, -y, -z, -w }; }
	};

	union uVector2D {
		enum Traits {
			HasX = true,
			HasY = true,
			HasZ = false,
			HasW = false,
			IsFloat = false
		};
		struct {
			unsigned int x, y;
		};
		unsigned int Array[2];
		struct {
			unsigned int u, v;
		};
		struct {
			unsigned int Width, Height;
		};
	};
	union uVector3D {
		enum Traits {
			HasX = true,
			HasY = true,
			HasZ = true,
			HasW = false,
			IsFloat = false
		};
		struct {
			unsigned int x, y, z;
		};
		unsigned int Array[3];
		struct {
			unsigned int r, g, b;
		};
		struct {
			unsigned int Pitch, Yaw, Roll;
		};
		struct {
			unsigned int Width, Height, Depth;
		};
	};
	__declspec(align(16))
		union uVector4D {
		enum Traits {
			HasX = true,
			HasY = true,
			HasZ = true,
			HasW = true,
			IsFloat = false
		};
		struct {
			unsigned int x, y, z, w;
		};
		unsigned int Array[4];
		struct {
			unsigned int r, g, b, a;
		};
		struct {
			Vector3D Vec3;
			unsigned int Scalar;
		};
		struct {
			unsigned int Width, Height, Depth, Imagination;
		};
	};

	//////////////////////////////////////////////////////////////////////////////

	union Rect2D {
		struct {
			float x, y, Width, Height;
		};
		float Array[4];
		Vector4D Vec4;
	};
	union Quaternion {
		struct {
			float i, j, k, r;
		};
		float Array[4];
		Vector4D Vec;
		inline Quaternion& Normalize() {
			float w = 1.0f / sqrtf(i*i + j*j + k * k + r * r);
			i *= w;
			j *= w;
			k *= w;
			r *= w;
			return *this;
		}
		inline Quaternion Normalized() const {
			float w = 1.0f / sqrtf(i*i + j*j + k * k + r * r);
			return { i * w, j * w, k * w, r * w };
		}
		inline const Quaternion Conjugated() const {
			return { -i, -j, -k, r };
		}
	};
	union DualQuaternion {
		struct {
			Quaternion Real, Dual;
		};
		float Array[8];
		Quaternion q[2];
		struct {
			float r_i, r_j, r_k, r_r, d_i, d_j, d_k, d_r;
		};
		inline DualQuaternion& Normalize() {
			float l = 1.0f / Real.Vec.Length();
			Real.Vec.x *= l;
			Real.Vec.y *= l;
			Real.Vec.z *= l;
			Real.Vec.w *= l;
			Dual.Vec.x *= l;
			Dual.Vec.y *= l;
			Dual.Vec.z *= l;
			Dual.Vec.w *= l;
			return *this;
		}
		inline DualQuaternion Normalized() const {
			float l = 1.0f / Real.Vec.Length();
			return {
				Real.Vec.x * l,
				Real.Vec.y * l,
				Real.Vec.z * l,
				Real.Vec.w * l,
				Dual.Vec.x * l,
				Dual.Vec.y * l,
				Dual.Vec.z * l,
				Dual.Vec.w * l
			};
		}
		inline const DualQuaternion Conjugated() {
			return{ -r_i, -r_j, -r_k, r_r, -d_i, -d_j, -d_k, d_r };
		}
		//inline Matrix4x4 Matrix() const {
		//	Matrix4x4 ret;
		//	GetQuaternionMatrix(Real, ret);
		//	auto t = (Dual * 2.0f) * Conjugate(Real);
		//	ret.m30 = t.i;
		//	ret.m31 = t.j;
		//	ret.m32 = t.k;
		//	ret.Vec3.Vec3 = ((Dual * 2.0f) * Conjugate(Real)).Vec.Vec3;
		//	return ret;
		//}
	};

#pragma warning(disable:4201)
	__declspec(align(16))
		union Matrix4x4 {
		Matrix4x4() = default;
		float m[4][4];
		struct {
			float m00, m01, m02, m03,
				m10, m11, m12, m13,
				m20, m21, m22, m23,
				m30, m31, m32, m33;
		};
		float m16[16];
		struct {
			Vector4D Vec0, Vec1, Vec2, Vec3;
		};
		Vector4D m4[4];


		inline static constexpr Matrix4x4 Identity() {
			return {
				1.0f, 0, 0, 0,
				0, 1.0f, 0, 0,
				0, 0, 1.0f, 0,
				0, 0, 0, 1.0f
			};
		}

		inline void Scale(const PVX::Vector3D& s) {
			Vec0.x *= s.x;
			Vec0.y *= s.y;
			Vec0.z *= s.z;

			Vec1.x *= s.x;
			Vec1.y *= s.y;
			Vec1.z *= s.z;

			Vec2.x *= s.x;
			Vec2.y *= s.y;
			Vec2.z *= s.z;

			Vec3.x *= s.x;
			Vec3.y *= s.y;
			Vec3.z *= s.z;
		}
		template<typename T, typename std::enable_if_t<T::Traits::HasZ, int> = 0>
		inline void TranslateAfter(const T& p) {
			m30 = p.x;
			m31 = p.y;
			m32 = p.z;
		}
		template<typename T, typename std::enable_if_t<T::Traits::HasZ, int> = 0>
		inline void TranslateBefore(const T& p) {
			m30 = -p.x * m00 - p.y * m10 - p.z * m20;
			m31 = -p.x * m01 - p.y * m11 - p.z * m21;
			m32 = -p.x * m02 - p.y * m12 - p.z * m22;
		}

		inline void SetDiag3(const Vector3D& vec) {
			m00 = vec.x;
			m11 = vec.y;
			m22 = vec.z;
		}
		inline void SetDiag(const Vector3D& vec) {
			m00 = vec.x;
			m11 = vec.y;
			m22 = vec.z;
			m33 = 1.0f;
		}
		inline void SetDiag(const Vector4D& vec) {
			m00 = vec.x;
			m11 = vec.y;
			m22 = vec.z;
			m33 = vec.w;
		}

		bool IsIdentity(const float e = 1e-6) const {
			float sum = 0;
			for (auto i = 0; i<16; i++)
				sum += std::abs(Identity().m16[i] - m16[i]);
			return sum < e;
		}
		static constexpr Matrix4x4 ScaleMatrix(const Vector3D& s) {
			return {
				s.x, 0, 0, 0,
				0, s.y, 0, 0,
				0, 0, s.z, 0,
				0, 0, 0, 1.0f
			};
		}
		static constexpr Matrix4x4 ScaleMatrix(const float s) {
			return {
				s, 0, 0, 0,
				0, s, 0, 0,
				0, 0, s, 0,
				0, 0, 0, 1.0f
			};
		}
		static constexpr Matrix4x4 Translation(const Vector3D& t) {
			return {
				1.0f, 0, 0, 0,
				0, 1.0f, 0, 0,
				0, 0, 1.0f, 0,
				t.x, t.y, t.z, 1.0f
			};
		}
		template<typename T, typename std::enable_if_t<T::Traits::HasZ, int> = 0>
		inline void MatStoreX(const T& vec) {
			m00 = (vec).x;
			m10 = (vec).y;
			m20 = (vec).z;
		}
		template<typename T, typename std::enable_if_t<T::Traits::HasZ, int> = 0>
		inline void MatStoreY(const T& vec) {
			m01 = (vec).x;
			m11 = (vec).y;
			m21 = (vec).z;
		}
		template<typename T, typename std::enable_if_t<T::Traits::HasZ, int> = 0>
		inline void MatStoreZ(const T& vec) {
			m02 = (vec).x;
			m12 = (vec).y;
			m22 = (vec).z;
		}
	};
	union Matrix3x3 {
		float m[3][3];
		float m9[9];
		struct {
			float m00, m01, m02,
				m10, m11, m12,
				m20, m21, m22;
		};
		Vector3D m3[3];
		struct {
			Vector3D v0, v1, v2;
		};

		static constexpr Matrix3x3 Identity() {
			return {
				1.0f, 0, 0,
				0, 1.0f, 0,
				0, 0, 1.0f
			};
		}
	};
	union Matrix3x4 {
		float m[3][4];
		float m_12[12];
		struct {
			float m00, m01, m02, m03,
				m10, m11, m12, m13,
				m20, m21, m22, m23;
		};
		struct {
			Vector4D Vec0, Vec1, Vec2;
		};
		Vector4D m3[3];
		static constexpr Matrix3x4 Identity() {
			return {
				1.0f, 0, 0, 0,
				0, 1.0f, 0, 0,
				0, 0, 1.0f, 0
			};
		}
	};
	union Matrix2x2 {
		float m[2][2];
		float m4[4];
		struct {
			float m00, m01,
				m10, m11;
		};
		Vector2D m2[2];
		struct {
			Vector2D v0, v1;
		};
		static constexpr Matrix2x2 Identity() {
			return {
				1.0f, 0,
				0, 1.0f
			};
		}
	};
	struct Ray {
		Vector3D Position;
		Vector3D Direction;
	};
	struct Ray2D {
		Vector2D Position;
		Vector2D Direction;
	};
	struct Weight {
		float W[4];
		unsigned char I[4];
	};
	struct Plane {
		Vector3D Normal;
		float Distance;
	};
	union AABB {
		struct {
			Vector3D Min, Max;
		};
		float Array[6];
	};
	union Triangle {
		int Index[3];
		struct {
			int Index0, Index1, Index2;
		};
	};
	union Quad {
		int Index[4];
		struct {
			int  Index0, Index1, Index2, Index3;
		};
	};



#define DET3(m00, m01, m02, m10, m11, m12, m20, m21, m22) \
		(\
		((m00)*((m11)*(m22)-(m21)*(m12))) - \
		((m01)*((m10)*(m22)-(m20)*(m12))) + \
		((m02)*((m10)*(m21)-(m20)*(m11))) \
		)

#define DET4_00(m) DET3(\
		/*(m).m00,   (m).m01, (m).m02, (m).m03,*/	\
		/*(m).m10,*/ (m).m11, (m).m12, (m).m13,	\
		/*(m).m20,*/ (m).m21, (m).m22, (m).m23,	\
		/*(m).m30,*/ (m).m31, (m).m32, (m).m33	\
		)
#define DET4_01(m) DET3(\
		/*(m).m00, (m).m01, (m).m02, (m).m03,*/	\
		(m).m10, /*(m).m11,*/ (m).m12, (m).m13,	\
		(m).m20, /*(m).m21,*/ (m).m22, (m).m23,	\
		(m).m30, /*(m).m31,*/ (m).m32, (m).m33	\
		)
#define DET4_02(m) DET3(\
		/*(m).m00, (m).m01, (m).m02, (m).m03,*/	\
		(m).m10, (m).m11, /*(m).m12,*/ (m).m13,	\
		(m).m20, (m).m21, /*(m).m22,*/ (m).m23,	\
		(m).m30, (m).m31, /*(m).m32,*/ (m).m33	\
		)
#define DET4_03(m) DET3(\
		/*(m).m00, (m).m01, (m).m02, (m).m03,*/	\
		(m).m10, (m).m11, (m).m12, /*(m).m13,*/	\
		(m).m20, (m).m21, (m).m22, /*(m).m23,*/	\
		(m).m30, (m).m31, (m).m32, /*(m).m33)*/	\
		)


#define DET4_10(m) DET3(\
		/*(m).m00,*/ (m).m01, (m).m02, (m).m03,	\
		/*(m).m10,   (m).m11, (m).m12, (m).m13,*/\
		/*(m).m20,*/ (m).m21, (m).m22, (m).m23,	\
		/*(m).m30,*/ (m).m31, (m).m32, (m).m33	\
		)
#define DET4_11(m) DET3(\
		(m).m00, /*(m).m01,*/ (m).m02, (m).m03,	\
		/*(m).m10, (m).m11,   (m).m12, (m).m13,*/\
		(m).m20, /*(m).m21,*/ (m).m22, (m).m23,	\
		(m).m30, /*(m).m31,*/ (m).m32, (m).m33	\
		)
#define DET4_12(m) DET3(\
		(m).m00, (m).m01, /*(m).m02,*/ (m).m03,	\
		/*(m).m10, (m).m11, (m).m12,   (m).m13,*/\
		(m).m20, (m).m21, /*(m).m22,*/ (m).m23,	\
		(m).m30, (m).m31, /*(m).m32,*/ (m).m33	\
		)
#define DET4_13(m) DET3(\
		(m).m00, (m).m01, (m).m02, /*(m).m03,*/	\
		/*(m).m10, (m).m11, (m).m12, (m).m13,*/	\
		(m).m20, (m).m21, (m).m22, /*(m).m23,*/	\
		(m).m30, (m).m31, (m).m32, /*(m).m33)*/	\
		)

#define DET4_20(m) DET3(\
		/*(m).m00,*/ (m).m01, (m).m02, (m).m03,	\
		/*(m).m10,*/ (m).m11, (m).m12, (m).m13,	\
		/*(m).m20, (m).m21, (m).m22, (m).m23,*/	\
		/*(m).m30,*/ (m).m31, (m).m32, (m).m33	\
		)
#define DET4_21(m) DET3(\
		(m).m00, /*(m).m01,*/ (m).m02, (m).m03,	\
		(m).m10, /*(m).m11,*/ (m).m12, (m).m13,	\
		/*(m).m20, (m).m21, (m).m22, (m).m23,*/	\
		(m).m30, /*(m).m31,*/ (m).m32, (m).m33	\
		)
#define DET4_22(m) DET3(\
		(m).m00, (m).m01, /*(m).m02,*/ (m).m03,	\
		(m).m10, (m).m11, /*(m).m12,*/ (m).m13,	\
		/*(m).m20, (m).m21, (m).m22, (m).m23,*/	\
		(m).m30, (m).m31, /*(m).m32,*/ (m).m33	\
		)
#define DET4_23(m) DET3(\
		(m).m00, (m).m01, (m).m02, /*(m).m03,*/	\
		(m).m10, (m).m11, (m).m12, /*(m).m13,*/	\
		/*(m).m20, (m).m21, (m).m22, (m).m23,*/	\
		(m).m30, (m).m31, (m).m32, /*(m).m33)*/	\
		)

#define DET4_30(m) DET3(\
		/*(m).m00,*/ (m).m01, (m).m02, (m).m03,	\
		/*(m).m10,*/ (m).m11, (m).m12, (m).m13,	\
		/*(m).m20,*/ (m).m21, (m).m22, (m).m23,	\
		/*(m).m30, (m).m31, (m).m32, (m).m33*/	\
		)
#define DET4_31(m) DET3(\
		(m).m00, /*(m).m01,*/ (m).m02, (m).m03,	\
		(m).m10, /*(m).m11,*/ (m).m12, (m).m13,	\
		(m).m20, /*(m).m21,*/ (m).m22, (m).m23,	\
		/*(m).m30, (m).m31, (m).m32, (m).m33*/	\
		)
#define DET4_32(m) DET3(\
		(m).m00, (m).m01, /*(m).m02,*/ (m).m03,	\
		(m).m10, (m).m11, /*(m).m12,*/ (m).m13,	\
		(m).m20, (m).m21, /*(m).m22,*/ (m).m23,	\
		/*(m).m30, (m).m31, (m).m32, (m).m33*/	\
		)
#define DET4_33(m) DET3(\
		(m).m00, (m).m01, (m).m02, /*(m).m03,*/	\
		(m).m10, (m).m11, (m).m12, /*(m).m13,*/	\
		(m).m20, (m).m21, (m).m22, /*(m).m23,*/	\
		/*(m).m30, (m).m31, (m).m32, (m).m33)*/	\
		)

#define DET4(m) (\
		(m).m00 * DET4_00(m)\
		-(m).m01 * DET4_01(m)\
		+(m).m02 * DET4_02(m)\
		-(m).m03 * DET4_03(m)\
		)

	inline float MatrixDet4(const Matrix4x4& m) {
		const int8_t Comb[24][5] = {
			{ 0, 1, 2, 3, 1 },
			{ 1, 0, 2, 3, -1 },
			{ 0, 2, 1, 3, -1 },
			{ 2, 0, 1, 3, 1 },
			{ 1, 2, 0, 3, 1 },
			{ 2, 1, 0, 3, -1 },
			{ 0, 1, 3, 2, -1 },
			{ 1, 0, 3, 2, 1 },
			{ 0, 3, 1, 2, 1 },
			{ 3, 0, 1, 2, -1 },
			{ 1, 3, 0, 2, -1 },
			{ 3, 1, 0, 2, 1 },
			{ 0, 2, 3, 1, 1 },
			{ 2, 0, 3, 1, -1 },
			{ 0, 3, 2, 1, -1 },
			{ 3, 0, 2, 1, 1 },
			{ 2, 3, 0, 1, 1 },
			{ 3, 2, 0, 1, -1 },
			{ 1, 2, 3, 0, -1 },
			{ 2, 1, 3, 0, 1 },
			{ 1, 3, 2, 0, 1 },
			{ 3, 1, 2, 0, -1 },
			{ 2, 3, 1, 0, -1 },
			{ 3, 2, 1, 0, 1 },
		};
		float sum = 0, mul;
		int i, j;
		for (i = 0; i < 24; i++) {
			mul = Comb[i][4];
			for (j = 0; j < 4; j++) {
				mul *= m.m[j][Comb[i][j]];
			}
			sum += mul;
		}
		return(sum);
	}

	inline void sincosf(const float d, float* sin, float* cos) {
		(*sin) = sinf(d);
		(*cos) = cosf(d);
	}
	inline void sincosf(const float d, float& sin, float& cos) {
		sin = sinf(d);
		cos = cosf(d);
	}


	inline Matrix4x4& RotateXYZ(Matrix4x4& ypr, Vector3D& r) {
		float cy = cosf(r.Yaw);
		float sy = sinf(r.Yaw);
		float cp = cosf(r.Pitch);
		float sp = sinf(r.Pitch);
		float cr = cosf(r.Roll);
		float sr = sinf(r.Roll);

		ypr.m00 = cr*cy;
		ypr.m01 = cy*sr;
		ypr.m02 = -sy;
		ypr.m03 = 0;
		ypr.m10 = cr*sp*sy - cp*sr;
		ypr.m11 = cp*cr + sp*sr*sy;
		ypr.m12 = cy*sp;
		ypr.m13 = 0;
		ypr.m20 = sp*sr + cp*cr*sy;
		ypr.m21 = cp*sr*sy - cr*sp;
		ypr.m22 = cp*cy;
		ypr.m23 = 0;
		ypr.m30 = 0;
		ypr.m31 = 0;
		ypr.m32 = 0;
		ypr.m33 = 1;
		return ypr;
	}
	inline Matrix4x4& RotateXZY(Matrix4x4& ypr, Vector3D& r) {
		float cy = cosf(r.Yaw);
		float sy = sinf(r.Yaw);
		float cp = cosf(r.Pitch);
		float sp = sinf(r.Pitch);
		float cr = cosf(r.Roll);
		float sr = sinf(r.Roll);

		ypr.m00 = cr*cy;
		ypr.m01 = sr;
		ypr.m02 = -cr*sy;
		ypr.m03 = 0;
		ypr.m10 = sp*sy - cp*cy*sr;
		ypr.m11 = cp*cr;
		ypr.m12 = cy*sp + cp*sr*sy;
		ypr.m13 = 0;
		ypr.m20 = cp*sy + cy*sp*sr;
		ypr.m21 = -cr*sp;
		ypr.m22 = cp*cy - sp*sr*sy;
		ypr.m23 = 0;
		ypr.m30 = 0;
		ypr.m31 = 0;
		ypr.m32 = 0;
		ypr.m33 = 1;
		return ypr;
	}
	inline Matrix4x4& RotateYXZ(Matrix4x4& ypr, Vector3D& r) {
		float cy = cosf(r.Yaw);
		float sy = sinf(r.Yaw);
		float cp = cosf(r.Pitch);
		float sp = sinf(r.Pitch);
		float cr = cosf(r.Roll);
		float sr = sinf(r.Roll);

		ypr.m00 = cr*cy - sp*sr*sy;
		ypr.m01 = cy*sr + cr*sp*sy;
		ypr.m02 = -cp*sy;
		ypr.m03 = 0;
		ypr.m10 = -cp*sr;
		ypr.m11 = cp*cr;
		ypr.m12 = sp;
		ypr.m13 = 0;
		ypr.m20 = cr*sy + cy*sp*sr;
		ypr.m21 = sr*sy - cr*cy*sp;
		ypr.m22 = cp*cy;
		ypr.m23 = 0;
		ypr.m30 = 0;
		ypr.m31 = 0;
		ypr.m32 = 0;
		ypr.m33 = 1;
		return ypr;
	}
	inline Matrix4x4& RotateYZX(Matrix4x4& ypr, Vector3D& r) {
		float cy = cosf(r.Yaw);
		float sy = sinf(r.Yaw);
		float cp = cosf(r.Pitch);
		float sp = sinf(r.Pitch);
		float cr = cosf(r.Roll);
		float sr = sinf(r.Roll);

		ypr.m00 = cr*cy;
		ypr.m01 = sp*sy + cp*cy*sr;
		ypr.m02 = cy*sp*sr - cp*sy;
		ypr.m03 = 0;
		ypr.m10 = -sr;
		ypr.m11 = cp*cr;
		ypr.m12 = cr*sp;
		ypr.m13 = 0;
		ypr.m20 = cr*sy;
		ypr.m21 = cp*sr*sy - cy*sp;
		ypr.m22 = cp*cy + sp*sr*sy;
		ypr.m23 = 0;
		ypr.m30 = 0;
		ypr.m31 = 0;
		ypr.m32 = 0;
		ypr.m33 = 1;
		return ypr;
	}
	inline Matrix4x4& RotateZXY(Matrix4x4& ypr, Vector3D& r) {
		float cy = cosf(r.Yaw);
		float sy = sinf(r.Yaw);
		float cp = cosf(r.Pitch);
		float sp = sinf(r.Pitch);
		float cr = cosf(r.Roll);
		float sr = sinf(r.Roll);

		ypr.m00 = cr*cy + sp*sr*sy;
		ypr.m01 = cp*sr;
		ypr.m02 = cy*sp*sr - cr*sy;
		ypr.m03 = 0;
		ypr.m10 = cr*sp*sy - cy*sr;
		ypr.m11 = cp*cr;
		ypr.m12 = sr*sy + cr*cy*sp;
		ypr.m13 = 0;
		ypr.m20 = cp*sy;
		ypr.m21 = -sp;
		ypr.m22 = cp*cy;
		ypr.m23 = 0;
		ypr.m30 = 0;
		ypr.m31 = 0;
		ypr.m32 = 0;
		ypr.m33 = 1;
		return ypr;
	}
	inline Matrix4x4& RotateZYX(Matrix4x4& ypr, Vector3D& r) {
		float cy = cosf(r.Yaw);
		float sy = sinf(r.Yaw);
		float cp = cosf(r.Pitch);
		float sp = sinf(r.Pitch);
		float cr = cosf(r.Roll);
		float sr = sinf(r.Roll);

		ypr.m00 = cr*cy;
		ypr.m01 = cp*sr + cr*sp*sy;
		ypr.m02 = sp*sr - cp*cr*sy;
		ypr.m03 = 0;
		ypr.m10 = -cy*sr;
		ypr.m11 = cp*cr - sp*sr*sy;
		ypr.m12 = cr*sp + cp*sr*sy;
		ypr.m13 = 0;
		ypr.m20 = sy;
		ypr.m21 = -cy*sp;
		ypr.m22 = cp*cy;
		ypr.m23 = 0;
		ypr.m30 = 0;
		ypr.m31 = 0;
		ypr.m32 = 0;
		ypr.m33 = 1;
		return ypr;
	}

	enum class RotationOrder {
		XYZ,
		XZY,
		YXZ,
		YZX,
		ZXY,
		ZYX,
	};

	typedef Matrix4x4& (*_RotationFunc2)(Matrix4x4&, Vector3D&);

	inline Matrix4x4& Rotate(RotationOrder Order, Matrix4x4& ypr, Vector3D& r) {
		constexpr _RotationFunc2 _RotationOrder[]{
			PVX::RotateXYZ,
			PVX::RotateXZY,
			PVX::RotateYXZ,
			PVX::RotateYZX,
			PVX::RotateZXY,
			PVX::RotateZYX,
		};
		return _RotationOrder[int(Order)](ypr, r);
	}

	inline Matrix4x4 Rotate_XYZ(const Vector3D& r) {
		float cy = cosf(r.Yaw);
		float sy = sinf(r.Yaw);
		float cp = cosf(r.Pitch);
		float sp = sinf(r.Pitch);
		float cr = cosf(r.Roll);
		float sr = sinf(r.Roll);

		return {
			cr*cy, cy*sr, -sy, 0,
			cr*sp*sy - cp*sr, cp*cr + sp*sr*sy, cy*sp, 0,
			sp*sr + cp*cr*sy, cp*sr*sy - cr*sp, cp*cy, 0,
			0, 0, 0, 1.0f
		};
	}
	inline Matrix4x4 Rotate_XZY(const Vector3D& r) {
		float cy = cosf(r.Yaw);
		float sy = sinf(r.Yaw);
		float cp = cosf(r.Pitch);
		float sp = sinf(r.Pitch);
		float cr = cosf(r.Roll);
		float sr = sinf(r.Roll);

		return {
			cr*cy, sr, -cr*sy, 0,
			sp*sy - cp*cy*sr, cp*cr, cy*sp + cp*sr*sy, 0,
			cp*sy + cy*sp*sr, -cr*sp, cp*cy - sp*sr*sy, 0,
			0, 0, 0, 1.0f
		};
	}
	inline Matrix4x4 Rotate_YXZ(const Vector3D& r) {
		float cy = cosf(r.Yaw);
		float sy = sinf(r.Yaw);
		float cp = cosf(r.Pitch);
		float sp = sinf(r.Pitch);
		float cr = cosf(r.Roll);
		float sr = sinf(r.Roll);

		return {
			cr*cy - sp*sr*sy, cy*sr + cr*sp*sy, -cp*sy, 0,
			-cp*sr, cp*cr, sp, 0,
			cr*sy + cy*sp*sr, sr*sy - cr*cy*sp, cp*cy, 0,
			0, 0, 0, 1.0f
		};
	}
	inline Matrix4x4 Rotate_YZX(const Vector3D& r) {
		float cy = cosf(r.Yaw);
		float sy = sinf(r.Yaw);
		float cp = cosf(r.Pitch);
		float sp = sinf(r.Pitch);
		float cr = cosf(r.Roll);
		float sr = sinf(r.Roll);

		return {
			cr*cy, sp*sy + cp*cy*sr, cy*sp*sr - cp*sy, 0,
			-sr, cp*cr, cr*sp, 0,
			cr*sy, cp*sr*sy - cy*sp, cp*cy + sp*sr*sy, 0,
			0, 0, 0, 1.0f
		};
	}
	inline Matrix4x4 Rotate_ZXY(const Vector3D& r) {
		float cy = cosf(r.Yaw);
		float sy = sinf(r.Yaw);
		float cp = cosf(r.Pitch);
		float sp = sinf(r.Pitch);
		float cr = cosf(r.Roll);
		float sr = sinf(r.Roll);

		return {
			cr*cy + sp*sr*sy, cp*sr, cy*sp*sr - cr*sy, 0,
			cr*sp*sy - cy*sr, cp*cr, sr*sy + cr*cy*sp, 0,
			cp*sy, -sp, cp*cy, 0,
			0, 0, 0, 1.0f
		};
	}
	inline Matrix4x4 Rotate_ZYX(const Vector3D& r) {
		float cy = cosf(r.Yaw);
		float sy = sinf(r.Yaw);
		float cp = cosf(r.Pitch);
		float sp = sinf(r.Pitch);
		float cr = cosf(r.Roll);
		float sr = sinf(r.Roll);

		return {
			cr*cy, cp*sr + cr*sp*sy, sp*sr - cp*cr*sy, 0,
			-cy*sr, cp*cr - sp*sr*sy, cr*sp + cp*sr*sy, 0,
			sy, -cy*sp, cp*cy, 0,
			0, 0, 0, 1.0f
		};
	}

	typedef Matrix4x4(*_RotationFunc)(const Vector3D&);

	inline Matrix4x4 Rotate(RotationOrder Order, const Vector3D& r) {
		constexpr _RotationFunc _RotationOrder[]{
			PVX::Rotate_XYZ,
			PVX::Rotate_XZY,
			PVX::Rotate_YXZ,
			PVX::Rotate_YZX,
			PVX::Rotate_ZXY,
			PVX::Rotate_ZYX,
		};
		return _RotationOrder[int(Order)](r);
	}

	inline void RotateRoll(Matrix4x4& m, float Roll) {
		float cr = cosf(Roll);
		float sr = sinf(Roll);
		m = {
			cr, sr, 0, 0,
			-sr, cr, 0, 0,
			0, 0, 1.0f, 0,
			0, 0, 0, 1.0
		};
	}

	inline Matrix4x4 RotateRoll(float Roll) {
		float cr = cosf(Roll);
		float sr = sinf(Roll);
		return {
			cr, sr, 0, 0,
			-sr, cr, 0, 0,
			0, 0, 1.0f, 0,
			0, 0, 0, 1.0
		};
	}
	inline Matrix4x4 RotateYaw(float Roll) {
		float cr = cosf(Roll);
		float sr = sinf(Roll);
		return {
			cr, 0, sr, 0,
			0, 1.0f, 0, 0,
			-sr, 0, cr, 0,
			0, 0, 0, 1.0
		};
	}
	inline Matrix4x4 RotatePitch(float Roll) {
		float cr = cosf(Roll);
		float sr = sinf(Roll);
		return {
			1.0f, 0, 0, 0,
			0, cr, sr, 0,
			0, -sr, cr, 0,
			0, 0, 0, 1.0
		};
	}

	inline void RotateYawPitchRoll(Matrix4x4& ypr, const Vector3D& r) {
		float cy = cosf(r.Yaw);
		float sy = sinf(r.Yaw);
		float cp = cosf(r.Pitch);
		float sp = sinf(r.Pitch);
		float cr = cosf(r.Roll);
		float sr = sinf(r.Roll);

		ypr.m00 = cr*cy - sp*sr*sy;
		ypr.m01 = cy*sr + cr*sp*sy;
		ypr.m02 = -cp*sy;
		ypr.m03 = 0;
		ypr.m10 = -cp*sr;
		ypr.m11 = cp*cr;
		ypr.m12 = sp;
		ypr.m13 = 0;
		ypr.m20 = cr*sy + cy*sp*sr;
		ypr.m21 = sr*sy - cr*cy*sp;
		ypr.m22 = cp*cy;
		ypr.m23 = 0;
		ypr.m30 = 0;
		ypr.m31 = 0;
		ypr.m32 = 0;
		ypr.m33 = 1;
	}

	inline PVX::Matrix4x4 RotateYawPitch(const Vector3D& r) {
		float cy = cosf(r.Yaw);
		float sy = sinf(r.Yaw);
		float cp = cosf(r.Pitch);
		float sp = sinf(r.Pitch);
		return {
			cy,	sp* sy,	-cp*sy,	0,

			0,	cp,		sp,		0,

			sy,	-cy*sp,	cp* cy,	0,

			0,	0,		0,		1.0f
		};
	}

	inline void RotateYawPitch(Matrix4x4& ypr, const Vector3D& r) {
		float cy = cosf(r.Yaw);
		float sy = sinf(r.Yaw);
		float cp = cosf(r.Pitch);
		float sp = sinf(r.Pitch);

		ypr.m00 = cy;
		ypr.m01 = sp*sy;
		ypr.m02 = -cp*sy;
		ypr.m03 = 0;

		ypr.m10 = 0;
		ypr.m11 = cp;
		ypr.m12 = sp;
		ypr.m13 = 0;

		ypr.m20 = sy;
		ypr.m21 = -cy*sp;
		ypr.m22 = cp*cy;
		ypr.m23 = 0;

		ypr.m30 = 0;
		ypr.m31 = 0;
		ypr.m32 = 0;
		ypr.m33 = 1;
	}
	/*Sets only the 3x3 part of the matrix*/
	inline void RotateYawPitchRoll2(Matrix4x4& ypr, const Vector3D& r) {
		double cy = cos(r.Yaw);
		double sy = sin(r.Yaw);
		double cp = cos(r.Pitch);
		double sp = sin(r.Pitch);
		double cr = cos(r.Roll);
		double sr = sin(r.Roll);

		ypr.m00 = (float)(cr*cy - sp*sr*sy);
		ypr.m01 = (float)(cy*sr + cr*sp*sy);
		ypr.m02 = (float)(-cp*sy);

		ypr.m10 = (float)(-cp*sr);
		ypr.m11 = (float)(cp*cr);
		ypr.m12 = (float)(sp);

		ypr.m20 = (float)(cr*sy + cy*sp*sr);
		ypr.m21 = (float)(sr*sy - cr*cy*sp);
		ypr.m22 = (float)(cp*cy);
	}
	/*Sets only the 3x3 part of the matrix*/
	inline void RotateYawPitch2(Matrix4x4& ypr, const Vector3D& r) {
		double cy = cos(r.Yaw);
		double sy = sin(r.Yaw);
		double cp = cos(r.Pitch);
		double sp = sin(r.Pitch);

		ypr.m00 = (float)(cy);
		ypr.m01 = (float)(sp*sy);
		ypr.m02 = (float)(-cp*sy);

		ypr.m10 = 0;
		ypr.m11 = (float)(cp);
		ypr.m12 = (float)(sp);

		ypr.m20 = (float)(sy);
		ypr.m21 = (float)(-cy*sp);
		ypr.m22 = (float)(cp*cy);
	}

	inline void GetRotation(const Matrix3x3& m, Vector3D& Rot) {
		Rot.Pitch = asinf(-m.m21);
		Rot.Yaw = atan2f(m.m20, m.m22);
		if (m.m01 == m.m11&&m.m01 == 0)
			Rot.Roll = -3.1415926535897932384626433832795f;
		else
			Rot.Roll = atan2f(m.m01, m.m11);
	}

	inline void GetRotation(const Matrix3x4& m, Vector3D& Rot) {
		Rot.Pitch = asinf(-m.m12);
		Rot.Yaw = atan2f(m.m02, m.m22);
		if (m.m01 == m.m11&&m.m10 == 0)
			Rot.Roll = -3.1415926535897932384626433832795f;
		else
			Rot.Roll = atan2f(m.m10, m.m11);
	}

	inline void GetRotation(const Matrix4x4& m, Vector3D& Rot) {
		Rot.Pitch = asinf(-m.m21);
		Rot.Yaw = atan2f(m.m20, m.m22);
		if (m.m01 == m.m11&&m.m01 == 0)
			Rot.Roll = -3.1415926535897932384626433832795f;
		else
			Rot.Roll = atan2f(m.m01, m.m11);
	}

	inline float Det3(const Matrix4x4& m, int r, int c) {
		const int8_t Comb[24][5] = {
			{ 0, 1, 2, 3, 1 },
			{ 1, 0, 2, 3, -1 },
			{ 0, 2, 1, 3, -1 },
			{ 2, 0, 1, 3, 1 },
			{ 1, 2, 0, 3, 1 },
			{ 2, 1, 0, 3, -1 },
			{ 0, 1, 3, 2, -1 },
			{ 1, 0, 3, 2, 1 },
			{ 0, 3, 1, 2, 1 },
			{ 3, 0, 1, 2, -1 },
			{ 1, 3, 0, 2, -1 },
			{ 3, 1, 0, 2, 1 },
			{ 0, 2, 3, 1, 1 },
			{ 2, 0, 3, 1, -1 },
			{ 0, 3, 2, 1, -1 },
			{ 3, 0, 2, 1, 1 },
			{ 2, 3, 0, 1, 1 },
			{ 3, 2, 0, 1, -1 },
			{ 1, 2, 3, 0, -1 },
			{ 2, 1, 3, 0, 1 },
			{ 1, 3, 2, 0, 1 },
			{ 3, 1, 2, 0, -1 },
			{ 2, 3, 1, 0, -1 },
			{ 3, 2, 1, 0, 1 },
		};
		char Map[4][3] = { { 1, 2, 3 }, { 0, 2, 3 }, { 0, 1, 3 }, { 0, 1, 2 } };
		float sum = 0, mul;
		int i, j;
		for (i = 0; i < 6; i++) {
			mul = Comb[i][4];
			for (j = 0; j < 3; j++) {
				mul *= m.m[Map[r][j]][Map[c][Comb[i][j]]];
			}
			sum += mul;
		}
		return(sum);
	}

	inline void MatrixInv(const Matrix4x4& in, Matrix4x4& out) {
		float idet = 1.0f / MatrixDet4(in);
		int i, j;
		for (i = 0; i < 4; i++) {
			for (j = 0; j < 4; j++) {
				out.m[j][i] = Det3(in, i, j)*idet;
				*(int*)&out.m[j][i] ^= ((i + j) << 31);
			}
		}
	}

#define Det2x2(a,b,c,d) ((a)*(d)-(c)*(b))

	inline void MatrixInv(const Matrix3x3& Mat, Matrix3x3& Inv) {
		float idet = 1.0f / (
			(Det2x2(Mat.m00, Mat.m01,
				Mat.m10, Mat.m11) * Mat.m22)
			- (Det2x2(Mat.m00, Mat.m02,
				Mat.m10, Mat.m12) * Mat.m21)
			+ (Det2x2(Mat.m01, Mat.m02,
				Mat.m11, Mat.m12) * Mat.m20)
			);

		Inv.m00 = Det2x2(Mat.m11, Mat.m21, Mat.m12, Mat.m22)*idet;
		Inv.m01 = -Det2x2(Mat.m01, Mat.m21, Mat.m02, Mat.m22)*idet;
		Inv.m02 = Det2x2(Mat.m01, Mat.m11, Mat.m02, Mat.m12)*idet;
		Inv.m10 = -Det2x2(Mat.m10, Mat.m20, Mat.m12, Mat.m22)*idet;
		Inv.m11 = Det2x2(Mat.m00, Mat.m20, Mat.m02, Mat.m22)*idet;
		Inv.m12 = -Det2x2(Mat.m00, Mat.m10, Mat.m02, Mat.m12)*idet;
		Inv.m20 = Det2x2(Mat.m10, Mat.m20, Mat.m11, Mat.m21)*idet;
		Inv.m21 = -Det2x2(Mat.m00, Mat.m20, Mat.m01, Mat.m21)*idet;
		Inv.m22 = Det2x2(Mat.m00, Mat.m10, Mat.m01, Mat.m11)*idet;
	}

	inline void MatrixInverse(const Matrix3x4& in, Matrix3x4& out) {
		Vector3D cr = in.Vec0.Vec3.Cross(in.Vec1.Vec3);
		float idet = 1.0f / in.Vec2.Vec3.Dot(cr);
		out.m00 = (in.m11 * in.m22 - in.m12 * in.m21) * idet;
		out.m10 = -(in.m10 * in.m22 - in.m12 * in.m20) * idet;
		out.m20 = (in.m10 * in.m21 - in.m11 * in.m20) * idet;
		out.m01 = -(in.m01 * in.m22 - in.m02 * in.m21) * idet;
		out.m11 = (in.m00 * in.m22 - in.m02 * in.m20) * idet;
		out.m21 = -(in.m00 * in.m21 - in.m01 * in.m20) * idet;
		out.m02 = (in.m01 * in.m12 - in.m02 * in.m11) * idet;
		out.m12 = -(in.m00 * in.m12 - in.m02 * in.m10) * idet;
		out.m22 = (in.m00 * in.m11 - in.m01 * in.m10) * idet;
		out.m03 = -(in.m01 * in.m12 * in.m23 - in.m02 * in.m11 * in.m23 - in.m01 * in.m13 * in.m22 + in.m03 * in.m11 * in.m22 + in.m02 * in.m13 * in.m21 - in.m03 * in.m12 * in.m21) * idet;
		out.m13 = (in.m00 * in.m12 * in.m23 - in.m02 * in.m10 * in.m23 - in.m00 * in.m13 * in.m22 + in.m03 * in.m10 * in.m22 + in.m02 * in.m13 * in.m20 - in.m03 * in.m12 * in.m20) * idet;
		out.m23 = -(in.m00 * in.m11 * in.m23 - in.m01 * in.m10 * in.m23 - in.m00 * in.m13 * in.m21 + in.m03 * in.m10 * in.m21 + in.m01 * in.m13 * in.m20 - in.m03 * in.m11 * in.m20) * idet;
	}

	inline void MatrixInverse(const Matrix3x4& in, Matrix3x4& out, float det) {
		float idet = 1.0f / det;
		out.m00 = (in.m11 * in.m22 - in.m12 * in.m21) * idet;
		out.m10 = -(in.m10 * in.m22 - in.m12 * in.m20) * idet;
		out.m20 = (in.m10 * in.m21 - in.m11 * in.m20) * idet;
		out.m01 = -(in.m01 * in.m22 - in.m02 * in.m21) * idet;
		out.m11 = (in.m00 * in.m22 - in.m02 * in.m20) * idet;
		out.m21 = -(in.m00 * in.m21 - in.m01 * in.m20) * idet;
		out.m02 = (in.m01 * in.m12 - in.m02 * in.m11) * idet;
		out.m12 = -(in.m00 * in.m12 - in.m02 * in.m10) * idet;
		out.m22 = (in.m00 * in.m11 - in.m01 * in.m10) * idet;
		out.m03 = -(in.m01 * in.m12 * in.m23 - in.m02 * in.m11 * in.m23 - in.m01 * in.m13 * in.m22 + in.m03 * in.m11 * in.m22 + in.m02 * in.m13 * in.m21 - in.m03 * in.m12 * in.m21) * idet;
		out.m13 = (in.m00 * in.m12 * in.m23 - in.m02 * in.m10 * in.m23 - in.m00 * in.m13 * in.m22 + in.m03 * in.m10 * in.m22 + in.m02 * in.m13 * in.m20 - in.m03 * in.m12 * in.m20) * idet;
		out.m23 = -(in.m00 * in.m11 * in.m23 - in.m01 * in.m10 * in.m23 - in.m00 * in.m13 * in.m21 + in.m03 * in.m10 * in.m21 + in.m01 * in.m13 * in.m20 - in.m03 * in.m11 * in.m20) * idet;
	}

	inline void FastMatrixInverse(const Matrix3x4& in, Matrix3x4& out, float det) {
		float idet = 1.0f / det;
		out.m00 = in.m00 * idet;
		out.m10 = in.m01 * idet;
		out.m20 = in.m02 * idet;
		out.m01 = in.m10 * idet;
		out.m11 = in.m11 * idet;
		out.m21 = in.m12 * idet;
		out.m02 = in.m20 * idet;
		out.m12 = in.m21 * idet;
		out.m22 = in.m22 * idet;
		out.m03 = -(in.m00 * in.m03 + in.m10 * in.m13 + in.m20 * in.m23) * idet;
		out.m13 = -(in.m01 * in.m03 + in.m11 * in.m13 + in.m21 * in.m23) * idet;
		out.m23 = -(in.m02 * in.m03 + in.m12 * in.m13 + in.m22 * in.m23) * idet;
	}

	inline void FastMatrixInverse(const Matrix3x4& in, Matrix3x4& out) {
		Vector3D cr = in.Vec0.Vec3.Cross(in.Vec1.Vec3);
		float idet = 1.0f / in.Vec2.Vec3.Dot(cr);
		out.m00 = in.m00 * idet;
		out.m10 = in.m01 * idet;
		out.m20 = in.m02 * idet;
		out.m01 = in.m10 * idet;
		out.m11 = in.m11 * idet;
		out.m21 = in.m12 * idet;
		out.m02 = in.m20 * idet;
		out.m12 = in.m21 * idet;
		out.m22 = in.m22 * idet;
		out.m03 = -(in.m00 * in.m03 + in.m10 * in.m13 + in.m20 * in.m23) * idet;
		out.m13 = -(in.m01 * in.m03 + in.m11 * in.m13 + in.m21 * in.m23) * idet;
		out.m23 = -(in.m02 * in.m03 + in.m12 * in.m13 + in.m22 * in.m23) * idet;
	}

	inline void FasterMatrixInverse(const Matrix3x4& in, Matrix3x4& out) {
		out.m00 = in.m00;
		out.m01 = in.m10;
		out.m02 = in.m20;
		out.m10 = in.m01;
		out.m11 = in.m11;
		out.m12 = in.m21;
		out.m20 = in.m02;
		out.m21 = in.m12;
		out.m22 = in.m22;
		out.m03 = -(in.m00 * in.m03 + in.m10 * in.m13 + in.m20 * in.m23);
		out.m13 = -(in.m01 * in.m03 + in.m11 * in.m13 + in.m21 * in.m23);
		out.m23 = -(in.m02 * in.m03 + in.m12 * in.m13 + in.m22 * in.m23);
	}

	inline void FastMatrixInverse(const Matrix4x4& in, Matrix4x4& out, float det) {
		float idet = 1.0f / det;
		out.m00 = in.m00 * idet;
		out.m10 = in.m01 * idet;
		out.m20 = in.m02 * idet;
		out.m01 = in.m10 * idet;
		out.m11 = in.m11 * idet;
		out.m21 = in.m12 * idet;
		out.m02 = in.m20 * idet;
		out.m12 = in.m21 * idet;
		out.m22 = in.m22 * idet;
		out.m03 = -(in.m00 * in.m03 + in.m10 * in.m13 + in.m20 * in.m23) * idet;
		out.m13 = -(in.m01 * in.m03 + in.m11 * in.m13 + in.m21 * in.m23) * idet;
		out.m23 = -(in.m02 * in.m03 + in.m12 * in.m13 + in.m22 * in.m23) * idet;
		out.m30 = -(in.m00 * in.m30 + in.m01 * in.m31 + in.m02 * in.m32) * idet;
		out.m31 = -(in.m10 * in.m30 + in.m11 * in.m31 + in.m12 * in.m32) * idet;
		out.m32 = -(in.m20 * in.m30 + in.m21 * in.m31 + in.m22 * in.m32) * idet;
		out.m33 = 1.0f;
	}

	inline void FastMatrixInverse(const Matrix4x4& in, Matrix4x4& out) {
		Vector3D cr = in.Vec0.Vec3.Cross(in.Vec1.Vec3);
		float idet = 1.0f / in.Vec2.Vec3.Dot(cr);

		out.m00 = in.m00 * idet;
		out.m01 = in.m10 * idet;
		out.m02 = in.m20 * idet;
		out.m03 = -(in.m00 * in.m03 + in.m10 * in.m13 + in.m20 * in.m23) * idet;
		out.m10 = in.m01 * idet;
		out.m11 = in.m11 * idet;
		out.m12 = in.m21 * idet;
		out.m13 = -(in.m01 * in.m03 + in.m11 * in.m13 + in.m21 * in.m23) * idet;
		out.m20 = in.m02 * idet;
		out.m21 = in.m12 * idet;
		out.m22 = in.m22 * idet;
		out.m23 = -(in.m02 * in.m03 + in.m12 * in.m13 + in.m22 * in.m23) * idet;
		out.m30 = -(in.m00 * in.m30 + in.m01 * in.m31 + in.m02 * in.m32) * idet;
		out.m31 = -(in.m10 * in.m30 + in.m11 * in.m31 + in.m12 * in.m32) * idet;
		out.m32 = -(in.m20 * in.m30 + in.m21 * in.m31 + in.m22 * in.m32) * idet;
		out.m33 = 1.0f;
	}

	inline void FasterMatrixInverse(const Matrix4x4& in, Matrix4x4& out) {
		out.m00 = in.m00;
		out.m10 = in.m01;
		out.m20 = in.m02;
		out.m01 = in.m10;
		out.m11 = in.m11;
		out.m21 = in.m12;
		out.m02 = in.m20;
		out.m12 = in.m21;
		out.m22 = in.m22;
		out.m03 = -(in.m00 * in.m03 + in.m10 * in.m13 + in.m20 * in.m23);
		out.m13 = -(in.m01 * in.m03 + in.m11 * in.m13 + in.m21 * in.m23);
		out.m23 = -(in.m02 * in.m03 + in.m12 * in.m13 + in.m22 * in.m23);
		out.m30 = -(in.m00 * in.m30 + in.m01 * in.m31 + in.m02 * in.m32);
		out.m31 = -(in.m10 * in.m30 + in.m11 * in.m31 + in.m12 * in.m32);
		out.m32 = -(in.m20 * in.m30 + in.m21 * in.m31 + in.m22 * in.m32);
		out.m33 = 1.0f;
	}

	inline void FastestMatrixInverse(const Matrix4x4& in, Matrix4x4& out) {
		out.m00 = in.m00;
		out.m10 = in.m01;
		out.m20 = in.m02;
		out.m01 = in.m10;
		out.m11 = in.m11;
		out.m21 = in.m12;
		out.m02 = in.m20;
		out.m12 = in.m21;
		out.m22 = in.m22;
		out.m03 = 0;
		out.m13 = 0;
		out.m23 = 0;
		out.m30 = -(in.m00 * in.m30 + in.m01 * in.m31 + in.m02 * in.m32);
		out.m31 = -(in.m10 * in.m30 + in.m11 * in.m31 + in.m12 * in.m32);
		out.m32 = -(in.m20 * in.m30 + in.m21 * in.m31 + in.m22 * in.m32);
		out.m33 = 1.0f;
	}

	inline Vector3D Mul3x3(const Vector3D& v, const Matrix4x4& m) {
		Vector3D out;
		out.x = v.x*m.m00 + v.y*m.m10 + v.z*m.m20;
		out.y = v.x*m.m01 + v.y*m.m11 + v.z*m.m21;
		out.z = v.x*m.m02 + v.y*m.m12 + v.z*m.m22;
		return out;
	}

	inline Vector3D Mul3x3(const Matrix4x4& m, const Vector3D& v) {
		Vector3D out;
		out.x = v.x*m.m00 + v.y*m.m01 + v.z*m.m02;
		out.y = v.x*m.m10 + v.y*m.m11 + v.z*m.m12;
		out.z = v.x*m.m20 + v.y*m.m21 + v.z*m.m22;
		return out;
	}

	/// Vector2

	inline void operator+=(Vector2D& v1, const Vector2D& v2) {
		v1.x += v2.x;
		v1.y += v2.y;
	}

	inline void operator-=(Vector2D& v1, const Vector2D& v2) {
		v1.x -= v2.x;
		v1.y -= v2.y;
	}

	inline void operator*=(Vector2D& v1, const Vector2D& v2) {
		v1.x *= v2.x;
		v1.y *= v2.y;
	}

	inline void operator*=(Vector2D& v1, float f) {
		v1.x *= f;
		v1.y *= f;
	}

	inline Vector2D operator+(const Vector2D& v1, const Vector2D& v2) {
		Vector2D out = v1;
		out.x += v2.x;
		out.y += v2.y;
		return out;
	}

	inline Vector2D operator-(const Vector2D& v1, const Vector2D& v2) {
		Vector2D out = v1;
		out.x -= v2.x;
		out.y -= v2.y;
		return out;
	}

	inline Vector2D operator*(const Vector2D& v1, const Vector2D& v2) {
		Vector2D out = v1;
		out.x *= v2.x;
		out.y *= v2.y;
		return out;
	}

	inline Vector2D operator*(const Vector2D& v1, float f) {
		Vector2D out = v1;
		out.x *= f;
		out.y *= f;
		return out;
	}

	inline Vector2D operator*(float f, const Vector2D& v1) {
		Vector2D out = v1;
		out.x *= f;
		out.y *= f;
		return out;
	}

	inline Vector2D operator/(const Vector2D& v1, float f) {
		Vector2D out = v1;
		f = 1.0f / f;
		out.x *= f;
		out.y *= f;
		return out;
	}

	// Vector3

	inline Vector3D& operator+=(Vector3D& v1, const Vector3D& v2) {
		v1.x += v2.x;
		v1.y += v2.y;
		v1.z += v2.z;
		return v1;
	}

	inline Vector3D& operator-=(Vector3D& v1, const Vector3D& v2) {
		v1.x -= v2.x;
		v1.y -= v2.y;
		v1.z -= v2.z;
		return v1;
	}

	inline Vector3D& operator*=(Vector3D& v1, const Vector3D& v2) {
		v1.x *= v2.x;
		v1.y *= v2.y;
		v1.z *= v2.z;
		return v1;
	}

	inline Vector3D& operator*=(Vector3D& v1, float f) {
		v1.x *= f;
		v1.y *= f;
		v1.z *= f;
		return v1;
	}

	inline Vector3D& operator/=(Vector3D& v1, float d) {
		float f = 1.0f / d;
		v1.x *= f;
		v1.y *= f;
		v1.z *= f;
		return v1;
	}

	inline Vector3D operator+(const Vector3D& v1, const Vector3D& v2) {
#ifndef PVX_NO_INTRINSICS
		return (PVX::Vector4D{
			_mm_add_ps(_mm_set_ps(0, v1.z, v1.y, v1.x), _mm_set_ps(0, v2.z, v2.y, v2.x))
		}).Vec3;
#else
		return { v1.x + v2.x, v1.y + v2.y, v1.z + v2.z };
#endif
	}

	inline Vector3D operator-(const Vector3D& v1, const Vector3D& v2) {
#ifndef PVX_NO_INTRINSICS
		return (PVX::Vector4D{
			_mm_sub_ps(_mm_set_ps(0, v1.z, v1.y, v1.x), _mm_set_ps(0, v2.z, v2.y, v2.x))
		}).Vec3;
#else
		return { v1.x - v2.x, v1.y - v2.y, v1.z - v2.z };
#endif
	}

	inline Vector3D operator*(const Vector3D& v1, const Vector3D& v2) {
#ifndef PVX_NO_INTRINSICS
		return (PVX::Vector4D{
			_mm_mul_ps(_mm_set_ps(0, v1.z, v1.y, v1.x), _mm_set_ps(0, v2.z, v2.y, v2.x))
		}).Vec3;
#else
		return { v1.x * v2.x, v1.y * v2.y, v1.z * v2.z };
#endif
	}

	inline Vector3D operator/(const Vector3D& v1, const Vector3D& v2) {
#ifndef PVX_NO_INTRINSICS
		return (PVX::Vector4D{
			_mm_div_ps(_mm_set_ps(0, v1.z, v1.y, v1.x), _mm_set_ps(0, v2.z, v2.y, v2.x))
		}).Vec3;
#else
		return { v1.x / v2.x, v1.y / v2.y, v1.z / v2.z };
#endif
	}

	inline Vector3D operator*(const Vector3D& v1, float f) {
#ifndef PVX_NO_INTRINSICS
		return (PVX::Vector4D{
			_mm_mul_ps(_mm_set_ps(0, v1.z, v1.y, v1.x), _mm_set1_ps(f))
		}).Vec3;
#else
		return { v1.x * f, v1.y * f, v1.z * f };
#endif
	}

	inline Vector3D operator*(float f, const Vector3D& v1) {
#ifndef PVX_NO_INTRINSICS
		return (PVX::Vector4D{
			_mm_mul_ps(_mm_set_ps(0, v1.z, v1.y, v1.x), _mm_set1_ps(f))
		}).Vec3;
#else
		return { v1.x * f, v1.y * f, v1.z * f };
#endif
	}

	inline Vector3D operator/(const Vector3D& v1, float f) {
#ifndef PVX_NO_INTRINSICS
		return (PVX::Vector4D{
			_mm_div_ps(_mm_set_ps(0, v1.z, v1.y, v1.x), _mm_set1_ps(f))
		}).Vec3;
#else
		return { v1.x / f, v1.y / f, v1.z / f };
#endif
		//Vector3D out = v1;
		//out.x /= f;
		//out.y /= f;
		//out.z /= f;
		//return out;
	}

	inline Vector3D GramSchmit(const Vector3D& BaseVector, const Vector3D& toBeOrthoCorrected) {
		return (toBeOrthoCorrected - BaseVector.Dot(toBeOrthoCorrected) * BaseVector).Normalized();
	}

	inline Quaternion Conjugate(const Quaternion& q) {
		return{ -q.i, -q.j, -q.k, q.r };
	}

	inline DualQuaternion Conjugate(const DualQuaternion& q) {
		return{ -q.r_i, -q.r_j, -q.r_k, q.r_r, -q.d_i, -q.d_j, -q.d_k, q.d_r };
	}

	inline Quaternion& GetMatrixQuaternion(const Matrix4x4& m, Quaternion& Out) {
		float tr = m.m00 + m.m11 + m.m22;

		if (tr > 0) {
			float S = sqrtf(tr + 1.0f) * 2.0f; // S=4*qw 
			Out.r = 0.25f * S;
			Out.i = (m.m21 - m.m12) / S;
			Out.j = (m.m02 - m.m20) / S;
			Out.k = (m.m10 - m.m01) / S;
		} else if ((m.m00 > m.m11)&&(m.m00 > m.m22)) {
			float S = sqrt(1.0f + m.m00 - m.m11 - m.m22) * 2.0f; // S=4*Out.i 
			Out.r = (m.m21 - m.m12) / S;
			Out.i = 0.25f * S;
			Out.j = (m.m01 + m.m10) / S;
			Out.k = (m.m02 + m.m20) / S;
		} else if (m.m11 > m.m22) {
			float S = sqrt(1.0f + m.m11 - m.m00 - m.m22) * 2.0f; // S=4*Out.j
			Out.r = (m.m02 - m.m20) / S;
			Out.i = (m.m01 + m.m10) / S;
			Out.j = 0.25f * S;
			Out.k = (m.m12 + m.m21) / S;
		} else {
			float S = sqrt(1.0f + m.m22 - m.m00 - m.m11) * 2.0f; // S=4*Out.k
			Out.r = (m.m10 - m.m01) / S;
			Out.i = (m.m02 + m.m20) / S;
			Out.j = (m.m12 + m.m21) / S;
			Out.k = 0.25f * S;
		}

		return Out;
	}

	/*
	inline Vector operator*(Vector & v, Matrix4x4 & m){
	Vector out;
	out.x=v.x*m.m[0][0] + v.y*m.m[1][0] + v.z*m.m[2][0] + m.m[3][0];
	out.y=v.x*m.m[0][1] + v.y*m.m[1][1] + v.z*m.m[2][1] + m.m[3][1];
	out.z=v.x*m.m[0][2] + v.y*m.m[1][2] + v.z*m.m[2][2] + m.m[3][2];
	return out;
	}*/

	inline PVX::Vector3D operator*(const Matrix4x4& m, const Vector3D& v) {
		return {
			m.m00 * v.x + m.m01 * v.y + m.m02 * v.z,
			m.m10 * v.x + m.m11 * v.y + m.m12 * v.z,
			m.m20 * v.x + m.m21 * v.y + m.m22 * v.z
		};
	}


	inline Vector3D operator*(const Vector3D& v, const Matrix4x4& m) {
#ifndef PVX_NO_INTRINSICS
		float out[4];
		__m128 to, t;
		__m128 vc = _mm_load_ps1(&v.x);
		__m128 mc = _mm_loadu_ps(&m.m00);
		to = _mm_mul_ps(vc, mc);

		vc = _mm_load_ps1(&v.y);
		mc = _mm_loadu_ps(&m.m10);
		t = _mm_mul_ps(vc, mc);
		to = _mm_add_ps(to, t);

		vc = _mm_load_ps1(&v.z);
		mc = _mm_loadu_ps(&m.m20);
		t = _mm_mul_ps(vc, mc);
		to = _mm_add_ps(to, t);

		mc = _mm_loadu_ps(&m.m30);
		to = _mm_add_ps(to, mc);

		_mm_storeu_ps(out, to);
		return *(Vector3D*)out;
#else
		return{
			v.x*m.m00 + v.y*m.m10 + v.z*m.m20 + m.m30,
			v.x*m.m01 + v.y*m.m11 + v.z*m.m21 + m.m31,
			v.x*m.m02 + v.y*m.m12 + v.z*m.m22 + m.m32
		};
#endif
	}

	inline Vector2D operator*(const Vector2D& v, const Matrix4x4& m) {
		Vector2D out = {
			v.x*m.m00 + v.y*m.m10 + m.m30,
			v.x*m.m01 + v.y*m.m11 + m.m31
		};
		return out;
	}

	inline Vector2D operator*(const Matrix4x4& m, const Vector2D& v) {
		Vector2D out = {
			v.x*m.m00 + v.y*m.m01 + m.m03,
			v.x*m.m10 + v.y*m.m11 + m.m13
		};
		return out;
	}

	inline Vector3D operator*(const Vector3D& v, const Matrix3x4& m) {
		Vector3D out;
		out.x = m.Vec0.Vec3.Dot(v) + m.Vec0.w;
		out.y = m.Vec1.Vec3.Dot(v) + m.Vec1.w;
		out.z = m.Vec2.Vec3.Dot(v) + m.Vec2.w;
		return out;
	}

	inline Vector3D operator*(const Matrix3x4& m, const Vector3D& v) {
		Vector3D out;
		out.x = m.Vec0.Vec3.Dot(v) + m.Vec0.w;
		out.y = m.Vec1.Vec3.Dot(v) + m.Vec1.w;
		out.z = m.Vec2.Vec3.Dot(v) + m.Vec2.w;
		return out;
	}



	inline Vector3D operator*(const Vector3D& v, const Matrix3x3& m) {
		Vector3D out;
		out.x = v.x*m.m00 + v.y*m.m10 + v.z*m.m20;
		out.y = v.x*m.m01 + v.y*m.m11 + v.z*m.m21;
		out.z = v.x*m.m02 + v.y*m.m12 + v.z*m.m22;
		return out;
	}

	inline Vector3D operator*(const Matrix3x3& m, const Vector3D& v) {
		Vector3D out;
		out.x = v.x*m.m00 + v.y*m.m01 + v.z*m.m02;
		out.y = v.x*m.m10 + v.y*m.m11 + v.z*m.m12;
		out.z = v.x*m.m20 + v.y*m.m21 + v.z*m.m22;
		return out;
	}



	inline bool operator==(const Vector3D& v1, const Vector3D& v2) {
		return((v1.x == v2.x) && (v1.y == v2.y) && (v1.z == v2.z));
	}

	inline bool operator!=(const Vector3D& v1, const Vector3D& v2) {
		return((v1.x != v2.x) || (v1.y != v2.y) || (v1.z != v2.z));
	}

	inline bool operator==(const Vector2D& v1, const Vector2D& v2) {
		return((v1.u == v2.u) && (v1.v == v2.v));
	}

	inline bool operator!=(const Vector2D& v1, const Vector2D& v2) {
		return((v1.u != v2.u) || (v1.v != v2.v));
	}


	inline bool operator==(const Weight& w1, const Weight& w2) {
		int i;
		for (i = 0; (i < 3) && (w1.I[i] == w2.I[i]) && (w1.W[i] == w2.W[i]); i++);
		return(w1.I[i] == w2.I[i] && w1.W[i] == w2.W[i]);
	}

	//inline Matrix4x4 operator*(const Matrix4x4& m1, const Matrix4x4& m2) {		
	//	PVX::Matrix4x4 ret;

	//	const __m128 m10 = _mm_load_ps(m1.Vec0.Array);
	//	const __m128 m11 = _mm_load_ps(m1.Vec1.Array);
	//	const __m128 m12 = _mm_load_ps(m1.Vec2.Array);
	//	const __m128 m13 = _mm_load_ps(m1.Vec3.Array);

	//	for (int i = 0; i<4; i++) {
	//		__m128 a = _mm_mul_ps(_mm_set1_ps(m2.m4[i].x), m10);
	//		__m128 b = _mm_mul_ps(_mm_set1_ps(m2.m4[i].y), m11);
	//		__m128 c = _mm_mul_ps(_mm_set1_ps(m2.m4[i].z), m12);
	//		__m128 d = _mm_mul_ps(_mm_set1_ps(m2.m4[i].w), m13);
	//		_mm_store_ps(ret.m4[i].Array, _mm_add_ps(_mm_add_ps(a, b), _mm_add_ps(c, d)));
	//	}

	//	return ret;
	//}

#ifndef PVX_NO_INTRINSICS
	inline __m256 twolincomb_AVX_8(__m256 A01, const PVX::Matrix4x4& B) {
		__m256 result;
		result = _mm256_mul_ps(_mm256_shuffle_ps(A01, A01, 0x00), _mm256_broadcast_ps((__m128*)B.Vec0.Array));
		result = _mm256_add_ps(result, _mm256_mul_ps(_mm256_shuffle_ps(A01, A01, 0x55), _mm256_broadcast_ps((__m128*)B.Vec1.Array)));
		result = _mm256_add_ps(result, _mm256_mul_ps(_mm256_shuffle_ps(A01, A01, 0xaa), _mm256_broadcast_ps((__m128*)B.Vec2.Array)));
		result = _mm256_add_ps(result, _mm256_mul_ps(_mm256_shuffle_ps(A01, A01, 0xff), _mm256_broadcast_ps((__m128*)B.Vec3.Array)));
		return result;
	}

	// this should be noticeably faster with actual 256-bit wide vector units (Intel);
	// not sure about double-pumped 128-bit (AMD), would need to check.
	inline void matmult_AVX_8(PVX::Matrix4x4& out, const PVX::Matrix4x4& A, const PVX::Matrix4x4& B) {
		_mm256_zeroupper();
		__m256 A01 = _mm256_loadu_ps(&A.m[0][0]);
		__m256 A23 = _mm256_loadu_ps(&A.m[2][0]);

		__m256 out01x = twolincomb_AVX_8(A01, B);
		__m256 out23x = twolincomb_AVX_8(A23, B);

		_mm256_storeu_ps(&out.m[0][0], out01x);
		_mm256_storeu_ps(&out.m[2][0], out23x);
	}
	inline Matrix4x4 operator*(const Matrix4x4& m1, const Matrix4x4& m2) {
		PVX::Matrix4x4 ret;
		matmult_AVX_8(ret, m2, m1);
		return ret;
	}
#endif

	//inline Matrix4x4 operator*(const Matrix4x4& m1, const Matrix4x4& m2) {
	//	return Matrix4x4{
	//		m2.m00*m1.m00 + m2.m01*m1.m10 + m2.m02*m1.m20 + m2.m03 * m1.m30,
	//		m2.m00*m1.m01 + m2.m01*m1.m11 + m2.m02*m1.m21 + m2.m03 * m1.m31,
	//		m2.m00*m1.m02 + m2.m01*m1.m12 + m2.m02*m1.m22 + m2.m03 * m1.m32,
	//		m2.m00*m1.m03 + m2.m01*m1.m13 + m2.m02*m1.m23 + m2.m03 * m1.m33,
	//		m2.m10*m1.m00 + m2.m11*m1.m10 + m2.m12*m1.m20 + m2.m13 * m1.m30,
	//		m2.m10*m1.m01 + m2.m11*m1.m11 + m2.m12*m1.m21 + m2.m13 * m1.m31,
	//		m2.m10*m1.m02 + m2.m11*m1.m12 + m2.m12*m1.m22 + m2.m13 * m1.m32,
	//		m2.m10*m1.m03 + m2.m11*m1.m13 + m2.m12*m1.m23 + m2.m13 * m1.m33,
	//		m2.m20*m1.m00 + m2.m21*m1.m10 + m2.m22*m1.m20 + m2.m23 * m1.m30,
	//		m2.m20*m1.m01 + m2.m21*m1.m11 + m2.m22*m1.m21 + m2.m23 * m1.m31,
	//		m2.m20*m1.m02 + m2.m21*m1.m12 + m2.m22*m1.m22 + m2.m23 * m1.m32,
	//		m2.m20*m1.m03 + m2.m21*m1.m13 + m2.m22*m1.m23 + m2.m23 * m1.m33,
	//		m2.m30*m1.m00 + m2.m31*m1.m10 + m2.m32*m1.m20 + m2.m33 * m1.m30,
	//		m2.m30*m1.m01 + m2.m31*m1.m11 + m2.m32*m1.m21 + m2.m33 * m1.m31,
	//		m2.m30*m1.m02 + m2.m31*m1.m12 + m2.m32*m1.m22 + m2.m33 * m1.m32,
	//		m2.m30*m1.m03 + m2.m31*m1.m13 + m2.m32*m1.m23 + m2.m33 * m1.m33
	//	};
	//}

	inline void MatrixMutily(Matrix4x4& out, const Matrix4x4& m1, const Matrix4x4& m2) {
		out.m00 = m2.m00*m1.m00 + m2.m01*m1.m10 + m2.m02*m1.m20 + m2.m03 * m1.m30;
		out.m01 = m2.m00*m1.m01 + m2.m01*m1.m11 + m2.m02*m1.m21 + m2.m03 * m1.m31;
		out.m02 = m2.m00*m1.m02 + m2.m01*m1.m12 + m2.m02*m1.m22 + m2.m03 * m1.m32;
		out.m03 = m2.m00*m1.m03 + m2.m01*m1.m13 + m2.m02*m1.m23 + m2.m03 * m1.m33;

		out.m10 = m2.m10*m1.m00 + m2.m11*m1.m10 + m2.m12*m1.m20 + m2.m13 * m1.m30;
		out.m11 = m2.m10*m1.m01 + m2.m11*m1.m11 + m2.m12*m1.m21 + m2.m13 * m1.m31;
		out.m12 = m2.m10*m1.m02 + m2.m11*m1.m12 + m2.m12*m1.m22 + m2.m13 * m1.m32;
		out.m13 = m2.m10*m1.m03 + m2.m11*m1.m13 + m2.m12*m1.m23 + m2.m13 * m1.m33;

		out.m20 = m2.m20*m1.m00 + m2.m21*m1.m10 + m2.m22*m1.m20 + m2.m23 * m1.m30;
		out.m21 = m2.m20*m1.m01 + m2.m21*m1.m11 + m2.m22*m1.m21 + m2.m23 * m1.m31;
		out.m22 = m2.m20*m1.m02 + m2.m21*m1.m12 + m2.m22*m1.m22 + m2.m23 * m1.m32;
		out.m23 = m2.m20*m1.m03 + m2.m21*m1.m13 + m2.m22*m1.m23 + m2.m23 * m1.m33;

		out.m30 = m2.m30*m1.m00 + m2.m31*m1.m10 + m2.m32*m1.m20 + m2.m33 * m1.m30;
		out.m31 = m2.m30*m1.m01 + m2.m31*m1.m11 + m2.m32*m1.m21 + m2.m33 * m1.m31;
		out.m32 = m2.m30*m1.m02 + m2.m31*m1.m12 + m2.m32*m1.m22 + m2.m33 * m1.m32;
		out.m33 = m2.m30*m1.m03 + m2.m31*m1.m13 + m2.m32*m1.m23 + m2.m33 * m1.m33;
	}

	inline void MatrixMutily_M4x3_x_M3x3(Matrix4x4& out, const Matrix4x4& m1, const Matrix4x4& m2) {
		out.m00 = m2.m00*m1.m00 + m2.m01*m1.m10 + m2.m02*m1.m20;
		out.m01 = m2.m00*m1.m01 + m2.m01*m1.m11 + m2.m02*m1.m21;
		out.m02 = m2.m00*m1.m02 + m2.m01*m1.m12 + m2.m02*m1.m22;
		out.m03 = 0;

		out.m10 = m2.m10*m1.m00 + m2.m11*m1.m10 + m2.m12*m1.m20;
		out.m11 = m2.m10*m1.m01 + m2.m11*m1.m11 + m2.m12*m1.m21;
		out.m12 = m2.m10*m1.m02 + m2.m11*m1.m12 + m2.m12*m1.m22;
		out.m13 = 0;

		out.m20 = m2.m20*m1.m00 + m2.m21*m1.m10 + m2.m22*m1.m20;
		out.m21 = m2.m20*m1.m01 + m2.m21*m1.m11 + m2.m22*m1.m21;
		out.m22 = m2.m20*m1.m02 + m2.m21*m1.m12 + m2.m22*m1.m22;
		out.m23 = 0;

		out.m30 = m1.m30;
		out.m31 = m1.m31;
		out.m32 = m1.m32;
		out.m33 = 1.0f;
	}

#define _M4x3_x_M3x3_Init(m1, m2){\
		m2.m00*m1.m00 + m2.m01*m1.m10 + m2.m02*m1.m20,\
		m2.m00*m1.m01 + m2.m01*m1.m11 + m2.m02*m1.m21,\
		m2.m00*m1.m02 + m2.m01*m1.m12 + m2.m02*m1.m22,\
		0,\
		m2.m10*m1.m00 + m2.m11*m1.m10 + m2.m12*m1.m20,\
		m2.m10*m1.m01 + m2.m11*m1.m11 + m2.m12*m1.m21,\
		m2.m10*m1.m02 + m2.m11*m1.m12 + m2.m12*m1.m22,\
		0,\
		m2.m20*m1.m00 + m2.m21*m1.m10 + m2.m22*m1.m20,\
		m2.m20*m1.m01 + m2.m21*m1.m11 + m2.m22*m1.m21,\
		m2.m20*m1.m02 + m2.m21*m1.m12 + m2.m22*m1.m22,\
		0,\
		m1.m30,\
		m1.m31,\
		m1.m32,\
		1.0f\
	}

#define _ScaleMatrix(mat, s){\
		(mat).m00 *= (s).x;\
		(mat).m01 *= (s).x;\
		(mat).m02 *= (s).x;\
		(mat).m03 *= (s).x;\
		\
		(mat).m10 *= (s).y;\
		(mat).m11 *= (s).y;\
		(mat).m12 *= (s).y;\
		(mat).m13 *= (s).y;\
		\
		(mat).m20 *= (s).z;\
		(mat).m21 *= (s).z;\
		(mat).m22 *= (s).z;\
		(mat).m23 *= (s).z;\
	}

#define _TranslateMatrix(mat, p){\
		(mat).m30+=((p).x * (mat).m00 + (p).y * (mat).m10 + (p).z * (mat).m20);\
		(mat).m31+=((p).x * (mat).m01 + (p).y * (mat).m11 + (p).z * (mat).m21);\
		(mat).m32+=((p).x * (mat).m02 + (p).y * (mat).m12 + (p).z * (mat).m22);\
	}

#define _ScaleMatrix_Init(mat, s){\
		(mat).m00 * (s).x,\
		(mat).m01 * (s).x,\
		(mat).m02 * (s).x,\
		(mat).m03 * (s).x,\
		\
		(mat).m10 * (s).y,\
		(mat).m11 * (s).y,\
		(mat).m12 * (s).y,\
		(mat).m13 * (s).y,\
		\
		(mat).m20 * (s).z,\
		(mat).m21 * (s).z,\
		(mat).m22 * (s).z,\
		(mat).m23 * (s).z,\
		\
		(mat).m30,\
		(mat).m31,\
		(mat).m32,\
		(mat).m33\
	}
	/*
	ret = mat * | 1   0   0   0 |
				| 0   1   0   0 |
				| 0   0   1   0 |
				| p.x p.y p.z 1 |
	*/
#define _TranslateMatrix_Init(mat, p){\
		(mat).m00,\
		(mat).m01,\
		(mat).m02,\
		(mat).m03,\
		\
		(mat).m10,\
		(mat).m11,\
		(mat).m12,\
		(mat).m13,\
		\
		(mat).m20,\
		(mat).m21,\
		(mat).m22,\
		(mat).m23,\
		\
		(mat).m30+((p).x * (mat).m00 + (p).y * (mat).m10 + (p).z * (mat).m20),\
		(mat).m31+((p).x * (mat).m01 + (p).y * (mat).m11 + (p).z * (mat).m21),\
		(mat).m32+((p).x * (mat).m02 + (p).y * (mat).m12 + (p).z * (mat).m22),\
		(mat).m33\
	}

	inline Matrix3x4 operator*(const Matrix3x4& m1, const Matrix3x4& m2) {
		return Matrix3x4{
			m1.m00*m2.m00 + m1.m01*m2.m10 + m1.m02*m2.m20,
			m1.m00*m2.m01 + m1.m01*m2.m11 + m1.m02*m2.m21,
			m1.m00*m2.m02 + m1.m01*m2.m12 + m1.m02*m2.m22,
			m1.m00*m2.m03 + m1.m01*m2.m13 + m1.m02*m2.m23 + m1.m03,

			m1.m10*m2.m00 + m1.m11*m2.m10 + m1.m12*m2.m20,
			m1.m10*m2.m01 + m1.m11*m2.m11 + m1.m12*m2.m21,
			m1.m10*m2.m02 + m1.m11*m2.m12 + m1.m12*m2.m22,
			m1.m10*m2.m03 + m1.m11*m2.m13 + m1.m12*m2.m23 + m1.m13,

			m1.m20*m2.m00 + m1.m21*m2.m10 + m1.m22*m2.m20,
			m1.m20*m2.m01 + m1.m21*m2.m11 + m1.m22*m2.m21,
			m1.m20*m2.m02 + m1.m21*m2.m12 + m1.m22*m2.m22,
			m1.m20*m2.m03 + m1.m21*m2.m13 + m1.m22*m2.m23 + m1.m23
		};
		//Matrix3x4 out;
		//out.m00 = m1.m00*m2.m00 + m1.m01*m2.m10 + m1.m02*m2.m20;
		//out.m01 = m1.m00*m2.m01 + m1.m01*m2.m11 + m1.m02*m2.m21;
		//out.m02 = m1.m00*m2.m02 + m1.m01*m2.m12 + m1.m02*m2.m22;
		//out.m03 = m1.m00*m2.m03 + m1.m01*m2.m13 + m1.m02*m2.m23 + m1.m03;

		//out.m10 = m1.m10*m2.m00 + m1.m11*m2.m10 + m1.m12*m2.m20;
		//out.m11 = m1.m10*m2.m01 + m1.m11*m2.m11 + m1.m12*m2.m21;
		//out.m12 = m1.m10*m2.m02 + m1.m11*m2.m12 + m1.m12*m2.m22;
		//out.m13 = m1.m10*m2.m03 + m1.m11*m2.m13 + m1.m12*m2.m23 + m1.m13;

		//out.m20 = m1.m20*m2.m00 + m1.m21*m2.m10 + m1.m22*m2.m20;
		//out.m21 = m1.m20*m2.m01 + m1.m21*m2.m11 + m1.m22*m2.m21;
		//out.m22 = m1.m20*m2.m02 + m1.m21*m2.m12 + m1.m22*m2.m22;
		//out.m23 = m1.m20*m2.m03 + m1.m21*m2.m13 + m1.m22*m2.m23 + m1.m23;

		//return out;
	}

	inline Vector3D VectorMin(const Vector3D& a, const Vector3D& b) {
		Vector3D min;
		*((unsigned int*)&min.x) = (*(unsigned int*)&a.x & -(a.x < b.x)) | (*(unsigned int*)&b.x & -(b.x <= a.x));
		*((unsigned int*)&min.y) = (*(unsigned int*)&a.y & -(a.y < b.y)) | (*(unsigned int*)&b.y & -(b.y <= a.y));
		*((unsigned int*)&min.z) = (*(unsigned int*)&a.z & -(a.z < b.z)) | (*(unsigned int*)&b.z & -(b.z <= a.z));
		return min;
	}

	inline Vector3D VectorMax(const Vector3D& a, const Vector3D& b) {
		Vector3D max;
		*((unsigned int*)&max.x) = (*(unsigned int*)&a.x & -(a.x > b.x)) | (*(unsigned int*)&b.x & -(b.x >= a.x));
		*((unsigned int*)&max.y) = (*(unsigned int*)&a.y & -(a.y > b.y)) | (*(unsigned int*)&b.y & -(b.y >= a.y));
		*((unsigned int*)&max.z) = (*(unsigned int*)&a.z & -(a.z > b.z)) | (*(unsigned int*)&b.z & -(b.z >= a.z));
		return max;
	}

	inline Vector4D VectorMin(const Vector4D& a, const Vector4D& b) {
		Vector4D min;
		*((unsigned int*)&min.x) = (*(unsigned int*)&a.x & -(a.x < b.x)) | (*(unsigned int*)&b.x & -(b.x <= a.x));
		*((unsigned int*)&min.y) = (*(unsigned int*)&a.y & -(a.y < b.y)) | (*(unsigned int*)&b.y & -(b.y <= a.y));
		*((unsigned int*)&min.z) = (*(unsigned int*)&a.z & -(a.z < b.z)) | (*(unsigned int*)&b.z & -(b.z <= a.z));
		*((unsigned int*)&min.w) = (*(unsigned int*)&a.w & -(a.w < b.w)) | (*(unsigned int*)&b.w & -(b.w <= a.w));
		return min;
	}

	inline Vector4D VectorMax(const Vector4D& a, const Vector4D& b) {
		Vector4D max;
		*((unsigned int*)&max.x) = (*(unsigned int*)&a.x & -(a.x > b.x)) | (*(unsigned int*)&b.x & -(b.x >= a.x));
		*((unsigned int*)&max.y) = (*(unsigned int*)&a.y & -(a.y > b.y)) | (*(unsigned int*)&b.y & -(b.y >= a.y));
		*((unsigned int*)&max.z) = (*(unsigned int*)&a.z & -(a.z > b.z)) | (*(unsigned int*)&b.z & -(b.z >= a.z));
		*((unsigned int*)&max.z) = (*(unsigned int*)&a.z & -(a.z > b.z)) | (*(unsigned int*)&b.z & -(b.z >= a.z));
		return max;
	}

	inline Vector3D Orbit(float u, float v) {
		Vector3D Ret;
		float usin, ucos, vsin, vcos;
		sincosf(u, &usin, &ucos);
		sincosf(v, &vsin, &vcos);
		Ret.x = usin*vcos;
		Ret.y = vsin;
		Ret.z = vcos*ucos;
		return Ret;
	}

	inline Matrix4x4 rYaw(float y) {
		float c = 0, s = 0;
		sincosf(y, s, c);
		return {
			c, 0, -s, 0,
			0, 1, 0, 0,
			s, 0, c, 0,
			0, 0, 0, 1
		};
	}

	inline Matrix4x4 rPitch(float p) {
		float c = 0, s = 0;
		sincosf(p, s, c);
		return {
			1, 0, 0, 0,
			0, c, s, 0,
			0, -s, c, 0,
			0, 0, 0, 1
		};
	}

	inline Matrix4x4 rRoll(float r) {
		float c = 0, s = 0;
		sincosf(r, &s, &c);
		return {
			c, s, 0, 0,
			-s, c, 0, 0,
			0, 0, 1, 0,
			0, 0, 0, 1
		};
	}

	inline constexpr Matrix4x4 mTran(const Vector3D& p) {
		return {
			1, 0, 0, 0,
			0, 1, 0, 0,
			0, 0, 1, 0,
			p.x, p.y, p.z, 1
		};
	}

	inline constexpr Matrix4x4 mScale(const Vector3D& s) {
		return {
			s.x, 0, 0, 0,
			0, s.y, 0, 0,
			0, 0, s.z, 0,
			0, 0, 0, 1
		};
	}

	inline void MatrixTranspose(Matrix4x4& m) {
		std::swap(m.m01, m.m10);
		std::swap(m.m02, m.m20);
		std::swap(m.m03, m.m30);

		std::swap(m.m12, m.m21);
		std::swap(m.m13, m.m31);

		std::swap(m.m23, m.m32);
	}

		//#define MatStoreDiagonal(mat, v){\
		//	(mat).m00=(v).x;\
		//	(mat).m11=(v).y;\
		//	(mat).m22=(v).z;}

		//#define _MatStoreDiagonal(mat, x, y, z){\
		//	(mat).m00=x;\
		//	(mat).m11=y;\
		//	(mat).m22=z;}

	inline Quaternion operator+(const Quaternion& q1, const Quaternion& q2) {
		return{ q1.i + q2.i, q1.j + q2.j, q1.k + q2.k, q1.r + q2.r };
	}

	inline Quaternion operator-(const Quaternion& q1, const Quaternion& q2) {
		return{ q1.i - q2.i, q1.j - q2.j, q1.k - q2.k, q1.r - q2.r };
	}

	inline Quaternion operator*(float x, const Quaternion& q) {
		return{ q.i*x, q.j*x, q.k*x, q.r*x };
	}

	inline Quaternion operator*(const Quaternion& q, float x) {
		return{ q.i*x, q.j*x, q.k*x, q.r*x };
	}

	inline Quaternion operator*(const Quaternion& q1, const Quaternion& q2) {
		return{
			q1.i*q2.r + q1.r*q2.i + q1.j*q2.k - q1.k*q2.j,
			q1.r*q2.j - q1.i*q2.k + q1.j*q2.r + q1.k*q2.i,
			q1.r*q2.k + q1.i*q2.j - q1.j*q2.i + q1.k*q2.r,
			q1.r*q2.r - q1.i*q2.i - q1.j*q2.j - q1.k*q2.k
		};
	}

	inline Quaternion& operator*=(Quaternion& q1, const Quaternion& q2) {
		q1 = {
			q1.i*q2.r + q1.r*q2.i + q1.j*q2.k - q1.k*q2.j,
			q1.r*q2.j - q1.i*q2.k + q1.j*q2.r + q1.k*q2.i,
			q1.r*q2.k + q1.i*q2.j - q1.j*q2.i + q1.k*q2.r,
			q1.r*q2.r - q1.i*q2.i - q1.j*q2.j - q1.k*q2.k
		};
		return q1;
	}

	inline DualQuaternion operator+(const DualQuaternion& q1, const DualQuaternion& q2) {
		return{
			q1.r_i + q2.r_i, q1.r_j + q2.r_j, q1.r_k + q2.r_k, q1.r_r + q2.r_r,
			q1.d_i + q2.d_i, q1.d_j + q2.d_j, q1.d_k + q2.d_k, q1.d_r + q2.d_r
		};
	}
	inline DualQuaternion operator-(const DualQuaternion& q1, const DualQuaternion& q2) {
		return{
			q1.r_i - q2.r_i, q1.r_j - q2.r_j, q1.r_k - q2.r_k, q1.r_r - q2.r_r,
			q1.d_i - q2.d_i, q1.d_j - q2.d_j, q1.d_k - q2.d_k, q1.d_r - q2.d_r
		};
	}

	inline DualQuaternion operator*(const DualQuaternion& dq, const float factor) {
		return{
			dq.r_i * factor, dq.r_j * factor, dq.r_k * factor, dq.r_r * factor,
			dq.d_i * factor, dq.d_j * factor, dq.d_k * factor, dq.d_r * factor
		};
	}

	inline DualQuaternion operator*(const DualQuaternion& dq1, const DualQuaternion& dq2) {
		return {
			dq1.r_i*dq2.r_r + dq1.r_j*dq2.r_k + dq1.r_r*dq2.r_i - dq1.r_k*dq2.r_j,
			dq1.r_r*dq2.r_j - dq1.r_i*dq2.r_k + dq1.r_j*dq2.r_r + dq1.r_k*dq2.r_i,
			dq1.r_r*dq2.r_k + dq1.r_i*dq2.r_j - dq1.r_j*dq2.r_i + dq1.r_k*dq2.r_r,
			dq1.r_r*dq2.r_r - dq1.r_i*dq2.r_i - dq1.r_j*dq2.r_j - dq1.r_k*dq2.r_k,
			dq1.d_j*dq2.r_k - dq1.d_k*dq2.r_j - dq1.r_k*dq2.d_j + dq1.r_j*dq2.d_k + dq1.d_i*dq2.r_r + dq1.r_r*dq2.d_i + dq1.d_r*dq2.r_i + dq1.r_i*dq2.d_r,
			dq1.r_k*dq2.d_i - dq1.d_i*dq2.r_k + dq1.d_k*dq2.r_i - dq1.r_i*dq2.d_k + dq1.d_j*dq2.r_r + dq1.r_r*dq2.d_j + dq1.d_r*dq2.r_j + dq1.r_j*dq2.d_r,
			dq1.d_i*dq2.r_j - dq1.d_j*dq2.r_i - dq1.r_j*dq2.d_i + dq1.r_i*dq2.d_j + dq1.d_k*dq2.r_r + dq1.r_r*dq2.d_k + dq1.d_r*dq2.r_k + dq1.r_k*dq2.d_r,
			dq1.d_r*dq2.r_r - dq1.r_i*dq2.d_i - dq1.d_j*dq2.r_j - dq1.r_j*dq2.d_j - dq1.d_k*dq2.r_k - dq1.r_k*dq2.d_k - dq1.d_i*dq2.r_i + dq1.r_r*dq2.d_r
		};
	}
	inline DualQuaternion operator*=(DualQuaternion& dq1, const DualQuaternion& dq2) {
		return dq1 = dq1 * dq2;
	}

	inline void toEulerianAngle(const Quaternion& q, Vector3D& Rotation) {
		float ysqr = q.j * q.j;

		float t2 = 2.0f * (q.r * q.j - q.k * q.i);
		t2 = t2 > 1.0f ? 1.0f : t2;
		t2 = t2 < -1.0f ? -1.0f : t2;
		Rotation.Pitch = asin(t2);

		Rotation.Yaw = atan2(2.0f * (q.r * q.k + q.i * q.j), 1.0f - 2.0f * (ysqr + q.k * q.k));

		Rotation.Roll = atan2(2.0f * (q.r * q.i + q.j * q.k), 1.0f - 2.0f * (q.i * q.i + ysqr));
	}

	inline Quaternion CreateQuaternionXYZ(const Vector3D& rot) {
		float cosRoll, cosYaw, cosPitch, sinRoll, sinYaw, sinPitch;
		sincosf(rot.Yaw * 0.5f, &sinYaw, &cosYaw);
		sincosf(rot.Pitch * 0.5f, &sinPitch, &cosPitch);
		sincosf(rot.Roll * 0.5f, &sinRoll, &cosRoll);
		return{
			cosYaw*cosRoll*sinPitch - cosPitch*sinYaw*sinRoll,
			cosRoll*cosPitch*sinYaw + cosYaw*sinRoll*sinPitch,
			cosYaw*cosPitch*sinRoll - cosRoll*sinYaw*sinPitch,
			cosYaw*cosRoll*cosPitch + sinYaw*sinRoll*sinPitch
		};
	}

	inline Quaternion CreateQuaternionXZY(const Vector3D& rot) {
		float cosRoll, cosYaw, cosPitch, sinRoll, sinYaw, sinPitch;
		sincosf(rot.Yaw * 0.5f, &sinYaw, &cosYaw);
		sincosf(rot.Pitch * 0.5f, &sinPitch, &cosPitch);
		sincosf(rot.Roll * 0.5f, &sinRoll, &cosRoll);
		return{
			cosYaw*cosRoll*sinPitch + cosPitch*sinYaw*sinRoll,
			cosRoll*cosPitch*sinYaw + cosYaw*sinRoll*sinPitch,
			cosYaw*cosPitch*sinRoll - cosRoll*sinYaw*sinPitch,
			cosYaw*cosRoll*cosPitch - sinYaw*sinRoll*sinPitch
		};
	}

	inline Quaternion CreateQuaternionYXZ(const Vector3D& rot) {
		float cosRoll, cosYaw, cosPitch, sinRoll, sinYaw, sinPitch;
		sincosf(rot.Yaw * 0.5f, &sinYaw, &cosYaw);
		sincosf(rot.Pitch * 0.5f, &sinPitch, &cosPitch);
		sincosf(rot.Roll * 0.5f, &sinRoll, &cosRoll);
		return{
			cosYaw*cosRoll*sinPitch - cosPitch*sinYaw*sinRoll,
			cosRoll*cosPitch*sinYaw + cosYaw*sinRoll*sinPitch,
			cosYaw*cosPitch*sinRoll + cosRoll*sinYaw*sinPitch,
			cosYaw*cosRoll*cosPitch - sinYaw*sinRoll*sinPitch
		};
	}

	inline Quaternion CreateQuaternionYZX(const Vector3D& rot) {
		float cosRoll, cosYaw, cosPitch, sinRoll, sinYaw, sinPitch;
		sincosf(rot.Yaw * 0.5f, &sinYaw, &cosYaw);
		sincosf(rot.Pitch * 0.5f, &sinPitch, &cosPitch);
		sincosf(rot.Roll * 0.5f, &sinRoll, &cosRoll);
		return{
			cosYaw*cosRoll*sinPitch - cosPitch*sinYaw*sinRoll,
			cosRoll*cosPitch*sinYaw - cosYaw*sinRoll*sinPitch,
			cosYaw*cosPitch*sinRoll + cosRoll*sinYaw*sinPitch,
			cosYaw*cosRoll*cosPitch + sinYaw*sinRoll*sinPitch
		};
	}

	inline Quaternion CreateQuaternionZXY(const Vector3D& rot) {
		float cosRoll, cosYaw, cosPitch, sinRoll, sinYaw, sinPitch;
		sincosf(rot.Yaw * 0.5f, &sinYaw, &cosYaw);
		sincosf(rot.Pitch * 0.5f, &sinPitch, &cosPitch);
		sincosf(rot.Roll * 0.5f, &sinRoll, &cosRoll);
		return{
			cosYaw*cosRoll*sinPitch + cosPitch*sinYaw*sinRoll,
			cosRoll*cosPitch*sinYaw - cosYaw*sinRoll*sinPitch,
			cosYaw*cosPitch*sinRoll - cosRoll*sinYaw*sinPitch,
			cosYaw*cosRoll*cosPitch + sinYaw*sinRoll*sinPitch
		};
	}

	inline Quaternion CreateQuaternionZYX(const Vector3D& rot) {
		float cosRoll, cosYaw, cosPitch, sinRoll, sinYaw, sinPitch;
		sincosf(rot.Yaw * 0.5f, &sinYaw, &cosYaw);
		sincosf(rot.Pitch * 0.5f, &sinPitch, &cosPitch);
		sincosf(rot.Roll * 0.5f, &sinRoll, &cosRoll);
		return{
			cosYaw*cosRoll*sinPitch + cosPitch*sinYaw*sinRoll,
			cosRoll*cosPitch*sinYaw - cosYaw*sinRoll*sinPitch,
			cosYaw*cosPitch*sinRoll + cosRoll*sinYaw*sinPitch,
			cosYaw*cosRoll*cosPitch - sinYaw*sinRoll*sinPitch
		};
	}

	/////////////// Dual /////////////////////////////

	inline DualQuaternion CreateDualQuaternionXYZ(const Vector3D& rot, const Vector3D& pos) {
		Quaternion Real = CreateQuaternionXYZ(rot);
		return{
			Real.i,
			Real.j,
			Real.k,
			Real.r,
			(pos.x*Real.r + pos.y*Real.k - pos.z*Real.j)*0.5f,
			(-pos.x*Real.k + pos.y*Real.r + pos.z*Real.i)*0.5f,
			(pos.x*Real.j - pos.y*Real.i + pos.z*Real.r)*0.5f,
			(-pos.x*Real.i - pos.y*Real.j - pos.z*Real.k)*0.5f
		};
	}

	inline DualQuaternion CreateDualQuaternionXZY(const Vector3D& rot, const Vector3D& pos) {
		Quaternion Real = CreateQuaternionXZY(rot);
		return{
			Real.i,
			Real.j,
			Real.k,
			Real.r,
			(pos.x * Real.r + pos.y * Real.k - pos.z * Real.j) * 0.5f,
			(-pos.x * Real.k + pos.y * Real.r + pos.z * Real.i) * 0.5f,
			(pos.x * Real.j - pos.y * Real.i + pos.z * Real.r) * 0.5f,
			(-pos.x * Real.i - pos.y * Real.j - pos.z * Real.k) * 0.5f
		};
	}

	inline DualQuaternion CreateDualQuaternionYXZ(const Vector3D& rot, const Vector3D& pos) {
		Quaternion Real = CreateQuaternionYXZ(rot);
		return{
			Real.i,
			Real.j,
			Real.k,
			Real.r,
			(pos.x * Real.r + pos.y * Real.k - pos.z * Real.j) * 0.5f,
			(-pos.x * Real.k + pos.y * Real.r + pos.z * Real.i) * 0.5f,
			(pos.x * Real.j - pos.y * Real.i + pos.z * Real.r) * 0.5f,
			(-pos.x * Real.i - pos.y * Real.j - pos.z * Real.k) * 0.5f
		};
	}

	inline DualQuaternion CreateDualQuaternionYZX(const Vector3D& rot, const Vector3D& pos) {
		Quaternion Real = CreateQuaternionYZX(rot);
		return{
			Real.i,
			Real.j,
			Real.k,
			Real.r,
			(pos.x * Real.r + pos.y*Real.k - pos.z * Real.j) * 0.5f,
			(-pos.x * Real.k + pos.y*Real.r + pos.z * Real.i) * 0.5f,
			(pos.x * Real.j - pos.y*Real.i + pos.z * Real.r) * 0.5f,
			(-pos.x * Real.i - pos.y*Real.j - pos.z * Real.k) * 0.5f
		};
	}

	inline DualQuaternion CreateDualQuaternionZXY(const Vector3D& rot, const Vector3D& pos) {
		Quaternion Real = CreateQuaternionZXY(rot);
		return{
			Real.i,
			Real.j,
			Real.k,
			Real.r,
			(pos.x*Real.r + pos.y*Real.k - pos.z*Real.j) * 0.5f,
			(-pos.x*Real.k + pos.y*Real.r + pos.z*Real.i) * 0.5f,
			(pos.x*Real.j - pos.y*Real.i + pos.z*Real.r) * 0.5f,
			(-pos.x*Real.i - pos.y*Real.j - pos.z*Real.k) * 0.5f
		};
	}

	inline DualQuaternion CreateDualQuaternionZYX(const Vector3D& rot, const Vector3D& pos) {
		float  cy, cp, cr, sy, sp, sr;
		sincosf(rot.Yaw*0.5f, &sy, &cy);
		sincosf(rot.Pitch*0.5f, &sp, &cp);
		sincosf(rot.Roll*0.5f, &sr, &cr);
		Quaternion Real = {
			cp*cy*sr - cr*sp*sy,
			cr*cy*sp - cp*sr*sy,
			cp*cr*sy + cy*sp*sr,
			cp*cr*cy + sp*sr*sy
		};
		return{
			Real.i,
			Real.j,
			Real.k,
			Real.r,
			(pos.x*Real.r + pos.y*Real.k - pos.z*Real.j)*0.5f,
			(-pos.x*Real.k + pos.y*Real.r + pos.z*Real.i)*0.5f,
			(pos.x*Real.j - pos.y*Real.i + pos.z*Real.r)*0.5f,
			(-pos.x*Real.i - pos.y*Real.j - pos.z*Real.k)*0.5f
		};
	}

	////////////////////////////////////////////////////////////////////

	inline float RayPointDistance(const Ray& r, const Vector3D& p) {
		auto dif = (r.Position - p);
		return (dif - r.Direction * r.Direction.Dot(dif)).Length();
	}
	inline float RayPointDistanceSquared(const Ray& r, const Vector3D& p) {
		auto dif = (r.Position - p);
		return (dif - r.Direction * r.Direction.Dot(dif)).Length2();
	}
	inline float RayPointDistance(const Ray2D& r, const Vector2D& p) {
		auto dif = r.Position - p;
		return (dif - r.Direction * r.Direction.Dot(dif)).Length();
	}
	inline float RayPointDistanceSquared(const Ray2D& r, const Vector2D& p) {
		auto dif = (r.Position - p);
		return (dif - r.Direction * r.Direction.Dot(dif)).Length2();
	}
	inline float PointPlaneDistanse(const Vector3D& p, const Plane& Plane) {
		return fabsf(p.Dot(Plane.Normal) + Plane.Distance);
	}
	inline float PointPlaneSignedDistanse(const Vector3D& p, const Plane& Plane) {
		return p.Dot(Plane.Normal) + Plane.Distance;
	}
	inline float RayRayDistance(const Ray& r1, const Ray& r2) {
		if (fabsf(r1.Direction.Dot(r2.Direction)) < 1.0f) {
			Vector3D dir = r1.Direction.Cross(r2.Direction).Normalized();
			Vector3D dif = r1.Position - r2.Position;
			return fabsf(dif.Dot(dir));
		} else {
			return RayPointDistance(r1, r2.Position);
		}
	}

	inline Vector3D RayPlaneIntersection(const Ray& Ray, const Plane& Plane) {
		float num = Plane.Distance - Ray.Position.Dot(Plane.Normal);
		float denom = Ray.Direction.Dot(Plane.Normal);
		return (num / denom)*Ray.Direction + Ray.Position;
	}

	inline Vector3D RayPlaneIntersectionZ(const Ray& Ray, const Matrix4x4& Plane) {
		Vector3D d = Plane.Vec3.Vec3 - Ray.Position;
		float num = d.Dot(Plane.Vec2.Vec3);
		float denom = Ray.Direction.Dot(Plane.Vec2.Vec3);

		Vector3D ret = { 0, 0, num / denom };
		if (denom) {
			Vector3D inter = {
				(ret.z * Ray.Direction.x - d.x),
				(ret.z * Ray.Direction.y - d.y),
				(ret.z * Ray.Direction.z - d.z)
			};
			ret.x = inter.Dot(Plane.Vec0.Vec3);
			ret.y = inter.Dot(Plane.Vec1.Vec3);
		}
		return ret;
	}

	inline Vector3D RayPlaneIntersectionY(const Ray& Ray, const Matrix4x4& Plane) {
		Vector3D d = Plane.Vec3.Vec3 - Ray.Position;
		float num = d.Dot(Plane.Vec1.Vec3);
		float denom = Ray.Direction.Dot(Plane.Vec1.Vec3);

		Vector3D ret = { 0, 0, num / denom };
		if (denom) {
			Vector3D inter = {
				(ret.z * Ray.Direction.x - d.x),
				(ret.z * Ray.Direction.y - d.y),
				(ret.z * Ray.Direction.z - d.z)
			};
			ret.x = inter.Dot(Plane.Vec0.Vec3);
			ret.y = inter.Dot(Plane.Vec2.Vec3);
		}
		return ret;
	}

	inline float RayPlaneIntersectionDistance(const Ray& Ray, const Plane& Plane) {
		float num = Plane.Distance - Ray.Position.Dot(Plane.Normal);
		float denom = Ray.Direction.Dot(Plane.Normal);
		return (num / denom);
	}

	inline Plane CreatePlane(const Vector3D& a, const Vector3D& b, const Vector3D& c) {
		auto vec = (c - a).Cross(b - a).Normalized();
		return { vec, -vec.Dot(a) };
	}

	inline Vector3D TriangleNormal(const Vector3D& a, const Vector3D& b, const Vector3D& c) {
		return (c - a).Cross(b - a).Normalized();
	}

	inline float TriangleArea(const Vector3D& a, const Vector3D& b, const Vector3D& c) {
		return (c - a).Cross(b - a).Length() *0.5f;
	}

	inline Plane CreatePlaneZ(const Vector3D& Position, const Vector3D& Rotation) {
		Plane ret;
		float cy = cosf(Rotation.Yaw);
		float sy = sinf(Rotation.Yaw);
		float cp = cosf(Rotation.Pitch);
		float sp = sinf(Rotation.Pitch);

		ret.Normal.x = -cp*sy;
		ret.Normal.y = sp;
		ret.Normal.z = cp*cy;

		ret.Distance = -Position.Dot(ret.Normal);
		return ret;
	}

	inline Plane CreatePlaneY(const Vector3D& Position, const Vector3D& Rotation) {
		Plane ret;
		float cy = cosf(Rotation.Yaw);
		float sy = sinf(Rotation.Yaw);
		float cp = cosf(Rotation.Pitch);
		float sp = sinf(Rotation.Pitch);

		ret.Normal.x = sp*sy;
		ret.Normal.y = cp;
		ret.Normal.z = -cy*sp;

		ret.Distance = -Position.Dot(ret.Normal);
		return ret;
	}

	inline Plane Matrix_PlaneZ(const Matrix4x4& m) {
		Plane ret;
		ret.Normal.x = m.m02;
		ret.Normal.y = m.m12;
		ret.Normal.z = m.m22;
		ret.Distance = -m.Vec3.Vec3.Dot(ret.Normal);
		return ret;
	}

	inline Plane Matrix_PlaneY(const Matrix4x4& m) {
		Plane ret;
		ret.Normal.x = m.m01;
		ret.Normal.y = m.m11;
		ret.Normal.z = m.m21;
		ret.Distance = -m.Vec3.Vec3.Dot(ret.Normal);
		return ret;
	}

	inline Plane Matrix_PlaneX(const Matrix4x4& m) {
		Plane ret;
		ret.Normal.x = m.m00;
		ret.Normal.y = m.m10;
		ret.Normal.z = m.m20;
		ret.Distance = -m.Vec3.Vec3.Dot(ret.Normal);
		return ret;
	}

	inline Matrix4x4 MatrixFromPlaneZ(const Plane& Plane, const Vector3D& Position) {
		Vector3D Pos = RayPlaneIntersection(Ray{ Position, Plane.Normal }, Plane);
		Vector3D Y{ 0, 1.0f, 0 };
		Vector3D X{ 1.0f, 0, 0 };
		if (fabsf(Y.Dot(Plane.Normal)) < 0.9f) {
			X = Y.Cross(Plane.Normal).Normalized();
			Y = Plane.Normal.Cross(X).Normalized();
			return Matrix4x4{
				X.x, Y.x, Plane.Normal.x, 0,
				X.y, Y.y, Plane.Normal.y, 0,
				X.z, Y.z, Plane.Normal.z, 0,
				Pos.x, Pos.y, Pos.z, 1.0f
			};
		} else {
			Y = Plane.Normal.Cross(X).Normalized();
			X = Y.Cross(Plane.Normal).Normalized();
			return Matrix4x4{
				X.x, Y.x, Plane.Normal.x, 0,
				X.y, Y.y, Plane.Normal.y, 0,
				X.z, Y.z, Plane.Normal.z, 0,
				Pos.x, Pos.y, Pos.z, 1.0f
			};
		}
	}

	inline Vector3D Vector_Average(const Vector3D* List, size_t Size) {
		Vector3D Sum{ 0, 0, 0 };
		for (auto i = 0; i < Size; i++) {
			Sum.x += List[i].x;
			Sum.y += List[i].y;
			Sum.z += List[i].z;
		}
		float ic = 1.0f / Size;
		return Vector3D{ Sum.x*ic, Sum.y*ic, Sum.y*ic };
	}

	inline float Vector_Variance(const Vector3D* List, size_t Size, Vector3D& Mean) {
		float ret = 0;
		for (auto i = 0; i < Size; i++) {
			ret += (Mean - List[i]).Length2();
		}
		return ret / Size;
	}

	inline float Vector_Variance(const Vector3D* List, size_t Size) {
		Vector3D Sum{ 0, 0, 0 };
		for (auto i = 0; i < Size; i++) {
			Sum.x += List[i].x;
			Sum.y += List[i].y;
			Sum.z += List[i].z;
		}
		float ic = 1.0f / Size;
		Vector3D Mean{ Sum.x*ic, Sum.y*ic, Sum.z*ic };
		float ret = 0;
		for (auto i = 0; i < Size; i++) {
			ret += (Mean - List[i]).Length2();
		}
		return ret * ic;
	}

	inline Matrix3x3& CovarienceMatrix(Matrix3x3& out, Vector3D& Avg, const Vector3D* Vertices, int Count) {
		out = Matrix3x3{ 0 };
		Avg = Vector3D{ 0,0,0 };
		for (int i = 0; i < Count; i++) {
			Avg.x += Vertices[i].x;
			Avg.y += Vertices[i].y;
			Avg.z += Vertices[i].z;
		}
		float invCount = 1.0f / Count;
		Avg.x *= invCount;
		Avg.y *= invCount;
		Avg.z *= invCount;

		Matrix3x3 m2{ 0 };
		Vector3D m;

		for (int i = 0; i < Count; i++) {
			m.x = Vertices[i].x - Avg.x;
			m.y = Vertices[i].y - Avg.y;
			m.z = Vertices[i].z - Avg.z;

			m2.m00 += m.x * m.x;	m2.m01 += m.x * m.y;	m2.m02 += m.x * m.z;
			/* m10            */	m2.m11 += m.y * m.y;	m2.m12 += m.y * m.z;
			/* m20            */	/* m21            */	m2.m22 += m.z * m.z;
		}
		out.m00 = m2.m00 * invCount;	out.m01 = m2.m01 * invCount;	out.m02 = m2.m02 * invCount;
		out.m10 = out.m01; /* m10 */	out.m11 = m2.m11 * invCount;	out.m12 = m2.m12 * invCount;
		out.m20 = out.m02; /* m20 */	out.m21 = out.m12; /* m21 */	out.m22 = m2.m22 * invCount;

		return out;
	}

	inline Matrix4x4& GetQuaternionMatrix2(const Quaternion& q, Matrix4x4& Out) {
		Out.m22 = 1.0f - 2.0f * q.j * q.j - 2.0f * q.k * q.k;
		Out.m12 = 2.0f * q.i * q.j + 2.0f * q.k * q.r;
		Out.m02 = 2.0f * q.i * q.k - 2.0f * q.j * q.r;

		Out.m21 = 2.0f * q.i * q.j - 2.0f * q.k * q.r;
		Out.m11 = 1.0f - 2.0f * q.i * q.i - 2.0f * q.k * q.k;
		Out.m01 = 2.0f * q.j * q.k + 2.0f * q.i * q.r;

		Out.m20 = 2.0f * q.i * q.k + 2.0f * q.j * q.r;
		Out.m10 = 2.0f * q.j * q.k - 2.0f * q.i * q.r;
		Out.m00 = 1.0f - 2.0f * q.i * q.i - 2.0f * q.j * q.j;

		Out.m03 = Out.m13 = Out.m23 = Out.m30 = Out.m31 = Out.m32 = 0;
		Out.m33 = 1.0f;
		return Out;
	}

	inline Matrix4x4& GetQuaternionMatrix_3x3(const Quaternion& q, Matrix4x4& Out) {
		Out.m00 = 1.0f - 2.0f * q.j * q.j - 2.0f * q.k * q.k;
		Out.m01 = 2.0f * q.i * q.j + 2.0f * q.k*q.r;
		Out.m02 = 2.0f * q.i * q.k - 2.0f * q.j*q.r;

		Out.m10 = 2.0f * q.i * q.j - 2.0f * q.k * q.r;
		Out.m11 = 1.0f - 2.0f * q.i * q.i - 2.0f * q.k * q.k;
		Out.m12 = 2.0f * q.j * q.k + 2.0f * q.i *q.r;

		Out.m20 = 2.0f * q.i * q.k + 2.0f * q.j * q.r;
		Out.m21 = 2.0f * q.j * q.k - 2.0f * q.i * q.r;
		Out.m22 = 1.0f - 2.0f * q.i * q.i - 2.0f * q.j * q.j;

		return Out;
	}

	inline Matrix4x4& GetQuaternionMatrix(const Quaternion& q, Matrix4x4& Out) {
		Out.m00 = 1.0f - 2.0f * q.j * q.j - 2.0f * q.k * q.k;
		Out.m01 = 2.0f * q.i * q.j + 2.0f * q.k*q.r;
		Out.m02 = 2.0f * q.i * q.k - 2.0f * q.j*q.r;
		Out.m03 = 0;

		Out.m10 = 2.0f * q.i * q.j - 2.0f * q.k * q.r;
		Out.m11 = 1.0f - 2.0f * q.i * q.i - 2.0f * q.k * q.k;
		Out.m12 = 2.0f * q.j * q.k + 2.0f * q.i *q.r;
		Out.m13 = 0;

		Out.m20 = 2.0f * q.i * q.k + 2.0f * q.j * q.r;
		Out.m21 = 2.0f * q.j * q.k - 2.0f * q.i * q.r;
		Out.m22 = 1.0f - 2.0f * q.i * q.i - 2.0f * q.j * q.j;
		Out.m23 = 0;

		Out.m30 = 0;
		Out.m31 = 0;
		Out.m32 = 0;
		Out.m33 = 1.0f;
		return Out;
	}

	inline Matrix4x4 GetMatrix(const Quaternion& q) {
		return {
			1.0f -	2.0f * (q.j * q.j + q.k * q.k),
					2.0f * (q.i * q.j + q.k * q.r),
					2.0f * (q.i * q.k - q.j * q.r),
			0,

					2.0f * (q.i * q.j - q.k * q.r),
			1.0f -	2.0f * (q.i * q.i + q.k * q.k),
					2.0f * (q.j * q.k + q.i * q.r),
			0,

					2.0f * (q.i * q.k + q.j * q.r),
					2.0f * (q.j * q.k - q.i * q.r),
			1.0f -	2.0f * (q.i * q.i + q.j * q.j),
			0,

			0, 0, 0, 1.0f
		};
	}

	inline DualQuaternion CreateDualQuaternion(const Quaternion& Rotation, const Vector3D& Position) {
		return{
			Rotation.i,
			Rotation.j,
			Rotation.k,
			Rotation.r,
			(Position.x*Rotation.r + Position.y*Rotation.k - Position.z*Rotation.j)*0.5f,
			(-Position.x*Rotation.k + Position.y*Rotation.r + Position.z*Rotation.i)*0.5f,
			(Position.x*Rotation.j - Position.y*Rotation.i + Position.z*Rotation.r)*0.5f,
			(-Position.x*Rotation.i - Position.y*Rotation.j - Position.z*Rotation.k)*0.5f
		};
	}

	inline Matrix4x4 GetDualQuaternionMatrix2(const DualQuaternion& q) {
		Matrix4x4 ret;
		GetQuaternionMatrix(q.Real, ret);
		auto t = (q.Dual * 2.0f) * Conjugate(q.Real);
		ret.TranslateBefore(t.Vec.Vec3);
		return ret;
	}

	inline Matrix4x4 GetMatrix(const DualQuaternion& q) {
		Matrix4x4 ret;
		GetQuaternionMatrix(q.Real, ret);
		auto t = (q.Dual * 2.0f) * Conjugate(q.Real);
		ret.m30 = t.i;
		ret.m31 = t.j;
		ret.m32 = t.k;
		return ret;
	}

	inline Matrix4x4& GetDualQuaternionMatrix(const DualQuaternion& q, Matrix4x4& ret) {
		GetQuaternionMatrix(q.Real, ret);
		auto t = (q.Dual * 2.0f) * Conjugate(q.Real);
		ret.m30 = t.i;
		ret.m31 = t.j;
		ret.m32 = t.k;
		return ret;
	}

}
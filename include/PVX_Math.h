#pragma once
#include <cmath>
#include <vector>

namespace PVX::Math {
	struct Complex {
		double Real, Imaginary;

		inline Complex operator+(const Complex& x) const {
			return { Real + x.Real, Imaginary + x.Imaginary };
		}
		inline Complex operator-(const Complex& x) const {
			return { Real - x.Real, Imaginary - x.Imaginary };
		}
		inline Complex operator*(const double& x) const {
			return { Real * x, Imaginary * x };
		}
		inline Complex operator*(const Complex& x) const {
			return {
				Real * x.Real - Imaginary * x.Imaginary,
				Real * x.Imaginary + Imaginary * x.Real
			};
		}
		inline Complex& operator+=(const Complex& x) {
			Real += x.Real;
			Imaginary += x.Imaginary;
			return *this;
		}
		inline Complex& operator-=(const Complex& x) {
			Real -= x.Real;
			Imaginary -= x.Imaginary;
			return *this;
		}
		inline Complex& operator*=(const Complex& x) {
			Complex y = *this;
			Real = y.Real * x.Real - y.Imaginary * x.Imaginary;
			Imaginary = y.Real * x.Imaginary + y.Imaginary * x.Real;
			return *this;
		}
		inline Complex& operator/=(const Complex& x) {
			double inv = 1.0f / (x.Real * x.Real - x.Imaginary * x.Imaginary);
			Complex i2{
				x.Real * inv,
				-x.Imaginary * inv
			};
			(*this)* i2;
			return (*this);
		}
		inline double Length2() const {
			return Real*Real + Imaginary*Imaginary;
		}
		inline double Length() const {
			return sqrt(Real*Real + Imaginary*Imaginary);
		}
		inline double Phase() const {
			return atan2(Imaginary, Real);
		}
		inline Complex operator/(const Complex&& x) {
			double inv = 1.0f / (x.Real * x.Real - x.Imaginary * x.Imaginary);
			return {
				(Real*x.Real + Imaginary * x.Imaginary) * inv,
				(Imaginary*x.Real + Real * x.Imaginary) * inv
			};
		}
		inline Complex Conjugate() const {
			return { Real, -Imaginary };
		}
		static inline Complex Make(double length, double phase) {
			return { length * cos(phase), length * sin(phase) };
		}
	};

	inline Complex operator/(double a, const Complex& x) {
		double inv = 1.0f / (x.Real * x.Real - x.Imaginary * x.Imaginary);
		return {
			x.Real * inv,
			-x.Imaginary * inv
		};
	}

	std::vector<Complex> FFT(const float* Array, size_t Size, int Window = 0, int Step = 0);
	std::vector<Complex> FFT(const std::vector<float>& Array, int Window = 0, int Step = 0);
	std::vector<float> Length(const std::vector<Complex> & Coefs);
	std::vector<float> Phase(const std::vector<Complex> & Coefs);
}
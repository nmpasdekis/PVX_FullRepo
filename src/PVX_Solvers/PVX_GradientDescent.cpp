#include <PVX_Solvers.h>
#include <Eigen/Eigen>

namespace PVX::Solvers {

	inline size_t GetModelSize(const std::vector<std::pair<float*, size_t>>& m) {
		size_t ret = 0;
		for (auto [d, s]:m)ret += s;
		return ret;
	}
	inline void Memcpy(float* dst, const std::vector<std::pair<float*, size_t>>& m) {
		for (auto [f, s]: m) {
			memcpy(dst, f, s * sizeof(float));
			dst += s;
		}
	}
	inline void Memcpy(std::vector<std::pair<float*, size_t>>& dst, const float* m) {
		float* src = (float*)m;
		for (auto [f, s]: dst) {
			memcpy(f, src, s * sizeof(float));
			src += s;
		}
	}

	GradientDescent::GradientDescent(
		std::function<float()> ErrorFunction,
		float* Model, size_t ModelSize,
		float LearnRate, float Momentum, float RMSprop
	) : GradientDescent(ErrorFunction, { {Model, ModelSize} }, LearnRate, Momentum, RMSprop) {}


	void ForEach(std::vector<std::pair<float*, size_t>>& Model, std::function<void(float&)> fnc) {
		for (auto [w, sz] : Model) {
			for (auto i = 0; i<sz; i++) {
				fnc(w[i]);
			}
		}
	}

	float GradientDescent::Iterate(float dx) {
		size_t c = 0;
		ForEach(Updater, [&](float& w) {
			float save = w;
			w += dx;
			float nError = ErrorFnc();
			vGradient[c++] = (Error - nError)/dx;
			w = save;
		});

		for (auto i = 0; i<vGradient.size(); i++) {
			float& g = vGradient[i];
			float& rms = vRMSprop[i];
			rms = RMSprop * rms + iRMSprop * g*g;
			g /= sqrtf(rms+1e-8f);
			vMomentum[i] = vMomentum[i] * Momentum + LearnRate * g;
		}

		c = 0;
		ForEach(Updater, [&](float& w) {
			w += vMomentum[c++];
		});
		
		Error = ErrorFnc();
		if (Error<LastError) {
			LastError = Error;
			Memcpy(SavePoint.data(), Updater);
		}
		if (Error / LastError > 1.1f) {
			ClearMomentum();
			Error = LastError;
			Memcpy(Updater, SavePoint.data());
		}
		return Error;
	}

	void GradientDescent::RecalculateError() {
		Error = ErrorFnc();
	}

	void GradientDescent::ClearMomentum() {
		//for (auto& m: vMomentum)m *= 1.0f - Momentum;
		//for (auto& r: vRMSprop)r = RMSprop + r * iRMSprop;

		for (auto& m: vMomentum)m = 0;
		for (auto& r: vRMSprop)r = 1.0f;
	}

	GradientDescent::GradientDescent(
		std::function<float()> ErrorFunction,
		std::vector<std::pair<float*, size_t>> Model,
		float LearnRate, float Momentum, float RMSprop
	) : ErrorFnc{ ErrorFunction },
		Updater{ Model },
		vRMSprop(GetModelSize(Model), 1.0f),
		vMomentum(GetModelSize(Model)),
		vGradient(GetModelSize(Model)),
		SavePoint(GetModelSize(Model)),
		ModelSize{ GetModelSize(Model) },
		LearnRate{ LearnRate * (1.0f - Momentum) },
		Momentum{ Momentum },
		RMSprop{ RMSprop },
		iRMSprop{ 1.0f - RMSprop },
		Error{ ErrorFunction() }
	{ LastError = Error; }

	struct LM_internalData {
		size_t N; // param count
		size_t M; // residual count

		std::span<float> params;
		std::function<void(std::span<const float>, std::span<float>)> errorFunc;

		// Buffers
		Eigen::VectorXf r;      // current residual
		Eigen::VectorXf r_tmp;  // temp residual
		Eigen::MatrixXf J;      // Jacobian
		Eigen::MatrixXf A;      // J^T J
		Eigen::VectorXf g;      // J^T r
		Eigen::VectorXf delta;  // step

		float lambda = 1e-3f;
		float nu = 10.0f;
		float epsilon = 1e-6f;
	};

	LevenbergMarquardt::LevenbergMarquardt(
		std::span<float> Params,
		size_t residualCount,
		std::function<void(std::span<const float>, std::span<float>)> ErrorFunction
	) {
		Data = std::make_unique<LM_internalData>();

		Data->params = Params;
		Data->N = Params.size();
		Data->M = residualCount;
		Data->errorFunc = ErrorFunction;

		Data->r.resize(Data->M);
		Data->r_tmp.resize(Data->M);
		Data->J.resize(Data->M, Data->N);
		Data->A.resize(Data->N, Data->N);
		Data->g.resize(Data->N);
		Data->delta.resize(Data->N);

		// Initial residual
		Data->errorFunc(Data->params, std::span<float>(Data->r.data(), Data->M));
	}
	float LevenbergMarquardt::Iterate(float dt_override) {
		auto& d = *Data;

		float dt;

		for(size_t i = 0; i < d.N; ++i) {
			float orig = d.params[i];
			dt = dt_override > 0 ? dt_override : d.epsilon * std::max(1.0f, std::abs(orig));
			d.params[i] = orig + dt;
			d.errorFunc(d.params, std::span<float>(d.r_tmp.data(), d.M));
			Eigen::VectorXf r_plus = d.r_tmp;
			d.params[i] = orig - dt;
			d.errorFunc(d.params, std::span<float>(d.r_tmp.data(), d.M));
			Eigen::VectorXf r_minus = d.r_tmp;
			d.params[i] = orig;
			d.J.col(i) = (r_plus - r_minus) / (2.0f * dt);
		}

		d.A = d.J.transpose() * d.J;
		d.g = d.J.transpose() * d.r;

		Eigen::MatrixXf A_damped = d.A;
		A_damped.diagonal() += d.lambda * d.A.diagonal();

		d.delta = -A_damped.ldlt().solve(d.g);
		std::vector<float> backup(d.params.begin(), d.params.end());
		for(size_t i = 0; i < d.N; ++i)
			d.params[i] += d.delta[i];

		d.errorFunc(d.params, std::span<float>(d.r_tmp.data(), d.M));

		float oldError = d.r.squaredNorm();
		float newError = d.r_tmp.squaredNorm();

		if(newError < oldError) {
			d.r = d.r_tmp;
			d.lambda /= d.nu;
		} else {
			for(size_t i = 0; i < d.N; ++i)
				d.params[i] = backup[i];

			d.lambda *= d.nu;
		}

		return d.r.squaredNorm();
	}
}
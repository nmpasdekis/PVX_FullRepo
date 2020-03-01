#include <PVX_Solvers.h>

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
			ClearMomentum();
			LastError = Error;
			Memcpy(SavePoint.data(), Updater);
		}
		if (Error / LastError > 1.1f) {
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
}
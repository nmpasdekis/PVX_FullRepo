#include <PVX_Solvers.h>
#include <iostream>
#include <limits>

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



	GeneticSolver::GeneticSolver(std::function<float()> ErrorFunction, float* Model, size_t ModelSize, int Population, int Survive, float MutationVariance, float MutateProbability, float Combine) :
		GeneticSolver(ErrorFunction, { {Model, ModelSize} }, Population, Survive, MutationVariance, MutateProbability, Combine) {}

	GeneticSolver::GeneticSolver(
		std::function<float()> ErrorFunction,
		std::vector<std::pair<float*, size_t>> Model,
		int Population,
		int Survive,
		float MutationVariance,
		float Mutate,
		float Combine) :
		ErrorFnc{ ErrorFunction },
		Updater{ Model },
		Population{ Population },
		Survive{ Survive },
		Generation(Population),
		Survived(Survive),
		ModelSize{ GetModelSize(Model) },
		Memory((size_t(Population) + Survive)* GetModelSize(Model)),
		MutationVariance{ MutationVariance },
		Mutate{ Mutate },
		Combine{ Combine },
		dist(-1.0f, 1.0f),
		dist01(0.0f, 1.0f),
		intDist(0, Survive - 1),
		curIter{ Population },
		GenId{ -1 },
		GenPc{ 0 }
	{
		Generation[0].Error = 1.0f;
		Generation[0].Model = &Memory[0];
		for(auto i = 1; i<Generation.size();i++) {
			Generation[i].Model = Generation[i - 1ll].Model + ModelSize;
		}

		Survived[0].Model = &Memory[Population * ModelSize];
		Memcpy(Survived[0].Model, Updater);
		Survived[0].Error = ErrorFnc();
		for(auto i = 1;i<Survived.size();i++){
			Survived[i].Model = Survived[i - size_t(1)].Model + ModelSize;
			for (int j = 0; j < ModelSize; j++)
				Survived[i].Model[j] = float(Survived[0].Model[j] + MutationVariance * dist(gen));
		}
	}


	float GeneticSolver::Iterate() {
		if (curIter == Population) {
			NextGeneration();
			curIter = 1;
			Generation[0].Index = 0;
			GenId++;
			GenPc = 0;
			return Generation[0].Error;
		} else if (curIter==1 && NewGenEvent.size()) {
			for (auto f : NewGenEvent) {
				GetItem(0);
				auto& g = Generation[curIter];
				g.Error = f();
				g.Index = curIter;
				if (g.Error<0) g.Error = ErrorFnc();
				Memcpy(g.Model, Updater);
				if (Generation[0].Error > g.Error)
					std::swap(g, Generation[0]);
				curIter++;
			}
			return Generation[0].Error;
		} else {
			GenPc = float(curIter) / Population;
			auto& g = Generation[curIter];
			g.Index = curIter;
			Memcpy(Updater, g.Model);
			g.Error = ErrorFnc();
			if (Generation[0].Error > g.Error)
				std::swap(g, Generation[0]);
			curIter++;
			if (curIter == Population) {
				//std::nth_element(Generation.begin()+1, Generation.begin() + Survive, Generation.end(), [](auto a, auto b) { return a.Error<b.Error; });
				std::partial_sort(Generation.begin()+1, Generation.begin() + Survive, Generation.end(), [](auto a, auto b) { return a.Error<b.Error; });
				memcpy(Survived[0].Model, Generation[0].Model, sizeof(float) * ModelSize);
				Survived[0].Error = Generation[0].Error;
				for (auto i = 1; i<Survive; i++) std::swap(Generation[i], Survived[i]);
			}
			return Generation[0].Error;
		}
	}
	float GeneticSolver::Update() {
		return GetItem(0);
	}
	int GeneticSolver::BestId() {
		return Generation[0].Index;
	}
	float GeneticSolver::GetItem(int Index) {
		Memcpy(Updater, Generation[Index].Model);
		return Generation[Index].Error;
	}
	void GeneticSolver::SetItem(int Index) {
		Memcpy(Generation[Index].Model, Updater);
	}

	void GeneticSolver::OnNewGeneration(std::function<float()> Event) {
		NewGenEvent.push_back(Event);
	}

	void GeneticSolver::SetItem(const float* w, int Index, float err) {
		memcpy(Generation[Index].Model, w, ModelSize * sizeof(float));
		Memcpy(Updater, w);
		if (err >= 0) Generation[Index].Error = err;
		else Generation[Index].Error = ErrorFnc();
	}
	void GeneticSolver::NextGeneration() {
		memcpy(Generation[0].Model, Survived[0].Model, sizeof(float) * ModelSize);
		Generation[0].Error = Survived[0].Error;
		Generation[0].Index = 0;
		for (auto i = 1; i<Population; i++) {
			auto& g = Generation[i];

			int i1 = intDist(gen);
			auto& g1 = Survived[i1];

			if (dist01(gen) < Combine) {
				int i2 = intDist(gen);
				while (i1 == i2) i2 = intDist(gen);
				auto& g2 = Survived[i2];
				for (auto j = 0; j<ModelSize; j++) {
					g.Model[j] = (intDist(gen) & 1) ? g1.Model[j] : g2.Model[j];
				}
			} else {
				memcpy(g.Model, g1.Model, sizeof(float) * ModelSize);
			}

			for (auto j = 0; j<ModelSize; j++) {
				if (dist01(gen) < Mutate) {
					g.Model[j] += float(dist(gen) * MutationVariance);
				}
			}
		}
	}
}
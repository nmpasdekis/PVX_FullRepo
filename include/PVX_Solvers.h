#include <functional>
#include <vector>
#include <random>

namespace PVX {
	namespace Solvers {
		class GradientDescent {
			float LearnRate, Momentum, RMSprop, iRMSprop;
			std::function<float()> ErrorFnc;
			std::vector<std::pair<float*, size_t>> Updater;
			std::vector<float> vRMSprop, vMomentum, vGradient, SavePoint;
			size_t ModelSize;
			float Error, LastError;
		public:
			GradientDescent(
				std::function<float()> ErrorFunction,
				std::vector<std::pair<float*, size_t>> Model,
				float LearnRate, float Momentum, float RMSprop
			);
			GradientDescent(
				std::function<float()> ErrorFunction,
				float* Model, size_t ModelSize,
				float LearnRate = 1e-5f, float Momentum = 0.9999f, float RMSprop = 0.9999f
			);
			float Iterate(float dx = 1e-5f);
			void RecalculateError();
			void ClearMomentum();
		};

		class GeneticSolver {
		protected:
			struct ErrorData {
				float Error;
				float* Model;
				int Index;
			};
			size_t ModelSize;

			std::function<float()> ErrorFnc;
			int Population, Survive;
			std::vector<float> Memory;
			std::vector<ErrorData> Generation, Survived;

			std::default_random_engine gen;
			std::uniform_real_distribution<double> dist;
			std::uniform_real_distribution<float> dist01;
			std::uniform_int_distribution<int> intDist;
			int curIter;
			void NextGeneration();
			std::vector<std::pair<float*, size_t>> Updater;

			std::vector<std::function<float()>> NewGenEvent;
		public:
			GeneticSolver(
				std::function<float()> ErrorFunction,
				float* Model, size_t ModelSize,
				int Population = 100,
				int Survive = 10,
				float MutationVariance = 1e-5f,
				float MutateProbability = 0.1f,
				float Combine = 0.5f);

			GeneticSolver(
				std::function<float()> ErrorFunction,
				std::vector<std::pair<float*, size_t>> Model,
				int Population = 100,
				int Survive = 10,
				float MutationVariance = 1e-5f,
				float MutateProbability = 0.1f,
				float Combine = 0.5f);

			void SetItem(const float* w, int Index = 1, float err = -1.0f);
			float Iterate();
			float Update();
			int BestId();
			float GetItem(int Index);
			void SetItem(int Index);

			void OnNewGeneration(std::function<float()> Event);

			float
				MutationVariance,
				Mutate,
				Combine;

			int GenId;
			float GenPc;
		}; 
		std::vector<std::vector<float>> kMean(const std::vector<std::vector<float>>& vecs, size_t nClusters);
		std::vector<size_t> kMean_Classify(const std::vector<std::vector<float>>& Clusters, const std::vector<std::vector<float>>& vecs);
	}
}
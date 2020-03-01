#include <PVX_Solvers.h>
#include <vector>

namespace PVX {
	namespace Solvers {

		inline static float dist2(const std::vector<float>& a, const std::vector<float>& b) {
			float dist = 0;
			for (size_t i = 0; i < a.size(); i++) {
				float sub = a[i] - b[i];
				dist += sub * sub;
			}
			return dist;
		}
		inline static void AddTo(std::vector<float>& a, const std::vector<float>& b) {
			for (auto i = 0; i<a.size(); i++) {
				a[i] += b[i];
			}
		}

		std::vector<size_t> kMean_Classify(const std::vector<std::vector<float>>& Clusters, const std::vector<std::vector<float>>& vecs) {
			std::vector<size_t> Class;
			Class.reserve(vecs.size());

			for (auto& vec : vecs) {
				size_t min = 0;
				float minDist = dist2(Clusters[0], vec);
				for (auto c = 1; c<Clusters.size(); c++) {
					float dist = dist2(Clusters[c], vec);
					if (dist < minDist) { min = c; minDist = dist; }
				}
				Class.push_back(min);
			}
			return std::move(Class);
		}

		std::vector<std::vector<float>> kMean(const std::vector<std::vector<float>>& vecs, size_t nClusters) {
			auto vecSize = vecs[0].size();
			std::default_random_engine gen;
			std::uniform_int_distribution<int> rBool(0, 1);
			std::vector<size_t> Cluster(vecs.size());
			for (size_t i = 0; i<vecs.size(); i++) Cluster[i] = i;
			//std::partial_sort(Cluster.begin(), Cluster.begin() + nClusters, Cluster.end(), [&rBool, &gen](const auto& a, const auto& b) { return rBool(gen); });
			std::shuffle(Cluster.begin(), Cluster.end(), gen);

			std::vector<std::vector<float>> ret, tmp;
			std::vector<size_t> Counts(nClusters);
			ret.reserve(nClusters);
			tmp.reserve(nClusters);
			for (auto i = 0; i<nClusters; i++) {
				ret.push_back(vecs[Cluster[i]]);
				tmp.emplace_back(vecSize);
			}

			size_t notFinished = 1;
			while(notFinished) {
				notFinished = 0;

				size_t cur = 0;
				for (auto& t : tmp) {
					memset(t.data(), 0, sizeof(float) * vecSize);
					Counts[cur++] = 0;
				}
				cur = 0;

				for (auto& vec : vecs) {
					size_t min = 0;
					float minDist = dist2(ret[0], vec);
					for (auto c = 1; c<nClusters; c++) {
						float dist = dist2(ret[c], vec);
						if (dist < minDist) { min = c; minDist = dist; }
					}
					notFinished += min!=Cluster[cur];
					Cluster[cur++] = min;
					AddTo(tmp[min], vec);
					Counts[min]++;
				}
				for (auto i = 0; i<nClusters; i++) {
					float inv = 1.0f / Counts[i];
					auto& t = tmp[i];
					auto& r = ret[i];
					for (auto j = 0; j<vecSize; j++) {
						r[j] = t[j] * inv;
					}
				}
			}
			return std::move(ret);
		}
	}
}
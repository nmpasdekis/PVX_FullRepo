#define EIGEN_MPL2_ONLY
#include <Eigen/dense>
#include <Eigen/SVD>

using netData = Eigen::MatrixXf;

inline Eigen::Block<netData, -1, -1, false> outPart(netData & m) {
	return m.block(0, 0, m.rows() - 1, m.cols());
}

inline Eigen::Block<netData, -1, -1, false> outPart(netData& m, size_t i) {
	return m.block(0, i, m.rows() - 1, 1);
}

inline Eigen::Map<netData> Map(float * data, size_t Rows, size_t Cols) {
	return Eigen::Map<netData>(data, Rows, Cols);
}
#ifdef _DEBUG
#define CorrectMat(m) _CorrectMat(m)
#else
#define CorrectMat(m)
#endif

inline size_t _CorrectMat(const netData& m) {
	auto sz = m.size();
	const auto* dt = m.data();
	for (volatile auto i = 0; i<sz; i++) {
		if (!std::isfinite(dt[i])) {
			return i + 1;
		}
	}
	return 0;
}

inline void FixMatrix(netData& m, std::function<float()> fnc = [] { return 0; }) {
	auto sz = m.size();
	auto* dt = m.data();
	for (auto i = 0; i<sz; i++) {
		if (!std::isfinite(dt[i]))
			dt[i] = fnc();
	}
}

inline netData makeUnit(const netData& mat) {
	Eigen::JacobiSVD svd(mat, Eigen::ComputeThinU | Eigen::ComputeThinV);
	return svd.matrixU() * svd.matrixV().transpose();
}
#define EIGEN_MPL2_ONLY
#include <Eigen/dense>
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
		auto& d = dt[i];
		if ((!(d>0) && !(d<=0)) || d==(d+1)) {
			return i + 1;
		}
	}
	return 0;
}
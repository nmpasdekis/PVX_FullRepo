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

inline int CorrectMat(netData& m) {
	auto sz = m.size();
	auto* dt = m.data();
	int cnt = 0;
	for (auto i = 0; i<sz; i++) {
		auto& d = dt[i];
		if (!(d>0) && !(d<=0)) {
			cnt++;
			d = 0;
		}
	}
	return cnt;
}
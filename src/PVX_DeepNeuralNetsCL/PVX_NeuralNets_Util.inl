#define EIGEN_MPL2_ONLY

#include <PVX_File.h>

#include <PVX_clMatrix.h>
using clMatrix = PVX::NeuralNets::clMatrix;

inline clMatrix outPart(const clMatrix & m) {
	return m.block(0, 0, m.rows() - 1, m.cols());
}

inline clMatrix outPart(clMatrix& m, size_t i) {
	return m.block(0, i, m.rows() - 1, 1);
}

inline clMatrix Map(float * data, size_t Rows, size_t Cols) {
	return clMatrix(Rows, Cols, data);
}
#ifdef _DEBUG
#define CorrectMat(m) _CorrectMat(m)
#define dbgMat(m) _dbgMat(m);
#define dbgMat2(m) _dbgMat2(m);
#else
#define CorrectMat(m)
#define dbgMat(m)
#define dbgMat2(m)
#endif

inline void _dbgMat(const clMatrix& m) {
	auto dbg = m.ReadBlocking();
	dbg.size();
}

inline void _dbgMat2(const clMatrix& m) {
	auto dbg = m.ReadBlocking();
	dbg.size();
}

inline size_t _CorrectMat(const clMatrix& mat) {
	auto m = mat.ReadBlocking();
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

inline void SaveMat(const clMatrix& m, const std::string& gn) {
	PVX::BinWriter file(gn);
	file.Write(m.rows());
	file.Write(m.cols());
	file.Write(m.ReadBlocking());
}
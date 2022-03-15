#include <eigen/dense>

namespace PVX::Eigen {
	inline Eigen::MatrixXf CovarienceMatrix(const Eigen::MatrixXf& mat) {
		Eigen::MatrixXf centered = mat.rowwise() - mat.colwise().mean();
		return (centered.adjoint() * centered) / double(mat.rows() - 1);
	}
}
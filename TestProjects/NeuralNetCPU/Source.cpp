#include <PVX_NeuralNetsCPU.h>
#include <iostream>
#include <iomanip>

#include <PVX_Math3D.h>

#include <eigen/dense>
#include <eigen/svd>
#include <Eigen/Eigenvalues>


//#include <PVX_Eigen.inl>

using namespace PVX::DeepNeuralNets;

inline netData makeUnit(const netData& mat) {
	Eigen::JacobiSVD svd(mat, Eigen::ComputeThinU | Eigen::ComputeThinV);
	return svd.matrixU() * svd.matrixV().transpose();
}

inline Eigen::MatrixXf CovarienceMatrix(const Eigen::MatrixXf& mat) {
	Eigen::MatrixXf centered = mat.rowwise() - mat.colwise().mean();
	return (centered.adjoint() * centered) / double(mat.rows() - 1);
}



int main() {

	//auto testRot = PVX::Matrix4x4::RotationYXZ({ 3.0f, 0.5f, 1.99f });

	////netData test = Eigen::MatrixXf::Random(3, 2);
	//netData test = Eigen::Map<netData>(testRot.m16, 4, 4);

	//netData test2 = makeUnit(test);

	//std::cout << test << "\n\n";
	//std::cout << test2 << "\n\n";

	netData test = Eigen::MatrixXf::Random(1000, 10);
	auto Cov = CovarienceMatrix(test);

	auto solver = Eigen::EigenSolver<Eigen::MatrixXf>();

	auto eig = solver.compute(Cov, true);

	auto eigVals = eig.eigenvalues();

	Eigen::JacobiSVD svd(Cov, Eigen::ComputeThinU | Eigen::ComputeThinV);

	//auto mag = Eigen::abs2(eigVals);

	std::cout << svd.singularValues() << "\n\n";

	std::cout << eigVals.cwiseAbs() << "\n\n";

	std::cout << Cov.eigenvalues() << "\n\n";


	NeuralLayer_Base::LearnRate(0.001);
	NeuralLayer_Base::L2Regularization(0.01);

	PVX::DeepNeuralNets::InputLayer Input("Input", 2);
	PVX::DeepNeuralNets::NeuronLayer Layer1("Layer 1", &Input, 8);
	PVX::DeepNeuralNets::NeuronLayer Layer2("Layer 2", &Layer1, 8);
	PVX::DeepNeuralNets::NeuronLayer Layer3("Layer 3", &Layer1, 2, LayerActivation::Linear);
	PVX::DeepNeuralNets::OutputLayer Output(&Layer3, OutputType::StableSoftMax);

	auto inp = Input.MakeRawInput(std::vector<float>{
		0.0f, 0.0f,
		1.0f, 0.0f,
		0.0f, 1.0f,
		1.0f, 1.0f,
	});

	float outp[]{
		0.0f, 1.0f,
		1.0f, 0.0f,
		1.0f, 0.0f,
		0.0f, 1.0f,
	};

	Input.InputRaw(inp);
	Output.Result();
	float err = Output.Train(outp);

	for (int j = 0; j<10000 && err>1e-5; j++) {
		for (int i = 0; i<1000 && err>1e-5; i++) {
			Output.Result();
			err = err * 0.9 + 0.1 * Output.Train(outp);
		}
		printf("Error: %.5f\r", err);
		if (!(j%10))
			std::cout << "\n" << std::setprecision(2) << Output.Result() << "\n";
	}
	return 0;
}
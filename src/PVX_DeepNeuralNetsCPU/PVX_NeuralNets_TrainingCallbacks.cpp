#include <PVX_NeuralNetsCPU.h>
#include "PVX_NeuralNets_Util.inl"

namespace PVX::DeepNeuralNets {


	void NeuronLayer::AdamF(const netData& Gradient) {
		netData g1 = (Gradient * PreviousLayer->Output().transpose()) / Gradient.cols();
		RMSprop = _RMSprop * RMSprop.array() + _iRMSprop * (g1.array()*g1.array());
		netData gr = g1.array() / ((Eigen::sqrt(RMSprop.array()) + 1e-5f));

		DeltaWeights = (DeltaWeights * _Momentum) + (gr * (_LearnRate * _iMomentum));
		Weights += DeltaWeights;
		CorrectMat(Weights);
	}

	void NeuronLayer::Adam_L2_F(const netData& Gradient) {
		netData g1 = (Gradient * PreviousLayer->Output().transpose()) / Gradient.cols();
		RMSprop = _RMSprop * RMSprop.array() + _iRMSprop * (g1.array()*g1.array());
		netData gr = g1.array() / ((Eigen::sqrt(RMSprop.array()) + 1e-5f));

		DeltaWeights = (DeltaWeights * _Momentum) + (gr * (_LearnRate * _iMomentum));
		Weights = Weights * (1.0f - _LearnRate * _L2 / Weights.size()) + DeltaWeights;
	}

	void NeuronLayer::MomentumF(const netData& Gradient) {
		DeltaWeights = (Gradient * PreviousLayer->Output().transpose() * (_LearnRate * _iMomentum / Gradient.cols())) + DeltaWeights * _Momentum;
		Weights += DeltaWeights;
	}
	void NeuronLayer::RMSpropF(const netData& Gradient) {
		netData g1 = (Gradient * PreviousLayer->Output().transpose()) / Gradient.cols();
		RMSprop = _RMSprop * RMSprop.array() + _iRMSprop * (g1.array()*g1.array());
		netData gr = g1.array() / ((Eigen::sqrt(RMSprop.array()) + 1e-5f));

		DeltaWeights += gr * _LearnRate;
		Weights += DeltaWeights;
	}
	void NeuronLayer::Momentum_L2_F(const netData& Gradient) {
		DeltaWeights = (Gradient * PreviousLayer->Output().transpose() * (_LearnRate * _iMomentum / Gradient.cols())) + DeltaWeights * _Momentum;
		Weights = Weights * (1.0f - _LearnRate * _L2 / Weights.size()) + DeltaWeights;
	}
	void NeuronLayer::RMSprop_L2_F(const netData& Gradient) {
		netData g1 = (Gradient * PreviousLayer->Output().transpose()) / Gradient.cols();
		RMSprop = _RMSprop * RMSprop.array() + _iRMSprop * (g1.array()*g1.array());
		netData gr = g1.array() / ((Eigen::sqrt(RMSprop.array()) + 1e-5f));

		Weights = Weights * (1.0f - _LearnRate * _L2 / Weights.size()) + (gr * PreviousLayer->Output().transpose() * _LearnRate);
	}

	void NeuronLayer::SgdF(const netData& Gradient) {
		Weights += (Gradient * PreviousLayer->Output().transpose() * _LearnRate / Gradient.cols());
	}
	void NeuronLayer::Sgd_L2_F(const netData& Gradient) {
		Weights = Weights * (1.0f - _LearnRate * _L2 / Weights.size()) + (Gradient * PreviousLayer->Output().transpose() * _LearnRate / Gradient.cols());
	}

	void NeuronLayer::AdaGradF(const netData& Gradient) {
		netData g1 = ((Gradient * PreviousLayer->Output().transpose()) / Gradient.cols());
		RMSprop += (g1.array()*g1.array()).matrix();
		netData gr = g1.array() / ((Eigen::sqrt(RMSprop.array()) + 1e-5f));
		
		Weights += (gr * PreviousLayer->Output().transpose() * _LearnRate);
	}
	void NeuronLayer::AdaGrad_L2_F(const netData& Gradient) {
		netData g1 = ((Gradient * PreviousLayer->Output().transpose()) / Gradient.cols());
		RMSprop += (g1.array()*g1.array()).matrix();
		netData gr = g1.array() / ((Eigen::sqrt(RMSprop.array()) + 1e-5f));

		Weights = Weights * (1.0f - _LearnRate * _L2 / Weights.size()) + (gr * PreviousLayer->Output().transpose() * _LearnRate);
	}


	//void NeuronLayer::AdamF(const netData& Gradient) {
	//	const auto& g1 = Gradient.array();
	//	if (RMSprop.cols() != Gradient.cols())RMSprop = netData::Ones(Gradient.rows(), Gradient.cols());
	//	//if (RMSprop.cols() != Gradient.cols())RMSprop = netData::Zero(Gradient.rows(), Gradient.cols());
	//	RMSprop = _RMSprop * RMSprop.array() + _iRMSprop * (g1*g1);
	//	netData gr = g1 / ((Eigen::sqrt(RMSprop.array()) + 1e-5f));

	//	DeltaWeights = (gr * PreviousLayer->Output().transpose() * (_LearnRate * _iMomentum)) + DeltaWeights * _Momentum;
	//	Weights += DeltaWeights;
	//}
	//void NeuronLayer::Adam_L2_F(const netData& Gradient) {
	//	auto g1 = Gradient.array();
	//	if (RMSprop.cols() != Gradient.cols())RMSprop = netData::Ones(Gradient.rows(), Gradient.cols());
	//	//if (RMSprop.cols() != Gradient.cols())RMSprop = netData::Zero(Gradient.rows(), Gradient.cols());
	//	RMSprop = _RMSprop * RMSprop.array() + _iRMSprop * (g1*g1);
	//	netData gr = g1 / ((Eigen::sqrt(RMSprop.array())) + 1e-5f);;

	//	DeltaWeights = (gr * PreviousLayer->Output().transpose() * (_LearnRate * _iMomentum)) + DeltaWeights * _Momentum;
	//	Weights = Weights * (1.0f - _LearnRate * _L2 / Weights.size()) + DeltaWeights;
	//}

	//void NeuronLayer::MomentumF(const netData& Gradient) {
	//	DeltaWeights = (Gradient * PreviousLayer->Output().transpose() * (_LearnRate * _iMomentum)) + DeltaWeights * _Momentum;
	//	Weights += DeltaWeights;
	//}
	//void NeuronLayer::RMSpropF(const netData& Gradient) {
	//	auto g1 = Gradient.array();
	//	if (RMSprop.cols() != Gradient.cols()) RMSprop = netData::Ones(Gradient.rows(), Gradient.cols());
	//	//if (RMSprop.cols() != Gradient.cols()) RMSprop = netData::Zero(Gradient.rows(), Gradient.cols());
	//	RMSprop = _RMSprop * RMSprop.array() + _iRMSprop * (g1*g1);
	//	netData gr = g1 / (Eigen::sqrt(RMSprop.array()) + 1e-5f);

	//	Weights += (gr * PreviousLayer->Output().transpose() * _LearnRate);
	//}
	//void NeuronLayer::SgdF(const netData& Gradient) {
	//	Weights += (Gradient * PreviousLayer->Output().transpose() * _LearnRate);
	//}
	//void NeuronLayer::AdaGradF(const netData& Gradient) {
	//	auto g1 = Gradient.array();
	//	if (RMSprop.cols() != Gradient.cols()) RMSprop = netData::Ones(Gradient.rows(), Gradient.cols());
	//	//if (RMSprop.cols() != Gradient.cols()) RMSprop = netData::Zero(Gradient.rows(), Gradient.cols());
	//	RMSprop = RMSprop.array() + (g1*g1);
	//	netData gr = g1 / (Eigen::sqrt(RMSprop.array()) + 1e-5f);

	//	Weights += (gr * PreviousLayer->Output().transpose() * _LearnRate);
	//}
	//void NeuronLayer::Momentum_L2_F(const netData& Gradient) {
	//	DeltaWeights = (Gradient * PreviousLayer->Output().transpose() * (_LearnRate * _iMomentum)) + DeltaWeights * _Momentum;
	//	Weights = Weights * (1.0f - _LearnRate * _L2 / Weights.size()) + DeltaWeights;
	//}
	//void NeuronLayer::RMSprop_L2_F(const netData& Gradient) {
	//	auto g1 = Gradient.array();
	//	if (RMSprop.cols() != Gradient.cols()) RMSprop = netData::Ones(Gradient.rows(), Gradient.cols());
	//	//if (RMSprop.cols() != Gradient.cols()) RMSprop = netData::Zero(Gradient.rows(), Gradient.cols());
	//	RMSprop = _RMSprop * RMSprop.array() + _iRMSprop * (g1*g1);
	//	netData gr = g1 / (Eigen::sqrt(RMSprop.array()) + 1e-5f);

	//	Weights = Weights * (1.0f - _LearnRate * _L2 / Weights.size()) + (gr * PreviousLayer->Output().transpose() * _LearnRate);
	//}
	//void NeuronLayer::Sgd_L2_F(const netData& Gradient) {
	//	Weights = Weights * (1.0f - _LearnRate * _L2 / Weights.size()) + (Gradient * PreviousLayer->Output().transpose() * _LearnRate);
	//}
	//void NeuronLayer::AdaGrad_L2_F(const netData& Gradient) {
	//	auto g1 = Gradient.array();
	//	if (RMSprop.cols() != Gradient.cols()) RMSprop = netData::Ones(Gradient.rows(), Gradient.cols());
	//	//if (RMSprop.cols() != Gradient.cols()) RMSprop = netData::Zero(Gradient.rows(), Gradient.cols());
	//	RMSprop = RMSprop.array() + (g1*g1);
	//	netData gr = g1 / (Eigen::sqrt(RMSprop.array()) + 1e-5f);

	//	Weights = Weights * (1.0f - _LearnRate * _L2 / Weights.size()) + (gr * PreviousLayer->Output().transpose() * _LearnRate);
	//}
}
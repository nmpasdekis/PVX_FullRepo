#include <PVX_NeuralNetsCL.h>
#include "PVX_NeuralNets_Util.inl"

namespace PVX::NeuralNets {
	void NeuronLayer::AdamF(const clMatrix& Gradient) {
		clMatrix g1 = (Gradient * PreviousLayer->Output().Transpose()) / Gradient.cols();
		CorrectMat(g1);
		RMSprop = _RMSprop * RMSprop + _iRMSprop * g1.Element_Multiply(g1);
		CorrectMat(RMSprop);
		auto gr = g1.Element_Div(sqrt(RMSprop + 1e-10f));

		DeltaWeights = (DeltaWeights * (_LearnRate * _Momentum)) + (gr * (_LearnRate * _iMomentum));
		//DeltaWeights = (DeltaWeights * _Momentum) + (gr * (_LearnRate * _iMomentum));
		Weights += DeltaWeights;
	}

	void NeuronLayer::Adam_L2_F(const clMatrix& Gradient) {
		clMatrix g1 = (Gradient * PreviousLayer->Output().Transpose()) / Gradient.cols();
		RMSprop = _RMSprop * RMSprop + _iRMSprop * g1.Element_Multiply(g1);
		auto gr = g1.Element_Div(sqrt(RMSprop + 1e-10f));

		DeltaWeights = (DeltaWeights * (_LearnRate * _Momentum)) + (gr * (_LearnRate * _iMomentum));
		//DeltaWeights = (DeltaWeights * (_Momentum)) + (gr * (_LearnRate * _iMomentum));
		Weights = Weights * (1.0f - _LearnRate * _L2 / Weights.size()) + DeltaWeights;
	}

	void NeuronLayer::MomentumF(const clMatrix& Gradient) {
		DeltaWeights = (Gradient * PreviousLayer->Output().Transpose() * (_LearnRate * _iMomentum / Gradient.cols())) + DeltaWeights * _Momentum;
		Weights += DeltaWeights;
	}
	void NeuronLayer::RMSpropF(const clMatrix& Gradient) {
		clMatrix g1 = (Gradient * PreviousLayer->Output().Transpose()) / Gradient.cols();
		RMSprop = _RMSprop * RMSprop + _iRMSprop * g1.Element_Multiply(g1);

		DeltaWeights += g1.Element_Div(sqrt(RMSprop + 1e-10f)) * _LearnRate;
		Weights += DeltaWeights;
	}
	void NeuronLayer::Momentum_L2_F(const clMatrix& Gradient) {
		DeltaWeights = (Gradient * PreviousLayer->Output().Transpose() * (_LearnRate * _iMomentum / Gradient.cols())) + DeltaWeights * (_LearnRate * _Momentum);
		Weights = Weights * (1.0f - _LearnRate * _L2 / Weights.size()) + DeltaWeights;
	}
	void NeuronLayer::RMSprop_L2_F(const clMatrix& Gradient) {
		clMatrix g1 = (Gradient * PreviousLayer->Output().Transpose()) / Gradient.cols();
		RMSprop = _RMSprop * RMSprop + _iRMSprop * g1.Element_Multiply(g1);

		Weights = Weights * (1.0f - _LearnRate * _L2 / Weights.size()) + (g1.Element_Div(sqrt(RMSprop + 1e-10f)) * PreviousLayer->Output().Transpose() * _LearnRate);
	}

	void NeuronLayer::SgdF(const clMatrix& Gradient) {
		Weights += (Gradient * PreviousLayer->Output().Transpose() * _LearnRate / Gradient.cols());
	}
	void NeuronLayer::Sgd_L2_F(const clMatrix& Gradient) {
		Weights = Weights * (1.0f - _LearnRate * _L2 / Weights.size()) + (Gradient * PreviousLayer->Output().Transpose() * _LearnRate / Gradient.cols());
	}

	void NeuronLayer::AdaGradF(const clMatrix& Gradient) {
		clMatrix g1 = ((Gradient * PreviousLayer->Output().Transpose()) / Gradient.cols());
		RMSprop += g1.Element_Multiply(g1);
		
		Weights += (g1.Element_Div(sqrt(RMSprop + 1e-10f)) * PreviousLayer->Output().Transpose() * _LearnRate);
	}
	void NeuronLayer::AdaGrad_L2_F(const clMatrix& Gradient) {
		clMatrix g1 = ((Gradient * PreviousLayer->Output().Transpose()) / Gradient.cols());
		RMSprop += g1.Element_Multiply(g1);

		Weights = Weights * (1.0f - _LearnRate * _L2 / Weights.size()) + (g1.Element_Div(sqrt(RMSprop + 1e-10f)) * PreviousLayer->Output().Transpose() * _LearnRate);
	}
}
#include <PVX_NeuralNetsCL.h>
#include <iostream>
#include "PVX_NeuralNets_Util.inl"

#pragma comment(lib, "PVX_General.lib")

namespace PVX::NeuralNets {
	extern int UseDropout;
	float NeuralLayer_Base::__LearnRate = 0.0001f;
	float NeuralLayer_Base::__Momentum = 0.999f;
	float NeuralLayer_Base::__iMomentum = 0.001f;
	float NeuralLayer_Base::__RMSprop = 0.999f;
	float NeuralLayer_Base::__iRMSprop = 0.1f;
	float NeuralLayer_Base::__Dropout = 0.8f;
	float NeuralLayer_Base::__iDropout = 1.0f / 0.8f;
	float NeuralLayer_Base::__L2 = 0.0f;
	int NeuralLayer_Base::OverrideOnLoad = 0;
	size_t NeuralLayer_Base::NextId = 0;

	clMatrix myRandom(int r, int c, float Max) {
		return clMatrix::Random(r, c, -Max, Max);
	}

	clMatrix NeuralLayer_Base::Output() {
		return output;
	}
	clMatrix NeuralLayer_Base::RealOutput() {
		return outPart(output);
	}
	clMatrix NeuralLayer_Base::RealOutput(int64_t Index) {
		return outPart(output, Index);
	}
	float NeuralLayer_Base::LearnRate() {
		return __LearnRate;
	}
	void NeuralLayer_Base::LearnRate(float Alpha) {
		__LearnRate = Alpha;
	}
	float NeuralLayer_Base::Momentum() {
		return __Momentum;
	}
	void NeuralLayer_Base::Momentum(float Beta) {
		__Momentum = Beta;
		__iMomentum = 1.0f - Beta;
	}
	float NeuralLayer_Base::RMSprop() {
		return __RMSprop;
	}
	void NeuralLayer_Base::RMSprop(float Beta) {
		__RMSprop = Beta;
		__iRMSprop = 1.0f - Beta;
	}
	void NeuralLayer_Base::L2Regularization(float lambda) {
		__L2 = lambda;
	}
	float NeuralLayer_Base::L2Regularization() {
		return __L2;
	}
	float NeuralLayer_Base::Dropout() {
		return __Dropout;
	}
	void NeuralLayer_Base::Dropout(float Beta) {
		__Dropout = Beta;
		__iDropout = 1.0f / Beta;
	}
	size_t NeuralLayer_Base::nOutput() const {
		return output.rows() - 1;
	}

	size_t NeuralLayer_Base::BatchSize() const {
		return output.cols();
	}

	void NeuralLayer_Base::UseDropout(int b) {
		PVX::NeuralNets::UseDropout = b;
	}
	void NeuralLayer_Base::OverrideParamsOnLoad(int b) {
		OverrideOnLoad = b;
	}

	void NeuralLayer_Base::SetLearnRate(float Beta) {
		if (PreviousLayer)PreviousLayer->SetLearnRate(Beta);
		for (auto& p : InputLayers) p->SetLearnRate(Beta);
	}
	void NeuralLayer_Base::SetRMSprop(float Beta) {
		if (PreviousLayer)PreviousLayer->SetRMSprop(Beta);
		for (auto& p : InputLayers) p->SetRMSprop(Beta);
	}
	void NeuralLayer_Base::SetMomentum(float Beta) {
		if (PreviousLayer)PreviousLayer->SetMomentum(Beta);
		for (auto& p : InputLayers) p->SetMomentum(Beta);
	}
	void NeuralLayer_Base::ResetMomentum() {
		if (PreviousLayer)PreviousLayer->ResetMomentum();
		for (auto& p : InputLayers) p->ResetMomentum();
	}

	void NeuralLayer_Base::Gather(std::set<NeuralLayer_Base*>& g) {
		g.insert(this);
		if (PreviousLayer)PreviousLayer->Gather(g);
		if (InputLayers.size())for (auto& i:InputLayers)i->Gather(g);
	}

	void NeuralLayer_Base::FixInputs(const std::vector<NeuralLayer_Base*>& ids) {
		if (PreviousLayer) {
			PreviousLayer = ids[(*(int*)&PreviousLayer)-1ll];
		}
		else for (auto& l : InputLayers) {
			l = ids[(*(int*)&l)-1ll];
		}
	}

	void NeuralLayer_Base::Input(NeuralLayer_Base* inp) {
		PreviousLayer = inp;
	}

	void NeuralLayer_Base::Inputs(const std::vector<NeuralLayer_Base*>& inp) {
		InputLayers = inp;
	}

	//void NetDNA::GetData(std::vector<float>& Data) {
	//	if (Size!=Data.size())Data.resize(Size);
	//	for (auto& w : Layers) {
	//		memcpy_s(Data.data() + w.Offset, sizeof(float) * w.Count, w.Weights, sizeof(float) * w.Count);
	//	}
	//}

	//std::vector<float> NetDNA::GetData() {
	//	std::vector<float> ret(Size);
	//	for (auto& w : Layers) {
	//		memcpy_s(ret.data() + w.Offset, sizeof(float) * w.Count, w.Weights, sizeof(float) * w.Count);
	//	}
	//	return std::move(ret);
	//}
	//void NetDNA::SetData(const float* Data) {
	//	for (auto& w : Layers)
	//		memcpy(w.Weights, &Data[w.Offset], sizeof(float) * w.Count);
	//}



	ResNetUtility::ResNetUtility(NeuralLayer_Base* inp, LayerActivation Activate, TrainScheme Train) :
		First{ inp, inp->nOutput(), Activate, Train },
		Second{ &First, inp->nOutput(), LayerActivation::Linear, Train },
		Adder({ inp, &Second }),
		Activation(&Adder, Activate) {}
	ResNetUtility::ResNetUtility(const std::string& Name, NeuralLayer_Base* inp, LayerActivation Activate, TrainScheme Train) :
		First{ Name + "_First", inp, inp->nOutput(), Activate, Train },
		Second{ Name + "_Second",  &First, inp->nOutput(), LayerActivation::Linear, Train },
		Adder(Name + "_Adder", { inp, &Second }),
		Activation(Name + "_Activation", &Adder, Activate) {}
	ResNetUtility::ResNetUtility(const ResNetUtility& inp, LayerActivation Activate, TrainScheme Train) :
		ResNetUtility(inp.OutputLayer(), Activate, Train) {}
	ResNetUtility::ResNetUtility(const std::string& Name, const ResNetUtility& inp, LayerActivation Activate, TrainScheme Train) :
		ResNetUtility(Name, inp.OutputLayer(), Activate, Train) {}
	NeuralLayer_Base* ResNetUtility::OutputLayer() const {
		return (NeuralLayer_Base*)&Activation;
	}
}
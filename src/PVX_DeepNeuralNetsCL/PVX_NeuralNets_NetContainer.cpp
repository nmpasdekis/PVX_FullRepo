#include <PVX_NeuralNetsCL.h>
#include "PVX_NeuralNets_Util.inl"

//#include <stdio.h>

#undef min
#undef max

namespace PVX::NeuralNets {

	void NetContainer::Save(const std::wstring& Filename) {
		std::map<NeuralLayer_Base*, size_t> g;
		std::vector<NeuralLayer_Base*> all;
		{
			std::set<NeuralLayer_Base*> All;
			LastLayer->Gather(All);

			size_t i = 1;
			for (auto l : All) {
				all.push_back(l);
				g[l] = i++;
			}
		}
		PVX::BinSaver bin(Filename.c_str(), "NWK2"); {
			bin.Write("TYPE", int(Type));
			bin.Write("ERRO", error);
			bin.Write("LAST", g[LastLayer]);
			bin.Begin("LYRS"); for (auto l: all) {
				l->Save(bin, g);
			} bin.End();
		}
	}
	//void NetContainer::SaveCheckpoint() {
	//	Checkpoint.GetData(CheckpointDNA);
	//	CheckpointError = error;
	//}
	//float NetContainer::LoadCheckpoint() {
	//	error = CheckpointError;
	//	Checkpoint.SetData(CheckpointDNA.data());
	//	return error;
	//}
	//float NetContainer::SaveCheckpoint(std::vector<float>& data) {
	//	Checkpoint.GetData(data);
	//	return error;
	//}
	//void NetContainer::LoadCheckpoint(const std::vector<float>& data, float Error) {
	//	Checkpoint.SetData(data.data());
	//	error = Error;
	//}
	void NetContainer::SetLearnRate(float a) { LastLayer->SetLearnRate(a); }
	void NetContainer::SetRMSprop(float Beta) { LastLayer->SetRMSprop(Beta); }
	void NetContainer::SetMomentum(float Beta) { LastLayer->SetMomentum(Beta); }
	void NetContainer::ResetMomentum() { LastLayer->ResetMomentum(); }

	void NetContainer::ResetRNN() {
		for (auto r : RNNs) r->Reset();
	}
	clMatrix NetContainer::MakeRawInput(const clMatrix& inp) {
		return Inputs[0]->MakeRawInput(inp);
	}
	clMatrix NetContainer::MakeRawInput(const std::vector<float>& inp) {
		return Inputs[0]->MakeRawInput(inp);
	}
	std::vector<clMatrix> NetContainer::MakeRawInput(const std::vector<clMatrix>& inp) {
		std::vector<clMatrix> ret;
		size_t i = 0;
		for (auto l: Inputs)
			ret.push_back(l->MakeRawInput(inp[i++]));
		return ret;
	}
	void NetContainer::Init() {
		switch (Type) {
			case PVX::NeuralNets::OutputType::MeanSquare:
				FeedForward = [this] { FeedForwardMeanSquare(); };
				GetErrorFnc = [this](const clMatrix& Data) { return GetError_MeanSquare(Data); };
				TrainFnc = [this](const clMatrix& Data) { return Train_MeanSquare(Data); };
				TrainFnc_NoError = [this](const clMatrix& Data) { Train_MeanSquare_NoError(Data); };
				break;
			case PVX::NeuralNets::OutputType::SoftMax:
				FeedForward = [this] { FeedForwardSoftMax(); };
				GetErrorFnc = [this](const clMatrix& Data) { return GetError_SoftMax(Data); };
				TrainFnc = [this](const clMatrix& Data) { return Train_SoftMax(Data); };
				TrainFnc_NoError = [this](const clMatrix& Data) { Train_MeanSquare_NoError(Data); };
				break;
			case PVX::NeuralNets::OutputType::StableSoftMax:
				FeedForward = [this] { FeedForwardStableSoftMax(); };
				GetErrorFnc = [this](const clMatrix& Data) { return GetError_SoftMax(Data); };
				TrainFnc = [this](const clMatrix& Data) { return Train_SoftMax(Data); };
				TrainFnc_NoError = [this](const clMatrix& Data) { Train_MeanSquare_NoError(Data); };
				break;
			default:
				break;
		}
		Ones.Reset(1, output.rows() - 1) = 1.0f;
		std::set<NeuralLayer_Base*> all;
		LastLayer->Gather(all);
		for (auto l : all) {
			auto Inp = dynamic_cast<InputLayer*>(l);
			if (Inp) 
				Inputs.push_back(Inp);
			else {
				auto dense = dynamic_cast<NeuronLayer*>(l);
				if (dense) 
					DenseLayers.push_back(dense);
				else {
					auto rnns = dynamic_cast<RecurrentLayer*>(l);
					if (rnns) RNNs.push_back(rnns);
				}
			}
		}
		std::sort(DenseLayers.begin(), DenseLayers.end(), [](NeuronLayer* a, NeuronLayer* b) {
			return a->Id<b->Id;
		});

		//Checkpoint = GetDNA();
	}
	NetContainer::NetContainer(NeuralLayer_Base* Last, OutputType Type) :LastLayer{ Last }, Type{ Type } {
		Init();
	}
	NetContainer::NetContainer(const std::wstring& Filename) {
		{
			size_t LastLayerIndex;
			int tp;
			PVX::BinLoader bin(Filename.c_str(), "NWK2");
			bin.Process("LYRS", [&](PVX::BinLoader& bin2) {
				bin2.Process("ACTV", [&](PVX::BinLoader& bin3) {
					this->OwnedLayers.push_back(ActivationLayer::Load2(bin3));
				});
				bin2.Process("INPT", [&](PVX::BinLoader& bin3) {
					this->OwnedLayers.push_back(new InputLayer(bin3));
				});
				bin2.Process("DENS", [&](PVX::BinLoader& bin3) {
					this->OwnedLayers.push_back(NeuronLayer::Load2(bin3));
				});
				bin2.Process("ADDR", [&](PVX::BinLoader& bin3) {
					this->OwnedLayers.push_back(NeuronAdder::Load2(bin3));
				});
				bin2.Process("MULP", [&](PVX::BinLoader& bin3) {
					this->OwnedLayers.push_back(NeuronMultiplier::Load2(bin3));
				});
				bin2.Process("CMBN", [&](PVX::BinLoader& bin3) {
					this->OwnedLayers.push_back(NeuronCombiner::Load2(bin3));
				});
				bin2.Process("RNNe", [&](PVX::BinLoader& bin3) {
					this->OwnedLayers.push_back(RecurrentLayer::Load2(bin3));
				});
				bin2.Process("RNNs", [&](PVX::BinLoader& bin3) {
					this->OwnedLayers.push_back(RecurrentInput::Load2(bin3));
				});
			});

			bin.Process("LAST", [&](PVX::BinLoader& bin2) { 
				LastLayerIndex = bin2.read<size_t>(); 
			});
			bin.Process("TYPE", [&](PVX::BinLoader& bin2) { 
				tp = bin2.read<int>();
			});
			bin.Process("ERRO", [&](PVX::BinLoader& bin2) { 
				error = bin2.read<float>(); 
			});
			bin.Execute();
			Type = OutputType(tp);
			LastLayer = OwnedLayers[LastLayerIndex-1ll];
		}
		for (auto l : OwnedLayers) {
			l->FixInputs(OwnedLayers);
			auto rnns = dynamic_cast<RecurrentLayer*>(l);
			if (rnns) 
				rnns->RNN_Input = (RecurrentInput*)OwnedLayers[(*(int*)&rnns->RNN_Input)-1ll];
		}
		Init();
	}
	NetContainer::~NetContainer() {
		if (OwnedLayers.size()) {
			for (auto l: OwnedLayers) delete l;
		}
	}

	clMatrix NetContainer::Process(const clMatrix& inp) const {
		Inputs[0]->Input(inp);
		FeedForward();
		return output;
	}
	clMatrix NetContainer::Process(const std::vector<clMatrix>& inp) const {
		for (auto i = 0; i<inp.size(); i++)
			Inputs[i]->Input(inp[i]);
		FeedForward();
		return output;
	}
	clMatrix NetContainer::ProcessRaw(const clMatrix& inp) const {
		Inputs[0]->InputRaw(inp);
		FeedForward();
		return output;
	}
	clMatrix NetContainer::ProcessRaw(const std::vector<clMatrix>& inp) const {
		for (auto i = 0; i<inp.size(); i++)
			Inputs[i]->InputRaw(inp[i]);
		FeedForward();
		return output;
	}
	float NetContainer::Train(const clMatrix& inp, const clMatrix& outp) {
		Inputs[0]->Input(inp);
		FeedForward();
		return TrainFnc(outp);
	}
	float NetContainer::TrainRaw(const clMatrix& inp, const clMatrix& outp) {
		Inputs[0]->InputRaw(inp);
		FeedForward();
		return TrainFnc(outp);
	}

	float NetContainer::Train(const clMatrix& inp, const clMatrix& outp, size_t BatchSize) {
		float Error = 0;
		size_t cur = 0;
		size_t rowsI = inp.rows();
		size_t rowsO = outp.rows();
		while (cur < size_t(inp.cols())) {
			size_t cols = std::min(inp.cols() - cur, BatchSize);
			Error += cols * Train(inp.block(0, cur, rowsI, cols), outp.block(0, cur, rowsO, cols));
			cur += BatchSize;
		}
		return Error / inp.cols();
	}
	float NetContainer::TrainRaw(const clMatrix& inp, const clMatrix& outp, size_t BatchSize) {
		float Error = 0;
		size_t cur = 0;
		size_t rowsI = inp.rows();
		size_t rowsO = outp.rows();
		while (cur < size_t(inp.cols())) {
			size_t cols = std::min(inp.cols() - cur, BatchSize);
			Error += cols * TrainRaw(inp.block(0, cur, rowsI, cols), outp.block(0, cur, rowsO, cols));
			cur += BatchSize;
		}
		return Error / inp.cols();
	}
	float NetContainer::Train(const std::vector<clMatrix>& inp, const clMatrix& outp) {
		for (auto i = 0; i<inp.size(); i++)
			Inputs[i]->Input(inp[i]);
		FeedForward();
		return TrainFnc(outp);
	}
	float NetContainer::TrainRaw(const std::vector<clMatrix>& inp, const clMatrix& outp) {
		for (auto i = 0; i<inp.size(); i++)
			Inputs[i]->InputRaw(inp[i]);
		FeedForward();
		return TrainFnc(outp);
	}

	void NetContainer::Train_NoError(const clMatrix& inp, const clMatrix& outp) {
		Inputs[0]->Input(inp);
		FeedForward();
		TrainFnc_NoError(outp);
	}
	void NetContainer::TrainRaw_NoError(const clMatrix& inp, const clMatrix& outp) {
		Inputs[0]->InputRaw(inp);
		FeedForward();
		TrainFnc_NoError(outp);
	}

	void NetContainer::Train_NoError(const clMatrix& inp, const clMatrix& outp, size_t BatchSize) {
		size_t cur = 0;
		size_t rowsI = inp.rows();
		size_t rowsO = outp.rows();
		while (cur < size_t(inp.cols())) {
			size_t cols = std::min(inp.cols() - cur, BatchSize);
			Train_NoError(inp.block(0, cur, rowsI, cols), outp.block(0, cur, rowsO, cols));
			cur += BatchSize;
		}
	}
	void NetContainer::TrainRaw_NoError(const clMatrix& inp, const clMatrix& outp, size_t BatchSize) {
		size_t cur = 0;
		size_t rowsI = inp.rows();
		size_t rowsO = outp.rows();
		while (cur < size_t(inp.cols())) {
			size_t cols = std::min(inp.cols() - cur, BatchSize);
			TrainRaw_NoError(inp.block(0, cur, rowsI, cols), outp.block(0, cur, rowsO, cols));
			cur += BatchSize;
		}
	}
	void NetContainer::Train_NoError(const std::vector<clMatrix>& inp, const clMatrix& outp) {
		for (auto i = 0; i<inp.size(); i++)
			Inputs[i]->Input(inp[i]);
		FeedForward();
		TrainFnc_NoError(outp);
	}
	void NetContainer::TrainRaw_NoError(const std::vector<clMatrix>& inp, const clMatrix& outp) {
		for (auto i = 0; i<inp.size(); i++)
			Inputs[i]->InputRaw(inp[i]);
		FeedForward();
		TrainFnc_NoError(outp);
	}


	float NetContainer::Error(const clMatrix& inp, const clMatrix& outp, size_t BatchSize) const {
		float Error = 0;
		size_t cur = 0;
		size_t rowsI = inp.rows();
		size_t rowsO = outp.rows();
		while (cur < size_t(inp.cols())) {
			size_t cols = std::min(inp.cols() - cur, BatchSize);
			Inputs[0]->Input(inp.block(0, cur, rowsI, cols));
			FeedForward();
			Error += cols * GetErrorFnc(outp.block(0, cur, rowsO, cols));
			cur += BatchSize;
		}
		return Error / inp.cols();
	}
	float NetContainer::ErrorRaw(const clMatrix& inp, const clMatrix& outp, size_t BatchSize) const {
		float Error = 0;
		size_t cur = 0;
		size_t rowsI = inp.rows();
		size_t rowsO = outp.rows();
		while (cur < size_t(inp.cols())) {
			size_t cols = std::min(inp.cols() - cur, BatchSize);
			Inputs[0]->InputRaw(inp.block(0, cur, rowsI, cols));
			FeedForward();
			Error += cols * GetErrorFnc(outp.block(0, cur, rowsO, cols));
			cur += BatchSize;
		}
		return Error / inp.cols();
	}

	float NetContainer::Error(const clMatrix& inp, const clMatrix& outp) const {
		Inputs[0]->Input(inp);
		FeedForward();
		return GetErrorFnc(outp);
	}
	float NetContainer::ErrorRaw(const clMatrix& inp, const clMatrix& outp) const {
		Inputs[0]->InputRaw(inp);
		FeedForward();
		return GetErrorFnc(outp);
	}
	float NetContainer::Error(const std::vector<clMatrix>& inp, const clMatrix& outp) const {
		for (auto i = 0; i<inp.size(); i++)
			Inputs[i]->Input(inp[i]);
		FeedForward();
		return GetErrorFnc(outp);
	}
	float NetContainer::ErrorRaw(const std::vector<clMatrix>& inp, const clMatrix& outp) const {
		for (auto i = 0; i<inp.size(); i++)
			Inputs[i]->InputRaw(inp[i]);
		FeedForward();
		return GetErrorFnc(outp);
	}


	float NetContainer::Train_MeanSquare(const clMatrix& TrainData) {
		clMatrix dif = clMatrix::Temp(TrainData - output);
		error = (0.5f * dif.Norm2()) / dif.cols();
		LastLayer->BackPropagate(dif);
		LastLayer->UpdateWeights();
		return error;
	}
	void NetContainer::Train_MeanSquare_NoError(const clMatrix& TrainData) {
		LastLayer->BackPropagate(TrainData - output);
		LastLayer->UpdateWeights();
	}
	float NetContainer::GetError_MeanSquare(const clMatrix& Data) {
		clMatrix dif = Data - output;
		return (0.5f * dif.Norm2()) / dif.cols();
	}

	float NetContainer::Train_SoftMax(const clMatrix& TrainData) {
		clMatrix tmp = clMatrix::Temp(TrainData.Element_Multiply_log(output) / -output.cols());

		float Error = tmp.Sum();

		LastLayer->BackPropagate(TrainData - output);
		LastLayer->UpdateWeights();
		return Error;
	}

	float NetContainer::GetError_SoftMax(const clMatrix& Data) {
		clMatrix tmp = clMatrix::Temp(Data.rows(), Data.cols());
		tmp = -Data.Element_Multiply(log(output)) / output.cols();
		float Error = tmp.Sum();
		return Error;
	}

	void NetContainer::FeedForwardMeanSquare() {
		LastLayer->FeedForward(++Version);
		output.Set(outPart(LastLayer->Output()));
	}
	void NetContainer::FeedForwardSoftMax() {
		LastLayer->FeedForward(++Version);

		clMatrix tmp = clMatrix::Temp(exp(outPart(LastLayer->Output())));
		if (Ones.cols()!=tmp.rows()) Ones.Reset(1, tmp.rows()) = 1.0f;
		clMatrix sums = clMatrix::Temp((Ones * tmp).Transpose());

		if (output.cols() == tmp.cols()) {
			output.Div_diag(tmp, sums);
			return;
		} else
			output.Reset(tmp.rows(), tmp.cols()).Div_diag(tmp, sums);
	}
	void NetContainer::FeedForwardStableSoftMax() {
		//LastLayer->FeedForward(++Version);
		//clMatrix tmp = LastLayer->Output();
		//output = outPart(tmp);
		////CorrectMat(output);

		//for (auto i = 0; i < output.cols(); i++) {
		//	auto r = output.col(i);
		//	r -= clMatrix::Constant(r.rows(), 1, r.maxCoeff());
		//	r = Eigen::exp(r.array());
		//	r *= 1.0f / r.sum();
		//}
	}
	
	//clMatrix Reorder2(const clMatrix& data, const size_t* Order, size_t count) {
	//	clMatrix ret(data.rows(), count);
	//	for (auto i = 0; i<count; i++)
	//		ret.col(i) = data.col(Order[i]);
	//	return ret;
	//}
	//void NetContainer::SetBatchSize(int sz) {
	//	tmpOrder.resize(sz);
	//}
	//float NetContainer::Iterate() {
	//	if (tmpOrder.size()<TrainOrder.size()) {
	//		int64_t i;
	//		for (i = 0; i<int64_t(tmpOrder.size()) && curIteration<int64_t(TrainOrder.size()); i++, curIteration++) {
	//			tmpOrder[i] = TrainOrder[curIteration];
	//		}
	//		if (i<int64_t(tmpOrder.size())) {
	//			std::random_device rd;
	//			std::mt19937 g(rd());
	//			std::shuffle(TrainOrder.begin(), TrainOrder.end(), g);
	//			curIteration = 0;

	//			for (; i<int64_t(tmpOrder.size()) && curIteration<int64_t(TrainOrder.size()); i++, curIteration++) {
	//				tmpOrder[i] = TrainOrder[curIteration];
	//			}
	//		}

	//		for (auto i = 0; i<Inputs.size(); i++) {
	//			Inputs[i]->InputRaw(Reorder2(AllInputData[i], tmpOrder.data(), tmpOrder.size()));
	//		}
	//		FeedForward();
	//		return TrainFnc(Reorder2(AllTrainData, tmpOrder.data(), tmpOrder.size()));
	//	} else {
	//		for (auto i = 0; i<Inputs.size(); i++) {
	//			Inputs[i]->InputRaw(AllInputData[i]);
	//		}
	//		FeedForward();
	//		return TrainFnc(AllTrainData);
	//	}
	//}

	//NetDNA NetContainer::GetDNA() {
	//	std::map<void*, WeightData> data;
	//	LastLayer->DNA(data);
	//	NetDNA ret;
	//	ret.Size = 0;
	//	for (auto& [l, dt] : data) {
	//		dt.Offset = ret.Size;
	//		ret.Size += dt.Count;
	//		ret.Layers.push_back(dt);
	//	}
	//	return ret;
	//}


	//void NetContainer::AddTrainDataRaw(const clMatrix& inp, const clMatrix& outp) {
	//	AddTrainDataRaw(std::vector<clMatrix>{inp}, outp);
	//}

	//template<typename Container>
	//void ForEach2(const Container& Data, std::function<void(decltype(Data.front())&, size_t)> fnc) {
	//	size_t i = 0;
	//	for (auto& it: Data)fnc(it, i++);
	//}
	//void NetContainer::AddTrainDataRaw(const std::vector<clMatrix>& inp, const clMatrix& outp) {
	//	curIteration = 0;
	//	if (!AllInputData.size()) {
	//		AllInputData.reserve(inp.size());
	//		for (auto& i : inp)
	//			AllInputData.push_back(i);
	//		AllTrainData = outp;
	//	} else {
	//		ForEach2(inp, [&](auto& item, auto i) {
	//			auto& t = AllInputData[i];
	//			auto newInput = clMatrix(item.rows(), t.cols() + item.cols());
	//			newInput << t, item;
	//			t = newInput;
	//		});
	//		auto newOut = clMatrix(outp.rows(), outp.cols() + AllTrainData.cols());
	//		newOut << AllTrainData, outp;
	//		AllTrainData = newOut;
	//	}

	//	auto next = TrainOrder.size();
	//	TrainOrder.resize(next + outp.cols());
	//	for (; next<TrainOrder.size(); next++)
	//		TrainOrder[next] = next;

	//	{
	//		std::random_device rd;
	//		std::mt19937 g(rd());
	//		std::shuffle(TrainOrder.begin(), TrainOrder.end(), g);
	//	}
	//}

	//void NetContainer::AddTrainData(const clMatrix& inp, const clMatrix& outp) {
	//	AddTrainData(std::vector<clMatrix>{inp}, outp);
	//}
	//void NetContainer::AddTrainData(const std::vector<clMatrix>& inp, const clMatrix& outp) {
	//	curIteration = 0;
	//	if (!AllInputData.size()) {
	//		AllInputData.reserve(inp.size());
	//		size_t c = 0;
	//		for (auto& i : inp)
	//			AllInputData.push_back(Inputs[c++]->MakeRawInput(i));
	//		AllTrainData = outp;
	//	} else {
	//		ForEach2(inp, [&](auto& item, auto i) {
	//			auto& t = AllInputData[i];
	//			auto newInput = clMatrix(item.rows() + 1, t.cols() + item.cols());
	//			newInput << t, Inputs[i]->MakeRawInput(item);
	//			t = newInput;
	//		});
	//		auto newOut = clMatrix(outp.rows(), outp.cols() + AllTrainData.cols());
	//		newOut << AllTrainData, outp;
	//		AllTrainData = newOut;
	//	}

	//	auto next = TrainOrder.size();
	//	TrainOrder.resize(next + outp.cols());
	//	for (; next<TrainOrder.size(); next++)
	//		TrainOrder[next] = next;

	//	{
	//		std::random_device rd;
	//		std::mt19937 g(rd());
	//		std::shuffle(TrainOrder.begin(), TrainOrder.end(), g);
	//	}
	//}

	clMatrix NetContainer::FromVector(const std::vector<float>& Data) {
		auto r = LastLayer->nOutput();
		clMatrix ret(r, Data.size()/r, Data);
		return ret;
	}
	std::vector<float> NetContainer::ProcessVec(const std::vector<float>& Inp) {
		auto tmp = clMatrix::GetReadTemp(output.rows(), output.cols());
		tmp.SetData(ProcessRaw(Inputs[0]->MakeRawInput(Inp)));
		return tmp.ReadBlocking();
	}
};
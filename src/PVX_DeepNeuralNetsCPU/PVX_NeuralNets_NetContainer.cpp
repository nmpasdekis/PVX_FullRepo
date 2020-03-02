#include <PVX_NeuralNetsCPU.h>
#include "PVX_NeuralNets_Util.inl"

namespace PVX::DeepNeuralNets {

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
	void NetContainer::SaveCheckpoint() {
		Checkpoint.GetData(CheckpointDNA);
		CheckpointError = error;
	}
	float NetContainer::LoadCheckpoint() {
		error = CheckpointError;
		Checkpoint.SetData(CheckpointDNA.data());
		return error;
	}
	float NetContainer::SaveCheckpoint(std::vector<float>& data) {
		Checkpoint.GetData(data);
		return error;
	}
	void NetContainer::LoadCheckpoint(const std::vector<float>& data, float Error) {
		Checkpoint.SetData(data.data());
		error = Error;
	}
	void NetContainer::SetLearnRate(float a) { LastLayer->SetLearnRate(a); }
	void NetContainer::SetRMSprop(float Beta) { LastLayer->SetRMSprop(Beta); }
	void NetContainer::SetMomentum(float Beta) { LastLayer->SetMomentum(Beta); }
	void NetContainer::ResetMomentum() { LastLayer->ResetMomentum(); }

	void NetContainer::ResetRNN() {
		for (auto r : RNNs) r->Reset();
	}
	netData NetContainer::MakeRawInput(const netData& inp) {
		return Inputs[0]->MakeRawInput(inp);
	}
	netData NetContainer::MakeRawInput(const std::vector<float>& inp) {
		return Inputs[0]->MakeRawInput(inp);
	}
	std::vector<netData> NetContainer::MakeRawInput(const std::vector<netData>& inp) {
		std::vector<netData> ret;
		size_t i = 0;
		for (auto l: Inputs)
			ret.push_back(l->MakeRawInput(inp[i++]));
		return ret;
	}
	void NetContainer::Init() {
		switch (Type) {
			case PVX::DeepNeuralNets::OutputType::MeanSquare:
				FeedForward = [this] { FeedForwardMeanSquare(); };
				GetErrorFnc = [this](const netData& Data) { return GetError_MeanSquare(Data); };
				TrainFnc = [this](const netData& Data) { return Train_MeanSquare(Data); };
				break;
			case PVX::DeepNeuralNets::OutputType::SoftMax:
				FeedForward = [this] { FeedForwardSoftMax(); };
				GetErrorFnc = [this](const netData& Data) { return GetError_SoftMax(Data); };
				TrainFnc = [this](const netData& Data) { return Train_SoftMax(Data); };
				break;
			case PVX::DeepNeuralNets::OutputType::StableSoftMax:
				FeedForward = [this] { FeedForwardStableSoftMax(); };
				GetErrorFnc = [this](const netData& Data) { return GetError_SoftMax(Data); };
				TrainFnc = [this](const netData& Data) { return Train_SoftMax(Data); };
				break;
			default:
				break;
		}

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

		Checkpoint = GetDNA();
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

	netData NetContainer::Process(const netData& inp) const {
		Inputs[0]->Input(inp);
		FeedForward();
		return output;
		//LastLayer->FeedForward(++Version);
		////return LastLayer->Output();
		//return outPart(LastLayer->output);
	}
	netData NetContainer::Process(const std::vector<netData>& inp) const {
		for (auto i = 0; i<inp.size(); i++)
			Inputs[i]->Input(inp[i]);
		FeedForward();
		return output;
		//LastLayer->FeedForward(++Version);
		////return LastLayer->Output();
		//return outPart(LastLayer->output);
	}
	netData NetContainer::ProcessRaw(const netData& inp) const {
		Inputs[0]->InputRaw(inp);
		FeedForward();
		return output;
		//LastLayer->FeedForward(++Version);
		////return LastLayer->Output();
		//return outPart(LastLayer->output);
	}
	netData NetContainer::ProcessRaw(const std::vector<netData>& inp) const {
		for (auto i = 0; i<inp.size(); i++)
			Inputs[i]->InputRaw(inp[i]);
		FeedForward();
		return output;
		//LastLayer->FeedForward(++Version);
		////return LastLayer->Output();
		//return outPart(LastLayer->output);
	}
	float NetContainer::Train(const netData& inp, const netData& outp) {
		Inputs[0]->Input(inp);
		FeedForward();
		return TrainFnc(outp);
	}
	float NetContainer::TrainRaw(const netData& inp, const netData& outp) {
		Inputs[0]->InputRaw(inp);
		FeedForward();
		return TrainFnc(outp);
	}

	float NetContainer::Train(const netData& inp, const netData& outp, size_t BatchSize) {
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
	float NetContainer::TrainRaw(const netData& inp, const netData& outp, size_t BatchSize) {
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

	float NetContainer::Train(const std::vector<netData>& inp, const netData& outp) {
		for (auto i = 0; i<inp.size(); i++)
			Inputs[i]->Input(inp[i]);
		FeedForward();
		return TrainFnc(outp);
	}
	float NetContainer::TrainRaw(const std::vector<netData>& inp, const netData& outp) {
		for (auto i = 0; i<inp.size(); i++)
			Inputs[i]->InputRaw(inp[i]);
		FeedForward();
		return TrainFnc(outp);
	}

	float NetContainer::Error(const netData& inp, const netData& outp, size_t BatchSize) const {
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
	float NetContainer::ErrorRaw(const netData& inp, const netData& outp, size_t BatchSize) const {
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

	float NetContainer::Error(const netData& inp, const netData& outp) const {
		Inputs[0]->Input(inp);
		FeedForward();
		return GetErrorFnc(outp);
	}
	float NetContainer::ErrorRaw(const netData& inp, const netData& outp) const {
		Inputs[0]->InputRaw(inp);
		FeedForward();
		return GetErrorFnc(outp);
	}
	float NetContainer::Error(const std::vector<netData>& inp, const netData& outp) const {
		for (auto i = 0; i<inp.size(); i++)
			Inputs[i]->Input(inp[i]);
		FeedForward();
		return GetErrorFnc(outp);
	}
	float NetContainer::ErrorRaw(const std::vector<netData>& inp, const netData& outp) const {
		for (auto i = 0; i<inp.size(); i++)
			Inputs[i]->InputRaw(inp[i]);
		FeedForward();
		return GetErrorFnc(outp);
	}


	float NetContainer::Train_MeanSquare(const netData& TrainData) {
		netData dif = TrainData - output;
		Eigen::Map<Eigen::RowVectorXf> vec(dif.data(), dif.size());
		error = (0.5f * (vec * vec.transpose())(0)) / dif.cols();
		LastLayer->BackPropagate(dif);
		LastLayer->UpdateWeights();
		return error;
	}
	float NetContainer::GetError_MeanSquare(const netData& Data) {
		netData dif = Data - output;
		Eigen::Map<Eigen::RowVectorXf> vec(dif.data(), dif.size());
		return (0.5f * (vec * vec.transpose())(0)) / dif.cols();
	}

	float NetContainer::Train_SoftMax(const netData& TrainData) {
		float Error = -(TrainData.array()* Eigen::log(output.array())).sum() / output.cols();
		LastLayer->BackPropagate(TrainData - output);
		LastLayer->UpdateWeights();
		return Error;
	}
	float NetContainer::GetError_SoftMax(const netData& Data) {
		return -(Data.array()* Eigen::log(output.array())).sum() / output.cols();
	}

	void NetContainer::FeedForwardMeanSquare() {
		LastLayer->FeedForward(++Version);
		auto tmp = LastLayer->Output();
		output = outPart(tmp);
	}
	void NetContainer::FeedForwardSoftMax() {
		LastLayer->FeedForward(++Version);
		auto tmp2 = LastLayer->Output();

		CorrectMat(tmp2);

		netData tmp = Eigen::exp(outPart(tmp2).array());
		CorrectMat(tmp);

		netData a = 1.0f / (netData::Ones(1, tmp.rows()) * tmp).array();
		CorrectMat(a);
		netData div = Eigen::Map<Eigen::RowVectorXf>(a.data(), a.size()).asDiagonal();
		CorrectMat(div);
		output = (tmp * div);
		CorrectMat(output);
	}
	void NetContainer::FeedForwardStableSoftMax() {
		LastLayer->FeedForward(++Version);
		netData tmp = LastLayer->Output();
		output = outPart(tmp);
		CorrectMat(output);

		for (auto i = 0; i < output.cols(); i++) {
			auto r = output.col(i);
			r -= netData::Constant(r.rows(), 1, r.maxCoeff());
			r = Eigen::exp(r.array());
			r *= 1.0f / r.sum();

			CorrectMat(r);
		}
	}
	
	netData Reorder2(const netData& data, const size_t* Order, size_t count) {
		netData ret(data.rows(), count);
		for (auto i = 0; i<count; i++)
			ret.col(i) = data.col(Order[i]);
		return ret;
	}
	void NetContainer::SetBatchSize(int sz) {
		tmpOrder.resize(sz);
	}
	float NetContainer::Iterate() {
		if (tmpOrder.size()<TrainOrder.size()) {
			int64_t i;
			for (i = 0; i<int64_t(tmpOrder.size()) && curIteration<int64_t(TrainOrder.size()); i++, curIteration++) {
				tmpOrder[i] = TrainOrder[curIteration];
			}
			if (i<int64_t(tmpOrder.size())) {
				std::random_device rd;
				std::mt19937 g(rd());
				std::shuffle(TrainOrder.begin(), TrainOrder.end(), g);
				curIteration = 0;

				for (; i<int64_t(tmpOrder.size()) && curIteration<int64_t(TrainOrder.size()); i++, curIteration++) {
					tmpOrder[i] = TrainOrder[curIteration];
				}
			}

			for (auto i = 0; i<Inputs.size(); i++) {
				Inputs[i]->InputRaw(Reorder2(AllInputData[i], tmpOrder.data(), tmpOrder.size()));
			}
			FeedForward();
			return TrainFnc(Reorder2(AllTrainData, tmpOrder.data(), tmpOrder.size()));
		} else {
			for (auto i = 0; i<Inputs.size(); i++) {
				Inputs[i]->InputRaw(AllInputData[i]);
			}
			FeedForward();
			return TrainFnc(AllTrainData);
		}
	}

	NetDNA NetContainer::GetDNA() {
		std::map<void*, WeightData> data;
		LastLayer->DNA(data);
		NetDNA ret;
		ret.Size = 0;
		for (auto& [l, dt] : data) {
			dt.Offset = ret.Size;
			ret.Size += dt.Count;
			ret.Layers.push_back(dt);
		}
		return ret;
	}


	void NetContainer::AddTrainDataRaw(const netData& inp, const netData& outp) {
		AddTrainDataRaw(std::vector<netData>{inp}, outp);
	}

	template<typename Container>
	void ForEach2(const Container& Data, std::function<void(decltype(Data.front())&, size_t)> fnc) {
		size_t i = 0;
		for (auto& it: Data)fnc(it, i++);
	}
	void NetContainer::AddTrainDataRaw(const std::vector<netData>& inp, const netData& outp) {
		curIteration = 0;
		if (!AllInputData.size()) {
			AllInputData.reserve(inp.size());
			for (auto& i : inp)
				AllInputData.push_back(i);
			AllTrainData = outp;
		} else {
			ForEach2(inp, [&](auto& item, auto i) {
				auto& t = AllInputData[i];
				auto newInput = netData(item.rows(), t.cols() + item.cols());
				newInput << t, item;
				t = newInput;
			});
			auto newOut = netData(outp.rows(), outp.cols() + AllTrainData.cols());
			newOut << AllTrainData, outp;
			AllTrainData = newOut;
		}

		auto next = TrainOrder.size();
		TrainOrder.resize(next + outp.cols());
		for (; next<TrainOrder.size(); next++)
			TrainOrder[next] = next;

		{
			std::random_device rd;
			std::mt19937 g(rd());
			std::shuffle(TrainOrder.begin(), TrainOrder.end(), g);
		}
	}

	void NetContainer::AddTrainData(const netData& inp, const netData& outp) {
		AddTrainData(std::vector<netData>{inp}, outp);
	}
	void NetContainer::AddTrainData(const std::vector<netData>& inp, const netData& outp) {
		curIteration = 0;
		if (!AllInputData.size()) {
			AllInputData.reserve(inp.size());
			size_t c = 0;
			for (auto& i : inp)
				AllInputData.push_back(Inputs[c++]->MakeRawInput(i));
			AllTrainData = outp;
		} else {
			ForEach2(inp, [&](auto& item, auto i) {
				auto& t = AllInputData[i];
				auto newInput = netData(item.rows() + 1, t.cols() + item.cols());
				newInput << t, Inputs[i]->MakeRawInput(item);
				t = newInput;
			});
			auto newOut = netData(outp.rows(), outp.cols() + AllTrainData.cols());
			newOut << AllTrainData, outp;
			AllTrainData = newOut;
		}

		auto next = TrainOrder.size();
		TrainOrder.resize(next + outp.cols());
		for (; next<TrainOrder.size(); next++)
			TrainOrder[next] = next;

		{
			std::random_device rd;
			std::mt19937 g(rd());
			std::shuffle(TrainOrder.begin(), TrainOrder.end(), g);
		}
	}

	netData NetContainer::FromVector(const std::vector<float>& Data) {
		auto r = LastLayer->nOutput();
		netData ret(r, Data.size()/r);
		memcpy(ret.data(), Data.data(), Data.size() * sizeof(float));
		return ret;
	}
	std::vector<float> NetContainer::ProcessVec(const std::vector<float>& Inp) {
		auto tmp = ProcessRaw(Inputs[0]->MakeRawInput(Inp));
		std::vector<float> ret(tmp.size());
		memcpy(ret.data(), tmp.data(), ret.size() * sizeof(float));
		return ret;
	}
};
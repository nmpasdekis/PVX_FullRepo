#include <PVX_NeuralNetsCPU.h>
#include "PVX_NeuralNets_Util.inl"


namespace PVX::DeepNeuralNets {
	NeuralLayer_Base* RecurrentLayer::newCopy(const std::map<NeuralLayer_Base*, size_t>& IndexOf) {
		auto ret = new RecurrentLayer(
			reinterpret_cast<NeuralLayer_Base*>(IndexOf.at(PreviousLayer)), 
			reinterpret_cast<RecurrentInput*>(IndexOf.at(RNN_Input))
		);
		ret->Id = Id;
		return ret;
	}
	void RecurrentLayer::FeedForward(int64_t Version) {
		if (Version > FeedVersion) {
			RNN_Input->FeedForward(Version);
			int64_t bSize = RNN_Input->BatchSize();
			if (output.cols() != bSize) {
				output = netData::Zero(output.rows(), bSize);
				output(output.rows()-1, 0) = 1.0f;
			}
			for (int64_t i = 0; i<bSize; i++) {
				RNN_Input->output.block(0, i, RNN_Input->RecurrentNeuronCount, 1) = output.block(0, (i + bSize - 1) % bSize, output.rows() - 1, 1);
				PreviousLayer->FeedForward(i, Version);
				output.col(i) = PreviousLayer->Output(i);
			}
			output.row(output.rows()-1) = netData::Ones(1, bSize);
		}
	}
	void RecurrentLayer::Reset() {
		output.block(0, output.cols()-1, output.rows()-1, 1) = netData::Zero(output.rows()-1, 1);
	}
	void RecurrentLayer::FeedForward(int64_t Index, int64_t Version) {
		throw "Unimplementable?";
	}

	RecurrentLayer::RecurrentLayer(int64_t outCount) {
		output = netData::Ones(outCount, 1);
	}

	RecurrentLayer::RecurrentLayer(NeuralLayer_Base* Input, RecurrentInput* RecurrentInput) :
		RNN_Input{ RecurrentInput } {
		PreviousLayer = Input;
		output = Input->Output();
	}

	void RecurrentLayer::Save(PVX::BinSaver& bin, const std::map<NeuralLayer_Base*, size_t>& IndexOf) const {
		bin.Begin("RNNe");
		{
			bin.Write("INPC", int(output.rows()));
			bin.Write("NDID", int(Id));
			bin.Begin("LYRS"); {
				bin.write(int(IndexOf.at(PreviousLayer)));
				bin.write(int(IndexOf.at(RNN_Input)));
			} bin.End();
		}
		bin.End();
	}
	RecurrentLayer* RecurrentLayer::Load2(PVX::BinLoader& bin) {
		int Id = -1;
		int outCount;
		std::vector<int> layers;
		bin.Process("INPC", [&](PVX::BinLoader& bin2) { outCount = bin2.read<int>(); });
		bin.Process("NDID", [&](PVX::BinLoader& bin2) { Id = bin2.read<int>(); });
		bin.Read("LYRS", layers);
		bin.Execute();
		auto rnn = new RecurrentLayer(outCount);
		rnn->PreviousLayer = reinterpret_cast<NeuralLayer_Base*>(((char*)0) + layers[0]);
		rnn->RNN_Input = reinterpret_cast<RecurrentInput*>(((char*)0) + layers[1]);
		if (Id>=0)rnn->Id = Id;
		return rnn;
	}

	size_t RecurrentLayer::DNA(std::map<void*, WeightData>& Weights) {
		return PreviousLayer->DNA(Weights);
	}
	void RecurrentLayer::BackPropagate(const netData& Gradient) {
		RNN_Input->ZeroGradient(Gradient.cols());
		netData grad = Gradient;
		int64_t i = Gradient.cols() - 1;
		PreviousLayer->BackPropagate(grad.col(i), i);
		i--;
		for (; i>=0; i--) {
			grad.col(i) += RNN_Input->gradient.block(0, i + 1, grad.rows(), 1);
			PreviousLayer->BackPropagate(grad.col(i), i);
		}
		RNN_Input->BackPropagate();
	}
	void RecurrentLayer::BackPropagate(const netData&, int64_t) {
		throw "Unimplementable?";
	}
	size_t RecurrentLayer::nInput() const {
		return PreviousLayer->nInput();
	}
	void RecurrentLayer::UpdateWeights() {
		PreviousLayer->UpdateWeights();
	}

	RecurrentInput::RecurrentInput(int64_t RecurrentNeurons, int64_t nOut) :
		RecurrentNeuronCount{ RecurrentNeurons } 
	{
		PreviousLayer = nullptr;
		output = netData::Zero(RecurrentNeurons + nOut + 1, 1);
	}
	RecurrentInput::RecurrentInput(NeuralLayer_Base* Input, int64_t RecurrentNeurons) : RecurrentNeuronCount{ RecurrentNeurons } {
		PreviousLayer = Input;
		output = netData::Zero(RecurrentNeurons + Input->nOutput() + 1, 1);
	}
	void RecurrentInput::Save(PVX::BinSaver& bin, const std::map<NeuralLayer_Base*, size_t>& IndexOf) const {
		bin.Begin("RNNs");
		{
			bin.Write("NDID", int(Id));
			bin.Write("INPC", int(nInput()));
			bin.Write("RNNC", int(RecurrentNeuronCount));
			bin.Begin("LYRS"); {
				bin.write(int(IndexOf.at(PreviousLayer)));
			} bin.End();
		}
		bin.End();
	}
	NeuralLayer_Base* RecurrentInput::newCopy(const std::map<NeuralLayer_Base*, size_t>& IndexOf) {
		auto ret = new RecurrentInput(
			reinterpret_cast<NeuralLayer_Base*>(IndexOf.at(PreviousLayer)),
			RecurrentNeuronCount
		);
		ret->Id = Id;
		return ret;
	}
	RecurrentInput* RecurrentInput::Load2(PVX::BinLoader& bin) {
		int Id = -1;
		int RecurrentNeuronCount;
		int outCount;
		int layer = -1;
		bin.Read("LYRS", layer);
		bin.Process("NDID", [&](PVX::BinLoader& bin2) { Id = bin2.read<int>(); });
		bin.Read("INPC", outCount);
		bin.Read("RNNC", RecurrentNeuronCount);
		bin.Execute();
		auto* rnn = new RecurrentInput(RecurrentNeuronCount, outCount);
		rnn->PreviousLayer = reinterpret_cast<NeuralLayer_Base*>(((char*)0) + layer);

		if (Id>=0)rnn->Id = Id;
		return rnn;
	}
	//int RecurrentInput::BatchSize() {
	//	return int(PreviousLayer->Output().cols());
	//}
	//void RecurrentInput::FeedIndex(int i) {
	//	output.block(RecurrentNeuronCount, 0, rnnData.rows(), 1) = rnnData.col(i);
	//}
	Eigen::Block<netData, -1, -1, false> RecurrentInput::Recur(int64_t Index) {
		return output.block(0, Index, RecurrentNeuronCount, 1);
	}
	void RecurrentInput::FeedForward(int64_t ver) {
		if (ver>FeedVersion) {
			FeedVersion = ver;
			PreviousLayer->FeedForward(ver);
			const auto& prev = PreviousLayer->Output();
			if (output.cols()!=prev.cols()) {
				output = netData::Zero(prev.rows() + RecurrentNeuronCount, prev.cols());
			}
			output.block(RecurrentNeuronCount, 0, prev.rows(), prev.cols()) = prev;
		}
	}
	void RecurrentInput::FeedForward(int64_t Index, int64_t ver) {
		FeedForward(ver);
	}




	size_t RecurrentInput::DNA(std::map<void*, WeightData>& Weights) {
		return PreviousLayer->DNA(Weights);
	}
	void RecurrentInput::BackPropagate() {
		PreviousLayer->BackPropagate(gradient.block(RecurrentNeuronCount, 0, gradient.rows()- RecurrentNeuronCount, gradient.cols()));
	}
	void RecurrentInput::ZeroGradient(int64_t cols) {
		if ((PreviousLayer->nOutput() + RecurrentNeuronCount)!= gradient.rows() || gradient.cols()!=cols) {
			gradient = netData::Zero(PreviousLayer->nOutput() + RecurrentNeuronCount, cols);
		} else {
			memset(gradient.data(), 0, sizeof(float) * gradient.cols() * gradient.rows());
		}
	}
	void RecurrentInput::BackPropagate(const netData& grad) {
		throw "Unimplementable?";
		//PreviousLayer->BackPropagate(grad.block(RecurrentNeuronCount, 0, grad.rows()- RecurrentNeuronCount, grad.cols()));
	}
	void RecurrentInput::BackPropagate(const netData& grad, int64_t Index) {
		gradient.col(Index) += grad;
	}
	size_t RecurrentInput::nInput() const {
		return output.rows() - 1;
	}
	void RecurrentInput::UpdateWeights() {
		PreviousLayer->UpdateWeights();
	}


	RecurrentUtility::RecurrentUtility(NeuralLayer_Base* inp, int64_t OutSize, LayerActivation Activate, TrainScheme Train) :
		Input(inp, OutSize),
		State(&Input, OutSize, Activate, Train),
		Layer(&State, &Input)
	{}
}
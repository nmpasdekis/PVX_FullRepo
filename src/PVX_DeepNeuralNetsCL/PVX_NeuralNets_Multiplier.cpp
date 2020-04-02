#include <PVX_NeuralNetsCL.h>
#include "PVX_NeuralNets_Util.inl"

namespace PVX::NeuralNets {
	void NeuronMultiplier::Save(PVX::BinSaver& bin, const std::map<NeuralLayer_Base*, size_t>& IndexOf) const {
		bin.Begin("MULP");
		{
			bin.Write("NDID", int(Id));
			bin.Write("INPC", int(nInput()));
			bin.Begin("LYRS");
			{
				for (auto& i: InputLayers)
					bin.write(int(IndexOf.at(i)));
			} bin.End();
		}
		bin.End();
	}

	NeuronMultiplier* NeuronMultiplier::Load2(PVX::BinLoader& bin) {
		int inp, Id = -1;
		std::vector<int> layers;
		bin.Read("INPC", inp);
		bin.Read("LYRS", layers);
		bin.Process("NDID", [&](PVX::BinLoader& bin2) { Id = bin2.read<int>(); });
		bin.Execute();
		auto add = new NeuronMultiplier(inp);
		if (Id>=0)add->Id = Id;
		for (auto l : layers) {
			add->InputLayers.push_back(reinterpret_cast<NeuralLayer_Base*>(((char*)0) + l));
		}
		return add;
	}

	NeuralLayer_Base* NeuronMultiplier::newCopy(const std::map<NeuralLayer_Base*, size_t>& IndexOf) {
		auto ret = new NeuronMultiplier(nInput());
		for (auto l : InputLayers)
			ret->InputLayers.push_back(reinterpret_cast<NeuralLayer_Base*>(IndexOf.at(l)));
		ret->Id = Id;
		return ret;
	}

	NeuronMultiplier::NeuronMultiplier(const size_t inputs) {
		output.SetData(clMatrix(inputs + 1, 1, 0.0f));
		Id = ++NextId;
	}

	NeuronMultiplier::NeuronMultiplier(const std::vector<NeuralLayer_Base*>& inputs) {
		for (auto& i : inputs) InputLayers.push_back(i);
		output.SetData(clMatrix(InputLayers[0]->Output().rows(), 1, 0.0f));
	}
	//size_t NeuronMultiplier::DNA(std::map<void*, WeightData>& Weights) {
	//	size_t ret = 0;
	//	for (auto l : InputLayers)
	//		ret += l->DNA(Weights);
	//	return ret;
	//}

	void NeuronMultiplier::FeedForward(int64_t Version) {
		if (Version > FeedVersion) {
			InputLayers[0]->FeedForward(Version);
			output.CopyOf(InputLayers[0]->Output());
			for (auto i = 1; i < InputLayers.size(); i++) {
				InputLayers[i]->FeedForward(Version);
				output = output.Element_Multiply(InputLayers[i]->Output());
			}
			FeedVersion = Version;
			FeedIndexVersion = output.cols();
		}
	}
	void NeuronMultiplier::FeedForward(int64_t Index, int64_t Version) {
		if (Version > FeedVersion) {
			FeedVersion = Version;
			FeedIndexVersion = -1;
		}
		if (Index > FeedIndexVersion) {
			FeedIndexVersion = Index;
			InputLayers[0]->FeedForward(Index, Version);
			const auto& pro = InputLayers[0]->Output();
			if (pro.cols() != output.cols()) {
				output.Reset(output.rows(), pro.cols());
			}
			auto col = output.col(Index).SetData(pro.col(Index));

			for (auto i = 1; i < InputLayers.size(); i++) {
				InputLayers[i]->FeedForward(Index, Version);
				col = col.Element_Multiply(InputLayers[i]->Output().col(Index));
			}
		}
	}

	void NeuronMultiplier::BackPropagate(clMatrixExpr&& Gradient) {
		grad = std::move(Gradient);
		{
			clMatrixExpr tmp = InputLayers[1]->RealOutput();
			for (auto i = 2; i < InputLayers.size(); i++) {
				tmp *= InputLayers[i]->RealOutput();
			}
			InputLayers[0]->BackPropagate(grad * std::move(tmp));
		}
		for (int j = 1; j < InputLayers.size(); j++) {
			clMatrixExpr tmp = InputLayers[0]->RealOutput();
			for (int i = 1; i < InputLayers.size(); i++)
				if (i != j)
					tmp *= InputLayers[i]->RealOutput();
			InputLayers[j]->BackPropagate(grad * std::move(tmp));
		}
	}
	void NeuronMultiplier::BackPropagate(clMatrixExpr&& Gradient, int64_t Index) {
		grad = std::move(Gradient);
		{
			clMatrixExpr tmp = InputLayers[1]->RealOutput(Index);
			for (auto i = 2; i < InputLayers.size(); i++) {
				tmp *= InputLayers[i]->RealOutput(Index);
			}
			InputLayers[0]->BackPropagate(grad * std::move(tmp), Index);
		}
		for (int j = 1; j < InputLayers.size(); j++) {
			clMatrixExpr tmp = InputLayers[0]->RealOutput(Index);
			for (int i = 1; i < InputLayers.size(); i++)
				if (i != j)
					tmp *= InputLayers[i]->RealOutput(Index);
			InputLayers[j]->BackPropagate(grad * std::move(tmp), Index);
		}
	}

	void NeuronMultiplier::UpdateWeights() {
		for (auto i: InputLayers) i->UpdateWeights();
	}
	size_t NeuronMultiplier::nInput() const {
		return output.cols();
	}
}
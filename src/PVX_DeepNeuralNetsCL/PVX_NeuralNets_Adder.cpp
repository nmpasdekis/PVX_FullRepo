#include <PVX_NeuralNetsCL.h>
#include "PVX_NeuralNets_Util.inl"

namespace PVX::NeuralNets {
	void NeuronAdder::Save(PVX::BinSaver& bin, const std::map<NeuralLayer_Base*, size_t>& IndexOf) const {
		bin.Begin("ADDR");
		{
			bin.Write("NDID", int(Id));
			bin.Write("INPC", int(nInput()));
			bin.Begin("LYRS"); {
				for (auto& i: InputLayers)
					bin.write(int(IndexOf.at(i)));
			} bin.End();
		}
		bin.End();
	}
	NeuronAdder* NeuronAdder::Load2(PVX::BinLoader& bin) {
		int inp, Id = -1;
		std::vector<int> layers;
		bin.Read("INPC", inp);
		bin.Read("LYRS", layers);
		bin.Process("NDID", [&](PVX::BinLoader& bin2) { Id = bin2.read<int>(); });
		bin.Execute();
		auto add = new NeuronAdder(inp);
		if (Id>=0)add->Id = Id;
		for (auto l : layers) {
			add->InputLayers.push_back(reinterpret_cast<NeuralLayer_Base*>(((char*)0) + l));
		}
		return add;
	}
	NeuralLayer_Base* NeuronAdder::newCopy(const std::map<NeuralLayer_Base*, size_t>& IndexOf) {
		auto ret = new NeuronAdder(nInput());
		for (auto l : InputLayers)
			ret->InputLayers.push_back(reinterpret_cast<NeuralLayer_Base*>(IndexOf.at(l)));
		ret->Id = Id;
		return ret;
	}
	NeuronAdder::NeuronAdder(const size_t InputSize) {
		output.Reset(InputSize + 1, 1) = 1.0f;
		Id = ++NextId;
	}
	NeuronAdder::NeuronAdder(const std::vector<NeuralLayer_Base*>& Inputs) : NeuronAdder(Inputs[0]->nOutput()) {
		InputLayers = Inputs;
	}
	NeuronAdder::NeuronAdder(const std::string& Name, const size_t InputSize) {
		name = Name;
		output.Reset(InputSize + 1, 1) = 1.0f;
		Id = ++NextId;
	}
	NeuronAdder::NeuronAdder(const std::string& Name, const std::vector<NeuralLayer_Base*>& Inputs) : NeuronAdder(Inputs[0]->nOutput()) {
		name = Name;
		InputLayers = Inputs;
	}
	void NeuronAdder::FeedForward(int64_t Version) {
		if (Version > FeedVersion) {
			InputLayers[0]->FeedForward(Version);
			auto InBlock = outPart(output.CopyOf(InputLayers[0]->Output()));
			for (auto i = 1; i < InputLayers.size(); i++) {
				InputLayers[i]->FeedForward(Version);
				InBlock += InputLayers[i]->Output();
			}
			FeedVersion = Version;
			FeedIndexVersion = output.cols();
		}
	}
	void NeuronAdder::FeedForward(int64_t Index, int64_t Version) {
		if (Version > FeedVersion) {
			FeedVersion = Version;
			FeedIndexVersion = -1;
		}
		if (Index > FeedIndexVersion) {
			FeedIndexVersion = Index;
			InputLayers[0]->FeedForward(Index, Version);

			const auto& pro = InputLayers[0]->Output();
			if (pro.cols() != output.cols()) {
				output.Reset(output.rows(), pro.cols()) = 0.0f;
			}
			output.col(Index).SetData(pro.col(Index));
			for (auto i = 1; i < InputLayers.size(); i++) {
				InputLayers[i]->FeedForward(Index, Version);
				output.col(Index) += InputLayers[i]->Output(Index);
			}
			output(output.rows()-1, Index) = 1.0f;
		}
	}

	void NeuronAdder::BackPropagate(clMatrixExpr&& Gradient) {
		grad = std::move(Gradient);
		for (auto i : InputLayers) i->BackPropagate(grad);
	}
	void NeuronAdder::BackPropagate(clMatrixExpr&& Gradient, int64_t Index) {
		grad = std::move(Gradient);
		for (auto i : InputLayers) i->BackPropagate(grad, Index);
	}
	void NeuronAdder::UpdateWeights() {
		for (auto i: InputLayers) i->UpdateWeights();
	}
	size_t NeuronAdder::nInput() const {
		return output.rows() - 1;
	}
}
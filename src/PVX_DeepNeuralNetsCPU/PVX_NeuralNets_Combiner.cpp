#include <PVX_NeuralNetsCPU.h>
#include <iostream>

namespace PVX {
	namespace DeepNeuralNets {
		static netData makeComb(const std::vector<NeuralLayer_Base*> & inputs) {
			size_t count = 0;
			for (auto i : inputs) count += i->nOutput();
			return netData::Zero(count + 1, 1);
		}

		void NeuronCombiner::Save(PVX::BinSaver& bin, const std::map<NeuralLayer_Base*, size_t>& IndexOf) const {
			bin.Begin("CMBN");
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

		NeuronCombiner* NeuronCombiner::Load2(PVX::BinLoader& bin) {
			int inp, Id = -1;
			std::vector<int> layers;
			bin.Read("INPC", inp);
			bin.Read("LYRS", layers);
			bin.Process("NDID", [&](PVX::BinLoader& bin2) { Id = bin2.read<int>(); });
			bin.Execute();
			auto add = new NeuronCombiner(inp);
			if (Id>=0)add->Id = Id;
			for (auto l : layers) {
				add->InputLayers.push_back(reinterpret_cast<NeuralLayer_Base*>(((char*)0) + l));
			}
			return add;
		}

		NeuralLayer_Base* NeuronCombiner::newCopy(const std::map<NeuralLayer_Base*,size_t>& IndexOf) {
			auto ret = new NeuronCombiner(nInput());
			for (auto l : InputLayers)
				ret->InputLayers.push_back(reinterpret_cast<NeuralLayer_Base*>(IndexOf.at(l)));
			ret->Id = Id;
			return ret;
		}

		size_t NeuronCombiner::DNA(std::map<void*, WeightData>& Weights) {
			size_t ret = 0;
			for (auto l : InputLayers)
				ret += l->DNA(Weights);
			return ret;
		}

		NeuronCombiner::NeuronCombiner(const size_t inputs) {
			output = netData::Zero(inputs + 1ll, 1ll);
			Id = ++NextId;
		}

		NeuronCombiner::NeuronCombiner(const std::vector<NeuralLayer_Base*> & inputs) {
			for (auto& i:inputs)InputLayers.push_back(i);
			output = makeComb(inputs);
		}
		void NeuronCombiner::FeedForward(int64_t Version) {
			if (Version > FeedVersion) {
				InputLayers[0]->FeedForward(Version);
				size_t Start = 0;
				if (InputLayers[0]->BatchSize() != output.cols()) {
					output.conservativeResize(Eigen::NoChange, InputLayers[0]->BatchSize());
				}
				{
					const auto & o = InputLayers[0]->Output();
					output.block(Start, 0, o.rows(), o.cols()) = o;
					Start += o.rows() - 1;
				}
				for (auto i = 1; i < InputLayers.size();i++) {
					InputLayers[i]->FeedForward(Version);
					const auto & o = InputLayers[i]->Output();
					output.block(Start, 0, o.rows(), o.cols()) = o;
					Start += o.rows() - 1;
				}
				FeedVersion = Version;
			}
		}
		void NeuronCombiner::FeedForward(int64_t Index, int64_t Version) {
			if (Version > FeedVersion) {
				FeedVersion = Version;
				FeedIndexVersion = -1;
			}
			if (Index > FeedIndexVersion) {
				FeedIndexVersion = Index;
				InputLayers[0]->FeedForward(Index, Version);
				size_t Start = 0;
				if (InputLayers[0]->BatchSize() != output.cols()) {
					output.conservativeResize(Eigen::NoChange, InputLayers[0]->BatchSize());
				}
				{
					const auto& o = InputLayers[0]->Output(Index);
					output.block(Start, Index, o.rows(), 1) = o;
					Start += o.rows() - 1;
				}
				for (auto i = 1; i < InputLayers.size(); i++) {
					InputLayers[i]->FeedForward(Index, Version);
					const auto& o = InputLayers[i]->Output(Index);
					output.block(Start, Index, o.rows(), 1) = o;
					Start += o.rows() - 1; 
				}
			}
		}

		void NeuronCombiner::BackPropagate(const netData & Gradient) {
			size_t Start = 0;
			for (auto i : InputLayers) {
				i->BackPropagate(Gradient.block(Start, 0, i->nOutput(), Gradient.cols()));
				Start += i->nOutput();
			}
		}

		void NeuronCombiner::BackPropagate(const netData& Gradient, int64_t Index) {
			size_t Start = 0;
			for (auto i : InputLayers) {
				i->BackPropagate(Gradient.block(Start, Index, i->nOutput(), 1), Index);
				Start += i->nOutput();
			}
		}

		void NeuronCombiner::UpdateWeights() {
			for (auto i: InputLayers) i->UpdateWeights();
		}

		size_t NeuronCombiner::nInput() const {
			return output.rows() - 1;
		}
	}
}
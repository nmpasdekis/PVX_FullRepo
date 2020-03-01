#include <PVX_NeuralNetsCPU.h>
#include "PVX_NeuralNets_Util.inl"
#include <iostream>
#include <future>

namespace PVX {
	namespace DeepNeuralNets {
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
		NeuralLayer_Base* NeuronAdder::newCopy(const std::map<NeuralLayer_Base*,size_t>& IndexOf) {
			auto ret =  new NeuronAdder(nInput());
			for (auto l : InputLayers)
				ret->InputLayers.push_back(reinterpret_cast<NeuralLayer_Base*>(IndexOf.at(l)));
			ret->Id = Id;
			return ret;
		}
		NeuronAdder::NeuronAdder(const size_t InputSize) {
			output = netData::Zero(InputSize + 1, 1);
			Id = ++NextId;
		}
		NeuronAdder::NeuronAdder(const std::vector<NeuralLayer_Base*>& Inputs) : NeuronAdder(Inputs[0]->nOutput()) {
			InputLayers = Inputs;
		}
		NeuronAdder::NeuronAdder(const std::string& Name, const size_t InputSize) {
			name = Name;
			output = netData::Zero(InputSize + 1, 1);
			Id = ++NextId;
		}
		NeuronAdder::NeuronAdder(const std::string& Name, const std::vector<NeuralLayer_Base*>& Inputs) : NeuronAdder(Inputs[0]->nOutput()) {
			name = Name;
			InputLayers = Inputs;
		}

		size_t NeuronAdder::DNA(std::map<void*, WeightData>& Weights) {
			size_t ret = 0;
			for (auto l : InputLayers)
				ret += l->DNA(Weights);
			return ret;
		}
		//void NeuronAdder::FeedForward(int64_t Version) {
		//	if (Version > FeedVersion) {
		//		InputLayers[0]->FeedForward(Version);
		//		const auto& inp = PreviousLayer->Output();
		//		if (inp.cols() != output.cols()) {
		//			output = netData::Ones(output.rows(), inp.cols());
		//		}
		//		outPart(output) = inp;
		//		for (auto i = 1; i < InputLayers.size(); i++) {
		//			InputLayers[i]->FeedForward(Version);
		//			outPart(output) += InputLayers[i]->Output();
		//		}
		//		FeedVersion = Version;
		//	}
		//}

		void NeuronAdder::FeedForward(int64_t Version) {
			if (Version > FeedVersion) {
				InputLayers[0]->FeedForward(Version);
				output = InputLayers[0]->Output();
				for (auto i = 1; i < InputLayers.size(); i++) {
					InputLayers[i]->FeedForward(Version);
					output += InputLayers[i]->Output();
				}
				output.row(output.rows() - 1) = netData::Ones(1, output.cols());
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
					output = netData::Zero(output.rows(), pro.cols());
				}
				output.col(Index) = pro.col(Index);
				for (auto i = 1; i < InputLayers.size(); i++) {
					InputLayers[i]->FeedForward(Index, Version);
					output.col(Index) += InputLayers[i]->Output(Index);
				}
				output(output.rows()-1, Index) = 1.0f;
			}
		}

		void NeuronAdder::BackPropagate(const netData & Gradient) {
			for (auto i : InputLayers) i->BackPropagate(Gradient);
		}
		void NeuronAdder::BackPropagate(const netData& Gradient, int64_t Index) {
			for (auto i : InputLayers) i->BackPropagate(Gradient, Index);
		}
		void NeuronAdder::UpdateWeights() {
			for (auto i: InputLayers) i->UpdateWeights();
		}
		size_t NeuronAdder::nInput() const {
			return output.rows() - 1;
		}

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

		NeuralLayer_Base* NeuronMultiplier::newCopy(const std::map<NeuralLayer_Base*,size_t>& IndexOf) {
			auto ret = new NeuronMultiplier(nInput());
			for (auto l : InputLayers)
				ret->InputLayers.push_back(reinterpret_cast<NeuralLayer_Base*>(IndexOf.at(l)));
			ret->Id = Id;
			return ret;
		}

		NeuronMultiplier::NeuronMultiplier(const size_t inputs) {
			output = netData::Zero(inputs + 1, 1);
			Id = ++NextId;
		}

		NeuronMultiplier::NeuronMultiplier(const std::vector<NeuralLayer_Base*> & inputs) {
			for (auto& i : inputs) InputLayers.push_back(i);
			output = netData::Zero(InputLayers[0]->Output().rows(), 1);
		}
		size_t NeuronMultiplier::DNA(std::map<void*, WeightData>& Weights) {
			size_t ret = 0;
			for (auto l : InputLayers)
				ret += l->DNA(Weights);
			return ret;
		}
		//void NeuronMultiplier::FeedForward(int64_t Version) {
		//	if (Version > FeedVersion) {
		//		InputLayers[0]->FeedForward(Version);
		//		auto tmp = InputLayers[0]->Output().array();
		//		for (auto i = 1; i < InputLayers.size(); i++) {
		//			InputLayers[i]->FeedForward(Version);
		//			tmp *= InputLayers[i]->Output().array();
		//		}
		//		output = tmp;
		//		FeedVersion = Version;
		//		FeedIndexVersion = output.cols();
		//	}
		//}

		void NeuronMultiplier::FeedForward(int64_t Version) {
			if (Version > FeedVersion) {
				InputLayers[0]->FeedForward(Version);
				output = InputLayers[0]->Output();
				for (auto i = 1; i < InputLayers.size(); i++) {
					InputLayers[i]->FeedForward(Version);
					output *= InputLayers[i]->Output();
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
					output = netData::Zero(output.rows(), pro.cols());
				}
				output.col(Index) = pro.col(Index);

				for (auto i = 1; i < InputLayers.size(); i++) {
					InputLayers[i]->FeedForward(Index, Version);
					output.col(Index) *= InputLayers[i]->Output().col(Index);
				}
			}
		}

		void NeuronMultiplier::BackPropagate(const netData & Gradient) {
			{
				auto tmp = InputLayers[1]->RealOutput().array();
				for (auto i = 2; i < InputLayers.size(); i++) {
					tmp *= InputLayers[i]->RealOutput().array();
				}
				InputLayers[0]->BackPropagate(Gradient.array() * tmp);
			}
			for (int j = 1; j < InputLayers.size(); j++) {
				auto tmp = InputLayers[0]->RealOutput().array();
				for (int i = 1; i < InputLayers.size(); i++)
					if (i != j) 
						tmp *= InputLayers[i]->RealOutput().array();
				InputLayers[j]->BackPropagate(Gradient.array() * tmp);
			}
		}
		void NeuronMultiplier::BackPropagate(const netData& Gradient, int64_t Index) {
			{
				auto tmp = InputLayers[1]->RealOutput(Index).array();
				for (auto i = 2; i < InputLayers.size(); i++) {
					tmp *= InputLayers[i]->RealOutput(Index).array();
				}
				InputLayers[0]->BackPropagate(Gradient.array() * tmp, Index);
			}
			for (int j = 1; j < InputLayers.size(); j++) {
				auto tmp = InputLayers[0]->RealOutput(Index).array();
				for (int i = 1; i < InputLayers.size(); i++)
					if (i != j)
						tmp *= InputLayers[i]->RealOutput(Index).array();
				InputLayers[j]->BackPropagate(Gradient.array() * tmp, Index);
			}
		}

		void NeuronMultiplier::UpdateWeights() {
			for (auto i: InputLayers) i->UpdateWeights();
		}
		size_t NeuronMultiplier::nInput() const {
			return output.cols();
		}

		netData Concat(const std::vector<netData>& M) {
			size_t cols = 0;
			for (auto& m : M) cols += m.cols();
			netData ret(M[0].rows(), cols);
			size_t offset = 0;
			for (auto& m : M) {
				memcpy(ret.data() + offset, m.data(), m.size() * sizeof(float));
				offset += m.size();
			}
			return ret;
		}

		ResNetUtility::ResNetUtility(NeuralLayer_Base* inp, LayerActivation Activate, TrainScheme Train) :
			First{ inp, inp->nOutput(), Activate, Train },
			Second{ &First, inp->nOutput(), LayerActivation::Linear, Train },
			Adder({ inp, &Second }),
			Activation(&Adder, Activate) {}
		ResNetUtility::ResNetUtility(const std::string& Name, NeuralLayer_Base* inp, LayerActivation Activate, TrainScheme Train) :
			First { Name + "_First", inp, inp->nOutput(), Activate, Train },
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
}
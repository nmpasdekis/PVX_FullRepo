#include <PVX_NeuralNetsCPU.h>
#include "PVX_NeuralNets_Util.inl"

namespace PVX {
	namespace DeepNeuralNets {
		void InputLayer::Save(PVX::BinSaver& bin, const std::map<NeuralLayer_Base*, size_t>& IndexOf) const {
			bin.Begin("INPT"); {
				bin.Write("NDID", int(Id));
				if (name.size()) bin.Write("NAME", name);
				bin.Write("ICNT", int(nInput()));
			} bin.End();
		}
		InputLayer::InputLayer(PVX::BinLoader& bin){
			int ic;
			bin.Process("NDID", [&](PVX::BinLoader& bin2) { Id = bin2.read<int>(); });
			bin.Read("NAME", name);
			bin.Read("ICNT", ic);
			bin.Execute();
			output = netData::Ones(ic + 1ll, 1ll);
		}
		NeuralLayer_Base* InputLayer::newCopy(const std::map<NeuralLayer_Base*,size_t>& IndexOf) {
			auto ret = new InputLayer(nOutput());
			ret->Id = Id;
			return ret;
		}
		InputLayer::InputLayer(const size_t Size) {
			output = netData::Ones(Size + 1, 1);
			Id = ++NextId;
		}
		InputLayer::InputLayer(const std::string& Name, const size_t Size) {
			name = Name;
			output = netData::Ones(Size + 1ll, 1ll);
		}

		size_t InputLayer::DNA(std::map<void*, WeightData>& Weights) { return 0; }

		void InputLayer::Input(const float * Data, int64_t Count) {
			if (output.cols() != Count)
				output = netData::Ones(output.rows(), Count);
			outPart(output) = Map((float*)Data, output.rows() - 1ll, output.cols());
		}

		void InputLayer::Input(const netData & Data) {
			if (output.rows() == Data.rows() + 1) {
				if (output.cols() != Data.cols()) {
					output = netData::Ones(output.rows(), Data.cols());
				}
				outPart(output) = Data;
			}
		}

		void InputLayer::InputRaw(const netData & Data) {
			output = Data;
		}

		netData InputLayer::MakeRawInput(const netData & Data) {
			netData ret = netData::Ones(Data.rows() + 1, Data.cols());
			outPart(ret) = Data;
			return ret;
		}

		netData InputLayer::MakeRawInput(const std::vector<float>& Input) {
			return MakeRawInput(Input.data(), Input.size() / nInput());
		}

		netData InputLayer::MakeRawInput(const float * Data, size_t Count) {
			return MakeRawInput(Eigen::Map<netData>((float*)Data, output.rows() - 1, Count));
		}

		size_t InputLayer::nInput() const {
			return output.rows() - 1;
		}
	}
}
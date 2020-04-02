#include <PVX_NeuralNetsCL.h>
#include "PVX_NeuralNets_Util.inl"

namespace PVX {
	namespace NeuralNets {
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
			output.Reset(ic + 1ll, 1ll) = 1.0f;
		}
		NeuralLayer_Base* InputLayer::newCopy(const std::map<NeuralLayer_Base*,size_t>& IndexOf) {
			auto ret = new InputLayer(nOutput());
			ret->Id = Id;
			return ret;
		}
		InputLayer::InputLayer(const size_t Size) {
			output.Reset(Size + 1, 1) = 1.0f;
			Id = ++NextId;
		}
		InputLayer::InputLayer(const std::string& Name, const size_t Size) {
			name = Name;
			output.Reset(Size + 1ll, 1ll) = 1.0f;
		}

		size_t InputLayer::DNA(std::map<void*, WeightData>& Weights) { return 0; }

		void InputLayer::Input(const float * Data, int64_t Count) {
			if (output.cols() != Count)
				output.Reset(output.rows(), Count).LastRow() = 1.0f;
			outPart(output).Write(Data);
		}

		void InputLayer::Input(const clMatrix & Data) {
			if (output.rows() == Data.rows() + 1) {
				if (output.cols() != Data.cols()) {
					output.Reset(output.rows(), Data.cols()).LastRow() = 1.0f;
				}
				outPart(output).SetData(Data);
			}
		}

		void InputLayer::InputRaw(const clMatrix & Data) {
			output.Set(Data);
		}

		clMatrix InputLayer::MakeRawInput(const clMatrix & Data) {
			clMatrix ret = clMatrix(Data.rows() + 1, Data.cols(), 1.0f);
			outPart(ret).SetData(Data);
			return ret;
		}

		clMatrix InputLayer::MakeRawInput(const std::vector<float>& Input) {
			return MakeRawInput(Input.data(), Input.size() / nInput());
		}

		clMatrix InputLayer::MakeRawInput(const float * Data, size_t Count) {
			return MakeRawInput(clMatrix::Temp(output.rows() - 1, Count, Data));
		}

		size_t InputLayer::nInput() const {
			return output.rows() - 1;
		}
	}
}
#include <PVX_NeuralNetsCL.h>
#include "PVX_NeuralNets_Util.inl"

namespace PVX {
	namespace NeuralNets {
		ActivationLayer::ActivationLayer(NeuralLayer_Base* inp, LayerActivation Activation) :
			ActivationLayer(inp->nOutput(), Activation) {
			PreviousLayer = inp;
		}
		ActivationLayer::ActivationLayer(size_t inp, LayerActivation Activation) : 
			activation{ Activation }
		{
			PreviousLayer = nullptr;
			Id = ++NextId;
			output = 1.0f;
			switch (Activation) {
				case LayerActivation::Tanh:
					Activate = Tanh;
					Derivative = dTanh;
					break;
				case LayerActivation::TanhBias:
					Activate = TanhBias;
					Derivative = dTanhBias;
					break;
				case LayerActivation::ReLU:
					Activate = ReLU;
					Derivative = dReLU;
					break;
				case LayerActivation::Sigmoid:
					Activate = Sigmoid;
					Derivative = dSigmoid;
					break;
				case LayerActivation::Linear:
					Activate = Linear;
					Derivative = dLinear;
					break;
			}
		}
		ActivationLayer::ActivationLayer(const std::string& Name, NeuralLayer_Base* inp, LayerActivation Activation):
			ActivationLayer(inp, Activation) {
			name = Name;
		}
		ActivationLayer::ActivationLayer(const std::string& Name, size_t inp, LayerActivation Activation) :
			ActivationLayer(inp, Activation) {
			name = Name;
		}


		void ActivationLayer::FeedForward(int64_t Version) {
			if (Version > FeedVersion) {
				PreviousLayer->FeedForward(Version);
				output = Activate(PreviousLayer->Output());
				output.row(output.rows() - 1) = 1.0f;
				FeedVersion = Version;
				FeedIndexVersion = output.cols();
			}
		}
		void ActivationLayer::FeedForward(int64_t Index, int64_t Version) {
			if (Version > FeedVersion) {
				FeedVersion = Version;
				FeedIndexVersion = -1;
			}
			if (Index > FeedIndexVersion) {
				FeedIndexVersion = Index;
				PreviousLayer->FeedForward(Version, Index);
				auto pro = PreviousLayer->Output();
				if (pro.cols() != output.cols()) {
					output = 1.0f;
				}

				output.col(Index) = Activate(pro.col(Index));
				output(output.rows() - 1, Index) = 1.0f;
			}
		}
		void ActivationLayer::BackPropagate(clMatrixExpr&& Gradient) {
			PreviousLayer->BackPropagate(Gradient.Element_Multiply(Derivative(outPart(output))));
		}

		void ActivationLayer::BackPropagate(clMatrixExpr&& Gradient, int64_t Index) {
			PreviousLayer->BackPropagate(Gradient.Element_Multiply(Derivative(outPart(output, Index))), Index);
		}

		void ActivationLayer::UpdateWeights() {
			PreviousLayer->UpdateWeights();
		}

		void ActivationLayer::Save(PVX::BinSaver& bin, const std::map<NeuralLayer_Base*, size_t>& IndexOf) const {
			bin.Begin("ACTV");
			{
				bin.Write("NDID", int(Id));
				bin.Write("ACTV", int(activation));
				bin.Write("OUTC", nOutput());
				bin.Write("INPT", IndexOf.at(PreviousLayer));
			}
			bin.End();
		}

		ActivationLayer* ActivationLayer::Load2(PVX::BinLoader& bin) {
			int act, Id = -1;
			size_t outc, prev;
			bin.Read("OUTC", outc);
			bin.Read("ACTV", act);
			bin.Read("INPT", prev);
			bin.Process("NDID", [&](PVX::BinLoader& bin2) { Id = bin2.read<int>(); });
			bin.Execute();
			auto ret = new ActivationLayer(outc, LayerActivation(act));
			if (Id>=0)ret->Id = Id;
			ret->PreviousLayer = (NeuralLayer_Base*)prev;
			return ret;
		}

		NeuralLayer_Base* ActivationLayer::newCopy(const std::map<NeuralLayer_Base*,size_t>& IndexOf) {
			auto ret = new ActivationLayer(nInput(), activation);
			ret->PreviousLayer = reinterpret_cast<NeuralLayer_Base*>(IndexOf.at(PreviousLayer));
			ret->Id = Id;
			return ret;
		}

		//size_t ActivationLayer::DNA(std::map<void*, WeightData>& Weights) {
		//	return PreviousLayer->DNA(Weights);
		//}
		size_t ActivationLayer::nInput() const {
			return nOutput();
		}
	}
}
#include <PVX_NeuralNetsCPU.h>
#include "PVX_NeuralNets_Util.inl"

namespace PVX {
	namespace DeepNeuralNets {
				/////////////////////////////////////

		static netData Tanh(const netData& x) {
			return Eigen::tanh(x.array());
		}
		static netData TanhDer(const netData& x) {
			auto tmp = Eigen::tanh(x.array());
			return 1.0f - tmp * tmp;
		}
		static netData TanhBias(const netData& x) {
			netData tmp = Eigen::tanh(x.array());
			auto dt = tmp.data();
			size_t sz = x.cols()*x.rows();
			for (auto i = 0; i < sz; i++)
				dt[i] = dt[i] * 0.5f + 0.5f;
			return tmp;
		}
		static netData TanhBiasDer(const netData & x) {
			auto tmp = Eigen::tanh(x.array());
			return 0.5f* (1.0f - tmp * tmp);
		}
		static netData Relu(const netData & x) {
			return x.array()* (x.array() > 0).cast<float>();
		}
		static netData ReluDer(const netData & x) {
			netData ret(x.rows(), x.cols());
			float* dt = (float*)x.data();
			float* o = ret.data();
			size_t sz = x.cols()*x.rows();
			for (int i = 0; i < sz; i++) o[i] = (dt[i] > 0) ? 1.0f : 0.0001f;
			return ret;
		}
		static netData Sigmoid(const netData & x) {
			netData ex = Eigen::exp(-x.array());
			netData ret = 1.0f / (1.0f + ex.array());
			return ret;
		}
		static netData SigmoidDer(const netData & x) {
			netData tmp = Sigmoid(x);
			return tmp.array()* (1.0f - tmp.array());
		}
		static netData Linear(const netData & x) {
			return x;
		}
		static netData LinearDer(const netData & x) {
			return netData::Ones(x.rows(), x.cols());
		}

		////////////////////////////////////

		ActivationLayer::ActivationLayer(NeuralLayer_Base* inp, LayerActivation Activation) :
			ActivationLayer(inp->nOutput(), Activation) {
			PreviousLayer = inp;
		}
		ActivationLayer::ActivationLayer(size_t inp, LayerActivation Activation) : activation{ Activation } {
			PreviousLayer = nullptr;
			Id = ++NextId;
			output = netData::Ones(inp + size_t(1), 1);
			switch (Activation) {
				case LayerActivation::Tanh:
					Activate = Tanh;
					Derivative = TanhDer;
					break;
				case LayerActivation::TanhBias:
					Activate = TanhBias;
					Derivative = TanhBiasDer;
					break;
				case LayerActivation::ReLU:
					Activate = Relu;
					Derivative = ReluDer;
					break;
				case LayerActivation::Sigmoid:
					Activate = Sigmoid;
					Derivative = SigmoidDer;
					break;
				case LayerActivation::Linear:
					Activate = Linear;
					Derivative = LinearDer;
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
				//const auto& inp = PreviousLayer->Output();
				//if (inp.cols() != output.cols()) {
				//	output = netData::Ones(output.rows(), inp.cols());
				//}
				//
				//outPart(output) = Activate(outPart(inp));
				output = Activate(PreviousLayer->Output());
				output.row(output.rows() - 1) = netData::Ones(1, output.cols());
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
				const auto& pro = PreviousLayer->Output();
				if (pro.cols() != output.cols()) {
					output = netData::Ones(output.rows(), pro.cols());
				}
				//auto inp = PreviousLayer->Output(Index);

				//outPart(output, Index) = Activate(outPart(inp));
				output.col(Index) = Activate(pro.col(Index));
				output(output.rows() - 1, Index) = 1.0f;
			}
		}
		void ActivationLayer::BackPropagate(const netData& Gradient) {
			netData grad = Gradient.array() * Derivative(outPart(output)).array();
			PreviousLayer->BackPropagate(grad);
		}

		void ActivationLayer::BackPropagate(const netData& Gradient, int64_t Index) {
			netData grad = Gradient.array() * Derivative(outPart(output, Index)).array();
			PreviousLayer->BackPropagate(grad, Index);
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

		size_t ActivationLayer::DNA(std::map<void*, WeightData>& Weights) {
			return PreviousLayer->DNA(Weights);
		}
		size_t ActivationLayer::nInput() const {
			return nOutput();
		}
	}
}
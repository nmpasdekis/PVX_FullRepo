#include <PVX_NeuralNetsCPU.h>
#include "PVX_NeuralNets_Util.inl"

namespace PVX {
	namespace DeepNeuralNets {
		auto RandomBias(size_t r, size_t c) {
			return netData::Random(r, c).array() * 0.5f + 0.5f;
		}

		int UseDropout = 0;
		/////////////////////////////////////

		static netData Tanh(const netData & x) {
			return Eigen::tanh(x.array());
		}
		static netData TanhDer(const netData & x) {
			auto tmp = Eigen::tanh(x.array());
			return 1.0f - tmp * tmp;
		}
		static netData TanhBias(const netData & x) {
			netData tmp = Eigen::tanh(x.array());
			auto dt = tmp.data();
			size_t sz = x.cols()*x.rows();
			for (auto i = 0; i < sz; i++)
				dt[i] = dt[i] * 0.5f + 0.5f;
			return tmp;
		}
		static netData TanhBiasDer(const netData & x) {
			auto tmp = Eigen::tanh(x.array());
			return 0.5f * (1.0f - tmp * tmp);
		}
		static netData Relu(const netData & x) {
			return x.array() * (x.array() > 0).cast<float>();
		}
		static netData ReluDer(const netData & x) {
			netData ret(x.rows(), x.cols());
			float * dt = (float*)x.data();
			float * o = ret.data();
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
			return tmp.array() * (1.0f - tmp.array());
		}
		static netData Linear(const netData & x) {
			return x;
		}
		static netData LinearDer(const netData & x) {
			return netData::Ones(x.rows(), x.cols());
		}

		////////////////////////////////////

		void NeuronLayer::AdamF(const netData & Gradient) {
			const auto& g1 = Gradient.array();
			if (RMSprop.cols() != Gradient.cols())RMSprop = netData::Ones(Gradient.rows(), Gradient.cols());
			RMSprop = _RMSprop * RMSprop.array() + _iRMSprop * (g1*g1);
			netData gr = g1 / ((Eigen::sqrt(RMSprop.array()) + 1e-8));

			DeltaWeights = (gr * PreviousLayer->Output().transpose() * (_LearnRate * _iMomentum)) + DeltaWeights * _Momentum;
			Weights += DeltaWeights;
		}
		void NeuronLayer::Adam_WeightDecayF(const netData& Gradient) {
			auto g1 = Gradient.array();
			if (RMSprop.cols() != Gradient.cols())RMSprop = netData::Ones(Gradient.rows(), Gradient.cols());
			RMSprop = _RMSprop * RMSprop.array() + _iRMSprop * (g1*g1);
			netData gr = g1 / ((Eigen::sqrt(RMSprop.array()) + 1e-8));

			DeltaWeights = (gr * PreviousLayer->Output().transpose() * (_LearnRate * _iMomentum)) + DeltaWeights * _Momentum;
			Weights = Weights * (1.0f - _LearnRate * _L2 / Weights.size()) + DeltaWeights;

			//DeltaWeights = (gr * PreviousLayer->Output().transpose() * (_LearnRate * _iMomentum)) + DeltaWeights * _Momentum;
			//Weights = Weights * (1.0f - _LearnRate * _L2 / Gradient.cols()) + DeltaWeights;
		}


		void NeuronLayer::MomentumF(const netData & Gradient) {
			DeltaWeights = (Gradient * PreviousLayer->Output().transpose() * (_LearnRate * _iMomentum)) + DeltaWeights * _Momentum;
			Weights += DeltaWeights;
		}
		void NeuronLayer::RMSpropF(const netData & Gradient) {
			auto g1 = Gradient.array();
			if (RMSprop.cols() != Gradient.cols())
				RMSprop = netData::Ones(Gradient.rows(), Gradient.cols());
			RMSprop = _RMSprop * RMSprop.array() + _iRMSprop * (g1*g1);
			netData gr = g1 / (Eigen::sqrt(RMSprop.array()) + 1e-8);

			Weights += (gr * PreviousLayer->Output().transpose() * _LearnRate);
		}
		void NeuronLayer::SgdF(const netData & Gradient) {
			Weights += (Gradient * PreviousLayer->Output().transpose() * _LearnRate);
		}
		void NeuronLayer::AdaGradF(const netData & Gradient) {
			auto g1 = Gradient.array();
			if (RMSprop.cols() != Gradient.cols())
				RMSprop = netData::Ones(Gradient.rows(), Gradient.cols());
			RMSprop = RMSprop.array() + (g1*g1);
			netData gr = g1 / (Eigen::sqrt(RMSprop.array()) + 1e-8);

			Weights += (gr * PreviousLayer->Output().transpose() * _LearnRate);
		}
		void NeuronLayer::Momentum_WeightDecayF(const netData& Gradient) {
			DeltaWeights = (Gradient * PreviousLayer->Output().transpose() * (_LearnRate * _iMomentum)) + DeltaWeights * _Momentum;
			Weights = Weights * (1.0f - _LearnRate * _L2 / Weights.size()) + DeltaWeights;
		}
		void NeuronLayer::RMSprop_WeightDecayF(const netData& Gradient) {
			auto g1 = Gradient.array();
			if (RMSprop.cols() != Gradient.cols())
				RMSprop = netData::Ones(Gradient.rows(), Gradient.cols());
			RMSprop = _RMSprop * RMSprop.array() + _iRMSprop * (g1*g1);
			netData gr = g1 / (Eigen::sqrt(RMSprop.array()) + 1e-8);

			Weights = Weights * (1.0f - _LearnRate * _L2 / Weights.size()) + (gr * PreviousLayer->Output().transpose() * _LearnRate);
		}
		void NeuronLayer::Sgd_WeightDecayF(const netData& Gradient) {
			Weights = Weights * (1.0f - _LearnRate * _L2 / Weights.size()) + (Gradient * PreviousLayer->Output().transpose() * _LearnRate);
		}
		void NeuronLayer::AdaGrad_WeightDecayF(const netData& Gradient) {
			auto g1 = Gradient.array();
			if (RMSprop.cols() != Gradient.cols())
				RMSprop = netData::Ones(Gradient.rows(), Gradient.cols());
			RMSprop = RMSprop.array() + (g1*g1);
			netData gr = g1 / (Eigen::sqrt(RMSprop.array()) + 1e-8);

			Weights = Weights * (1.0f - _LearnRate * _L2 / Weights.size()) + (gr * PreviousLayer->Output().transpose() * _LearnRate);
		}

		////////////////////////////////////

		void NeuronLayer::Save(PVX::BinSaver& bin, const std::map<NeuralLayer_Base*, size_t>& IndexOf) const {
			bin.Begin("DENS");
			{
				bin.Write("NDID", int(Id));
				if (name.size()) bin.Write("NAME", name);
				bin.Write("ROWS", int(Weights.rows()));
				bin.Write("COLS", int(Weights.cols()));
				bin.Write("WGHT", Weights.data(), Weights.size());
				bin.Write("RATE", _LearnRate);
				bin.Write("MMNT", _Momentum);
				bin.Write("RMSP", _RMSprop);
				bin.Write("DRPT", _Dropout);
				bin.Write("L2RG", _L2);
				bin.Write("ACTV", (int)(activation));
				bin.Write("TRNS", (int)(training));
				bin.Write("INPT", (int)(IndexOf.at(PreviousLayer)));
			}
			bin.End();
		}
		NeuralLayer_Base* NeuronLayer::Load2(PVX::BinLoader& bin) {
			int rows = 0, cols = 0, act = 0, train = 0, prev = 0;
			float rate, rms, drop, momentum;// , l2;
			std::string Name;
			int Id = -1;
			std::vector<float> Weights;
			bin.Read("ROWS", rows);
			bin.Read("COLS", cols);
			bin.Read("WGHT", Weights);
			bin.Read("RATE", rate);
			bin.Read("MMNT", momentum);
			bin.Read("RMSP", rms);
			bin.Read("DRPT", drop);
			bin.Read("ACTV", act);
			bin.Read("TRNS", train);
			//bin.Read("L2RG", l2);
			bin.Read("INPT", prev);
			bin.Read("NAME", Name);
			bin.Process("NDID", [&](PVX::BinLoader& bin2) { Id = bin2.read<int>(); });
			bin.Execute();
			auto ret = new NeuronLayer(cols-1, rows, LayerActivation(act), TrainScheme(train));
			if(Id >= 0) ret->Id = Id;
			if (Name.size()) ret->name = Name;
			ret->GetWeights() = Eigen::Map<netData>(Weights.data(), rows, cols);
			if (!OverrideOnLoad) {
				ret->_Dropout = drop;
				ret->_iDropout = 1.0f / drop;
				ret->_Momentum = momentum;
				ret->_iMomentum = 1.0f - momentum;
				ret->_LearnRate = rate;
				ret->_RMSprop = rms;
				ret->_iRMSprop = 1.0f - rms;
				ret->_L2 = __L2;
			}
			ret->PreviousLayer = reinterpret_cast<NeuralLayer_Base*>(((char*)0) + prev);// (NeuralLayer_Base*)prev;
			return ret;
		}
		NeuralLayer_Base* NeuronLayer::newCopy(const std::map<NeuralLayer_Base*,size_t>& IndexOf) {
			auto ret = new NeuronLayer(nInput(), nOutput(), activation, training);

			ret->Weights = Weights;
			ret->_LearnRate = _LearnRate;
			ret->_Momentum = _Momentum;
			ret->_iMomentum = _iMomentum;
			ret->_RMSprop = _RMSprop;
			ret->_iRMSprop = _iRMSprop;
			ret->_Dropout = _Dropout;
			ret->_iDropout = _iDropout;
			ret->_L2 = _L2;
			ret->PreviousLayer = reinterpret_cast<NeuralLayer_Base*>(IndexOf.at(PreviousLayer));
			ret->Id = Id;
			return ret;
		}
		void NeuronLayer::SetLearnRate(float a) {
			_LearnRate = a;
			PreviousLayer->SetLearnRate(a);
		}
		void NeuronLayer::SetRMSprop(float Beta) {
			_RMSprop = Beta;
			_iRMSprop = 1.0f - Beta;
			PreviousLayer->SetRMSprop(Beta);
		}
		void NeuronLayer::SetMomentum(float Beta) {
			_Momentum = Beta;
			_iMomentum = 1.0f - Beta;
			PreviousLayer->SetMomentum(Beta);
		}
		void NeuronLayer::ResetMomentum() {
			this->RMSprop = netData::Ones(this->RMSprop.rows(), this->RMSprop.cols());
			this->DeltaWeights = netData::Zero(this->DeltaWeights.rows(), this->DeltaWeights.cols());
			PreviousLayer->ResetMomentum();
		}

		netData& NeuronLayer::GetWeights() {
			return Weights;
		}
		static int InitOpenMP = 0;

		NeuronLayer::NeuronLayer(size_t nInput, size_t nOutput, LayerActivation Activation, TrainScheme Train) :
			training{ Train },
			activation{ Activation },
			DeltaWeights{ netData::Zero(nOutput, nInput + 1ll) },
			Weights{ netData::Random(nOutput, nInput + 1ll) },
			RMSprop{ netData::Ones(nOutput, 1ll) }
		{
			if (!InitOpenMP) {
				Eigen::initParallel();
				InitOpenMP = 1;
			}
			Id = ++NextId;

			_LearnRate = __LearnRate;
			_Momentum = __Momentum;
			_iMomentum = __iMomentum;
			_RMSprop = __RMSprop;
			_iRMSprop = __iRMSprop;
			_Dropout = __Dropout;
			_iDropout = __iDropout;
			_L2 = __L2;

			float randScale = sqrtf(2.0f / (nInput + 1));

			output = netData::Ones(nOutput + size_t(1), 1);
			switch (Activation) {
			case LayerActivation::Tanh:
				randScale = sqrtf(1.0f / (nInput + 1));
				Activate = Tanh;
				Derivative = TanhDer;
				break;
			case LayerActivation::TanhBias:
				randScale = sqrtf(1.0f / (nInput + 1));
				Activate = TanhBias;
				Derivative = TanhBiasDer;
				break;
			case LayerActivation::ReLU:
				Activate = Relu;
				Derivative = ReluDer;
				break;
			case LayerActivation::Sigmoid:
				randScale = sqrtf(1.0f / (nInput + 1));
				Activate = Sigmoid;
				Derivative = SigmoidDer;
				break;
			case LayerActivation::Linear:
				Activate = Linear;
				Derivative = LinearDer;
				break;
			}
			Weights *= randScale;
			if (_L2>0.0f) {
				switch (Train) {
					case TrainScheme::Adam: updateWeights = &NeuronLayer::Adam_WeightDecayF; break;
					case TrainScheme::RMSprop: updateWeights = &NeuronLayer::RMSprop_WeightDecayF; break;
					case TrainScheme::Momentum: updateWeights = &NeuronLayer::Momentum_WeightDecayF; break;
					case TrainScheme::AdaGrad: updateWeights = &NeuronLayer::AdaGrad_WeightDecayF; break;
					case TrainScheme::Sgd: updateWeights = &NeuronLayer::Sgd_WeightDecayF; break;
				}
			} else {
				switch (Train) {
					case TrainScheme::Adam: updateWeights = &NeuronLayer::AdamF; break;
					case TrainScheme::RMSprop: updateWeights = &NeuronLayer::RMSpropF; break;
					case TrainScheme::Momentum: updateWeights = &NeuronLayer::MomentumF; break;
					case TrainScheme::AdaGrad: updateWeights = &NeuronLayer::AdaGradF; break;
					case TrainScheme::Sgd: updateWeights = &NeuronLayer::SgdF; break;
				}
			}
		}

		NeuronLayer::NeuronLayer(const std::string& Name, size_t nInput, size_t nOutput, LayerActivation Activate, TrainScheme Train):
			NeuronLayer(nInput, nOutput, Activate, Train) {
			name = Name;
		}

		NeuronLayer::NeuronLayer(NeuralLayer_Base * inp, size_t nOutput, LayerActivation Activate, TrainScheme Train) :
			NeuronLayer(inp->nOutput(), nOutput, Activate, Train) {
			PreviousLayer = inp;
		}

		NeuronLayer::NeuronLayer(const std::string& Name, NeuralLayer_Base* inp, size_t nOutput, LayerActivation Activate, TrainScheme Train):
			NeuronLayer(inp, nOutput, Activate, Train) {
			name = Name;
		}

		void NeuralLayer_Base::UseDropout(int b) {
			PVX::DeepNeuralNets::UseDropout = b;
		}
		void NeuralLayer_Base::OverrideParamsOnLoad(int b) {
			OverrideOnLoad = b;
		}
		void NeuronLayer::FeedForward(int64_t Version) {
			if (Version > FeedVersion) {
				PreviousLayer->FeedForward(Version);
				const auto& inp = PreviousLayer->Output();
				if (inp.cols() != output.cols()) {
					output = netData::Ones(output.rows(), inp.cols());
				}
				if (PVX::DeepNeuralNets::UseDropout && _Dropout < 1.0f) {
					outPart(output) = 
						Activate(Weights * inp).array() * 
						(RandomBias(output.rows() - 1ll, output.cols()) < _Dropout).cast<float>() * 
						_iDropout;
				} else {
					outPart(output) = Activate(Weights * inp);
				}
				FeedVersion = Version;
				FeedIndexVersion = output.cols();
			}
		}

		/*
		
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
			}
		}
		void NeuronAdder::FeedForward(int64_t Index, int Version) {
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

		*/

		void NeuronLayer::FeedForward(int64_t Index, int64_t Version) {
			if (Version > FeedVersion) {
				FeedVersion = Version;
				FeedIndexVersion = -1;
			}
			if (Index > FeedIndexVersion) {
				FeedIndexVersion = Index;
				PreviousLayer->FeedForward(Index, Version);
				const auto& pro = PreviousLayer->Output();
				if (pro.cols() != output.cols()) {
					output = netData::Ones(output.rows(), pro.cols());
				}
				const auto& inp = PreviousLayer->Output(Index);

				if (PVX::DeepNeuralNets::UseDropout && _Dropout < 1.0f) {
					outPart(output, Index) =
						Activate(Weights * inp).array() *
						(RandomBias(output.rows() - 1ll, output.cols()) < _Dropout).cast<float>() *
						_iDropout;
				} else {
					outPart(output, Index) = Activate(Weights * inp);
				}
			}
		}

		void NeuronLayer::BackPropagate(const netData & Gradient) {
			netData grad = Gradient.array() * Derivative(outPart(output)).array();
			netData prop = Weights.transpose() * grad;
			PreviousLayer->BackPropagate(outPart(prop));
			if (curGradient.cols()!=grad.cols()) {
				curGradient.resizeLike(grad);
				memset(curGradient.data(), 0, sizeof(float) * curGradient.size());
			}
			curGradient += grad;
			//(this->*updateWeights)(grad);
		}

		void NeuronLayer::BackPropagate(const netData& Gradient, int64_t Index) {
			netData grad = Gradient.array() * Derivative(outPart(output, Index)).array();
			netData prop = Weights.transpose() * grad;
			PreviousLayer->BackPropagate(outPart(prop), Index);
			if (curGradient.cols()!=grad.cols()) {
				curGradient.resizeLike(grad);
				memset(curGradient.data(), 0, sizeof(float) * curGradient.size());
			}
			curGradient += grad;
		}

		void NeuronLayer::UpdateWeights() {
			(this->*updateWeights)(curGradient);
			memset(curGradient.data(), 0, sizeof(float) * curGradient.size());
		}

		size_t NeuronLayer::nInput() const {
			return Weights.cols() - 1;
		}

		size_t NeuronLayer::DNA(std::map<void*, WeightData>& w) {
			if (!w.count(this)) {
				WeightData ret;
				ret.Weights = Weights.data();
				ret.Count = Weights.size();
				w[this] = ret;
				return PreviousLayer->DNA(w) + ret.Count;
			}
			return 0;
		}
	}
}
#include <PVX_NeuralNetsCPU.h>
#include "PVX_NeuralNets_Util.inl"

namespace PVX {
	namespace DeepNeuralNets {
		OutputLayer::OutputLayer(NeuralLayer_Base * Last, OutputType Type) :LastLayer{ Last }, Type{ Type } {
			switch (Type) {
				case PVX::DeepNeuralNets::OutputType::MeanSquare:
					FeedForward = [this] { FeedForwardMeanSquare(); };
					GetErrorFnc = [this](const netData& Data) { return GetError_MeanSquare(Data); };
					TrainFnc = [this](const netData& Data) { return Train_MeanSquare(Data); };
					break;
				case PVX::DeepNeuralNets::OutputType::SoftMax:
					FeedForward = [this] { FeedForwardSoftMax(); };
					GetErrorFnc = [this](const netData& Data) { return GetError_SoftMax(Data); };
					TrainFnc = [this](const netData& Data) { return Train_SoftMax(Data); };
					break;
				case PVX::DeepNeuralNets::OutputType::StableSoftMax:
					FeedForward = [this] { FeedForwardStableSoftMax(); };
					GetErrorFnc = [this](const netData& Data) { return GetError_SoftMax(Data); };
					TrainFnc = [this](const netData& Data) { return Train_SoftMax(Data); };
					break;
				default:
					break;
			}
		}
		float OutputLayer::GetError(const netData & Data) {
			return GetErrorFnc(Data);
		}
		float OutputLayer::Train(const netData & Data) {
			return TrainFnc(Data);
		}
		float OutputLayer::Train(const float * Data) {
			return Train(Eigen::Map<netData>((float*)Data, 1, output.cols()));;
		}

		float OutputLayer::Train_MeanSquare(const netData & TrainData) {
			netData dif = TrainData - output;
			Eigen::Map<Eigen::RowVectorXf> vec(dif.data(), dif.size());
			Error = (0.5f * (vec * vec.transpose())(0)) / dif.cols();
			LastLayer->BackPropagate(dif);
			LastLayer->UpdateWeights();
			return Error;
		}
		float OutputLayer::GetError_MeanSquare(const netData & Data) {
			netData dif = Data - output;
			Eigen::Map<Eigen::RowVectorXf> vec(dif.data(), dif.size());
			return (0.5f * (vec * vec.transpose())(0)) / dif.cols();
		}

		float OutputLayer::Train_SoftMax(const netData & TrainData) {
			float Error = -(TrainData.array()* Eigen::log(output.array())).sum() / output.cols();
			LastLayer->BackPropagate(TrainData - output);
			LastLayer->UpdateWeights();
			return Error;
		}
		float OutputLayer::GetError_SoftMax(const netData& Data) {
			return -(Data.array()* Eigen::log(output.array())).sum() / output.cols();
		}

		void OutputLayer::FeedForwardMeanSquare() {
			LastLayer->FeedForward(++Version);
			auto tmp = LastLayer->Output();
			output = outPart(tmp);
		}
		void OutputLayer::FeedForwardSoftMax() {
			LastLayer->FeedForward(++Version);
			auto tmp2 = LastLayer->Output();
			netData tmp = Eigen::exp(outPart(tmp2).array());
			netData a = 1.0f / (netData::Ones(1, tmp.rows()) * tmp).array();
			netData div = Eigen::Map<Eigen::RowVectorXf>(a.data(), a.size()).asDiagonal();
			output = (tmp * div);
		}
		void OutputLayer::FeedForwardStableSoftMax() {
			LastLayer->FeedForward(++Version);
			netData tmp = LastLayer->Output();
			output = outPart(tmp);

			for (auto i = 0; i < output.cols(); i++) {
				auto r = output.col(i);
				r -= netData::Constant(r.rows(), 1, r.maxCoeff());
				r = Eigen::exp(r.array());
				r *= 1.0f / r.sum();
			}
		}
		void OutputLayer::Result(float* res) {
			auto r = Result();
			memcpy(res, r.data(), sizeof(float) * r.cols());
		}

		const netData& OutputLayer::Result() {
			FeedForward();
			return output;
		}
		std::set<NeuralLayer_Base*> OutputLayer::Gather() {
			std::set<NeuralLayer_Base*> g;
			LastLayer->Gather(g);
			return g;
		}
		size_t OutputLayer::nOutput() {
			return LastLayer->output.rows() - 1;
		}
		void OutputLayer::Save(PVX::BinSaver& bin, const std::map<NeuralLayer_Base*, size_t>& IndexOf) {
			bin.Write("TYPE", int(Type));
			bin.Write("LAST", int(IndexOf.at(LastLayer)));
		}
	
		void OutputLayer::SaveCheckpoint() {
			if (Checkpoint.Layers.size()==0) {
				Checkpoint = GetDNA();
			}
			CheckpointDNA = Checkpoint.GetData();
			CheckpointError = Error;
		}

		float OutputLayer::LoadCheckpoint() {
			Error = CheckpointError;
			Checkpoint.SetData(CheckpointDNA.data());
			return Error;
		}

		void OutputLayer::ResetMomentum() {
			LastLayer->ResetMomentum();
		}

		NetDNA OutputLayer::GetDNA() {
			std::map<void*, WeightData> data;
			LastLayer->DNA(data);
			NetDNA ret;
			ret.Size = 0;
			for (auto&[l, dt] : data) {
				dt.Offset = ret.Size;
				ret.Size += dt.Count;
				ret.Layers.push_back(dt);
			}
			return ret;
		}
	}
}
#pragma once
#include <PVX_clMatrix.h>

class Globals_ {
public:
	PVX::OpenCL cl;
	PVX::Kernel* kAddMatrix[4];
	PVX::Kernel* kFactorMat[2];
	PVX::Kernel* kElemMul[4];
	PVX::Kernel* kElemDiv[4];

	PVX::Kernel* kReLU[2];
	PVX::Kernel* kTanh[2];
	PVX::Kernel* kTanhBias[2];
	PVX::Kernel* kSigmoid[2];

	PVX::Kernel* kdReLU[2];
	PVX::Kernel* kdTanh[2];
	PVX::Kernel* kdTanhBias[2];
	PVX::Kernel* kdSigmoid[2];

	PVX::Kernel* kMul_dReLU[4];
	PVX::Kernel* kMul_dTanh[4];
	PVX::Kernel* kMul_dTanhBias[4];
	PVX::Kernel* kMul_dSigmoid[4];

	PVX::Kernel* kLog[2];
	PVX::Kernel* kExp[2];
	PVX::Kernel* kSqrt[2];

	PVX::Kernel* kSetConst1;
	PVX::Kernel* kSetConst;
	PVX::Kernel* kAddConst;

	PVX::Kernel* kNorm2;
	PVX::Kernel* kSum;

	PVX::Kernel* kDivDiag;
	PVX::Kernel* kMulDiag;

	PVX::Kernel* kMul_log[4];

	size_t WorkGroupSize;
	size_t SquareSide;
	constexpr size_t matPadding(int x) {
		return SquareSide * ((x + SquareSide -1) / SquareSide);
	}

	PVX::NeuralNets::clMatrix& Push(int r, int c);
	PVX::NeuralNets::clMatrix& Push(int r, int c, const std::vector<float>& Data);
	PVX::NeuralNets::clMatrix& Push(int r, int c, const float* Data);
	void Pop(int Count);
	PVX::Buffer& GetReadBuffer(int size);
	
	cl_command_queue* q;

	Globals_();
	~Globals_();
private:
	std::vector<PVX::NeuralNets::clMatrix*> tmpStack;
	PVX::Buffer ReadTemp;
	int64_t StackTop = -1;
};
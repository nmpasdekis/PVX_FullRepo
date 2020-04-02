#include "Globals.h"
#include <tuple>
#include <PVX_String.h>

namespace {
	std::string GenerateUnaryOp(const std::string& Name, const std::string& Code, const std::string& Ret) {
		const std::vector<std::tuple<std::string, std::string>> Tp{
			{ "_0", "(FactorA * A[OffA + StrideA * j + i] + BiasA)" },
			{ "_T", "(FactorA * A[OffA + j + StrideA * i] + BiasA)" }
		};
		std::string ret = "";
		for (auto& [t, A1]:Tp) {
			ret += "__kernel void " + Name + t +
R"CODE((__global float* Out, int OffOut, int StrideOut, const float FactorOut, float BiasOut,
	__global const float* A, int OffA, int StrideA, const float FactorA, float BiasA,
	int Rows, int Cols){
	size_t i = get_global_id(0); size_t j = get_global_id(1);
	if(i < Rows && j < Cols) {
		)CODE" + PVX::String::Replace(Code, "A1", A1) + (Code.size() ? "\n\t\t" : "") +
				"Out[OffOut + StrideOut * j + i] = BiasOut + FactorOut * (" + PVX::String::Replace(Ret, "A1", A1) +
				R"CODE();
	}
}
)CODE";
		}
		return ret;
	}
	std::string GenerateUnaryOp(const std::string& Name, const std::string& Ret) {
		return GenerateUnaryOp(Name, "", Ret);
	}

	std::string GenerateBinaryOp(const std::string& Name, const std::string& Code, const std::string& Ret) {
		const std::string _A1_ = "(FactorA * A[OffA + StrideA * j + i] + BiasA)";
		const std::string _A1T = "(FactorA * A[OffA + j + StrideA * i] + BiasA)";
		const std::string _B1_ = "(FactorB * B[OffB + StrideB * j + i] + BiasB)";
		const std::string _B1T = "(FactorB * B[OffB + j + StrideB * i] + BiasB)";

		const std::vector<std::tuple<std::string, std::string, std::string>> Tp{
			{ "_00", _A1_, _B1_ },
			{ "_0T", _A1_, _B1T },
			{ "_T0", _A1T, _B1_ },
			{ "_TT", _A1T, _B1T },
		};
		std::string ret = "";
		for (auto& [t, A1, B1] : Tp) {
			ret += "__kernel void " + Name + t +
R"CODE((__global float* Out, int OffOut, int StrideOut, float FactorOut, float BiasOut,
	__global const float* A, int OffA, int StrideA, float FactorA, float BiasA,
	__global const float* B, int OffB, int StrideB, float FactorB, float BiasB,
	int Rows, int Cols)
{
	size_t i = get_global_id(0); size_t j = get_global_id(1);
	if(i < Rows && j < Cols) {
		)CODE" + PVX::String::Replace(PVX::String::Replace(Code, "A1", A1), "B1", B1) + (Code.size() ? "\n\t\t" : "") + R"CODE(Out[OffOut + StrideOut * j + i] = BiasOut + FactorOut * ()CODE" + 
				PVX::String::Replace(PVX::String::Replace(Ret, "A1", A1), "B1", B1) + R"CODE();
	}
}
)CODE";
		}
		return ret;
	}

	std::string GenerateBinaryOp(const std::string& Name, const std::string& Ret) {
		return GenerateBinaryOp(Name, "", Ret);
	}

	std::string Consts() {
		return R"CODE(
__kernel void SetConst1(__global float* Out, int Offset, float Value){
	Out[Offset] = Value;
}
__kernel void SetConst(__global float* Out, int OffOut, int StrideOut, const float FactorOut, 
	int Rows, int Cols, float Value)
{
	size_t i = get_global_id(0); size_t j = get_global_id(1);
	if(i < Rows && j < Cols) Out[OffOut + StrideOut * j + i] = FactorOut * Value;
}
__kernel void AddConst(__global float* Out, int OffOut, int StrideOut, const float FactorOut, 
	int Rows, int Cols, float Value)
{
	size_t i = get_global_id(0); size_t j = get_global_id(1);
	if(i < Rows && j < Cols) {
		Out[OffOut + StrideOut * j + i] = FactorOut * (Out[OffOut + StrideOut * j + i] + Value);
	}
}
)CODE";
	}
}

inline std::string Reduce() {
	return R"CODE(
__kernel void Norm2(__global float * Out, const __global float4 * mat, int Rows, int Cols, int Stride, __local float4 * mem){
	int gy = get_global_id(0);
	int gs = get_local_size(0);
	int localIndex = get_local_id(0);

	int realY = gy << 2;
	int col = realY / Stride;
	int row = realY % Stride;

	mem[localIndex] = (mat[gy] * mat[gy]) * step((float)(col), (float)(Cols-1)) * step((float4)(row, row+1, row+2, row+3), (float4)(Rows-1));
	barrier(CLK_LOCAL_MEM_FENCE);
	for(int i = (gs >> 1);i;i>>=1){
		if(localIndex < i)
			mem[localIndex] += mem[localIndex + i];
		barrier(CLK_LOCAL_MEM_FENCE);
	}
	if(!localIndex)
		Out[get_group_id(0)] = dot(mem[0], (float4)(1.0f));
}
__kernel void Sum(__global float * Out, const __global float4 * mat, int Rows, int Cols, int Stride, __local float4 * mem){
	int gy = get_global_id(0);
	int gs = get_local_size(0);
	int localIndex = get_local_id(0);

	int realY = gy << 2;
	int col = realY / Stride;
	int row = realY % Stride;

	mem[localIndex] = mat[gy] * step((float)(col), (float)(Cols-1)) * step((float4)(row, row+1, row+2, row+3), (float4)(Rows-1));
	barrier(CLK_LOCAL_MEM_FENCE);
	for(int i = (gs >> 1);i;i>>=1){
		if(localIndex < i)
			mem[localIndex] += mem[localIndex + i];
		barrier(CLK_LOCAL_MEM_FENCE);
	}
	if(!localIndex)
		Out[get_group_id(0)] = dot(mem[0], (float4)(1.0f));
}
)CODE";
}

inline std::string Diag() {
	return R"CODE(

__kernel void divDiag(
	__global float * Out, int strideOut, 
	__global const float * mat, int strideMat,
	__global const float * diag,
	int Rows, int Cols){
	
	int gy = get_global_id(0);
	int gx = get_global_id(1);
	if(gx < Cols && gy < Rows)
		Out[gy + gx * strideOut] = mat[gy + gx * strideMat] / diag[gx];
}

__kernel void mulDiag(
	__global float * Out, int strideOut, 
	__global const float * mat, int strideMat,
	__global const float * diag,
	int Rows, int Cols){
	
	int gy = get_global_id(0);
	int gx = get_global_id(1);
	if(gx < Cols && gy < Rows)
		Out[gy + gx * strideOut] = mat[gy + gx * strideMat] * diag[gx];
}
)CODE";
}

Globals_::Globals_() : 
	cl(1, 0), q{ &cl.Queue()() }, ReadTemp{ cl.MakeBuffer(4, PVX::BufferAccess::GPU_0W_CPU_R0) }
{
	clblasSetup();
	cl.LoadProgram(
		GenerateBinaryOp("addMat", "A1 + B1")+
		GenerateBinaryOp("elemMul", "A1 * B1")+
		GenerateBinaryOp("elemDiv", "A1 / B1")+
		GenerateUnaryOp("factorMat", "A1")+

		GenerateUnaryOp("ReLU", "max(A1, 0.0f)")+
		GenerateUnaryOp("tanh", "tanh(A1)")+
		GenerateUnaryOp("tanhBiased", "0.5f + 0.5f * tanh(A1)")+
		GenerateUnaryOp("Sigmoid", "1.0f / (1.0f + exp(-A1))")+

		GenerateUnaryOp("ReLU_Der", "(1.0f - step(0.0f, -A1) * 0.999f)")+
		GenerateUnaryOp("tanh_Der", "float tmp = tanh(A1);", "1.0f - tmp * tmp")+
		GenerateUnaryOp("tanhBiased_Der", "float tmp = tanh(A1);", "5.0f * (1.0f - tmp * tmp)")+
		GenerateUnaryOp("Sigmoid_Der", "float tmp = 1.0f / (1.0f + exp(-A1));", "tmp * (1.0f - tmp)")+

		GenerateBinaryOp("mul_ReLU_Der", "A1 * (1.0f - step(0.0f, -B1) * 0.999f)")+
		GenerateBinaryOp("mul_tanh_Der", "float tmp = tanh(B1);", "A1 * 1.0f - tmp * tmp")+
		GenerateBinaryOp("mul_tanhBiased_Der", "float tmp = tanh(B1);", "A1 * 5.0f * (1.0f - tmp * tmp)")+
		GenerateBinaryOp("mul_Sigmoid_Der", "float tmp = 1.0f / (1.0f + exp(-B1));", "A1 * tmp * (1.0f - tmp)")+

		GenerateUnaryOp("log", "log(A1)")+
		GenerateUnaryOp("exp", "exp(A1)")+
		GenerateUnaryOp("sqrt", "sqrt(A1)")+
		GenerateUnaryOp("elemInv", "1.0f / A1")+

		GenerateBinaryOp("mul_log", "A1 * log(B1)")+

		Consts() + 
		Reduce() + 
		Diag()
	);

	kAddMatrix[0] = cl.GetKernel("addMat_00");
	//kAddMatrix[1] = cl.GetKernel("addMat_0T");
	//kAddMatrix[2] = cl.GetKernel("addMat_T0");
	//kAddMatrix[3] = cl.GetKernel("addMat_TT");

	kFactorMat[0] = cl.GetKernel("factorMat_0");
	//kFactorMat[1] = cl.GetKernel("factorMat_T");

	kElemMul[0] = cl.GetKernel("elemMul_00");
	//kElemMul[1] = cl.GetKernel("elemMul_0T");
	//kElemMul[2] = cl.GetKernel("elemMul_T0");
	//kElemMul[3] = cl.GetKernel("elemMul_TT");

	kElemDiv[0] = cl.GetKernel("elemDiv_00");
	//kElemDiv[1] = cl.GetKernel("elemDiv_0T");
	//kElemDiv[2] = cl.GetKernel("elemDiv_T0");
	//kElemDiv[3] = cl.GetKernel("elemDiv_TT");

	kReLU[0] = cl.GetKernel("ReLU_0");
	//kReLU[1] = cl.GetKernel("ReLU_T");

	kdReLU[0] = cl.GetKernel("ReLU_Der_0");
	//kdReLU[1] = cl.GetKernel("ReLU_Der_T");

	kTanh[0] = cl.GetKernel("tanh_0");
	//kTanh[1] = cl.GetKernel("tanh_T");

	kdTanh[0] = cl.GetKernel("tanh_Der_0");
	//kdTanh[1] = cl.GetKernel("tanh_Der_T");

	kTanhBias[0] = cl.GetKernel("tanhBiased_0");
	//kTanhBias[1] = cl.GetKernel("tanhBiased_T");

	kdTanhBias[0] = cl.GetKernel("tanhBiased_Der_0");
	//kdTanhBias[1] = cl.GetKernel("tanhBiased_Der_T");

	kSigmoid[0] = cl.GetKernel("Sigmoid_0");
	//kSigmoid[1] = cl.GetKernel("Sigmoid_T");

	kdSigmoid[0] = cl.GetKernel("Sigmoid_Der_0");
	//kdSigmoid[1] = cl.GetKernel("Sigmoid_Der_T");

	kLog[0] = cl.GetKernel("log_0");
	//kLog[1] = cl.GetKernel("log_T");

	kExp[0] = cl.GetKernel("exp_0");
	//kExp[1] = cl.GetKernel("exp_T");

	kSqrt[0] = cl.GetKernel("sqrt_0");
	//kSqrt[1] = cl.GetKernel("sqrt_T");


	kSetConst1 = cl.GetKernel("SetConst1");
	kSetConst = cl.GetKernel("SetConst");
	kAddConst = cl.GetKernel("AddConst");

	kNorm2 = cl.GetKernel("Norm2");
	kSum = cl.GetKernel("Sum");

	kDivDiag = cl.GetKernel("divDiag");
	kMulDiag = cl.GetKernel("mulDiag");

	kMul_dReLU[0] = cl.GetKernel("mul_ReLU_Der_00");
	//kMul_dReLU[1] = cl.GetKernel("mul_ReLU_Der_0T");
	//kMul_dReLU[2] = cl.GetKernel("mul_ReLU_Der_T0");
	//kMul_dReLU[3] = cl.GetKernel("mul_ReLU_Der_TT");

	kMul_dTanh[0] = cl.GetKernel("mul_tanh_Der_00");
	//kMul_dTanh[1] = cl.GetKernel("mul_tanh_Der_0T");
	//kMul_dTanh[2] = cl.GetKernel("mul_tanh_Der_T0");
	//kMul_dTanh[3] = cl.GetKernel("mul_tanh_Der_TT");

	kMul_dTanhBias[0] = cl.GetKernel("mul_tanhBiased_Der_00");
	//kMul_dTanhBias[1] = cl.GetKernel("mul_tanhBiased_Der_0T");
	//kMul_dTanhBias[2] = cl.GetKernel("mul_tanhBiased_Der_T0");
	//kMul_dTanhBias[3] = cl.GetKernel("mul_tanhBiased_Der_TT");

	kMul_dSigmoid[0] = cl.GetKernel("mul_Sigmoid_Der_00");
	//kMul_dSigmoid[1] = cl.GetKernel("mul_Sigmoid_Der_0T");
	//kMul_dSigmoid[2] = cl.GetKernel("mul_Sigmoid_Der_T0");
	//kMul_dSigmoid[3] = cl.GetKernel("mul_Sigmoid_Der_TT");

	kMul_log[0] = cl.GetKernel("mul_log_00");
	//kMul_log[1] = cl.GetKernel("mul_log_0T");
	//kMul_log[2] = cl.GetKernel("mul_log_T0");
	//kMul_log[3] = cl.GetKernel("mul_log_TT");

	WorkGroupSize = kAddMatrix[0]->WorkGroupSize();
	for (SquareSide = 1; ((SquareSide*SquareSide)<<1) <= WorkGroupSize; SquareSide <<= 1);
}

PVX::NeuralNets::clMatrix& Globals_::Push(int r, int c) {
	StackTop++;
	if (StackTop>=int64_t(tmpStack.size())) {
		return tmpStack.emplace_back(new PVX::NeuralNets::clMatrix(r, c))->SetTemp();
	} else {
		return tmpStack[StackTop]->Reset(r, c);
	}
}
PVX::NeuralNets::clMatrix& Globals_::Push(int r, int c, const std::vector<float>& Data) {
	StackTop++;
	if (StackTop>=int64_t(tmpStack.size())) {
		return tmpStack.emplace_back(new PVX::NeuralNets::clMatrix(r, c, Data))->SetTemp();
	} else {
		tmpStack[StackTop]->Reset(r, c).Write(Data);
		return *tmpStack[StackTop];
	}
}
PVX::NeuralNets::clMatrix& Globals_::Push(int r, int c, const float* Data) {
	StackTop++;
	if (StackTop>=int64_t(tmpStack.size())) {
		return tmpStack.emplace_back(new PVX::NeuralNets::clMatrix(r, c, Data))->SetTemp();
	} else {
		tmpStack[StackTop]->Reset(r, c).Write(Data);
		return *tmpStack[StackTop];
	}
}
void Globals_::Pop(int Count) {
	StackTop -= Count;
}

PVX::Buffer& Globals_::GetReadBuffer(int size) {
	if (ReadTemp.Size()<size*sizeof(float)) {
		clFlush(*q);
		ReadTemp = cl.MakeBuffer(size*sizeof(float), PVX::BufferAccess::GPU_0W_CPU_R0);
	}
	return ReadTemp;
}

Globals_::~Globals_() {
	clblasTeardown();
	for (auto p : tmpStack) delete p;
}
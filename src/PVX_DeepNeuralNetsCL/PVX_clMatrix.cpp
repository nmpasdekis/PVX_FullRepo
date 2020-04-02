#include <PVX_clMatrix.h>
#include <clBLAS.h>
#include "Globals.h"
#include <cassert>
#include <iostream>

#include "PVX_NeuralNets_Util.inl"

namespace PVX::NeuralNets {
	namespace {
		constexpr clblasTranspose NotTranspose(const clblasTranspose tr) {
			if constexpr (clblasTranspose::clblasTrans==1 && clblasTranspose::clblasNoTrans == 0) {
				return clblasTranspose(tr^1);
			} else {
				return tr == clblasTranspose::clblasNoTrans ? clblasTranspose::clblasTrans : clblasTranspose::clblasNoTrans;
			}
		}
		constexpr int IsTransposed(const clblasTranspose x) {
			if constexpr (clblasTranspose::clblasTrans==1 && clblasTranspose::clblasNoTrans == 0) {
				return x;
			} else {
				return x == clblasTranspose::clblasTrans;
			}
		}

		constexpr clblasTranspose ToTran(bool t) {
			if constexpr (clblasTranspose::clblasTrans==1 && clblasTranspose::clblasNoTrans == 0) {
				return clblasTranspose(t);
			} else {
				return t ? clblasTranspose::clblasTrans : clblasTranspose::clblasNoTrans;
			}
		}
		constexpr clblasTranspose Combine(clblasTranspose x, clblasTranspose y) {
			if constexpr (clblasTranspose::clblasTrans==1 && clblasTranspose::clblasNoTrans == 0) {
				return clblasTranspose(x ^ y);
			} else {
				return IsTransposed(x) ? NotTranspose(y) : y;
			}
		}

		template<typename T>
		constexpr int K(T&& Name) {
			if constexpr (sizeof(Name)==2)
				return Name[0];
			else if constexpr (sizeof(Name)==3)
				return Name[0]|(Name[1]<<8);
			else if constexpr (sizeof(Name)==4)
				return Name[0]|(Name[1]<<8)|(Name[2]<<16);
			else if constexpr (sizeof(Name)==5)
				return Name[0]|(Name[1]<<8)|(Name[2]<<16)|(Name[3]<<24);
			else
				return 0;
		}
		Globals_ g;
	}

	clMatrix& clMatrix::Div_diag(const clMatrix& mat, const clMatrix& diag) {
		g.kDivDiag->Run(Range, {
			Data(), Stride,
			mat.Data(), mat.Stride,
			diag.Data(),
			Rows, Cols
		});
		return *this;
	}
	clMatrix& clMatrix::Mul_diag(const clMatrix& mat, const clMatrix& diag) {
		g.kMulDiag->Run(mat.Range, {
			Data(), Stride,
			mat.Data(), mat.Stride,
			diag.Data(),
			Rows, Cols
		});
		return *this;
	}

	void clMatrix::divDiag(clMatrix& out, const clMatrix& mat, const clMatrix& diag) {
		g.kDivDiag->Run(out.Range, {
			out.Data(), out.Stride,
			mat.Data(), mat.Stride,
			diag.Data(),
			mat.rows(), mat.cols()
		});
	}
	void clMatrix::mulDiag(clMatrix& out, const clMatrix& mat, const clMatrix& diag) {
		g.kMulDiag->Run(mat.Range, {
			out.Data(), out.Offset, out.Stride, 1.0f, 0.0f,
			mat.Data(), mat.Offset, mat.Stride, 1.0f, 0.0f,
			diag.Data(), diag.Offset, diag.Stride, 1.0f, 0.0f,
			mat.rows(), mat.cols()
		});
	}

	void Flush() { g.cl.Queue().flush(); };
	void Finish() { g.cl.Queue().finish(); };

	void clMatrix::Read(float* data) const {
		if (int(Data.GetAccess())&CL_MEM_HOST_READ_ONLY) {
			clblasReadMatrix(clblasOrder::clblasColumnMajor,
				Rows, Cols, sizeof(float),
				Data()(), Offset, Stride,
				data, 0, Rows,
				*g.q, 0, nullptr
			);
		} else {
			auto sz = Rows * Cols;
			auto& tmp = g.GetReadBuffer(sz);
			clblasCopyMatrix(clblasOrder::clblasColumnMajor,
				Rows, Cols, sizeof(float),
				Data()(), Offset, Stride,
				tmp()(), 0, Rows,
				*g.q, 0, nullptr
			);
			tmp.Read(data, sz * sizeof(float));
		}
	}
	std::vector<float> clMatrix::Read() const {
		std::vector<float> ret(size_t(Rows) * Cols);
		Read(ret.data());
		return ret;
	}

	void clMatrix::ReadBlocking(float* data) const {
		if (int(Data.GetAccess())&CL_MEM_HOST_READ_ONLY) {
			clblasReadMatrix(clblasOrder::clblasColumnMajor,
				Rows, Cols, sizeof(float),
				Data()(), Offset, Stride,
				data, 0, Rows,
				*g.q, 0, nullptr
			);
			Flush();
		} else {
			auto sz = Rows * Cols;
			auto& tmp = g.GetReadBuffer(sz);
			clblasCopyMatrix(clblasOrder::clblasColumnMajor,
				Rows, Cols, sizeof(float),
				Data()(), Offset, Stride,
				tmp()(), 0, Rows,
				*g.q, 0, nullptr
			);
			tmp.ReadBlocking(data, sz * sizeof(float));
		}
	}
	std::vector<float> clMatrix::ReadBlocking() const {
		std::vector<float> ret(size_t(Rows) * Cols);
		ReadBlocking(ret.data());
		return ret;
	}

	void clMatrix::Write(const float* data) {
		if (int(Data.GetAccess())&CL_MEM_HOST_WRITE_ONLY) {
			clblasWriteMatrix(clblasOrder::clblasColumnMajor,
				Rows, Cols, sizeof(float),
				data, 0, Rows,
				Data()(), Offset, Stride,
				*g.q, 0, nullptr
			);
		} else {
			auto sz = size_t(Rows) * Cols * sizeof(float);
			auto tmp = g.cl.MakeBuffer(sz, PVX::BufferAccess::GPU_R0_CPU_00, data);
			clblasCopyMatrix(clblasOrder::clblasColumnMajor,
				Rows, Cols, sizeof(float),
				tmp()(), 0, Rows,
				Data()(), Offset, Stride,
				*g.q, 0, nullptr
			);
		}
	}
	void clMatrix::Write(const std::vector<float>& Data) {
		Write(Data.data());
	}

	clMatrix::clMatrix() :
		Range{ 0 },
		Rows{ 0 }, Cols{ 0 },
		Stride{ 0 },
		Offset{ 0 }, Data{ g.cl.MakeBuffer() }{}

	clMatrix::~clMatrix() {
		if (IsTemp)
			g.Pop(1);
	}

	clMatrix::clMatrix(const PVX::Buffer& dt, int Rows, int Cols, int Offset, int Stride) :
		Range{ g.matPadding(Rows), g.matPadding(Cols) },
		Data{ dt },
		Rows{ Rows }, Cols{ Cols },
		Offset{ Offset }, Stride{ Stride }
	{}

	clMatrix::clMatrix(int rows, int cols, BufferAccess Access) :
		Range{ g.matPadding(rows), g.matPadding(cols) },
		Rows{ rows }, Cols{ cols },
		Stride{ int(g.matPadding(rows)) },
		Offset{ 0 },
		Data{ g.cl.MakeBuffer(g.matPadding(cols) * g.matPadding(rows) * sizeof(float), Access) }
	{}
	clMatrix::clMatrix(int rows, int cols, const float* Data, BufferAccess Access) :
		Range{ g.matPadding(rows), g.matPadding(cols) },
		Rows{ rows }, Cols{ cols },
		Stride{ int(g.matPadding(rows)) },
		Offset{ 0 },
		Data{ g.cl.MakeBuffer(g.matPadding(cols) * g.matPadding(rows) * sizeof(float), Access) }
	{
		Write(Data);
	}
	clMatrix::clMatrix(int rows, int cols, const std::vector<float>& Data, BufferAccess Access) : clMatrix(rows, cols, Data.data(), Access) {}
	clMatrix& clMatrix::SetTemp() {
		IsTemp = true;
		return *this;
	}
	clMatrixExpr clMatrix::Transpose() const {
		return clMatrixExpr(*(clMatrix*)this, 1.0f, true);
	}
	clMatrix clMatrix::block(int row, int col, int rowCount, int colCount) const {
		return clMatrix(Data, rowCount, colCount, Offset + row + col * Stride, Stride);
	}
	clMatrix clMatrix::row(int Index) const {
		return clMatrix(Data, 1, Cols, Offset + Index, Stride);
	}
	clMatrix clMatrix::col(int Index) const {
		return clMatrix(Data, Rows, 1, Offset + Stride * Index, Stride);
	}
	clMatrix clMatrix::LastRow() const {
		return clMatrix(Data, 1, Cols, Offset + (Rows - 1), Stride);
		//return clMatrix(Data, Rows, 1, Offset + (Rows - 1), Stride);
	}
	clMatrix& clMatrix::Reset(int r, int c) {
		Rows = r;
		Cols = c;
		Offset = 0;
		Stride = int(g.matPadding(r));
		Range = cl::NDRange(Stride, g.matPadding(c));
		size_t sz = Range[0] * Range[1];
		if (sz > Data.Size()) {
			Flush();
			Data = g.cl.MakeBuffer(sz * sizeof(float), BufferAccess::GPU_RW_CPU_00);
		}
		return *this;
	}
	clMatrix clMatrix::Row(int size, BufferAccess Access) {
		return clMatrix(g.cl.MakeBuffer(g.matPadding(size) * sizeof(float), Access), 1, size, 0, 1);
	}
	clMatrix clMatrix::Col(int size, BufferAccess Access) {
		return clMatrix(g.cl.MakeBuffer(g.matPadding(size) * sizeof(float), Access), size, 1, 0, 1);
	}

	clMatrix clMatrix::Random(int Rows, int Cols, float min, float max) {
		std::vector<float> dt(Rows * Cols);
		for (auto& d:dt) d = (double)rand() / (RAND_MAX + 1) * (max - min) + min;
		return clMatrix(Rows, Cols, dt.data());
	}

	//clMatrixExpr clMatrix::Ones(int Rows, int Cols) {
	//	auto ret = clMatrix(Rows, Cols)
	//	std::vector<float> dt(Rows * Cols, 1.0f);
	//	return clMatrix(Rows, Cols, dt.data());
	//}

	clMatrixExpr::clMatrixExpr(std::unique_ptr<clMatrixExpr> Left, std::unique_ptr<clMatrixExpr> Right, int Op) :
		Left{ std::move(Left) }, Right{ std::move(Right) }, Op{ Op } {
		_Rows = this->Left->Rows();
		_Cols = this->Right->Cols();
	}
	//clMatrixExpr::clMatrixExpr(std::unique_ptr<clMatrixExpr> Right, int Op) :
	//	_Rows{ Right->Rows() }, _Cols{ Right->Cols() }, Right{ std::move(Right) }, Op{ Op } {
	//}

	clMatrixExpr::clMatrixExpr(clMatrixExpr&& Right, float alpha, bool transposed, int Op) {
		if (!Op || !Right.Op) {
			this->Op = Right.Op + Op;
			this->matrix = Right.matrix;
			this->alpha = Right.alpha * alpha;
			this->beta = Right.beta;
			if (Right.Left.get()) this->Left = std::move(Right.Left);
			if (Right.Right.get()) this->Right = std::move(Right.Right);
			this->transpose = Combine(ToTran(transposed), Right.transpose);
			this->_Rows = Right._Rows;
			this->_Cols = Right._Cols;
		} else {
			this->Op = Op;
			this->alpha = alpha;
			this->transpose = ToTran(transposed);
			this->_Rows = Right.Rows();
			this->_Cols = Right.Cols();
			this->Right = std::make_unique<clMatrixExpr>(std::move(Right));
		}
	}

	clMatrixExpr::clMatrixExpr(const clMatrix& rhs, float alpha, bool transposed, int Op) :
		matrix{ &rhs },
		_Rows{ rhs.Rows },
		_Cols{ rhs.Cols },
		alpha{ alpha },
		Op{ Op },
		transpose{ transposed ? clblasTranspose::clblasTrans : clblasTranspose::clblasNoTrans }
	{
	}

	clMatrixExpr::clMatrixExpr(int Op, clMatrixExpr&& Param) :
		Op{ Op }, Right{ std::make_unique<clMatrixExpr>(std::move(Param)) }, _Rows{ Param.Rows() }, _Cols{ Param.Cols() }
	{}

	clMatrixExpr clMatrixExpr::operator*(const clMatrix& rhs) {
		assert(Cols() == rhs.Rows);
		return clMatrixExpr(std::make_unique<clMatrixExpr>(std::move(*this)), std::make_unique<clMatrixExpr>(rhs), '*');
	}
	clMatrixExpr clMatrixExpr::operator+(const clMatrix& rhs) {
		assert(Rows()==rhs.Rows && Cols() == rhs.Cols);
		return clMatrixExpr(std::make_unique<clMatrixExpr>(std::move(*this)), std::make_unique<clMatrixExpr>(rhs), '+');
	}
	clMatrixExpr clMatrixExpr::operator-(const clMatrix& rhs) {
		assert(Rows()==rhs.Rows && Cols() == rhs.Cols);
		return clMatrixExpr(std::make_unique<clMatrixExpr>(std::move(*this)), std::make_unique<clMatrixExpr>(rhs, -1.0f), '+');
	}
	clMatrixExpr clMatrixExpr::operator*(const float rhs) {
		beta *= rhs;
		return clMatrixExpr(std::move(*this), rhs);
	}
	clMatrixExpr clMatrixExpr::operator/(float rhs) {
		beta /= rhs;
		return clMatrixExpr(std::move(*this), 1.0f / rhs);
	}
	clMatrixExpr clMatrixExpr::operator+(float rhs) {
		beta += rhs;
		return std::move(*this);
	}
	clMatrixExpr clMatrixExpr::operator-(float rhs) {
		beta = -rhs;
		return std::move(*this);
	}

	clMatrixExpr clMatrixExpr::operator-() {
		return clMatrixExpr(std::move(*this), -1.0f, false, 0);
	}
	clMatrixExpr clMatrixExpr::Transpose() {
		return clMatrixExpr(std::move(*this), 1.0f, true, 0);
	}
	clMatrixExpr clMatrixExpr::operator*(clMatrixExpr&& rhs) {
		//if (IsNull()) return std::move(rhs);
		//if (rhs.IsNull()) return std::move(*this);
		return clMatrixExpr(std::make_unique<clMatrixExpr>(std::move(*this)), std::make_unique<clMatrixExpr>(std::move(rhs)), '*');
	}

	clMatrixExpr clMatrixExpr::operator+(clMatrixExpr&& rhs) {
		//if (IsNull()) return std::move(rhs);
		//if (rhs.IsNull()) return std::move(*this);
		return clMatrixExpr(std::make_unique<clMatrixExpr>(std::move(*this)), std::make_unique<clMatrixExpr>(std::move(rhs)), '+');
	}

	clMatrixExpr clMatrixExpr::operator-(clMatrixExpr&& rhs) {
		//if (IsNull()) return clMatrixExpr(std::move(rhs), -1.0f);
		//if (rhs.IsNull()) return std::move(*this);
		return clMatrixExpr(std::make_unique<clMatrixExpr>(std::move(*this), 1.0f), std::make_unique<clMatrixExpr>(std::move(rhs), -1.0f), '+');
	}


	clMatrixExpr clMatrix::operator*(clMatrixExpr&& rhs) const {
		if (rhs.IsNull()) return clMatrixExpr(*this);
		return clMatrixExpr(std::make_unique<clMatrixExpr>(*this), std::make_unique<clMatrixExpr>(std::move(rhs)), '*');
	}
	clMatrixExpr clMatrix::operator+(clMatrixExpr&& rhs) const {
		if (rhs.IsNull()) return clMatrixExpr(*this);
		return clMatrixExpr(std::make_unique<clMatrixExpr>(*this), std::make_unique<clMatrixExpr>(std::move(rhs)), '+');
	}
	clMatrixExpr clMatrix::operator-(clMatrixExpr&& rhs) const {
		if (rhs.IsNull()) return clMatrixExpr(*this);
		return clMatrixExpr(std::make_unique<clMatrixExpr>(*this), std::make_unique<clMatrixExpr>(std::move(rhs), -1.0f), '+');
	}


	clMatrixExpr clMatrix::operator*(const clMatrix& rhs) const {
		return clMatrixExpr(std::make_unique<clMatrixExpr>(*this), std::make_unique<clMatrixExpr>(rhs), '*');
	}
	clMatrixExpr clMatrix::operator+(const clMatrix& rhs) const {
		return clMatrixExpr(std::make_unique<clMatrixExpr>(*this), std::make_unique<clMatrixExpr>(rhs), '+');
	}
	clMatrixExpr clMatrix::operator-(const clMatrix& rhs) const {
		return clMatrixExpr(std::make_unique<clMatrixExpr>(*this), std::make_unique<clMatrixExpr>(rhs, -1.0f), '+');
	}
	clMatrixExpr clMatrix::operator*(float rhs) const {
		return clMatrixExpr(*this, rhs);
	}
	clMatrixExpr clMatrix::operator/(float rhs) const {
		return clMatrixExpr(*this, 1.0f / rhs);
	}

	clMatrixExpr clMatrix::operator+(float rhs) const {
		auto ret = clMatrixExpr(*this);
		ret.beta = rhs;
		return std::move(ret);
	}

	clMatrixExpr clMatrix::operator-(float rhs) const {
		auto ret = clMatrixExpr(*this);
		ret.beta = -rhs;
		return std::move(ret);
	}

	clMatrix& clMatrix::operator+=(const clMatrix& rhs) {
		*this = *this + rhs;
		return *this;
	}

	clMatrix& clMatrix::operator+=(clMatrixExpr&& rhs) {
		*this = (*this) + std::move(rhs);
		return *this;
	}

	clMatrix& clMatrix::operator+=(float rhs) {
		*this = (*this) + rhs;
		return *this;
	}

	clMatrix& clMatrix::operator-=(const clMatrix& rhs) {
		*this = *this - rhs;
		return *this;
	}

	clMatrix& clMatrix::operator-=(clMatrixExpr&& rhs) {
		*this = (*this) - std::move(rhs);
		return *this;
	}

	clMatrix& clMatrix::operator-=(float rhs) {
		*this = (*this) - rhs;
		return *this;
	}

	clMatrix& clMatrix::operator*=(float rhs) {
		(*this) = (*this) * rhs;
		return (*this);
	}

	clMatrix& clMatrix::operator*=(const clMatrix& rhs) {
		(*this) = (*this).Element_Multiply(rhs);
		return (*this);
	}

	clMatrix& clMatrix::operator*=(clMatrixExpr&& rhs) {
		(*this) = (*this).Element_Multiply(std::move(rhs));
		return (*this);
	}

	clMatrix& clMatrix::operator/=(float rhs) {
		(*this) = (*this) * (1.0f / rhs);
		return (*this);
	}

	clMatrix& clMatrix::operator/=(const clMatrix& rhs) {
		(*this) = (*this).Element_Div(rhs);
		return (*this);
	}

	clMatrix& clMatrix::operator/=(clMatrixExpr&& rhs) {
		(*this) = (*this).Element_Div(std::move(rhs));
		return (*this);
	}

	clMatrixExpr clMatrix::operator-() {
		return clMatrixExpr(*this, -1.0f);
	}

	void clMatrixExpr::operator+=(float rhs) {
		(*this) = (*this) + rhs;
	}
	void clMatrixExpr::operator+=(const clMatrix& rhs) {
		(*this) = (*this) + rhs;
	}
	void clMatrixExpr::operator+=(clMatrixExpr&& rhs) {
		(*this) = (*this) + std::move(rhs);
	}

	void clMatrixExpr::operator-=(float rhs) {
		(*this) = (*this) - rhs;
	}
	void clMatrixExpr::operator-=(const clMatrix& rhs) {
		(*this) = (*this) - rhs;
	}
	void clMatrixExpr::operator-=(clMatrixExpr&& rhs) {
		(*this) = (*this) - std::move(rhs);
	}

	void clMatrixExpr::operator*=(float rhs) {
		(*this) = (*this) * rhs;
	}
	void clMatrixExpr::operator*=(const clMatrix& rhs) {
		(*this) = (*this).Element_Multiply(rhs);
	}
	void clMatrixExpr::operator*=(clMatrixExpr&& rhs) {
		(*this) = (*this).Element_Multiply(std::move(rhs));
	}

	void clMatrixExpr::operator/=(float rhs) {
		(*this) = (*this)*(1.0f/rhs);
	}
	void clMatrixExpr::operator/=(const clMatrix& rhs) {
		(*this) = (*this).Element_Div(std::move(rhs));
	}
	void clMatrixExpr::operator/=(clMatrixExpr&& rhs) {
		(*this) = (*this).Element_Div(std::move(rhs));
	}

	clMatrixExpr clMatrixExpr::Element_Multiply(const clMatrix& rhs) {
		if (IsNull()) return clMatrixExpr(rhs);
		return clMatrixExpr(std::make_unique<clMatrixExpr>(std::move(*this), 1.0f), std::make_unique<clMatrixExpr>(rhs), 'm');
	}
	clMatrixExpr clMatrixExpr::Element_Multiply(clMatrixExpr&& rhs) {
		if (rhs.IsNull()) return std::move(*this);
		return clMatrixExpr(std::make_unique<clMatrixExpr>(std::move(*this), 1.0f), std::make_unique<clMatrixExpr>(std::move(rhs), 1.0f), 'm');
	}

	clMatrixExpr clMatrixExpr::Element_Div(const clMatrix& rhs) {
		if (IsNull()) return clMatrixExpr(rhs);
		return clMatrixExpr(std::make_unique<clMatrixExpr>(std::move(*this), 1.0f), std::make_unique<clMatrixExpr>(rhs), 'd');
	}

	clMatrixExpr clMatrixExpr::Element_Div(clMatrixExpr&& rhs) {
		if (rhs.IsNull()) return std::move(*this);
		return clMatrixExpr(std::make_unique<clMatrixExpr>(std::move(*this), 1.0f), std::make_unique<clMatrixExpr>(std::move(rhs), 1.0f), 'd');
	}

	clMatrixExpr clMatrix::Element_Multiply(const clMatrix& rhs) const {
		return clMatrixExpr(std::make_unique<clMatrixExpr>(*this), std::make_unique<clMatrixExpr>(rhs), 'm');
	}

	clMatrixExpr clMatrix::Element_Multiply(clMatrixExpr&& rhs) const {
		if (rhs.IsNull()) return std::move(*this);
		return clMatrixExpr(std::make_unique<clMatrixExpr>(*this), std::make_unique<clMatrixExpr>(std::move(rhs), 1.0f), 'm');
	}

	clMatrixExpr clMatrix::Element_Div(const clMatrix& rhs) const {
		return clMatrixExpr(std::make_unique<clMatrixExpr>(*this), std::make_unique<clMatrixExpr>(rhs), 'd');
	}

	clMatrixExpr clMatrix::Element_Div(clMatrixExpr&& rhs) const {
		if (rhs.IsNull()) return std::move(*this);
		return clMatrixExpr(std::make_unique<clMatrixExpr>(*this), std::make_unique<clMatrixExpr>(std::move(rhs), 1.0f), 'd');
	}

	clMatrixExpr clMatrix::Element_Multiply_log(const clMatrix& rhs) const {
		return clMatrixExpr(std::make_unique<clMatrixExpr>(*this), std::make_unique<clMatrixExpr>(std::move(rhs), 1.0f), K("mllg"));
	}

	float clMatrix::Norm2() const {
		float ret = 0;
		auto wgs = g.kNorm2->WorkGroupSize();
		int OutSize = (Range[0] * Range[1] + (g.SquareSide * g.SquareSide * 4) - 1) / (g.SquareSide * g.SquareSide * 4);
		auto& tmp = g.GetReadBuffer(OutSize * sizeof(float) * 64);

		int err1 = g.kNorm2->Run(cl::NDRange(OutSize * wgs), cl::NDRange(wgs), {
			tmp, Data, Rows, Cols, Stride, LocalBuffer(wgs * (4 * sizeof(float)))
		});
		std::vector<float> sums(OutSize);
		int err2 = tmp.ReadBlocking(&sums[0], OutSize * sizeof(float));
		for (auto s : sums) ret += s;
		return ret;
	}

	float clMatrix::Sum() const {
		float ret = 0;
		auto wgs = g.kSum->WorkGroupSize();
		int OutSize = (Range[0] * Range[1] + (g.SquareSide * g.SquareSide * 4) - 1) / (g.SquareSide * g.SquareSide * 4);
		auto& tmp = g.GetReadBuffer(OutSize * sizeof(float) * 64);

		int err1 = g.kSum->Run(cl::NDRange(OutSize * wgs), cl::NDRange(wgs), {
			tmp, Data, Rows, Cols, Stride, LocalBuffer(wgs * (4 * sizeof(float)))
		});
		std::vector<float> sums(OutSize);
		int err2 = tmp.ReadBlocking(&sums[0], OutSize * sizeof(float));
		for (auto s : sums) ret += s;
		return ret;
	}

	clMatrixValue clMatrix::operator()(size_t Row, size_t Col) {
		return clMatrixValue(Data(), Offset + Row + Col * Stride);
	}


	clMatrix clMatrix::GetReadTemp(int rows, int cols) {
		return clMatrix(g.GetReadBuffer(rows * cols), rows, cols, 0, rows);
	}

	clMatrix& clMatrix::Set(const clMatrix& rhs) {
		Data = rhs.Data;
		Cols = rhs.Cols;
		Rows = rhs.Rows;
		Offset = rhs.Offset;
		Stride = rhs.Stride;
		Range = rhs.Range;
		return *this;
	}

	clMatrix& clMatrix::SetData(const clMatrix& rhs) {
		assert(Rows==rhs.Rows && Cols == rhs.Cols);
		clblasCopyMatrix(clblasOrder::clblasColumnMajor,
			Rows, Cols, sizeof(float),
			rhs.Data()(), rhs.Offset, rhs.Stride,
			Data()(), Offset, Stride,
			*g.q, 0, nullptr);
		return *this;
	}

	clMatrix& clMatrix::CopyOf(const clMatrix& rhs, PVX::BufferAccess Access) {
		this->Cols = rhs.Cols;
		this->Rows = rhs.Rows;
		this->Offset = 0;
		this->Stride = g.matPadding(Rows);
		this->Range = cl::NDRange(Stride, g.matPadding(Cols));
		this->Data = g.cl.MakeBuffer(Range[0] * Range[1] * sizeof(float), Access);
		clblasCopyMatrix(clblasOrder::clblasColumnMajor,
			Rows, Cols, sizeof(float),
			rhs.Data()(), rhs.Offset, rhs.Stride,
			Data()(), 0, Stride,
			*g.q, 0, nullptr
		);
		return *this;
	}

	clMatrixExpr operator*(float f, clMatrixExpr&& expr) { return expr * f; }
	clMatrixExpr operator*(float f, const clMatrix& mat) { return mat * f; }
	clMatrixExpr operator+(float f, clMatrixExpr&& expr) { return expr + f; }
	clMatrixExpr operator+(float f, const clMatrix& mat) { return mat + f; }
	clMatrixExpr operator-(float f, clMatrixExpr&& expr) { return -(expr - f); }
	clMatrixExpr operator-(float f, const clMatrix& mat) { return -(mat - f); }

	void clMatrixExpr::Evaluate(clMatrix& target) {
		int PushCount = 0;
		int Result = 0;
		auto GetLHS = [&]()->const clMatrix& {
			if (Left->IsMatrix()&&!Left->Op) return Left->GetMatrix();
			PushCount++;
			auto& ret = g.Push(Left->Rows(), Left->Cols());
			Left->Evaluate(ret);
			return ret;
		};
		auto GetRHS = [&]()->const clMatrix& {
			if (Right->IsMatrix()&&!Right->Op) return Right->GetMatrix();
			PushCount++;
			auto& ret = g.Push(Right->Rows(), Right->Cols());
			Right->Evaluate(ret);
			return ret;
		};
		float RightAlpha = 1.0f;
		float RightBeta = 0;
		auto GetUnaryOperant = [&]()->const clMatrix& {
			if (IsMatrix()) {
				return *matrix;
			} else {
				transpose = Combine(transpose, Right->transpose);
				RightAlpha = Right->alpha;
				RightBeta = Right->beta;
				return GetRHS();
			}
		};
		auto RunUnary = [&](PVX::Kernel** ker) {
			//std::cout << ker[IsTransposed(transpose)]->kernel.getInfo<CL_KERNEL_FUNCTION_NAME>() << "\n";

			const auto& rhs = GetUnaryOperant();
			Result = ker[IsTransposed(transpose)]->Run(target.GetRange(), {
				target.Data, target.Offset, target.Stride, alpha, beta,
				rhs.Data, rhs.Offset, rhs.Stride, RightAlpha, RightBeta,
				target.Rows, target.Cols
			});
			alpha = 1.0f;
			beta = 0.0f;
			transpose = clblasTranspose::clblasNoTrans;
		};
		auto RunUnaryConst = [&](PVX::Kernel** ker) {
			//std::cout << ker[IsTransposed(transpose)]->kernel.getInfo<CL_KERNEL_FUNCTION_NAME>() << "\n";

			const auto& rhs = GetUnaryOperant();
			Result = ker[IsTransposed(transpose)]->Run(target.GetRange(), {
				target.Data, target.Offset, target.Stride, alpha,
				target.Rows, target.Cols,
				beta
			});
			alpha = 1.0f;
			beta = 0.0f;
			transpose = clblasTranspose::clblasNoTrans;
		};
		auto RunBinary = [&](PVX::Kernel** ker) {
			//std::cout << ker[((IsTransposed(Left->transpose) ^ IsTransposed(transpose))<<1)|(IsTransposed(Right->transpose) ^ IsTransposed(transpose))]->kernel.getInfo<CL_KERNEL_FUNCTION_NAME>() << "\n";

			const auto& lhs = GetLHS();
			const auto& rhs = GetRHS();

			Result = ker[((IsTransposed(Left->transpose) ^ IsTransposed(transpose))<<1)|(IsTransposed(Right->transpose) ^ IsTransposed(transpose))]->
				Run(target.GetRange(), {
				target.Data, target.Offset, target.Stride, alpha, beta,
				lhs.Data, lhs.Offset, lhs.Stride, Left->alpha, Left->beta,
				rhs.Data, rhs.Offset, rhs.Stride, Right->alpha, Right->beta,
				target.Rows, target.Cols
			});
			alpha = 1.0f;
			beta = 0.0f;
			transpose = clblasTranspose::clblasNoTrans;
		};

		switch (Op) {
			case 0: RunUnary(g.kFactorMat); break;
			case K("*"): {
				const auto& lhs = GetLHS();
				const auto& rhs = GetRHS();
				
				if (IsTransposed(transpose)) {
					Result = clblasSgemm(clblasOrder::clblasColumnMajor,
						NotTranspose(Right->transpose), NotTranspose(Left->transpose),
						Right->Cols(), Left->Rows(), Left->Cols(),
						Left->alpha * Right->alpha * alpha,
						rhs.Data()(), rhs.Offset, rhs.Stride,
						lhs.Data()(), lhs.Offset, lhs.Stride,
						0,
						target.Data()(), target.Offset, target.Stride,
						1, g.q, 0, nullptr, nullptr
					);
					transpose = clblasTranspose::clblasNoTrans;
				} else {
					Result = clblasSgemm(clblasOrder::clblasColumnMajor,
						Left->transpose, Right->transpose,
						Left->Rows(), Right->Cols(), Left->Cols(),
						Left->alpha * Right->alpha * alpha,
						lhs.Data()(), lhs.Offset, lhs.Stride,
						rhs.Data()(), rhs.Offset, rhs.Stride,
						0,
						target.Data()(), target.Offset, target.Stride,
						1, g.q, 0, nullptr, nullptr
					);
				}
				alpha = 1.0f;
				break;
			}
			case K("+"): RunBinary(g.kAddMatrix); break;
			case K("m"): RunBinary(g.kElemMul); break;
			case K("d"): RunBinary(g.kElemDiv); break;

			case K("ReLU"): RunUnary(g.kReLU); break;
			case K("tanh"): RunUnary(g.kTanh); break;
			case K("tnhb"): RunUnary(g.kTanhBias); break;
			case K("sgmd"): RunUnary(g.kSigmoid); break;

			case K("dtnh"): RunUnary(g.kdTanh); break;
			case K("dthb"): RunUnary(g.kdTanhBias); break;
			case K("dRLU"): RunUnary(g.kdReLU); break;
			case K("dsgm"): RunUnary(g.kdSigmoid); break;

			case K("mdRL"): RunBinary(g.kMul_dReLU); break;
			case K("mdth"): RunBinary(g.kMul_dTanh); break;
			case K("mdtb"): RunBinary(g.kMul_dTanhBias); break;
			case K("mdSd"): RunBinary(g.kMul_dSigmoid); break;

			case K("log"): RunUnary(g.kLog); break;
			case K("exp"): RunUnary(g.kExp); break;
			case K("sqrt"): RunUnary(g.kSqrt); break; 

			case K("mllg"): RunBinary(g.kMul_log); break;

		}if (PushCount) g.Pop(PushCount);
	}

	clMatrixExpr Mul_Element_dReLU(clMatrixExpr&& lhs, const clMatrix& rhs) {
		return clMatrixExpr(std::make_unique<clMatrixExpr>(std::move(lhs)), std::make_unique<clMatrixExpr>(rhs), K("mdRL"));
	}
	clMatrixExpr Mul_Element_dTanh(clMatrixExpr&& lhs, const clMatrix& rhs) {
		return clMatrixExpr(std::make_unique<clMatrixExpr>(std::move(lhs)), std::make_unique<clMatrixExpr>(rhs), K("mdth"));
	}
	clMatrixExpr Mul_Element_dTanhBias(clMatrixExpr&& lhs, const clMatrix& rhs) {
		return clMatrixExpr(std::make_unique<clMatrixExpr>(std::move(lhs)), std::make_unique<clMatrixExpr>(rhs), K("mdtb"));
	}
	clMatrixExpr Mul_Element_dSigmoid(clMatrixExpr&& lhs, const clMatrix& rhs) {
		return clMatrixExpr(std::make_unique<clMatrixExpr>(std::move(lhs)), std::make_unique<clMatrixExpr>(rhs), K("mdSd"));
	}
	clMatrixExpr Mul_Element_dLinear(clMatrixExpr&& lhs, const clMatrix& rhs) {
		return std::move(lhs);
	}

	static int M_Index;

	clMatrix::clMatrix(clMatrixExpr&& rhs) : clMatrix(rhs.Rows(), rhs.Cols()) {
		// std::cout << rhs.DebugExpressionPrint() << "\n";
		rhs.Evaluate(*this);
	}
	clMatrixExpr clMatrix::operator=(clMatrixExpr&& rhs) {
		// std::cout << rhs.DebugExpressionPrint() << "\n";
		if (rhs.Cols()!=cols()||rhs.Rows()!=rows()) 
			Reset(rhs.Rows(), rhs.Cols());
		rhs.Evaluate(*this);
		return *this;
	}
	clMatrix::clMatrix(int rows, int cols, const float rhs) : clMatrix(rows, cols) {
		g.kSetConst->Run(this->GetRange(), {
			Data, Offset, Stride, 1.0f,
			Rows, Cols,
			rhs
		});
	}
	clMatrix& PVX::NeuralNets::clMatrix::operator=(float rhs) {
		g.kSetConst->Run(this->GetRange(), {
			Data, Offset, Stride, 1.0f,
			Rows, Cols,
			rhs
		});
		return *this;
	}
	clMatrixExpr  ReLU(const clMatrix& mat) { return clMatrixExpr(mat, 1.0f, false, K("ReLU")); }
	clMatrixExpr  ReLU(clMatrixExpr&& mat) { return clMatrixExpr(K("ReLU"), std::move(mat)); }
	clMatrixExpr dReLU(const clMatrix& mat) { return clMatrixExpr(mat, 1.0f, false, K("dRLU")); }
	clMatrixExpr dReLU(clMatrixExpr&& mat) { return clMatrixExpr(K("dRLU"), std::move(mat)); }

	clMatrixExpr  Tanh(const clMatrix& mat) { return clMatrixExpr(mat, 1.0f, false, K("tanh")); }
	clMatrixExpr  Tanh(clMatrixExpr&& mat) { return clMatrixExpr(K("tanh"), std::move(mat)); }
	clMatrixExpr dTanh(const clMatrix& mat) { return clMatrixExpr(mat, 1.0f, false, K("dtnh")); }
	clMatrixExpr dTanh(clMatrixExpr&& mat) { return clMatrixExpr(K("dtnh"), std::move(mat)); }

	clMatrixExpr  TanhBias(const clMatrix& mat) { return clMatrixExpr(mat, 1.0f, false, K("tnhb")); }
	clMatrixExpr  TanhBias(clMatrixExpr&& mat) { return clMatrixExpr(K("tnhb"), std::move(mat)); }
	clMatrixExpr dTanhBias(const clMatrix& mat) { return clMatrixExpr(mat, 1.0f, false, K("dthb")); }
	clMatrixExpr dTanhBias(clMatrixExpr&& mat) { return clMatrixExpr(K("dthb"), std::move(mat)); }

	clMatrixExpr  Sigmoid(const clMatrix& mat) { return clMatrixExpr(mat, 1.0f, false, K("sgmd")); }
	clMatrixExpr  Sigmoid(clMatrixExpr&& mat) { return clMatrixExpr(K("sgmd"), std::move(mat)); }
	clMatrixExpr dSigmoid(const clMatrix& mat) { return clMatrixExpr(mat, 1.0f, false, K("dsgm")); }
	clMatrixExpr dSigmoid(clMatrixExpr&& mat) { return clMatrixExpr(K("dsgm"), std::move(mat)); }

	clMatrixExpr Linear(const clMatrix& mat) { return mat; }
	clMatrixExpr Linear(clMatrixExpr&& mat) { return std::move(mat); }
	clMatrixExpr dLinear(const clMatrix& mat) { return clMatrixExpr(); }
	clMatrixExpr dLinear(clMatrixExpr&& mat) { return clMatrixExpr(); }

	clMatrixExpr log(const clMatrix& mat) { return clMatrixExpr(mat, 1.0f, false, K("log")); }
	clMatrixExpr log(clMatrixExpr&& mat) { return clMatrixExpr(K("log"), std::move(mat)); }

	clMatrixExpr exp(const clMatrix& mat) { return clMatrixExpr(mat, 1.0f, false, K("exp")); }
	clMatrixExpr exp(clMatrixExpr&& mat) { return clMatrixExpr(K("exp"), std::move(mat)); }

	clMatrixExpr sqrt(const clMatrix& mat) { return clMatrixExpr(mat, 1.0f, false, K("sqrt")); }
	clMatrixExpr sqrt(clMatrixExpr&& mat) { return clMatrixExpr(K("sqrt"), std::move(mat)); }




	float clMatrixValue::operator=(const float val) {
		auto dbg = g.cl.Queue().enqueueWriteBuffer(Buffer, false, Offset * sizeof(float), sizeof(float), (void*)&val);
		return val;
	}
	clMatrixValue::operator float() {
		float val;
		g.cl.Queue().enqueueReadBuffer(Buffer, true, Offset * sizeof(float), sizeof(float), (void*)&val);
		return val;
	}

	clMatrix& clMatrix::Temp(clMatrixExpr&& expr) {
		clMatrix& ret = g.Push(expr.Rows(), expr.Cols());
		ret = std::move(expr);
		return ret;
	}
	clMatrix& clMatrix::Temp(int rows, int cols) {
		return g.Push(rows, cols);
	}
	clMatrix& clMatrix::Temp(int rows, int cols, const std::vector<float>& Data) {
		return g.Push(rows, cols, Data);
	}
	clMatrix& clMatrix::Temp(int rows, int cols, const float* Data) {
		return g.Push(rows, cols, Data);
	}

	std::string clMatrixExpr::DebugExpressionPrint() {
		std::string ret = "";
		switch (Op) {
			case 0: ret = "M" + std::to_string(M_Index++); break;
			case K("*"): ret = Left->DebugExpressionPrint() + "*" + Right->DebugExpressionPrint();  break;
			case K("+"): ret = Left->DebugExpressionPrint() + "+" + Right->DebugExpressionPrint(); break;
			case K("m"): ret = Left->DebugExpressionPrint() + ".mul(" + Right->DebugExpressionPrint() + ")"; break;
			case K("d"): ret = Left->DebugExpressionPrint() + ".div(" + Right->DebugExpressionPrint() + ")"; break;
			case K("ReLU"): ret = "ReLU(" + (IsMatrix()?("M"+std::to_string(M_Index++)):Right->DebugExpressionPrint()) + ")"; break;
			case K("dRLU"): ret = "ReLU'(" + (IsMatrix() ? ("M"+std::to_string(M_Index++)) : Right->DebugExpressionPrint()) + ")"; break;
			case K("tanh"): ret = "tanh(" + (IsMatrix() ? ("M"+std::to_string(M_Index++)) : Right->DebugExpressionPrint()) + ")"; break;
			case K("dtnh"): ret = "tanh'(" + (IsMatrix() ? ("M"+std::to_string(M_Index++)) : Right->DebugExpressionPrint()) + ")"; break;
			case K("tnhb"): ret = "tanh_Bias(" + (IsMatrix() ? ("M"+std::to_string(M_Index++)) : Right->DebugExpressionPrint()) + ")"; break;
			case K("dthb"): ret = "tanh_Bias'(" + (IsMatrix() ? ("M"+std::to_string(M_Index++)) : Right->DebugExpressionPrint()) + ")"; break;
			case K("sgmd"): ret = "Sigmoid(" + (IsMatrix() ? ("M"+std::to_string(M_Index++)) : Right->DebugExpressionPrint()) + ")"; break;
			case K("dsgm"): ret = "Sigmoid'(" + (IsMatrix() ? ("M"+std::to_string(M_Index++)) : Right->DebugExpressionPrint()) + ")"; break;
			case K("log"): ret = "log(" + (IsMatrix() ? ("M"+std::to_string(M_Index++)) : Right->DebugExpressionPrint()) + ")"; break;
			case K("exp"): ret = "exp(" + (IsMatrix() ? ("M"+std::to_string(M_Index++)) : Right->DebugExpressionPrint()) + ")"; break;
			case K("sqrt"): ret = "sqrt(" + (IsMatrix() ? ("M"+std::to_string(M_Index++)) : Right->DebugExpressionPrint()) + ")"; break;
		}
		if (alpha==1.0f && beta == 0) return ret + (transpose ? ".tran()" : "");
		return "(" + (alpha!=1.0f ? (std::to_string(alpha) + "*") : "") + ret + (transpose?".tran()":"")  + (beta ? ("+" + std::to_string(beta) +")") : ")");
	}
}
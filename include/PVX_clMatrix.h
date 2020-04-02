#pragma once
#include <PVX_OpenCL.h>
#include <clBLAS.h>


namespace PVX::NeuralNets {
	class clMatrix;
	
	class clMatrixExpr {
		int _Rows, _Cols;
		std::unique_ptr<clMatrixExpr> Left, Right;
		const clMatrix* matrix = nullptr;
		int Op = 0;
		float alpha = 1.0f;
		float beta = 0.0f;
		clblasTranspose transpose = clblasTranspose::clblasNoTrans;

		friend class clMatrix;
		inline bool IsNull() { return !_Rows; };

		std::string DebugExpressionPrint();
	public:
		inline clMatrixExpr() : _Rows{ 0 }, _Cols{ 0 } {};
		clMatrixExpr(clMatrixExpr&& Right, float alpha, bool transposed = false, int Op = 0);
		clMatrixExpr(std::unique_ptr<clMatrixExpr> Left, std::unique_ptr<clMatrixExpr> Right, int Op);
		clMatrixExpr(const clMatrix& rhs, float alpha = 1.0f, bool transposed = false, int Op = 0);
		clMatrixExpr(int Op, clMatrixExpr&& Param);

		int Cols() { return clblasTranspose::clblasNoTrans==transpose ? _Cols : _Rows; }
		int Rows(){ return clblasTranspose::clblasNoTrans==transpose ? _Rows : _Cols; }
		inline bool IsMatrix() const { return matrix!=nullptr; };
		inline const clMatrix& GetMatrix() { return *matrix; };
		inline int& GetOp() { return Op; };

		clMatrixExpr Transpose();

		clMatrixExpr operator*(const clMatrix& rhs);
		clMatrixExpr operator+(const clMatrix& rhs);
		clMatrixExpr operator-(const clMatrix& rhs);

		clMatrixExpr operator*(clMatrixExpr&& rhs);
		clMatrixExpr operator+(clMatrixExpr&& rhs);
		clMatrixExpr operator-(clMatrixExpr&& rhs);

		clMatrixExpr operator*(float rhs);
		clMatrixExpr operator/(float rhs);

		clMatrixExpr operator+(float rhs);
		clMatrixExpr operator-(float rhs);

		clMatrixExpr operator-();

		void operator+=(const clMatrix& rhs);
		void operator+=(clMatrixExpr&& rhs);
		void operator+=(float rhs);

		void operator-=(const clMatrix& rhs);
		void operator-=(clMatrixExpr&& rhs);
		void operator-=(float rhs);

		void operator*=(float rhs);
		void operator*=(const clMatrix& rhs);
		void operator*=(clMatrixExpr&& rhs);

		void operator/=(float rhs);
		void operator/=(const clMatrix& rhs);
		void operator/=(clMatrixExpr&& rhs);

		clMatrixExpr Element_Multiply(const clMatrix& rhs);
		clMatrixExpr Element_Multiply(clMatrixExpr&& rhs);
		clMatrixExpr Element_Div(const clMatrix& rhs);
		clMatrixExpr Element_Div(clMatrixExpr&& rhs);

		void Evaluate(clMatrix& target);
	};

	class clMatrixValue {
		clMatrixValue(const clMatrixValue&) = delete;
		clMatrixValue(const cl::Buffer& buf, size_t Offset) :Buffer{ buf }, Offset{ Offset }{}
		cl::Buffer Buffer;
		size_t Offset;
		friend class clMatrix;
	public:
		float operator=(const float val);
		operator float();
	};

	class clMatrix {
		cl::NDRange Range;
		PVX::Buffer Data;
		int Rows, Cols;
		int Offset;
		int Stride;
		bool IsTemp = false;

		friend class clMatrixExpr;
		clMatrix(const PVX::Buffer& dt, int Rows, int Cols, int Offset, int Stride);
	public:

		clMatrix();
		~clMatrix();
		clMatrix(clMatrixExpr&& rhs);
		clMatrix(int rows, int cols, const float rhs);
		clMatrix(int rows, int cols, BufferAccess Access = BufferAccess::GPU_RW_CPU_00);
		clMatrix(int rows, int cols, const float* Data, BufferAccess Access = BufferAccess::GPU_RW_CPU_00);
		clMatrix(int rows, int cols, const std::vector<float>& Data, BufferAccess Access = BufferAccess::GPU_RW_CPU_00);

		clMatrix& SetTemp();

		inline int rows() const { return Rows; }
		inline int cols() const { return Cols; }
		inline int size() const { return Cols*Rows; }
		clMatrix block(int row, int col, int rowCount, int colCount) const;
		clMatrix row(int Index) const;
		clMatrix col(int Index) const;
		clMatrix LastRow() const;
		clMatrix& Reset(int x, int y);
		clMatrixExpr operator=(const clMatrix& rhs) = delete;
		clMatrixExpr operator=(clMatrixExpr&& rhs);
		clMatrix& operator=(float rhs);

		clMatrixExpr Transpose() const;
		
		clMatrixExpr operator*(const clMatrix& rhs) const;
		clMatrixExpr operator+(const clMatrix& rhs) const;
		clMatrixExpr operator-(const clMatrix& rhs) const;

		clMatrixExpr operator*(clMatrixExpr&& rhs) const;
		clMatrixExpr operator+(clMatrixExpr&& rhs) const;
		clMatrixExpr operator-(clMatrixExpr&& rhs) const;

		clMatrixExpr operator*(float rhs) const;
		clMatrixExpr operator/(float rhs) const;

		clMatrixExpr operator+(float rhs) const;
		clMatrixExpr operator-(float rhs) const;

		clMatrix& operator+=(const clMatrix& rhs);
		clMatrix& operator+=(clMatrixExpr&& rhs);
		clMatrix& operator+=(float rhs);

		clMatrix& operator-=(const clMatrix& rhs);
		clMatrix& operator-=(clMatrixExpr&& rhs);
		clMatrix& operator-=(float rhs);

		clMatrix& operator*=(float rhs);
		clMatrix& operator*=(const clMatrix& rhs);
		clMatrix& operator*=(clMatrixExpr&& rhs);

		clMatrix& operator/=(float rhs);
		clMatrix& operator/=(const clMatrix& rhs);
		clMatrix& operator/=(clMatrixExpr&& rhs);

		PVX::Buffer operator()() const { return Data; };

		clMatrixExpr operator-();

		clMatrixExpr Element_Multiply(const clMatrix& rhs) const;
		clMatrixExpr Element_Multiply(clMatrixExpr&& rhs) const;
		clMatrixExpr Element_Div(const clMatrix& rhs) const;
		clMatrixExpr Element_Div(clMatrixExpr&& rhs) const;

		clMatrixExpr Element_Multiply_log(const clMatrix& rhs) const;

		clMatrix& Div_diag(const clMatrix& mat, const clMatrix& diag);
		clMatrix& Mul_diag(const clMatrix& mat, const clMatrix& diag);

		float Norm2() const;
		float Sum() const;

		clMatrixValue operator()(size_t Row, size_t Col);


		static clMatrix Row(int size, BufferAccess Access = BufferAccess::GPU_RW_CPU_00);
		static clMatrix Col(int size, BufferAccess Access = BufferAccess::GPU_RW_CPU_00);

		static clMatrix Random(int Rows, int Cols, float min = 0.0f, float max = 1.0f);
		//static clMatrixExpr Ones(int Rows, int Cols);

		std::vector<float> Read() const;
		void Read(float * Data) const;
		std::vector<float> ReadBlocking() const;
		void ReadBlocking(float* Data) const;
		void Write(const float* Data);
		void Write(const std::vector<float>& Data);
		const cl::NDRange& GetRange() const { return Range; };

		static clMatrix GetReadTemp(int rows, int cols);
		static clMatrix& Temp(clMatrixExpr&& expr);
		static clMatrix& Temp(int rows, int cols);
		static clMatrix& Temp(int rows, int cols, const std::vector<float>& Data);
		static clMatrix& Temp(int rows, int cols, const float* Data);

		static void divDiag(clMatrix& out, const clMatrix& mat, const clMatrix& diag);
		static void mulDiag(clMatrix& out, const clMatrix& mat, const clMatrix& diag);

		clMatrix& Set(const clMatrix& rhs);
		clMatrix& SetData(const clMatrix& rhs);
		clMatrix& CopyOf(const clMatrix& rhs, PVX::BufferAccess Access = PVX::BufferAccess::GPU_RW_CPU_00);
	};

	clMatrixExpr operator*(float f, clMatrixExpr&& expr);
	clMatrixExpr operator*(float f, const clMatrix& expr);
	clMatrixExpr operator+(float f, clMatrixExpr&& expr);
	clMatrixExpr operator+(float f, const clMatrix& expr);
	clMatrixExpr operator-(float f, clMatrixExpr&& expr);
	clMatrixExpr operator-(float f, const clMatrix& expr);

	clMatrixExpr ReLU(const clMatrix& mat);
	clMatrixExpr ReLU(clMatrixExpr&& mat);
	clMatrixExpr dReLU(const clMatrix& mat);
	clMatrixExpr dReLU(clMatrixExpr&& mat);

	clMatrixExpr Tanh(const clMatrix& mat);
	clMatrixExpr Tanh(clMatrixExpr&& mat);
	clMatrixExpr dTanh(const clMatrix& mat);
	clMatrixExpr dTanh(clMatrixExpr&& mat);

	clMatrixExpr TanhBias(const clMatrix& mat);
	clMatrixExpr TanhBias(clMatrixExpr&& mat);
	clMatrixExpr dTanhBias(const clMatrix& mat);
	clMatrixExpr dTanhBias(clMatrixExpr&& mat);

	clMatrixExpr Sigmoid(const clMatrix& mat);
	clMatrixExpr Sigmoid(clMatrixExpr&& mat);
	clMatrixExpr dSigmoid(const clMatrix& mat);
	clMatrixExpr dSigmoid(clMatrixExpr&& mat);

	clMatrixExpr Linear(const clMatrix& mat);
	clMatrixExpr Linear(clMatrixExpr&& mat);
	clMatrixExpr dLinear(const clMatrix& mat);
	clMatrixExpr dLinear(clMatrixExpr&& mat);

	clMatrixExpr log(const clMatrix& mat);
	clMatrixExpr log(clMatrixExpr&& mat);

	clMatrixExpr exp(const clMatrix& mat);
	clMatrixExpr exp(clMatrixExpr&& mat);

	clMatrixExpr sqrt(const clMatrix& mat);
	clMatrixExpr sqrt(clMatrixExpr&& mat);

	clMatrixExpr Mul_Element_dReLU(clMatrixExpr&& lhs, const clMatrix& rhs);
	clMatrixExpr Mul_Element_dTanh(clMatrixExpr&& lhs, const clMatrix& rhs);
	clMatrixExpr Mul_Element_dTanhBias(clMatrixExpr&& lhs, const clMatrix& rhs);
	clMatrixExpr Mul_Element_dSigmoid(clMatrixExpr&& lhs, const clMatrix& rhs);
	clMatrixExpr Mul_Element_dLinear(clMatrixExpr&& lhs, const clMatrix& rhs);

	void Flush();
	void Finish();
}
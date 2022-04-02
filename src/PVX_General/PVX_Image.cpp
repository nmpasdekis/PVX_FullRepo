#define NOMINMAX

#include <PVX_Image.h>
#include <map>
#include <PVX_File.h>

namespace STB_Linear {
#include <stb/stb_image.h>
#include <stb/stb_image_write.h>
}

//#undef STBI_INCLUDE_STB_IMAGE_H
//
//namespace NonLinear {
//#include <stb/stb_image.h>
//}

#include <PVX_String.h>
#include <Eigen/dense>

#include <mutex>


#define __FLIP_V__ 0

namespace PVX {
	//ImageF::ImageF(const Eigen::MatrixXf& mat) : Width{ int(mat.rows()) }, Height{ int(mat.cols()) }, Pixels(mat.size()) {
	//	memcpy(Pixels.data(), mat.data(), sizeof(float)* mat.size());
	//}

	//Eigen::MatrixXf ImageF::Matrix() {
	//	return MatrixMap();
	//}
	//Eigen::Map<Eigen::MatrixXf> ImageF::MatrixMap() const {
	//	return Eigen::Map<Eigen::MatrixXf>((float*)Pixels.data(), Width, Height);
	//}

	ImageF::ImageF() :Width{ 0 }, Height{ 0 } {};
	ImageF::ImageF(int Width, int Height) :Width{ Width }, Height{ Height }, Pixels(size_t(Width)* size_t(Height)) {}
	ImageF::ImageF(const Image3F & img) {
		Width = img.Width;
		Height = img.Height;
		Pixels.resize(size_t(Width) * Height);
		for (auto i = 0; i < Pixels.size(); i++) {
			auto & p = img.Pixels[i];
			Pixels[i] = p.r * 0.2989f + p.g * 0.5870f + p.b * 0.1140f;
		}
	}

	ImageF::ImageF(const Image4F & img) {
		Width = img.Width;
		Height = img.Height;
		Pixels.resize(size_t(Width) * Height);
		for (auto i = 0; i < Pixels.size(); i++) {
			auto & p = img.Pixels[i];
			Pixels[i] = p.r * 0.2989f + p.g * 0.5870f + p.b * 0.1140f;
		}
	}

	ImageF ImageF::Load(const char * Filename) {
		STB_Linear::stbi_set_flip_vertically_on_load(__FLIP_V__);
		int w, h, c;
		auto vec = STB_Linear::stbi_loadf(Filename, &w, &h, &c, 1);
		ImageF ret(w, h);
		memcpy(ret.Pixels.data(), vec, sizeof(float) * ret.Pixels.size());
		STB_Linear::stbi_image_free(vec);
		return ret;
	}

	void ImageF::SetSize(int w, int h) {
		Width = w;
		Height = h;
		Pixels.resize(size_t(w) * h);
	}

	void ImageF::SetPixels(const void * Data) {
		memcpy(&Pixels[0], Data, Pixels.size() * sizeof(float));
	}

	void ImageF::SetPixels(int Width, int Height, const void * Data) {
		SetSize(Width, Height);
		SetPixels(Data);
	}

	float & ImageF::operator()(int x, int y) {
		return Pixels[x + y * size_t(Width)];
	}

	const float & ImageF::operator()(int x, int y) const {
		return Pixels[x + y * size_t(Width)];
	}

	void Image3F::SetSize(int w, int h) {
		Width = w;
		Height = h;
		Pixels.resize(size_t(w) * h);
	}

	void Image3F::SetPixels(const void * Data) {
		memcpy(&Pixels[0], Data, Pixels.size() * sizeof(float) * 3);
	}

	void Image3F::SetPixels(int Width, int Height, const void * Data) {
		SetSize(Width, Height);
		SetPixels(Data);
	}

	PVX::Vector3D & Image3F::operator()(int x, int y) {
		return Pixels[size_t(x) + size_t(y) * Width];
	}

	std::vector<unsigned char> Image3F::JPEG() const {
		std::vector<unsigned char> ret;
		return ret;
	}

	std::vector<unsigned char> Image3F::TGA() const {
		std::vector<unsigned char> ret;
		return ret;
	}

	Image3F Image3F::FromRGB(const ImageF & Red, const ImageF & Green, const ImageF & Blue) {
		Image3F ret(Red.Width, Red.Height);
		const float * r = Red.Pixels.data();
		const float * g = Green.Pixels.data();
		const float * b = Blue.Pixels.data();
		for (auto i = 0; i < ret.Pixels.size();i++) {
			auto & p = ret.Pixels[i];
			p.r = r[i];
			p.g = g[i];
			p.b = b[i];
		}
		return ret;
	}

	std::vector<unsigned char> Image3F::PNG() const {
		std::vector<unsigned char> ret;
		return ret;
	}

	void Image4F::SetSize(int w, int h) {
		Width = w;
		Height = h;
		Pixels.resize(size_t(w) * h);
	}

	void Image4F::SetPixels(const void * Data) {
		memcpy(&Pixels[0], Data, Pixels.size() * sizeof(float) * 4);
	}

	void Image4F::SetPixels(int Width, int Height, const void * Data) {
		SetSize(Width, Height);
		SetPixels(Data);
	}

	PVX::Vector4D & Image4F::operator()(int x, int y) {
		return Pixels[size_t(x) + size_t(y) * Width];
	}

	std::vector<unsigned char> ImageF::JPEG() const {
		std::vector<unsigned char> ret;
		return ret;
	}

	std::vector<unsigned char> ImageF::PNG() const {
		std::vector<unsigned char> ret;
		return ret;
	}

	std::vector<unsigned char> ImageF::TGA() const {
		std::vector<unsigned char> ret;
		return ret;
	}

	void ImageF::Region(float * Out, int x, int y, int w, int h) const {
		const float * c = &Pixels[size_t(x) + size_t(y) * Width];
		for (int i = 0; i < h; i++) {
			memcpy(Out, c, sizeof(float) * w);
			Out += w;
			c += Width;
		}
	}

	void ImageF::Save(const char * Filename) {

	}

	std::vector<unsigned char> Image4F::JPEG() const {
		std::vector<unsigned char> ret;
		return ret;
	}

	std::vector<unsigned char> Image4F::TGA() const {
		std::vector<unsigned char> ret;
		return ret;
	}

	std::vector<unsigned char> Image4F::PNG() const {
		std::vector<unsigned char> ret;
		return ret;
	}

	Image3F::Image3F(int Width, int Height) :Pixels{ size_t(Width) * Height }, Width{ Width }, Height{ Height } {}
	Image4F::Image4F(int Width, int Height) : Pixels(size_t(Width) * Height), Width{ Width }, Height{ Height } {}

	Image3F::Image3F(const ImageF & img) : Image3F(img.Width, img.Height) {
		for (auto i = 0; i < Pixels.size(); i++) {
			auto & p = img.Pixels[i];
			Pixels[i] = { p, p, p };
		}
	}

	Image3F::Image3F(const Image4F & img) : Image3F(img.Width, img.Height) {
		for (auto i = 0; i < Pixels.size(); i++) {
			auto & p = img.Pixels[i];
			Pixels[i] = { p.r, p.g, p.b };
		}
	}

	//Image3F Image3F::Load(const char* Filename) {
	//	int w, h, c;
	//	PVX::ucVector3D* vec2 = (PVX::ucVector3D*)stbi_load(Filename, &w, &h, &c, 3);
	//	PVX::Vector3D* vec = (PVX::Vector3D*)stbi_loadf(Filename, &w, &h, &c, 3);
	//	constexpr double inv = 1.0f / 255.0f;
	//	PVX::Vector3D sum{ 0 };
	//	for (int i = 0; i<w*c; i++) {
	//		PVX::Vector3D tmp = { vec2[i].r * inv, vec2[i].g * inv, vec2[i].b * inv };
	//		sum += tmp / vec[i];
	//	}
	//	sum /= w*h;

	//	Image3F ret(w, h);
	//	memcpy(&ret.Pixels[0], vec, sizeof(float)*size_t(w)*size_t(h)*3);
	//	stbi_image_free(vec);
	//	stbi_image_free(vec2);
	//	return ret;
	//}
	Image3F Image3F::Load(const char * Filename) {
		STB_Linear::stbi_set_flip_vertically_on_load(__FLIP_V__);
		int w, h, c;
		constexpr double inv = 1.0f / 255.0f;
		auto vec = STB_Linear::stbi_load(Filename, &w, &h, &c, 3);
		Image3F ret(w, h);

		float* dst = (float*)&ret.Pixels[0];

		size_t sz = size_t(w)*size_t(h)*3;
		for (size_t i = 0; i<sz; i++) {
			dst[i] = float(vec[i] * inv);
		}
		STB_Linear::stbi_image_free(vec);
		return ret;
	}
	Image3F Image3F::Load(const wchar_t* Filename) {
		STB_Linear::stbi_set_flip_vertically_on_load(__FLIP_V__);
		int w, h, c;
		FILE* fin;
		_wfopen_s(&fin, Filename, L"rb");
		if (fin) {
			auto vec = STB_Linear::stbi_load_from_file(fin, &w, &h, &c, 3);
			fclose(fin);
			Image3F ret(w, h);
			constexpr double inv = 1.0f / 255.0f;
			float* dst = (float*)&ret.Pixels[0];

			size_t sz = size_t(w)*size_t(h)*3;
			for (size_t i = 0; i<sz; i++) {
				dst[i] = float(vec[i] * inv);
			}
			STB_Linear::stbi_image_free(vec);
			return ret;
		}
		return {};
	}
	/* std::tuple<Width, Height, Channels, BitsPerChannel, IsHDR> */
	std::tuple<int, int, int, int, bool> ImageInfo(FILE* f) {
		int w = 0, h = 0, c = 0, BitsPerChannel = 0;
		bool IsHDR = false;;
		if (STB_Linear::stbi_info_from_file(f, &w, &h, &c)) {
			BitsPerChannel = STB_Linear::stbi_is_16_bit_from_file(f) ? 16 : 8;
			IsHDR = STB_Linear::stbi_is_hdr_from_file(f);
		}
		return { w, h, c, BitsPerChannel, IsHDR };
	}

	/* std::tuple<Width, Height, Channels, BitsPerChannel, IsHDR> */
	std::tuple<int, int, int, int, bool> ImageInfo(const char* Filename) {
		FILE* fin;
		fopen_s(&fin, Filename, "rb");
		if (fin) {
			auto ret = ImageInfo(fin);
			fclose(fin);
			return ret;
		}
		return { 0, 0, 0, 0, false };
	}

	/* std::tuple<Width, Height, Channels, BitsPerChannel, IsHDR> */
	std::tuple<int, int, int, int, bool> ImageInfo(const wchar_t* Filename) {
		FILE* fin;
		_wfopen_s(&fin, Filename, L"rb");
		if (fin) {
			auto ret = ImageInfo(fin);
			fclose(fin);
			return ret;
		}
		return { 0, 0, 0, 0, false };
	}

	std::tuple<ImageF, ImageF, ImageF> Image3F::SplitRGB() {
		return { Red(), Green(), Blue() };
	}


	Image3F Image3F::JPEG(const unsigned char * Data, size_t Size) {
		return Image3F();
	}

	Image4F::Image4F(const ImageF & img) {
		Width = img.Width;
		Height = img.Height;
		Pixels.resize(size_t(Width) * size_t(Height));
		for (auto i = 0; i < Pixels.size(); i++) {
			auto & p = img.Pixels[i];
			Pixels[i] = { p, p, p, 1.0f };
		}
	}

	Image4F::Image4F(const Image3F & img) {
		Width = img.Width;
		Height = img.Height;
		Pixels.resize(size_t(Width) * size_t(Height));
		for (auto i = 0; i < Pixels.size(); i++) {
			auto & p = img.Pixels[i];
			Pixels[i] = { p.r, p.g, p.b, 1.0f };
		}
	}

	Image4F Image4F::Load(const char * Filename) {
		STB_Linear::stbi_set_flip_vertically_on_load(__FLIP_V__);
		int w, h, c;
		auto vec = STB_Linear::stbi_load(Filename, &w, &h, &c, 4);
		Image4F ret(w, h);
		constexpr double inv = 1.0f / 255.0f;
		float* dst = (float*)&ret.Pixels[0];
		size_t sz = size_t(w)*size_t(h)*4;
		for (size_t i = 0; i<sz; i++) {
			dst[i] = float(vec[i] * inv);
		}
		STB_Linear::stbi_image_free(vec);
		return ret;
	}

	Image4F Image4F::Load(const wchar_t * Filename) {
		STB_Linear::stbi_set_flip_vertically_on_load(__FLIP_V__);
		int w, h, c;
		FILE* fin;
		_wfopen_s(&fin, Filename, L"rb");
		if (fin) {
			auto vec = STB_Linear::stbi_load_from_file(fin, &w, &h, &c, 3);
			fclose(fin);
			Image4F ret(w, h);
			constexpr double inv = 1.0f / 255.0f;
			float* dst = (float*)&ret.Pixels[0];
			size_t sz = size_t(w)*size_t(h)*4;
			for (size_t i = 0; i<sz; i++)
				dst[i] = float(vec[i] * inv);
			
			STB_Linear::stbi_image_free(vec);
			return ret;
		}
		return {};
	}

	ImageF Image3F::Red() {
		ImageF ret;
		ret.Width = Width;
		ret.Height = Height;
		ret.Pixels.resize(Pixels.size());
		const float * In = &Pixels[0].r;
		float * Out = &ret.Pixels[0];
		for (auto i = 0, j = 0; i < Pixels.size(); i++, j += 3)
			Out[i] = In[j];
		return ret;
	}
	ImageF Image3F::Green() {
		ImageF ret;
		ret.Width = Width;
		ret.Height = Height;
		ret.Pixels.resize(Pixels.size());
		const float * In = &Pixels[0].g;
		float * Out = &ret.Pixels[0];
		for (auto i = 0, j = 0; i < Pixels.size(); i++, j += 3)
			Out[i] = In[j];
		return ret;
	}
	ImageF Image3F::Blue() {
		ImageF ret;
		ret.Width = Width;
		ret.Height = Height;
		ret.Pixels.resize(Pixels.size());
		const float * In = &Pixels[0].b;
		float * Out = &ret.Pixels[0];
		for (auto i = 0, j = 0; i < Pixels.size(); i++, j += 3)
			Out[i] = In[j];
		return ret;
	}

	ImageF Image4F::Red() {
		ImageF ret;
		ret.Width = Width;
		ret.Height = Height;
		ret.Pixels.resize(Pixels.size());
		const float * In = &Pixels[0].r;
		float * Out = &ret.Pixels[0];
		for (auto i = 0, j = 0; i < Pixels.size(); i++, j += 4)
			Out[i] = In[j];
		return ret;
	}
	ImageF Image4F::Green() {
		ImageF ret;
		ret.Width = Width;
		ret.Height = Height;
		ret.Pixels.resize(Pixels.size());
		const float * In = &Pixels[0].g;
		float * Out = &ret.Pixels[0];
		for (auto i = 0, j = 0; i < Pixels.size(); i++, j += 4)
			Out[i] = In[j];
		return ret;
	}
	ImageF Image4F::Blue() {
		ImageF ret;
		ret.Width = Width;
		ret.Height = Height;
		ret.Pixels.resize(Pixels.size());
		const float * In = &Pixels[0].b;
		float * Out = &ret.Pixels[0];
		for (auto i = 0, j = 0; i < Pixels.size(); i++, j += 4)
			Out[i] = In[j];
		return ret;
	}
	ImageF Image4F::Alpha() {
		ImageF ret;
		ret.Width = Width;
		ret.Height = Height;
		ret.Pixels.resize(Pixels.size());
		const float * In = &Pixels[0].a;
		float * Out = &ret.Pixels[0];
		for (auto i = 0, j = 0; i < Pixels.size(); i++, j += 4)
			Out[i] = In[j];
		return ret;
	}

	ImageF Image3F::Grey() {
		ImageF ret;
		ret.Width = Width;
		ret.Height = Height;
		ret.Pixels.resize(Pixels.size());
		const PVX::Vector3D * In = &Pixels[0];
		float * Out = &ret.Pixels[0];
		for (auto i = 0, j = 0; i < Pixels.size(); i++, j += 4)
			Out[i] = (0.299f * In[i].r + 0.587f * In[i].g + 0.114f * In[i].b);
		return ret;
	}

	ImageF Image4F::Grey() {
		ImageF ret;
		ret.Width = Width;
		ret.Height = Height;
		ret.Pixels.resize(Pixels.size());
		const PVX::Vector4D * In = &Pixels[0];
		float * Out = &ret.Pixels[0];
		for (auto i = 0, j = 0; i < Pixels.size(); i++, j += 4)
			Out[i] = (0.299f * In[i].r + 0.587f * In[i].g + 0.114f * In[i].b);
		return ret;
	}

	PVX::Vector3D Rgb2HSL(const PVX::Vector3D & rgb) {
		float min = rgb.r < rgb.g ? (rgb.r < rgb.b ? rgb.r : rgb.b) : (rgb.g < rgb.b ? rgb.g : rgb.b);
		if (rgb.r > rgb.g && rgb.r > rgb.b) {
			float D = rgb.r - min;
			float L = (rgb.r + min) * 0.5f;
			if (!D) return { 0, 0, L };
			return {
				60.0f * (rgb.g - rgb.b) / (rgb.r - min),
				(L > 0.5f ? ((rgb.r - min) / (2.0f - rgb.r - min)) : ((rgb.r - min) / (rgb.r + min))),
				L
			};
		} else if (rgb.g > rgb.b) {
			float D = rgb.g - min;
			float L = (rgb.g + min) * 0.5f;
			if (!D) return { 0, 0, L };
			return {
				60.0f * (2.0f + (rgb.b - rgb.r) / (rgb.g - min)),
				(L > 0.5f ? ((rgb.g - min) / (2.0f - rgb.g - min)) : ((rgb.g - min) / (rgb.g + min))),
				L
			};
		} else {
			float D = rgb.b - min;
			float L = (rgb.b + min) * 0.5f;
			if (!D) return { 0, 0, L };
			return {
				60.0f * (4.0f + (rgb.r - rgb.g) / (rgb.b - min)),
				(L > 0.5f ? ((rgb.b - min) / (2.0f - rgb.b - min)) : ((rgb.b - min) / (rgb.b + min))),
				L
			};
		}
	}
	PVX::Vector3D HSL2Rgb(const PVX::Vector3D & v) {
		float t1 = (v.L < 0.5f) ? (v.L * (1.0f + v.S)) : (v.L + v.S - v.L * v.S);
		float t2 = 2.0f * v.L - t1;

		float g = v.H / 360.0f;
		float r = g + (1.0f / 3.0f);
		float b = g - (1.0f / 3.0f);

		if (r < 0)r += 1.0f; else if (r > 1.0f)r -= 1.0f;
		if (g < 0)g += 1.0f; else if (g > 1.0f)g -= 1.0f;
		if (b < 0)b += 1.0f; else if (b > 1.0f)b -= 1.0f;

		return{
			(r < (1.0f / 6.0f)) ? (t2 + (t1 - t2) * 6.0f * r) : (r < 0.5f) ? t1 : (r < (2.0f / 3.0f)) ? (t2 + (t1 - t2) *(2.0f / 3.0f - r) * 6.0f) : t2,
			(g < (1.0f / 6.0f)) ? (t2 + (t1 - t2) * 6.0f * g) : (g < 0.5f) ? t1 : (g < (2.0f / 3.0f)) ? (t2 + (t1 - t2) *(2.0f / 3.0f - g) * 6.0f) : t2,
			(b < (1.0f / 6.0f)) ? (t2 + (t1 - t2) * 6.0f * b) : (b < 0.5f) ? t1 : (b < (2.0f / 3.0f)) ? (t2 + (t1 - t2) *(2.0f / 3.0f - b) * 6.0f) : t2,
		};
	}

	static int conv(int x, int y, int WindowWidth, int StepX, int StepY, int StepXCount, int InWidth) {
		int WindowOffsetX = (y % StepXCount) * StepX;
		int WindowOffsetY = (y / StepXCount) * StepY;

		int localX = x % WindowWidth;
		int localY = x / WindowWidth;

		int Offset = (WindowOffsetX + localX) + (WindowOffsetY + localY) * InWidth;
		return Offset;
	}

	ImageF ImageF::MakeDeConv(int WindowWidth, int OriginalWidth, int StepX, int StepY) {
		int WindowHeight = (Width / WindowWidth);
		int StepsX = (OriginalWidth - WindowWidth + StepX) / StepX;
		OriginalWidth = (WindowWidth - StepX) + StepsX * StepX;
		int StepsY = Height / StepsX;

		int OriginalHeight = (WindowHeight - StepY) + StepsY * StepY;

		ImageF ret;
		ret.SetSize(OriginalWidth, OriginalHeight);
		int c = 0;

		int yStepSize = StepY * OriginalWidth;

		for (int ty = 0; ty < StepsY; ty++) {
			for (int tx = 0; tx < StepsX; tx++) {
				int wOffset = ty * yStepSize + tx * StepX;
				for (int y = 0; y < WindowHeight; y++)  for (int x = 0; x < WindowWidth; x++)
					ret.Pixels[size_t(wOffset) + size_t(x) + size_t(y) * size_t(OriginalWidth)] = Pixels[c++];
			}
		}
		return ret;
	}

	ImageF ImageF::MakeConv(int WindowWidth, int WindowHeight, int StepX, int StepY) {
		int StepCountX = (Width - WindowWidth + StepX) / StepX;
		int StepCountY = (Height - WindowHeight + StepY) / StepY;

		int outWidth = WindowWidth * WindowHeight;
		int outHeight = StepCountX * StepCountY;

		ImageF ret;
		ret.SetSize(outWidth, outHeight);

		for (int y = 0; y < outHeight; y++) {
			for (int x = 0; x < outWidth; x++) {
				ret.Pixels[size_t(x) + size_t(outWidth) * y] = Pixels[conv(x, y, WindowWidth, StepX, StepY, StepCountX, Width)];
			}
		}
		return ret;
	}

	ImageData ImageData::LoadLinear(const char* Filename) {
		FILE* fin;
		fopen_s(&fin, Filename, "rb");
		if (fin) {
			auto ret = LoadLinear(fin);
			fclose(fin);
			return ret;
		}
		return {};
	}
	ImageData ImageData::LoadLinear(const wchar_t* Filename) {
		FILE* fin;
		_wfopen_s(&fin, Filename, L"rb");
		if (fin) {
			auto ret = LoadLinear(fin);
			fclose(fin);
			return ret;
		}
		return {};
	}
	ImageData ImageData::LoadLinear(FILE* File) {
		STB_Linear::stbi_set_flip_vertically_on_load(1);
		auto [Width, Height, Channels, BitsPerChannel, IsHDR] = ImageInfo(File);
		if (Width) {
			std::vector<float> dataOut(size_t(Width) * Height * Channels);
			if (BitsPerChannel==8) {
				auto dataIn = STB_Linear::stbi_loadf_from_file(File, &Width, &Height, &Channels, Channels);
				memcpy(dataOut.data(), dataIn, dataOut.size() * sizeof(float));
				STB_Linear::stbi_image_free(dataIn);
			}
			else {
				constexpr float div = float(0x0000ffff);
				auto dataIn = STB_Linear::stbi_load_from_file_16(File, &Width, &Height, &Channels, Channels);
				for (auto i = 0; i<dataOut.size(); i++) 
					dataOut[i] = pow(float(dataIn[i]) / div, 2.2f);
				STB_Linear::stbi_image_free(dataIn);
			}
			return { Width, Height, Channels, std::move(dataOut) };
		}
		return {};
	}

	ImageData& ImageData::Bias(bool FlipR, bool FlipG, bool FlipB) {
		size_t pCount = size_t(Width) * Height;

		PVX::Vector3D bias = {
			FlipR ? 1.0f : -1.0f,
			FlipG ? 1.0f : -1.0f,
			FlipB ? 1.0f : -1.0f,
		};
		PVX::Vector3D mul = {
			FlipR ? -2.0f : 2.0f,
			FlipG ? -2.0f : 2.0f,
			FlipB ? -2.0f : 2.0f,
		};

		if (Channels == 4) {
			PVX::Vector4D* p = (PVX::Vector4D*) Data.data();
			for (auto i = 0; i<pCount; i++) {
				p[i] = PVX::Vector4D{ (p[i].Vec3 * mul + bias).Normalized(), p[i].w };
			}
		} else {
			PVX::Vector3D* p = (PVX::Vector3D*) Data.data();
			for (auto i = 0; i<pCount; i++) {
				p[i] = (p[i] * mul + bias).Normalized();
			}
		}
		return *this;
	}
	std::vector<uint8_t> ImageData::LinearJpeg(int Quality) {
		std::vector<uint8_t> ret;
		auto inp = To8bit();
		STB_Linear::stbi_flip_vertically_on_write(true);
		STB_Linear::stbi_write_jpg_to_func([](void* context, void* data, int size) {
			std::vector<uint8_t>& ret = *(std::vector<uint8_t>*)context;
			auto sz = ret.size();
			ret.resize(sz + size);
			memcpy(&ret[sz], data, size);
		}, &ret, Width, Height, Channels, inp.data(), Quality);
		return ret;
	}
	std::vector<uint8_t> ImageData::LinearPng() {
		std::vector<uint8_t> ret;
		auto inp = To8bit();
		STB_Linear::stbi_flip_vertically_on_write(true);
		STB_Linear::stbi_write_png_to_func([](void* context, void* data, int size) {
			std::vector<uint8_t>& ret = *(std::vector<uint8_t>*)context;
			auto sz = ret.size();
			ret.resize(sz + size);
			memcpy(&ret[sz], data, size);
		}, &ret, Width, Height, Channels, inp.data(), Width * Channels);
		return ret;
	}
	std::vector<uint8_t> ImageData::Hdr() {
		std::vector<uint8_t> ret;
		STB_Linear::stbi_flip_vertically_on_write(true);
		STB_Linear::stbi_write_hdr_to_func([](void* context, void* data, int size) {
			std::vector<uint8_t>& ret = *(std::vector<uint8_t>*)context;
			auto sz = ret.size();
			ret.resize(sz + size);
			memcpy(&ret[sz], data, size);
		}, &ret, Width, Height, Channels, Data.data());
		return ret;
	}
	std::vector<uint8_t> ImageData::Tga() {
		std::vector<uint8_t> ret;
		auto inp = To8bit();
		STB_Linear::stbi_flip_vertically_on_write(true);
		STB_Linear::stbi_write_tga_to_func([](void* context, void* data, int size) {
			std::vector<uint8_t>& ret = *(std::vector<uint8_t>*)context;
			auto sz = ret.size();
			ret.resize(sz + size);
			memcpy(&ret[sz], data, size);
		}, &ret, Width, Height, Channels, inp.data());
		return ret;
	}
	std::vector<uint8_t> ImageData::To8bit() {
		std::vector<uint8_t> ret(Width* Height* Channels);
		for (int i = 0; i<ret.size(); i++) {
			ret[i] = std::min(std::max(255 * pow(Data[i], 1.0f/2.2f), 0.0f), 255.0f);
		}
		return std::move(ret);
	}
	std::vector<uint8_t> ImageData::To8bitRaw() {
		std::vector<uint8_t> ret(Width* Height* Channels);
		for (int i = 0; i<ret.size(); i++) {
			ret[i] = std::min(std::max(255 * Data[i], 0.0f), 255.0f);
		}
		return std::move(ret);
	}
}
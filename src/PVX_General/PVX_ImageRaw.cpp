#include <PVX_Image.h>
#include <map>
#include <PVX_File.h>


namespace STB_Raw {
#define STBI_NO_LINEAR
#include <stb/stb_image.h>
}

#include <PVX_String.h>
#include <Eigen/dense>

#include <mutex>

#define __FLIP_V__ 0

namespace PVX {
	ImageData ImageData::LoadRaw(const char* Filename) {
		FILE* fin;
		fopen_s(&fin, Filename, "rb");
		if (fin) {
			auto ret = LoadRaw(fin);
			fclose(fin);
			return ret;
		}
		return {};
	}
	ImageData ImageData::LoadRaw(const wchar_t* Filename) {
		FILE* fin;
		_wfopen_s(&fin, Filename, L"rb");
		if (fin) {
			auto ret = LoadRaw(fin);
			fclose(fin);
			return ret;
		}
		return {};
	}
	ImageData ImageData::LoadRaw(FILE* File) {
		STB_Raw::stbi_set_flip_vertically_on_load(1);
		auto [Width, Height, Channels, BitsPerChannel, IsHDR] = ImageInfo(File);
		if (Width) {
			std::vector<float> dataOut(size_t(Width) * Height * Channels);
			constexpr float inv = 1.0f / 255.0f;
			if (BitsPerChannel==8) {
				//auto dataIn = STB_Raw::stbi_loadf_from_file(File, &Width, &Height, &Channels, Channels);
				auto dataIn = STB_Raw::stbi_load_from_file(File, &Width, &Height, &Channels, Channels);
				auto cur = dataIn;
				for (auto& o : dataOut) o = *(cur++) *inv;
				
				STB_Raw::stbi_image_free(dataIn);
			}
			else {
				constexpr float div = float(0x0000ffff);
				auto dataIn = STB_Raw::stbi_load_from_file_16(File, &Width, &Height, &Channels, Channels);
				for (auto i = 0; i<dataOut.size(); i++) 
					dataOut[i] = float(dataIn[i]) / div;
				STB_Raw::stbi_image_free(dataIn);
			}
			return { Width, Height, Channels, std::move(dataOut) };
		}
		return {};
	}
}
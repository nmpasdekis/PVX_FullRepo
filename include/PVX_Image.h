#ifndef __PVX_IMAGE_2018_H__
#define __PVX_IMAGE_2018_H__

#include<PVX_Math3D.h>
#include<vector>
#include<functional>

#ifdef __USEEIGEN__
#include <Eigen/dense>
#endif

namespace PVX {
	typedef struct ImageF ImageF;
	typedef struct Image3F Image3F;
	typedef struct Image4F Image4F;

	namespace Helpers {
		template<typename T>
		class Tiler {
			T* Pixels;
			struct {
				int NextPixelX;
				int NextPixelY;
				int NextTileX;
				int NextTileY;
			} Strides;

			int TileOffset;
		public:
			Tiler(T* Pixels, int NextPixelX, int NextPixelY, int NextTileX, int NextTileY) :
				Pixels{ Pixels }, Strides{ NextPixelX, NextPixelY, NextTileX, NextTileY }, TileOffset{ 0 } {}
			void CurrentTile(int x, int y) {
				TileOffset = x * Strides.NextTileX + y * Strides.NextTileY;
			}
			T& Get(int x, int y) {
				return Pixels[TileOffset + x * Strides.NextPixelX + y * Strides.NextPixelY];
			}
			T& Get(int TileX, int TileY, int PixelX, int PixelY) {
				return Pixels[TileX * Strides.NextTileX + TileY * Strides.NextTileY + PixelX * Strides.NextPixelX + PixelY * Strides.NextPixelY];
			}

			void ForEachPixelInTile(int TileX, int TileY, int TileWidth, int TileHeight, std::function<void(T&Pixel, int x, int y)> fnc) {
				CurrentTile(TileX, TileY);
				for (int y = 0; y < TileHeight; y++) {
					for (int x = 0; x < TileWidth; x++) {
						fnc(Get(x, y), x, y);
					}
				}
			}

			static Tiler<T> FromLayout(T* Data, int TileWidth, int TileHeight, int TilesX, int TilesY) {
				return Tiler<T>(Data, 1, TilesX * TileWidth, TileWidth, TileHeight * TilesX * TileWidth);
			}
		};

		template<typename Tout>
		inline Tout ReTile(const Tout& img, int srcTilesX, int srcTilesY, int dstTilesX, int dstTilesY) {
			using PixelType = std::decay_t<decltype(img.Pixels[0])>;
			int TileWidth = img.Width / srcTilesX;
			int TileHeight = img.Height / srcTilesY;

			int tileCount = srcTilesX * srcTilesY;

			Tout ret(TileWidth * dstTilesX, TileHeight * dstTilesY);

			auto t1 = Tiler<PixelType>::FromLayout((PixelType*)img.Pixels.data(), TileWidth, TileHeight, srcTilesX, srcTilesY);
			auto t2 = Tiler<PixelType>::FromLayout(ret.Pixels.data(), TileWidth, TileHeight, dstTilesX, dstTilesY);

			int curSrcX = 0;
			int curSrcY = 0;
			int curDstX = 0;
			int curDstY = 0;

			for (int i = 0; i< tileCount; i++) {
				t1.ForEachPixelInTile(curSrcX, curSrcY, TileWidth, TileHeight, [&](PixelType & p, int x, int y) {
					t2.Get(x, y) = p;
				});
				curSrcX++;
				if (curSrcX == srcTilesX) {
					curSrcY++;
					curSrcX = 0;
				}
				curDstX++;
				if (curDstX == dstTilesX) {
					curDstY++;
					curDstX = 0;
				}
				t2.CurrentTile(curDstX, curDstY);
			}
			return ret;
		}

		template<typename Tout>
		inline Tout ReTile(const Tout& img, int srcTilesX, int srcTilesY, int dstTilesX, int dstTilesY, const std::initializer_list<int>& Tiles) {
			using PixelType = std::decay_t<decltype(img.Pixels[0])>;
			int TileWidth = img.Width / srcTilesX;
			int TileHeight = img.Height / srcTilesY;

			int tileCount = srcTilesX * srcTilesY;

			Tout ret(TileWidth * dstTilesX, TileHeight * dstTilesY);

			auto t1 = Tiler<PixelType>::FromLayout((PixelType*)img.Pixels.data(), TileWidth, TileHeight, srcTilesX, srcTilesY);
			auto t2 = Tiler<PixelType>::FromLayout(ret.Pixels.data(), TileWidth, TileHeight, dstTilesX, dstTilesY);

			int curSrcX = 0;
			int curSrcY = 0;
			int curDstX = 0;
			int curDstY = 0;

			for (auto Index: Tiles) {
				t1.ForEachPixelInTile(Index % srcTilesX, Index / srcTilesX, TileWidth, TileHeight, [&](PixelType& p, int x, int y) {
					t2.Get(x, y) = p;
				});
				curDstX++;
				if (curDstX == dstTilesX) {
					curDstY++;
					curDstX = 0;
				}
				t2.CurrentTile(curDstX, curDstY);
			}
			return ret;
		}
	}

	struct ImageF {
		ImageF();
		ImageF(int Width, int Height);
		ImageF(const Image3F& img);
		ImageF(const Image4F& img);
		int Width = 0, Height = 0;
		std::vector<float> Pixels;
		static ImageF Load(const char* Filename);
		void SetSize(int Width, int Height);
		void SetPixels(const void* Data);
		void SetPixels(int Width, int Height, const void* Data);
		float& operator()(int x, int y);
		const float& operator()(int x, int y) const;
		std::vector<unsigned char> JPEG() const;
		std::vector<unsigned char> PNG() const;
		std::vector<unsigned char> TGA() const;
		void Region(float* Out, int x, int y, int Width, int Height) const;
		void Save(const char* Filename);

		ImageF MakeConv(int WindowWidth, int WindowHeight, int StepX, int StepY);
		ImageF MakeDeConv(int WindowWidth, int OriginalWidth, int StepX, int StepY);

		inline void ForEach(std::function<void(float& Pixel, int x, int y)> fnc) {
			for (int y = 0; y<Height; y++)
				for (int x = 0, i = 0; x<Width; x++, i++) fnc(Pixels[i], x, y);
		}

#ifdef __USEEIGEN__
		ImageF(const Eigen::MatrixXf& mat);
		Eigen::MatrixXf Matrix();
		Eigen::Map<Eigen::MatrixXf> MatrixMap() const;
#endif
	};



	struct Image3F {
		Image3F() {};
		Image3F(int Width, int Height);
		Image3F(const ImageF& img);
		Image3F(const Image4F& img);
		int Width = 0, Height = 0;
		std::vector<PVX::Vector3D> Pixels;
		static Image3F Load(const char* Filename);
		static Image3F Load(const wchar_t* Filename);
		static Image3F JPEG(const unsigned char* Data, size_t Size);
		ImageF Red();
		ImageF Green();
		ImageF Blue();
		ImageF Grey();

		std::tuple<ImageF, ImageF, ImageF> SplitRGB();

		void SetSize(int Width, int Height);
		void SetPixels(const void* Data);
		void SetPixels(int Width, int Height, const void* Data);
		PVX::Vector3D& operator()(int x, int y);
		std::vector<unsigned char> JPEG() const;
		std::vector<unsigned char> PNG() const;
		std::vector<unsigned char> TGA() const;
		static Image3F FromRGB(const ImageF& Red, const ImageF& Green, const ImageF& Blue);
	};

	struct Image4F {
		Image4F() {};
		Image4F(int Width, int Height);
		Image4F(const ImageF& img);
		Image4F(const Image3F& img);

		int Width = 0, Height = 0;
		std::vector<PVX::Vector4D> Pixels;
		static Image4F Load(const char* Filename);
		static Image4F Load(const wchar_t* Filename);
		ImageF Red();
		ImageF Green();
		ImageF Blue();
		ImageF Alpha();
		ImageF Grey();
		void SetSize(int Width, int Height);
		void SetPixels(const void* Data);
		void SetPixels(int Width, int Height, const void* Data);
		PVX::Vector4D& operator()(int x, int y);
		std::vector<unsigned char> JPEG() const;
		std::vector<unsigned char> PNG() const;
		std::vector<unsigned char> TGA() const;
	};

	PVX::Vector3D Rgb2HSL(const PVX::Vector3D& rgb);
	PVX::Vector3D HSL2Rgb(const PVX::Vector3D& rgb);
}

#endif
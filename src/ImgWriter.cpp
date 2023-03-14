#include "ImgWriter.h"
#include <cstdlib>
#include <exception>
#include <stdexcept>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#pragma warning (push)
#pragma warning (disable : 4996)
#include "external/stb/stb_image_write.h"
#pragma warning (pop)

ImgWriter::ImgWriter(uint32_t w, uint32_t h) : width(w), height(h)
{
	data = new uint32_t[width * height]();
}

ImgWriter::~ImgWriter()
{
	if (data)
		delete[] data;
}

ImgWriter& ImgWriter::operator=(ImgWriter&& other) noexcept
{
	if (&other != this)
	{
		if (data)
			delete[] data;

		data = other.data;
		width = other.width;
		height = other.height;
		other.data = nullptr;
	}

	return *this;
}

ImgWriter::ImgWriter(ImgWriter&& other) noexcept
{
	data = other.data;
	width = other.width;
	height = other.height;
	other.data = nullptr;
}

void ImgWriter::paste(const ImgLoaderExt& img, bool extendBorder)
{
	// !!! no outfill checks !!!

	if (img.rotated)
	{
		for (uint32_t y = 0; y < img.cropRect.w; ++y)
			for (uint32_t x = 0; x < img.cropRect.h; ++x)
				data[img.x + x + (img.y + y) * width] = img.getPixels()[(img.cropRect.y + img.cropRect.h - 1 - x) * img.getW() + img.cropRect.x + y];
	}
	else
	{
		for (uint32_t row = 0; row < img.cropRect.h; ++row)
			memcpy(data + ((row + img.y) * width + img.x), img.getPixels() + (row + img.cropRect.y) * img.getW() + img.cropRect.x, img.cropRect.w * 4);
	}

	if (extendBorder)
	{
		const uint32_t x = img.x;
		const uint32_t y = img.y;
		const uint32_t w = img.rotated ? img.cropRect.h : img.cropRect.w;
		const uint32_t h = img.rotated ? img.cropRect.w : img.cropRect.h;

		memcpy(data + (y - 1) * width + x, data + y * width + x, w * 4);
		memcpy(data + (y + h) * width + x, data + (y + h - 1) * width + x, w * 4);

		for (uint32_t row = 0; row < h; ++row)
		{
			data[x + (y + row) * width - 1] = data[x + (y + row) * width];
			data[x + (y + row) * width + w] = data[x + (y + row) * width + w - 1];
		}

		data[x + (y - 1) * width - 1] = data[x + y * width];
		data[x + (y - 1) * width + w] = data[x + y * width + w - 1];
		data[x + (y + h) * width - 1] = data[x + (y + h - 1) * width];
		data[x + (y + h) * width + w] = data[x + (y + h - 1) * width + w - 1];
	}
}

void ImgWriter::fill(uint32_t color)
{
	for (uint32_t row = 0; row < height; ++row)
		for (uint32_t col = 0; col < width; ++col)
			data[col + row * width] = color;
}

void ImgWriter::write(const std::string& fileName) const
{
	int ret = stbi_write_png(fileName.c_str(), width, height, 4, data, width * 4);
	if (!ret)
		throw std::runtime_error("stbi_write_png error");
}

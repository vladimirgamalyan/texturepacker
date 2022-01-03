#include "ImgWriter.h"
#include <cstdlib>
#include <exception>
#include <stdexcept>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#pragma warning (push)
#pragma warning (disable : 4996)
#include "external/stb/stb_image_write.h"
#pragma warning (pop)

ImgWriter::ImgWriter(uint32_t w, uint32_t h) : w(w), h(h)
{
	data = new uint32_t[w * h]();
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
		w = other.w;
		h = other.h;
		other.data = nullptr;
	}

	return *this;
}

ImgWriter::ImgWriter(ImgWriter&& other) noexcept
{
	data = other.data;
	w = other.w;
	h = other.h;
	other.data = nullptr;
}

void ImgWriter::paste(const ImgLoaderExt& img)
{
	// !!! no outfill checks !!!

	if (img.flipped)
	{
		for (uint32_t y = 0; y < img.cropRect.w; ++y)
			for (uint32_t x = 0; x < img.cropRect.h; ++x)
				data[img.x + x + (img.y + y) * w] = img.getPixels()[(img.cropRect.y + img.cropRect.h - 1 - x) * img.getW() + img.cropRect.x + y];
	}
	else
	{
		for (uint32_t row = 0; row < img.cropRect.h; ++row)
			memcpy(data + ((row + img.y) * w + img.x), img.getPixels() + (row + img.cropRect.y) * img.getW() + img.cropRect.x, img.cropRect.w * 4);
	}
}

void ImgWriter::fill(uint32_t color)
{
	for (uint32_t row = 0; row < h; ++row)
		for (uint32_t col = 0; col < w; ++col)
			data[col + row * w] = color;
}

void ImgWriter::write(const std::string& fileName) const
{
	int ret = stbi_write_png(fileName.c_str(), w, h, 4, data, w * 4);
	if (!ret)
		throw std::runtime_error("stbi_write_png error");
}

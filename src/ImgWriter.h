#pragma once
#include <cstdint>
#include <string>
#include "ImgLoaderExt.h"

class ImgWriter
{
public:
	ImgWriter(uint32_t w, uint32_t h);
	virtual ~ImgWriter();

	ImgWriter& operator=(const ImgWriter&) = delete;
	ImgWriter(const ImgWriter&) = delete;
	ImgWriter() = default;
	ImgWriter(ImgWriter&& other) noexcept;
	ImgWriter& operator=(ImgWriter&& other) noexcept;

	void paste(const ImgLoaderExt& img, bool extendBorder);
	void fill(uint32_t color);
	void write(const std::string& fileName) const;

private:
	uint32_t* data = nullptr;
	uint32_t width;
	uint32_t height;
};

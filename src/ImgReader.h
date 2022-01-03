#pragma once
#include <string>
#include <cstdint>

class ImgReader
{
public:
	ImgReader();
	bool load(const std::string& fileName);
	uint32_t width() const;
	uint32_t height() const;
private:
	uint32_t g_width;
	uint32_t g_height;
};

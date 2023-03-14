#pragma once
#include "StbImgLoader.h"
#include <cstdint>

class ImgLoaderExt : public StbImgLoader
{
public:
	struct CropRect
	{
		uint32_t x = 0;
		uint32_t y = 0;
		uint32_t w = 1;
		uint32_t h = 1;
	};


	virtual bool load(const std::string& fileName) override;

	uint32_t x = 0;
	uint32_t y = 0;
	int page = -1;
	bool rotated = false;
	std::string lastFileName;

	CropRect cropRect;

private:
	void crop();
	void extendCrop();
	void fixCrop();
	void fillCrop();
	void fill(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color);
};

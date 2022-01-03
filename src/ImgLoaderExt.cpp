#include "ImgLoaderExt.h"

bool ImgLoaderExt::load(const std::string& fileName)
{
	lastFileName = fileName;

	if (!StbImgLoader::load(fileName))
		return false;

	crop();
	fixCrop();
#ifdef _DEBUG
	fillCrop();
#endif
	return true;
}

void ImgLoaderExt::crop()
{
	/*
		�������� �����: ������� ������ �������� ����� �� ������, ���� �� ������ ������� � ��������� ������.
		��� ���������� ������� ������ ���������.
		���� ��������� ������ �� �����, �� ������ ��������� ���� ���������� ������� 0:0.

		����� �������� ����� ����� ����� �� ������, �������� ������� ����� ��������.

		����� �������� ����� ������� ����� ������ ����, �������� ��� ���������� ������� ������ � �����.
		���� � ������ ������.
	*/

	const uint32_t* pixels = getPixels();
	uint32_t pixelCount = w * h;

	// ���� ������ (������� �����) � ��������� (������ ������) ������� ������������, ��� ������ ������ ������.
	if ((pixels[0] & 0xff000000) && (pixels[pixelCount - 1] & 0xff000000))
	{
		cropRect.x = 0;
		cropRect.y = 0;
		cropRect.w = w;
		cropRect.h = h;
		return;
	}

	// �������� ������� � ������� ������� ����� �������, ������ ���� ���� �� �������� �� ������������ �������.
	while (pixelCount)
	{
		if (*pixels & 0xff000000)
			break;
		++pixels;
		--pixelCount;
	}

	// ���� �� ������ ������������� ������� �� �����������, ������ �������� ��������� ����������, 
	// ��������� ���� ����� ������� ������� (����������).
	if (!pixelCount)
	{
		cropRect.x = 0;
		cropRect.y = 0;
		cropRect.w = 1;
		cropRect.h = 1;
		return;
	}

	uint32_t y0 = (w * h - pixelCount) / w;

	// ������ �������� � �����.
	pixels = getPixels() + w * h - 1;
	while ((*pixels & 0xff000000) == 0)
		--pixels;
	uint32_t y1 = (pixels - getPixels()) / w + 1;

	cropRect.h = y1 - y0;


	// ������ ����� ������� ���� � ��������� ������������ �����.
	uint32_t x0 = 0;
	bool found = false;
	for (;;)
	{
		pixels = getPixels() + x0 + y0 * w;
		for (uint32_t i = cropRect.h; i > 0; --i)
		{
			if (*pixels & 0xff000000)
			{
				found = true;
				break;
			}
			pixels += w;
		}
		if (found)
			break;
		++x0;
	}

	// ������ ������ ������ ���� � ��������� ������������ �����.
	uint32_t x1 = w - 1;
	found = false;
	for (;;)
	{
		pixels = getPixels() + x1 + y0 * w;
		for (uint32_t i = cropRect.h; i > 0; --i)
		{
			if (*pixels & 0xff000000)
			{
				found = true;
				break;
			}
			pixels += w;
		}
		if (found)
			break;
		--x1;
	}
	++x1;

	cropRect.x = x0;
	cropRect.y = y0;
	cropRect.w = x1 - x0;
}

void ImgLoaderExt::fixCrop()
{
	/*
		���� �������������� ���, ����� �������� ������ ���������� � �� ���������� �������� 
		���� �� ����� ����� �������� (����������� ������).

		��� ����� ����� ����� �� ����� ������� (�� ����������� ��� ���������) ���� ������ ���������� ���������� ��������.

	*/
	const int hCrop = getW() - cropRect.w;
	const int vCrop = getH() - cropRect.h;

	if (hCrop % 2)
	{
		if (cropRect.x > 0)
			--cropRect.x;
		++cropRect.w;
	}

	if (vCrop % 2)
	{
		if (cropRect.y > 0)
			--cropRect.y;
		++cropRect.h;
	}
}

void ImgLoaderExt::fillCrop()
{
	const uint32_t c = 0xff00ffff;
	fill(0, 0, w, cropRect.y, c);
	fill(0, cropRect.y + cropRect.h, w, h - (cropRect.y + cropRect.h), c);
	fill(0, 0, cropRect.x, h, c);
	fill(cropRect.x + cropRect.w, 0, w - (cropRect.x + cropRect.w), h, c);
}

void ImgLoaderExt::fill(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color)
{
	for (uint32_t row = 0; row < h; ++row)
		for (uint32_t col = 0; col < w; ++col)
			reinterpret_cast<uint32_t*>(data)[x + col + (y + row) * getW()] = color;
}

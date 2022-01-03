#pragma once
#include <string>
#include <cassert>

class StbImgLoader
{
public:
	StbImgLoader& operator=(const StbImgLoader&) = delete;
	StbImgLoader(const StbImgLoader&) = delete;
	StbImgLoader() = default;
	StbImgLoader(StbImgLoader&& other) noexcept;
	StbImgLoader& operator=(StbImgLoader&& other) noexcept;
	virtual ~StbImgLoader();

	virtual bool load(const std::string& fileName);
	void unload();
	bool isValid() const
	{
		return data != nullptr;
	}
	uint32_t getW() const
	{
		assert(isValid());
		return w;
	}
	uint32_t getH() const
	{
		assert(isValid());
		return h;
	}
	const uint32_t* getPixels() const
	{
		assert(isValid());
		return reinterpret_cast<const uint32_t*>(data);
	}

protected:
	uint8_t* data = nullptr;
	uint32_t w = 0;
	uint32_t h = 0;
};

#include "StbImgLoader.h"

#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#define STBI_MAX_DIMENSIONS 8192
#define STBI_NO_FAILURE_STRINGS
#include "external/stb/stb_image.h"

StbImgLoader& StbImgLoader::operator=(StbImgLoader&& other) noexcept
{
	if (&other != this)
	{
		unload();

		data = other.data;
		w = other.w;
		h = other.h;
		other.data = nullptr;
	}

	return *this;
}

StbImgLoader::StbImgLoader(StbImgLoader&& other) noexcept
{
	data = other.data;
	w = other.w;
	h = other.h;
	other.data = nullptr;
}

StbImgLoader::~StbImgLoader()
{
	unload();
}

bool StbImgLoader::load(const std::string& fileName)
{
	unload();

	int n, w_, h_;
	data = stbi_load(fileName.c_str(), &w_, &h_, &n, 4);

	assert(w_ > 0);
	assert(h_ > 0);

	w = static_cast<uint32_t>(w_);
	h = static_cast<uint32_t>(h_);

	return isValid();
}

void StbImgLoader::unload()
{
	if (data)
	{
		stbi_image_free(data);
		data = nullptr;
	}
}

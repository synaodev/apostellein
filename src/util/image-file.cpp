#include <apostellein/cast.hpp>

#include "./image-file.hpp"

#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_STDIO
#define STBI_ONLY_PNG
#define STBI_MAX_DIMENSIONS APOSTELLEIN_MAXIMUM_IMAGE_FILE_LENGTH

#include <stb_image.h>

void image_file::clear() {
	dimensions_ = {};
	if (pixels_) {
		stbi_image_free(pixels_);
		pixels_ = nullptr;
	}
}

bool image_file::load(const std::vector<byte>& buffer) {
	this->clear();
	i32 components = 0;
	pixels_ = stbi_load_from_memory(
		buffer.data(),
		as<i32>(buffer.size()),
		&dimensions_.x,
		&dimensions_.y,
		&components,
		STBI_rgb_alpha
	);
	return pixels_ != nullptr;
}

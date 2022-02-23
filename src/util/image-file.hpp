#pragma once

#include <cassert>
#include <vector>
#include <glm/vec2.hpp>
#include <apostellein/struct.hpp>

#if !defined(APOSTELLEIN_MINIMUM_IMAGE_FILE_LENGTH)
	#define APOSTELLEIN_MINIMUM_IMAGE_FILE_LENGTH 64
#endif
#if !defined(APOSTELLEIN_MAXIMUM_IMAGE_FILE_LENGTH)
	#define APOSTELLEIN_MAXIMUM_IMAGE_FILE_LENGTH 2048
#endif

struct image_file : public not_copyable {
	image_file() noexcept = default;
	image_file(image_file&& that) noexcept {
		*this = std::move(that);
	}
	image_file& operator=(image_file&& that) noexcept {
		if (this != &that) {
			dimensions_ = that.dimensions_;
			that.dimensions_ = {};
			pixels_ = that.pixels_;
			that.pixels_ = nullptr;
		}
		return *this;
	}
	~image_file() { this->clear(); }
public:
	static constexpr i32 MINIMUM_LENGTH = APOSTELLEIN_MINIMUM_IMAGE_FILE_LENGTH;
	static constexpr i32 MAXIMUM_LENGTH = APOSTELLEIN_MAXIMUM_IMAGE_FILE_LENGTH;
	bool load(const std::vector<byte>& buffer);
	void clear();
	bool valid() const { return pixels_ != nullptr; }
	const glm::ivec2& dimensions() const { return dimensions_; }
	byte* pixels() {
		assert(pixels_ != nullptr);
		return pixels_;
	}
	const byte* pixels() const {
		assert(pixels_ != nullptr);
		return pixels_;
	}
	udx length() const {
		return (
			static_cast<udx>(dimensions_.x) *
			static_cast<udx>(dimensions_.y) *
			sizeof(chroma)
		);
	}
private:
	glm::ivec2 dimensions_ {};
	byte* pixels_ {};
};

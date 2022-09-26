#pragma once

#include "../util/image-file.hpp"

struct material : public not_copyable {
	material() noexcept = default;
	material(material&& that) noexcept {
		*this = std::move(that);
	}
	material& operator=(material&& that) noexcept {
		if (this != &that) {
			id_ = that.id_;
			that.id_ = 0;
			atlas_ = that.atlas_;
			that.atlas_ = 0;
			dimensions_ = that.dimensions_;
			that.dimensions_ = {};
			offset_ = that.offset_;
			that.offset_ = {};
			image_ = std::move(that.image_);
		}
		return *this;
	}
	~material() { this->destroy(); }
public:
	static constexpr glm::vec2 MAXIMUM_DIMENSIONS {
		image_file::MAXIMUM_LENGTH,
		image_file::MAXIMUM_LENGTH
	};
	void load(image_file image);
	void destroy();
	void offset(i32 atlas, i32 x, i32 y) {
		atlas_ = atlas;
		offset_ = { x, y };
	}
	bool valid() const { return id_ > 0; }
	r32 atlas() const { return static_cast<r32>(atlas_); }
	i32 id() const { return id_; }
	glm::vec2 dimensions() const {
		if (this->valid()) {
			return glm::vec2(dimensions_);
		}
		return {};
	}
	glm::ivec2 integral_dimensions() const {
		if (this->valid()) {
			return dimensions_;
		}
		return {};
	}
	glm::vec2 offset() const {
		if (this->valid()) {
			return glm::vec2(offset_);
		}
		return {};
	}
	glm::ivec2 integral_offset() const {
		if (this->valid()) {
			return offset_;
		}
		return {};
	}
	byte* pixels() { return image_.pixels(); }
	const byte* pixels() const { return image_.pixels(); }
	static i32 binding();
	static bool recalibrate();
private:
	i32 id_ {};
	i32 atlas_ {};
	glm::ivec2 dimensions_ {};
	glm::ivec2 offset_ {};
	image_file image_ {};
};

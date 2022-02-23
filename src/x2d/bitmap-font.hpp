#pragma once

#include <string>
#include <map>
#include <glm/vec2.hpp>
#include <apostellein/struct.hpp>

struct material;

struct bitmap_glyph {
	constexpr bitmap_glyph() noexcept = default;
	constexpr bitmap_glyph(
		r32 _x, r32 _y, r32 _w, r32 _h,
		r32 _x_offset, r32 _y_offset,
		r32 _x_advance, i32 _channel
	) noexcept : x{ _x }, y{ _y }, w{ _w }, h{ _h },
		x_offset{ _x_offset }, y_offset{ _y_offset },
		x_advance{ _x_advance }, channel{ _channel } {}

	r32 x {};
	r32 y {};
	r32 w {};
	r32 h {};
	r32 x_offset {};
	r32 y_offset {};
	r32 x_advance {};
	i32 channel {};
};

struct bitmap_font : public not_copyable {
	bitmap_font() noexcept = default;
	bitmap_font(bitmap_font&& that) noexcept {
		*this = std::move(that);
	}
	bitmap_font& operator=(bitmap_font&& that) noexcept {
		if (this != &that) {
			glyphs_ = std::move(that.glyphs_);
			that.glyphs_.clear();
			kernings_ = std::move(that.kernings_);
			that.kernings_.clear();
			dimensions_ = that.dimensions_;
			that.dimensions_ = {};
			texture_ = that.texture_;
			that.texture_ = nullptr;
		}
		return *this;
	}
	~bitmap_font() { this->destroy(); }
public:
	void load(const std::string& route, const std::string& path);
	void destroy();
	bool valid() const;
	const bitmap_glyph& glyph(char32_t code_point) const;
	r32 kerning(char32_t first, char32_t second) const;
	const glm::vec2& glyph_dimensions() const { return dimensions_; }
	glm::vec2 texture_dimensions() const;
	glm::vec2 texture_offset() const;
	r32 atlas() const;
private:
	std::map<char32_t, bitmap_glyph> glyphs_ {};
	std::map<std::pair<char32_t, char32_t>, r32> kernings_ {};
	glm::vec2 dimensions_ {};
	const material* texture_ {};
};

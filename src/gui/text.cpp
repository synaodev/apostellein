#include <algorithm>
#include <iterator>
#include <apostellein/cast.hpp>

#include "./text.hpp"
#include "../video/material.hpp"
#include "../x2d/bitmap-font.hpp"
#include "../x2d/renderer.hpp"

namespace {
	static constexpr r32 TAB_WIDTH = 4.0f;
	static constexpr i32 UNICODE_TRAILING[] {
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
		2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5
	};
	static constexpr char32_t UNICODE_OFFSETS[] {
		0x00000000, 0x00003080, 0x000E2080,
		0x03C82080, 0xFA082080, 0x82082080
	};
}

void gui::unicode(const std::string& utf8, std::u32string& utf32) {
	const auto end = std::end(utf8);
	auto decode_point = [end](auto iter) -> std::tuple<char32_t, decltype(iter)> {
		const auto trailing = UNICODE_TRAILING[as<byte>(*iter)];
		if ((iter + trailing) < end) {
			char32_t code = U'\0';
			switch (trailing) {
				case 5: code += as<byte>(*iter++); code <<= 6;
				[[fallthrough]];
				case 4: code += as<byte>(*iter++); code <<= 6;
				[[fallthrough]];
				case 3: code += as<byte>(*iter++); code <<= 6;
				[[fallthrough]];
				case 2: code += as<byte>(*iter++); code <<= 6;
				[[fallthrough]];
				case 1: code += as<byte>(*iter++); code <<= 6;
				[[fallthrough]];
				case 0: code += as<byte>(*iter++);
			}
			return std::make_tuple(code - UNICODE_OFFSETS[trailing], iter);
		}
		return std::make_tuple(U'\0', end);
	};
	auto iter = std::begin(utf8);
	auto out = std::back_inserter(utf32);
	while (iter != end) {
		auto&& [code, next] = decode_point(iter);
		iter = next;
		*out++ = code;
	}
}

void gui::text::build(
	const glm::vec2& position,
	const glm::vec2& origin,
	const chroma& color,
	const bitmap_font* font,
	const std::string& words
) {
	invalidated_ = true;
	position_ = position;
	origin_ = origin;
	color_ = color;
	font_ = font;
	if (!words.empty()) {
		this->replace(words);
	}
}

void gui::text::clear() {
	invalidated_ = true;
	letter_ = 0;
	buffer_.clear();
	vertices_.clear();
}

void gui::text::render(renderer& rdr) const {
	if (font_ and letter_ > 0 and !vertices_.empty()) {
		auto& list = rdr.query(
			priority_type::deferred,
			blending_type::alpha,
			pipeline_type::glyph
		);
		if (invalidated_) {
			invalidated_ = false;
			list.upload(vertices_, letter_ * display_list::QUAD);
		} else {
			list.skip(letter_ * display_list::QUAD);
		}
	}
}

void gui::text::position(glm::vec2 value) {
	value = glm::round(value);
	if (position_ != value) {
		invalidated_ = true;
		position_ = value;
		this->generate_attributes_();
	}
}

void gui::text::origin(glm::vec2 value) {
	value = glm::round(value);
	if (origin_ != value) {
		invalidated_ = true;
		origin_ = value;
		this->generate_attributes_();
	}
}

glm::vec2 gui::text::font_dimensions() const {
	if (font_) {
		return font_->glyph_dimensions();
	}
	return {};
}

rect gui::text::bounds() const {
	if (!vertices_.empty()) {
		r32 left = vertices_[0].position.x;
		r32 top = vertices_[0].position.y;
		r32 right = vertices_[0].position.x;
		r32 bottom = vertices_[0].position.y;
		const auto length = letter_ * display_list::QUAD;
		for (udx it = 1; it < length; ++it) {
			const glm::vec2 mark = vertices_[it].position;
			if (mark.x < left) {
				left = mark.x;
			} else if (mark.x > right) {
				right = mark.x;
			}
			if (mark.y < top) {
				top = mark.y;
			} else if (mark.y > bottom) {
				bottom = mark.y;
			}
		}
		return {
			left, top,
			right - left,
			bottom - top
		};
	}
	return {};
}

udx gui::text::drawable() const {
	const auto result = std::count_if(
		buffer_.begin(), buffer_.end(),
		[](auto c) { return c == U'\n' or c == U'\t'; }
	);
	return buffer_.size() - as<udx>(result);
}

void gui::text::generate_quads_() {
	if (font_ and !buffer_.empty()) {
		const auto length = this->drawable();
		if (length * display_list::QUAD > vertices_.size()) {
			vertices_.resize(length * display_list::QUAD);
		}
		glm::vec2 pos = position_ - origin_;
		const glm::vec2 dim = font_->glyph_dimensions();
		const glm::vec2 off = font_->texture_offset();
		const auto atlas = font_->atlas();
		char32_t previous = U'\0';
		udx idx = 0;
		for (auto&& c : buffer_) {
			switch (c) {
				case U'\t': {
					auto& g = font_->glyph(U' ');
					previous = U' ';
					pos.x += (g.w * TAB_WIDTH);
					break;
				}
				case U'\n': {
					previous = U'\0';
					pos.x = position_.x - origin_.x;
					pos.y += dim.y;
					break;
				}
				default: {
					auto& g = font_->glyph(c);
					const r32 k = font_->kerning(previous, c);
					previous = c;

					auto vtx = &vertices_[idx * display_list::QUAD];
					vtx[0].position = { pos.x + g.x_offset, pos.y + g.y_offset };
					vtx[0].index = g.channel;
					vtx[0].uvs = glm::vec2(g.x + off.x, g.y + off.y) / material::MAXIMUM_DIMENSIONS;
					vtx[0].atlas = atlas;
					vtx[0].color = color_;

					vtx[1].position = { pos.x + g.x_offset, pos.y + g.y_offset + g.h };
					vtx[1].index = g.channel;
					vtx[1].uvs = glm::vec2(g.x + off.x, g.y + g.h + off.y) / material::MAXIMUM_DIMENSIONS;
					vtx[1].atlas = atlas;
					vtx[1].color = color_;

					vtx[2].position = { pos.x + g.x_offset + g.w, pos.y + g.y_offset };
					vtx[2].index = g.channel;
					vtx[2].uvs = glm::vec2(g.x + g.w + off.x, g.y + off.y) / material::MAXIMUM_DIMENSIONS;
					vtx[2].atlas = atlas;
					vtx[2].color = color_;

					vtx[3].position = { pos.x + g.x_offset + g.w, pos.y + g.y_offset + g.h };
					vtx[3].index = g.channel;
					vtx[3].uvs = glm::vec2(g.x + g.w + off.x, g.y + g.h + off.y) / material::MAXIMUM_DIMENSIONS;
					vtx[3].atlas = atlas;
					vtx[3].color = color_;

					pos.x += (g.x_advance + k);
					++idx;
					break;
				}
			}
		}
	}
}

void gui::text::generate_attributes_() {
	if (!buffer_.empty() and font_ and !vertices_.empty()) {
		glm::vec2 pos = position_ - origin_;
		const glm::vec2 dim = font_->glyph_dimensions();
		char32_t previous = U'\0';
		udx idx = 0;
		for (auto&& c : buffer_) {
			switch (c) {
				case U'\t': {
					auto& g = font_->glyph(U' ');
					previous = U' ';
					pos.x += (g.w * TAB_WIDTH);
					break;
				}
				case U'\n': {
					previous = U'\0';
					pos.x = position_.x - origin_.x;
					pos.y += dim.y;
					break;
				}
				default: {
					auto& g = font_->glyph(c);
					const r32 k = font_->kerning(previous, c);
					previous = c;

					auto vtx = &vertices_[idx * display_list::QUAD];
					vtx[0].position = { pos.x + g.x_offset, pos.y + g.y_offset };
					vtx[0].color = color_;
					vtx[1].position = { pos.x + g.x_offset, pos.y + g.y_offset + g.h };
					vtx[1].color = color_;
					vtx[2].position = { pos.x + g.x_offset + g.w, pos.y + g.y_offset };
					vtx[2].color = color_;
					vtx[3].position = { pos.x + g.x_offset + g.w, pos.y + g.y_offset + g.h };
					vtx[3].color = color_;

					pos.x += (g.x_advance + k);
					++idx;
					break;
				}
			}
		}
	}
}

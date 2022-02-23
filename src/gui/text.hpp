#pragma once

#include <string>
#include <apostellein/rect.hpp>

#include "../video/vertex.hpp"

struct bitmap_font;
struct renderer;

namespace gui {
	// utf-8 to utf-32
	void unicode(const std::string& utf8, std::u32string& utf32);

	struct text : public not_copyable {
	public:
		void build(
			const glm::vec2& position,
			const glm::vec2& origin,
			const chroma& color,
			const bitmap_font* font,
			const std::string& words
		);
		void clear();
		void invalidate() const { invalidated_ = true; }
		void fix(const bitmap_font* font) {
			invalidated_ = true;
			font_ = font;
			this->generate_quads_();
		}
		void handle() {
			if (!this->finished()) {
				invalidated_ = true;
				++letter_;
			}
		}
		void render(renderer& rdr) const;
		void append(const std::string& words, bool immediate = true) {
			invalidated_ = true;
			gui::unicode(words, buffer_);
			const auto length = this->drawable();
			if (immediate or letter_ > length) {
				letter_ = length;
			}
			this->generate_quads_();
		}
		void replace(const std::string& words, bool immediate = true) {
			buffer_.clear();
			this->append(words, immediate);
		}
		void append(const std::u32string& words, bool immediate = true) {
			invalidated_ = true;
			buffer_.append(words);
			const auto length = this->drawable();
			if (immediate or letter_ > length) {
				letter_ = length;
			}
			this->generate_quads_();
		}
		void replace(const std::u32string& words, bool immediate = true) {
			invalidated_ = true;
			buffer_ = words;
			const auto length = this->drawable();
			if (immediate or letter_ > length) {
				letter_ = length;
			}
			this->generate_quads_();
		}
		void forward(std::u32string&& words, bool immediate = true) {
			invalidated_ = true;
			buffer_ = std::move(words);
			const auto length = this->drawable();
			if (immediate or letter_ > length) {
				letter_ = length;
			}
			this->generate_quads_();
		}
		void position(glm::vec2 value);
		void position(r32 x, r32 y) {
			const glm::vec2 p { x, y };
			this->position(p);
		}
		void origin(glm::vec2 value);
		void origin(r32 x, r32 y) {
			const glm::vec2 o { x, y };
			this->origin(o);
		}
		void color(const chroma& value) {
			if (color_ != value) {
				invalidated_ = true;
				color_ = value;
				this->generate_attributes_();
			}
		}
		const glm::vec2& position() const { return position_; }
		const glm::vec2& origin() const { return origin_; }
		const chroma& color() const { return color_; }
		glm::vec2 font_dimensions() const;
		const bitmap_font* font() const { return font_; }
		rect bounds() const;
		bool finished() const { return letter_ >= this->drawable(); }
		udx drawable() const;
	private:
		void generate_quads_();
		void generate_attributes_();
		mutable bool invalidated_ {};
		glm::vec2 position_ {};
		glm::vec2 origin_ {};
		chroma color_ { chroma::WHITE() };
		udx letter_ {};
		std::u32string buffer_ {};
		const bitmap_font* font_ {};
		std::vector<vtx_sprite> vertices_ {};
	};
}

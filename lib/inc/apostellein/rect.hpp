#pragma once

#include <glm/vec2.hpp>
#include <apostellein/def.hpp>

// Scoped enum with methods
// Yes, I know it's dumb
struct side_type {
	enum {
		right,
		left,
		top,
		bottom
	};
	constexpr side_type() noexcept = default;
	constexpr side_type(decltype(right) value) noexcept :
		value_{ value } {}
	constexpr side_type& operator=(decltype(right) value) noexcept {
		value_ = value;
		return *this;
	}
public:
	constexpr operator decltype(right)() const noexcept { return value_; }
	constexpr bool horizontal() const noexcept { return value_ == right or value_ == left; }
	constexpr bool vertical() const noexcept { return !this->horizontal(); }
	constexpr bool maximum() const noexcept { return value_ == right or value_ == bottom; }
	constexpr bool minimum() const noexcept { return !this->maximum(); }
	constexpr side_type opposite() const noexcept {
		switch (value_) {
		case side_type::right: return side_type::left;
		case side_type::left: return side_type::right;
		case side_type::top: return side_type::bottom;
		default: return side_type::top;
		}
	}
private:
	decltype(right) value_ { right };
};

struct rect {
	constexpr rect() noexcept = default;
	constexpr rect(r32 _x, r32 _y, r32 _w, r32 _h) noexcept :
		x{ _x }, y{ _y }, w{ _w }, h{ _h } {}
	constexpr rect(const glm::vec2& pos, const glm::vec2& dim) noexcept :
		x{ pos.x }, y{ pos.y }, w{ dim.x }, h{ dim.y } {}
	constexpr rect(const glm::vec2& dim) noexcept :
		w{ dim.x }, h{ dim.y } {}

	r32 x {};
	r32 y {};
	r32 w {};
	r32 h {};
public:
	constexpr bool operator==(const rect& that) const noexcept {
		return (
			this->x == that.x and
			this->y == that.y and
			this->w == that.w and
			this->h == that.h
		);
	}
	constexpr bool operator!=(const rect& that) const noexcept {
		return !(*this == that);
	}
	constexpr r32 right() const noexcept { return x + w; }
	constexpr r32 left() const noexcept { return x; }
	constexpr r32 top() const noexcept { return y; }
	constexpr r32 bottom() const noexcept { return y + h; }
	constexpr r32 width() const noexcept { return w; }
	constexpr r32 height() const noexcept { return h; }
	constexpr r32 side(side_type s) const noexcept {
		switch (s) {
		case side_type::right: return this->right();
		case side_type::left: return this->left();
		case side_type::top: return this->top();
		default: return this->bottom();
		}
	}
	constexpr glm::vec2 left_top() const noexcept { return { x, y }; }
	constexpr glm::vec2 left_bottom() const noexcept { return { x, y + h }; }
	constexpr glm::vec2 right_top() const noexcept { return { x + w, y }; }
	constexpr glm::vec2 right_bottom() const noexcept { return { x + w, y + h }; }
	constexpr glm::vec2 position() const noexcept { return this->left_top(); }
	constexpr glm::vec2 dimensions() const noexcept { return { w, h }; }
	constexpr glm::vec2 center() const noexcept { return { x + (w / 2.0f), y + (h / 2.0f) }; }
	constexpr bool overlaps(const rect& that) const noexcept {
		return (
			this->right() > that.x and
			this->x < that.right() and
			this->y < that.bottom() and
			this->bottom() > that.y
		);
	}
};

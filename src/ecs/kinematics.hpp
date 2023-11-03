#pragma once

#include <apostellein/rect.hpp>
#include <apostellein/struct.hpp>

struct tile_map;
struct renderer;
struct environment;

struct composite_rect {
	constexpr composite_rect() noexcept = default;
	constexpr composite_rect(
		const rect& left,
		const rect& right,
		const rect& top,
		const rect& bottom
	) noexcept : left_{ left }, right_{ right }, top_{ top }, bottom_{ bottom } {}
public:
	constexpr rect bounds() const noexcept {
		return {
			left_.x,
			top_.y,
			left_.w + right_.w,
			top_.h + bottom_.h
		};
	}
	constexpr rect left(const glm::vec2& position, r32 inertia) const noexcept {
		return {
			position.x + left_.x + inertia,
			position.y + left_.y,
			left_.w - inertia,
			left_.h
		};
	}
	constexpr rect right(const glm::vec2& position, r32 inertia) const noexcept {
		return {
			position.x + right_.x,
			position.y + right_.y,
			right_.w + inertia,
			right_.h
		};
	}
	constexpr rect top(const glm::vec2& position, r32 inertia) const noexcept {
		return {
			position.x + top_.x,
			position.y + top_.y + inertia,
			top_.w,
			top_.h - inertia
		};
	}
	constexpr rect bottom(const glm::vec2& position, r32 inertia) const noexcept {
		return {
			position.x + bottom_.x,
			position.y + bottom_.y,
			bottom_.w,
			bottom_.h + inertia
		};
	}
	constexpr rect side(side_type side, const glm::vec2& position, r32 inertia) const noexcept {
		switch (side) {
		case side_type::right: return this->right(position, inertia);
		case side_type::left: return this->left(position, inertia);
		case side_type::top: return this->top(position, inertia);
		default: return this->bottom(position, inertia);
		}
	}
private:
	rect left_ {};
	rect right_ {};
	rect top_ {};
	rect bottom_ {};
};

namespace ecs {
	struct location;
	struct trigger;
	struct anchor;

	struct kinematics {
		kinematics() noexcept = default;
		kinematics(const glm::vec2& _velocity) noexcept :
			velocity{ _velocity } {}
		union {
			bitfield_raw<u32> _raw {};
			bitfield_index<u32, 0> right;
			bitfield_index<u32, 1> left;
			bitfield_index<u32, 2> top;
			bitfield_index<u32, 3> bottom;
			bitfield_index<u32, 4> sloped;
			bitfield_index<u32, 5> no_clip;
			bitfield_index<u32, 6> out_of_bounds;
			bitfield_index<u32, 7> will_drop;
			bitfield_index<u32, 8> fall_through;
			bitfield_index<u32, 9> hooked;
			bitfield_index<u32, 10> constrained;
		} flags {};
		glm::vec2 velocity {};
		composite_rect hitbox {};
	public:
		void clear() {
			flags._raw = {};
			velocity = {};
		}
		bool hori_sides() const {
			static const u32 HORI_MASK = []{
				decltype(flags) r {};
				r.right = true;
				r.left = true;
				return r._raw.value();
			}();
			return flags._raw.value() & HORI_MASK;
		}
		bool vert_sides() const {
			static const u32 VERT_MASK = []{
				decltype(flags) r {};
				r.top = true;
				r.bottom = true;
				return r._raw.value();
			}();
			return flags._raw.value() & VERT_MASK;
		}
		bool any_side() const {
			static const u32 SIDE_MASK = []{
				decltype(flags) r {};
				r.right = true;
				r.left = true;
				r.top = true;
				r.bottom = true;
				return r._raw.value();
			}();
			return flags._raw.value() & SIDE_MASK;
		}
		r32 derive_angle() const;
		void move_at(r32 angle, r32 speed);
		void accel_x(r32 speed, r32 limit, r32 detract);
		void accel_x(r32 speed, r32 limit) {
			this->accel_x(speed, limit, speed);
		}
		void accel_y(r32 speed, r32 limit, r32 detract);
		void accel_y(r32 speed, r32 limit) {
			this->accel_y(speed, limit, speed);
		}
		void decel_x(r32 rate, r32 zero = 0.0f);
		void decel_y(r32 rate, r32 zero = 0.0f);
		void extend(ecs::location& loc, const tile_map& map, glm::vec2 inertia) {
			if (inertia.x != 0.0f) {
				this->try_x_(loc, map, inertia.x);
			}
			if (inertia.y != 0.0f) {
				this->try_y_(loc, map, inertia.y);
			}
		}
		static void handle(environment& env, const tile_map& map);
		static void render(renderer& rdr, const environment& env);
	private:
		void try_x_(ecs::location& loc, const tile_map& map, r32 inertia);
		void try_y_(ecs::location& loc, const tile_map& map, r32 inertia);
		void try_a_(ecs::location& loc, ecs::anchor& anc, glm::vec2& inertia);
	};

	struct anchor {
		glm::vec2 position {};
		r32 length {};
	public:
		void clear() {
			position = {};
			length = 0.0f;
		}
	};
}

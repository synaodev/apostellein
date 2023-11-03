#pragma once

#include <array>
#include <entt/core/hashed_string.hpp>
#include <apostellein/rect.hpp>
#include <apostellein/struct.hpp>

struct renderer;
struct environment;

namespace ecs {
	struct aktor {
		aktor() noexcept = default;
		aktor(const entt::hashed_string& _type) noexcept :
			type{ _type } {}

		entt::hashed_string type {};
	};

	enum class direction {
		right,
		left,
		up,
		down
	};

	struct location {
		static constexpr rect DEFAULT_HITBOX { 0.0f, 0.0f, 16.0f, 16.0f };
		location() noexcept = default;
		location(const glm::vec2& _position) noexcept :
			position{ _position } {}

		glm::vec2 position {};
		rect hitbox { DEFAULT_HITBOX };
		direction dir { direction::right };
	public:
		void clear() {
			position = {};
			hitbox = DEFAULT_HITBOX;
		}
		glm::vec2 center() const {
			return position + hitbox.center();
		}
		rect bounds() const {
			return {
				position + hitbox.position(),
				hitbox.dimensions()
			};
		}
		bool overlaps(const location& that) const {
			return this->bounds().overlaps(that.bounds());
		}
		bool overlaps(const rect& that) const {
			return this->bounds().overlaps(that);
		}
		static void render(const rect& view, renderer& rdr, const environment& env);
	};

	struct trigger {
		trigger() noexcept = default;
		trigger(i32 _id, u32 mask) noexcept :
			id{ _id } { flags._raw = mask; }
		union {
			bitfield_raw<u32> _raw {};
			bitfield_index<u32, 0> deterred;
			bitfield_index<u32, 1> interaction;
			bitfield_index<u32, 2> destruction;
			bitfield_index<u32, 3> facing_left;
		} flags {};
		i32 id {};
	public:
		void clear() {
			flags._raw = {};
			id = 0;
		}
		static bool will_deter(u32 mask) {
			static const u32 DETER_MASK = []{
				decltype(flags) r {};
				r.deterred = true;
				return r._raw.value();
			}();
			return mask & DETER_MASK;
		}
		static bool will_face_left(u32 mask) {
			static const u32 FACING_LEFT_MASK = []{
				decltype(flags) r {};
				r.facing_left = true;
				return r._raw.value();
			}();
			return mask & FACING_LEFT_MASK;
		}
	};

	using chroniker = std::array<i32, 4>;
}

#pragma once

#include <glm/vec2.hpp>

#include "../x2d/mirror-type.hpp"

struct renderer;
struct animation_group;

namespace gui {
	struct scheme {
		void build(
			const glm::vec2& position,
			udx state,
			udx variation,
			const animation_group* file
		);
		void invalidate() const { invalidated_ = true; }
		void fix() { this->invalidate(); }
		void update(i64 delta);
		void render(renderer& rdr) const;
		void position(const glm::vec2& value) {
			if (position_ != value) {
				position_ = value;
				invalidated_ = true;
			}
		}
		void position(r32 x, r32 y) {
			const glm::vec2 p { x, y };
			this->position(p);
		}
		void transform(const glm::vec2& delta) {
			position_ += delta;
			if (delta.x != 0.0f or delta.y != 0.0f) {
				invalidated_ = true;
			}
		}
		void transform(r32 x, r32 y) {
			const glm::vec2 delta { x, y };
			this->transform(delta);
		}
		void state(udx value) {
			if (state_ != value) {
				state_ = value;
				timer_ = 0;
				frame_ = 0;
				invalidated_ = true;
			}
		}
		void variation(udx value) {
			if (variation_ != value) {
				variation_ = value;
				invalidated_ = true;
			}
		}
		void frame(udx value) {
			if (frame_ != value) {
				frame_ = value;
				timer_ = 0;
				invalidated_ = true;
			}
		}
		const glm::vec2& position() const { return position_; }
		i64 timer() const { return timer_; }
		udx state() const { return state_; }
		udx variation() const { return variation_; }
		udx frame() const { return frame_; }
		bool finished() const;
	private:
		mutable bool invalidated_ {};
		const animation_group* file_ {};
		udx state_ {};
		i64 timer_ {};
		udx variation_ {};
		udx frame_ {};
		glm::vec2 position_ {};
	};
}

#pragma once

#include <glm/vec2.hpp>
#include <entt/core/hashed_string.hpp>
#include <apostellein/struct.hpp>

#include "../x2d/mirror-type.hpp"

struct rect;
struct renderer;
struct animation_group;
struct environment;

namespace ecs {
	struct sprite : public not_copyable {
		sprite() noexcept = default;
		sprite(const entt::hashed_string& name);
		sprite(sprite&& that) noexcept {
			*this = std::move(that);
		}
		sprite& operator=(sprite&& that) noexcept {
			if (this != &that) {
				file_ = that.file_;
				that.file_ = nullptr;
				state_ = that.state_;
				that.state_ = 0;
				previous_ = that.previous_;
				that.previous_ = {};
				current_ = that.current_;
				that.current_ = {};
				timer = that.timer;
				that.timer = 0;
				variation = that.variation;
				that.variation = 0;
				frame = that.frame;
				that.frame = 0;
				color = that.color;
				that.color = chroma::WHITE();
				layer = that.layer;
				that.layer = 0;
				mirror = that.mirror;
				that.mirror = {};
				scale = that.scale;
				that.scale = { 1.0f, 1.0f };
				pivot = that.pivot;
				that.pivot = {};
				angle = that.angle;
				that.angle = 0.0f;
				shake = that.shake;
				that.shake = 0.0f;
			}
			return *this;
		}
	public:
		void clear() {
			state_ = 0;
			current_ = {};
			timer = 0;
			variation = 0;
			frame = 0;
			color = chroma::WHITE();
			layer = 0;
			mirror = {};
			scale = { 1.0f, 1.0f };
			pivot = {};
			angle = 0.0f;
			shake = 0.0f;
		}
		void prepare(const glm::vec2& position) {
			previous_ = position;
			current_ = position;
		}
		void state(udx value) {
			if (state_ != value) {
				state_ = value;
				timer = 0;
				frame = 0;
			}
		}
		udx state() const { return state_; }
		bool finished() const;
		glm::vec2 action_point(udx state, const glm::vec2& position) const;
		static void prepare(environment& env);
		static void handle(environment& env);
		static void update(i64 delta, environment& env);
		static void render(r32 ratio, const rect& view, renderer& rdr, const environment& env);
		static udx visible(r32 ration, const rect& view, const environment& env);
	private:
		// weirdly specific layout, I know
		const animation_group* file_ {};
		udx state_ {};
		glm::vec2 previous_ {};
		glm::vec2 current_ {};
	public:
		i64 timer {};
		udx variation {};
		udx frame {};
		chroma color { chroma::WHITE() };
		i32 layer {};
		mirror_type mirror {};
		glm::vec2 scale { 1.0f };
		glm::vec2 pivot {};
		r32 angle {};
		r32 shake {};
	};

	struct blinker {
		bool enabled { true };
		i32 ticks {};
		udx return_state {};
		udx blink_state {};
	public:
		void clear() {
			enabled = false;
			ticks = 0;
			return_state = 0;
			blink_state = 0;
		}
	};
}

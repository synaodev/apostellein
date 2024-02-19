#include <glm/gtc/matrix_transform.hpp>
#include <apostellein/konst.hpp>

#include "./camera.hpp"
#include "./environment.hpp"
#include "./player.hpp"
#include "../ecs/aktor.hpp"
#include "../hw/rng.hpp"

namespace  {
	constexpr glm::vec2 TOP_LEFT_CORNER { 8.0f, 4.0f };
	constexpr glm::vec2 DEFAULT_POSITION { (konst::WINDOW_DIMENSIONS<r32>() / 2.0f) + TOP_LEFT_CORNER };
	constexpr glm::vec2 DEFAULT_WEIGHT { 1.0f / 16.0f, 1.0f / 8.0f };
	constexpr glm::vec3 DEFAULT_AXIS { 0.0f, 0.0f, 1.0f };

	glm::vec2 clamp_position_(const glm::vec2& center, const glm::vec2& dimensions, const rect& limit) {
		glm::vec2 point = center - (dimensions / 2.0f);
		if (point.x < limit.x) {
			point.x = limit.x;
		}
		if (point.y < limit.y) {
			point.y = limit.y;
		}
		if ((point.x + dimensions.x) > limit.right()) {
			point.x = limit.right() - dimensions.x;
		}
		if ((point.y + dimensions.y) > limit.bottom()) {
			point.y = limit.bottom() - dimensions.y;
		}
		return point + (dimensions / 2.0f);
	}
}

void camera::clear() {
	id_ = 0;
	cycling_ = false;
	indefinite_ = false;
	ticks_ = 0;
	limit_ = {
		TOP_LEFT_CORNER,
		konst::WINDOW_DIMENSIONS<r32>()
	};
	current_ = DEFAULT_POSITION;
	dimensions_ = konst::WINDOW_DIMENSIONS<r32>();
	offsets_ = {};
	power_ = 0.0f;
	tilt_ = 0.0f;
}

void camera::prepare() {
	previous_ = current_;
}

void camera::handle(const player& plr, const environment& env) {
	glm::vec2 point {};
	if (id_ > 0) {
		if (auto e = env.search(id_); e != entt::null) {
			point = env.get<ecs::location>(e).center();
		} else {
			id_ = 0;
		}
	} else {
		point = plr.viewpoint(env);
	}
	current_ = glm::mix(current_, point, DEFAULT_WEIGHT);
	current_ = clamp_position_(current_, dimensions_, limit_);
	if (power_ != 0.0f) {
		if (cycling_) {
			cycling_ = false;
			offsets_ = -offsets_;
			tilt_ = 0.0f;
		} else {
			cycling_ = true;
			offsets_ = {
				rng::effective::between(-power_, power_),
				rng::effective::between(-power_, power_)
			};
			tilt_ = glm::radians(
				rng::effective::between(-power_, power_)
			);
		}
		if (!indefinite_) {
			if (--ticks_; ticks_ <= 0) {
				cycling_ = false;
				ticks_ = 0;
				offsets_ = {};
				power_ = 0.0f;
				tilt_ = 0.0f;
			}
		}
	}
}

void camera::limit(const rect& bounds) {
	limit_ = {
		TOP_LEFT_CORNER,
		bounds.dimensions() - TOP_LEFT_CORNER * 2.0f
	};
	current_ = DEFAULT_POSITION;
	dimensions_ = konst::WINDOW_DIMENSIONS<r32>();
}

void camera::focus_on(const glm::vec2& point) {
	current_ = clamp_position_(point, dimensions_, limit_);
}

void camera::quake(r32 power, i32 ticks) {
	if (!indefinite_) {
		cycling_ = false;
		ticks_ = ticks;
		power_ = power;
		tilt_ = 0.0f;
	}
}

void camera::quake(r32 power) {
	if (power == 0.0f) {
		cycling_ = false;
		indefinite_ = false;
		ticks_ = 0;
		offsets_ = {};
		power_ = 0.0f;
		tilt_ = 0.0f;
	} else {
		cycling_ = false;
		indefinite_ = true;
		ticks_ = 0;
		offsets_ = {};
		power_ = power;
		tilt_ = 0.0f;
	}
}

rect camera::view(r32 ratio) const {
	const glm::vec2 position = konst::INTERPOLATE(
		previous_,
		current_,
		ratio
	);
	return {
		position - (dimensions_ / 2.0f),
		dimensions_
	};
}

glm::mat4 camera::matrix(r32 ratio) const {
	glm::mat4 result { 1.0f };
	result = glm::scale(
		result,
		glm::vec3 {
			2.0f / dimensions_.x,
			2.0f / -dimensions_.y,
			0.0f
		}
	);
	if (tilt_ != 0.0f) {
		result = glm::rotate(result, tilt_, DEFAULT_AXIS);
	}
	const glm::vec2 position = konst::INTERPOLATE(
		previous_,
		current_,
		ratio
	);
	return glm::translate(
		result,
		-glm::vec3 {
			position + offsets_,
			0.0f
		}
	);
}

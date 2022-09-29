#include <apostellein/konst.hpp>

#include "./sprite.hpp"
#include "./aktor.hpp"
#include "../field/environment.hpp"
#include "../hw/vfs.hpp"
#include "../hw/rng.hpp"
#include "../x2d/animation-group.hpp"

namespace {
	constexpr r32 SHAKING_AMOUNT = 0.01f;
	constexpr i32 BLINKING_INTERVAL = 6;
	constexpr i32 MINIMUM_BLINKING = BLINKING_INTERVAL * 5;
	constexpr i32 MAXIMUM_BLINKING = BLINKING_INTERVAL * 30;
}

ecs::sprite::sprite(const entt::hashed_string& data) {
	file_ = vfs::find_animation(data);
	previous_ = {};
	current_ = {};
}

bool ecs::sprite::finished() const {
	if (file_) {
		return file_->finished(state_, frame, timer);
	}
	return true;
}

glm::vec2 ecs::sprite::action_point(udx state, const glm::vec2& position) const {
	if (file_) {
		return position + file_->action_point(state, variation, mirror);
	}
	return position;
}

void ecs::sprite::prepare(environment& env) {
	env.slice<ecs::sprite>().each([](entt::entity, ecs::sprite& spt) {
		spt.previous_ = spt.current_;
	});
}

void ecs::sprite::handle(environment& env) {
	env.slice<ecs::location, ecs::sprite>().each(
	[&env](entt::entity e, const ecs::location& loc, ecs::sprite& spt) {
		spt.current_ = loc.position;
		if (spt.shake != 0.0f) {
			spt.shake = -spt.shake;
			spt.shake = spt.shake > 0.0f ?
				glm::max(0.0f, spt.shake - SHAKING_AMOUNT) :
				glm::min(0.0f, spt.shake + SHAKING_AMOUNT);
		}
		if (env.has<ecs::blinker>(e)) {
			if (auto& blk = env.get<ecs::blinker>(e); blk.enabled) {
				if (const udx s = spt.state(); s == blk.return_state) {
					if (--blk.ticks; blk.ticks <= 0) {
						blk.ticks = BLINKING_INTERVAL;
						spt.state(blk.blink_state);
					}
				} else if (s == blk.blink_state) {
					if (--blk.ticks; blk.ticks <= 0) {
						blk.ticks = rng::next(MINIMUM_BLINKING, MAXIMUM_BLINKING);
						spt.state(blk.return_state);
					}
				}
			}
		}
	});
}

void ecs::sprite::update(i64 delta, environment& env) {
	env.slice<ecs::sprite>().each([delta](entt::entity, ecs::sprite& spt) {
		if (spt.file_) {
			spt.file_->update(
				delta,
				spt.state_,
				spt.timer,
				spt.frame
			);
		}
	});
}

void ecs::sprite::render(r32 ratio, const rect& view, renderer& rdr, const environment& env) {
	env.slice<ecs::sprite>().each(
	[&ratio, &view, &rdr, &env](entt::entity, const ecs::sprite& spt) {
		if (spt.file_ and spt.color.a > 0x00U) {
			const glm::vec2 position = konst::INTERPOLATE(
				spt.previous_,
				spt.current_,
				ratio
			);
			if (spt.angle != 0.0f) {
				spt.file_->render(
					spt.state_,
					spt.frame,
					spt.variation,
					spt.mirror,
					spt.color,
					spt.scale,
					spt.angle + spt.shake,
					spt.pivot,
					position,
					view,
					rdr
				);
			} else {
				spt.file_->render(
					spt.state_,
					spt.frame,
					spt.variation,
					spt.mirror,
					spt.color,
					spt.scale,
					position,
					view,
					rdr
				);
			}
		}
	});
}

udx ecs::sprite::visible(r32 ratio, const rect& view, const environment& env) {
	udx count = 0;
	env.slice<ecs::sprite>().each(
	[&ratio, &count, &view, &env](entt::entity, const ecs::sprite& spt) {
		if (spt.file_ and spt.color.a > 0x00U) {
			const glm::vec2 position = konst::INTERPOLATE(
				spt.previous_,
				spt.current_,
				ratio
			);
			if (spt.angle != 0.0f) {
				if (spt.file_->visible(
					spt.state_,
					spt.frame,
					spt.variation,
					spt.mirror,
					spt.scale,
					spt.angle + spt.shake,
					spt.pivot,
					position,
					view
				)) {
					++count;
				}
			} else if (spt.file_->visible(
				spt.state_,
				spt.frame,
				spt.variation,
				spt.mirror,
				spt.scale,
				position,
				view
			)) {
				++count;
			}
		}
	});
	return count;
}

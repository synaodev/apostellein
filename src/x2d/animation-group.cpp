#include <spdlog/spdlog.h>
#include <glm/trigonometric.hpp>
#include <glm/gtc/constants.hpp>
#include <apostellein/konst.hpp>
#include <apostellein/cast.hpp>

#include "./animation-group.hpp"
#include "./renderer.hpp"
#include "./mirror-type.hpp"
#include "../hw/vfs.hpp"
#include "../video/material.hpp"

namespace {
	constexpr char MATERIAL_ENTRY[] = "Material";
	constexpr char ANIMATIONS_ENTRY[] = "Animations";
	constexpr char STARTS_ENTRY[] = "starts";
	constexpr char VKSIZE_ENTRY[] = "vksize";
	constexpr char TDELAY_ENTRY[] = "tdelay";
	constexpr char REPEAT_ENTRY[] = "repeat";
	constexpr char REFLECT_ENTRY[] = "reflect";
	constexpr char ACTION_ENTRY[] = "action";
	constexpr char FRAMES_ENTRY[] = "frames";
}

animation_raster::animation_raster(const glm::vec2& position, const glm::vec2& dimensions) noexcept {
	bounds = { position, dimensions };
	points = {
		position,
		{ position.x, position.y + dimensions.y },
		{ position.x + dimensions.x, position.y },
		position + dimensions
	};
}

animation_raster::animation_raster(const rect& quad, r32 angle, const glm::vec2& about) noexcept {
	auto find_extremes = [this](const auto& func) {
		glm::vec2 result {};
		for (auto&& point : this->points) {
			for (glm::length_t idx = 0; idx < point.length(); ++idx) {
				if (func(point[idx], result[idx])) {
					result[idx] = point[idx];
				}
			}
		}
		return result;
	};
	const glm::vec2 low_diff = quad.left_top() - about;
	const glm::vec2 high_diff = quad.right_bottom() - about;
	const glm::vec2 normal {
		glm::cos(angle),
		glm::sin(angle)
	};
	points = {
		glm::vec2 {
			about.x + low_diff.x * normal.x + low_diff.y * normal.y,
			about.y - low_diff.x * normal.y + low_diff.y * normal.x
		},
		glm::vec2 {
			about.x + low_diff.x * normal.x + high_diff.y * normal.y,
			about.y - low_diff.x * normal.y + high_diff.y * normal.x
		},
		glm::vec2 {
			about.x + high_diff.x * normal.x + low_diff.y * normal.y,
			about.y - high_diff.x * normal.y + low_diff.y * normal.x
		},
		glm::vec2 {
			about.x + high_diff.x * normal.x + high_diff.y * normal.y,
			about.y - high_diff.x * normal.y + high_diff.y * normal.x
		}
	};
	const glm::vec2 minimum = find_extremes(std::less<r32>{});
	const glm::vec2 maximum = find_extremes(std::greater<r32>{});
	bounds = {
		minimum.x,
		minimum.y,
		maximum.x - minimum.x,
		maximum.y - minimum.y
	};
}

void animation_sequence::append(const glm::vec2& action_point) {
	action_points_.push_back(action_point);
}

void animation_sequence::append(const glm::vec2& start, const glm::vec4& points) {
	const glm::vec2 position = start + (glm::vec2{ points[0], points[1] } * dimensions_);
	const glm::vec2 origin { points[2], points[3] };
	frames_.emplace_back(position, origin);
}

const animation_frame& animation_sequence::frame_with(udx frame, udx variation) const {
	const auto idx = frame + (variation * count_);
	if (idx < frames_.size()) {
		return frames_[idx];
	}
	static const animation_frame NULL_FRAME {};
	return NULL_FRAME;
}

rect animation_sequence::quad_with(udx frame, udx variation) const {
	const auto idx = frame + (variation * count_);
	if (idx < frames_.size()) {
		return this->unsafe_quad_with(idx);
	}
	return {
		glm::zero<glm::vec2>(),
		dimensions_
	};
}

rect animation_sequence::unsafe_quad_with(udx idx) const {
	return {
		frames_[idx].position,
		dimensions_
	};
}

animation_raster animation_sequence::raster_with(
	udx frame,
	udx variation,
	const mirror_type& mirror,
	const glm::vec2& scale,
	const glm::vec2& position
) const {
	const auto idx = frame + (variation * count_);
	if (idx < frames_.size()) {
		const glm::vec2 hotspot = position - this->unsafe_origin_with(idx, mirror) * scale;
		return {
			hotspot,
			dimensions_ * scale
		};
	}
	return {
		position,
		dimensions_ * scale
	};
}

animation_raster animation_sequence::raster_with(
	udx frame,
	udx variation,
	const mirror_type& mirror,
	const glm::vec2& scale,
	r32 angle,
	const glm::vec2& pivot,
	const glm::vec2& position
) const {
	const auto idx = frame + (variation * count_);
	if (idx < frames_.size()) {
		const glm::vec2 hotspot = position - this->unsafe_origin_with(idx, mirror) * scale;
		return {
			{ hotspot, dimensions_ * scale },
			angle,
			hotspot + pivot * scale
		};
	}
	return {
		position,
		dimensions_ * scale
	};
}

glm::vec2 animation_sequence::origin_with(udx frame, udx variation, const mirror_type& mirror) const {
	const auto idx = frame + (variation * count_);
	if (idx < frames_.size()) {
		return this->unsafe_origin_with(idx, mirror);
	}
	return {};
}

glm::vec2 animation_sequence::unsafe_origin_with(udx idx, const mirror_type& mirror) const {
	glm::vec2 result = frames_[idx].origin;
	if (reflecting_) {
		if (mirror.horizontally) {
			result.x = -result.x;
		}
		if (mirror.vertically) {
			result.y = -result.y;
		}
	}
	return result;
}

glm::vec2 animation_sequence::action_point_with(udx variation, const mirror_type& mirror) const {
	if (variation < action_points_.size()) {
		glm::vec2 action_point = action_points_[variation];
		if (mirror.horizontally) {
			const auto center = dimensions_.x / 2.0f;
			const auto distance = glm::abs(action_point.x - center);
			action_point.x = action_point.x > center ?
				glm::round(action_point.x - distance * 2.0f) :
				glm::round(action_point.x + distance * 2.0f);
		}
		if (mirror.vertically) {
			const auto center = dimensions_.y / 2.0f;
			const auto distance = glm::abs(action_point.y - center);
			action_point.y = action_point.y > center ?
				glm::round(action_point.y - distance * 2.0f) :
				glm::round(action_point.y + distance * 2.0f);
		}
		return action_point;
	}
	return {};
}

void animation_sequence::update(i64 delta, i64& timer, udx& frame) const {
	if (delay_ >= 2 and count_ > 1) {
		if (repeating_) {
			if (timer += delta; timer >= delay_) {
				timer %= delay_;
				++frame;
				frame %= count_;
			}
		} else {
			if (frame < (count_ - 1)) {
				if (timer += delta; timer >= delay_) {
					timer %= delay_;
					++frame;
				}
			} else if (timer <= delay_) {
				timer += delta;
			}
		}
	} else {
		timer = 0;
		frame = 0;
	}
}

void animation_sequence::update(i64 delta, bool& invalidated, i64& timer, udx& frame) const {
	if (delay_ >= 2 and count_ > 1) {
		if (repeating_) {
			if (timer += delta; timer >= delay_) {
				invalidated = true;
				timer %= delay_;
				++frame;
				frame %= count_;
			}
		} else {
			if (frame < (count_ - 1)) {
				if (timer += delta; timer >= delay_) {
					invalidated = true;
					timer %= delay_;
					++frame;
				}
			} else if (timer <= delay_) {
				timer += delta;
			}
		}
	} else {
		timer = 0;
		frame = 0;
	}
}

bool animation_sequence::finished(udx frame, i64 timer) const {
	if (frame >= (count_ - 1)) {
		return timer > delay_;
	}
	return false;
}

void animation_group::update(i64 delta, udx state, i64& timer, udx& frame) const {
	if (state < sequences_.size()) {
		sequences_[state].update(delta, timer, frame);
	}
}

void animation_group::update(i64 delta, bool& invalidated, udx state, i64& timer, udx& frame) const {
	if (state < sequences_.size()) {
		sequences_[state].update(delta, invalidated, timer, frame);
	}
}

void animation_group::render(
	udx state,
	udx frame,
	udx variation,
	const mirror_type& mirror,
	const chroma& color,
	const glm::vec2& scale,
	r32 angle,
	const glm::vec2& pivot,
	const glm::vec2& position,
	const rect& view,
	renderer& rdr
) const {
	if (texture_ and state < sequences_.size()) {
		auto& sequence = sequences_[state];
		const animation_raster raster = sequence.raster_with(
			frame,
			variation,
			mirror,
			scale,
			angle,
			pivot,
			position
		);
		if (view.overlaps(raster.bounds)) {
			const rect quad = sequence.quad_with(frame, variation);
			auto& list = rdr.query(
				priority_type::automatic,
				blending_type::alpha,
				pipeline_type::sprite
			);
			list.batch_sprite(raster.points, quad, *texture_, mirror, color);
		}
	}
}

bool animation_group::visible(
	udx state,
	udx frame,
	udx variation,
	const mirror_type& mirror,
	const glm::vec2& scale,
	r32 angle,
	const glm::vec2& pivot,
	const glm::vec2& position,
	const rect& view
) const {
	if (texture_ and state < sequences_.size()) {
		auto& sequence = sequences_[state];
		const animation_raster raster = sequence.raster_with(
			frame,
			variation,
			mirror,
			scale,
			angle,
			pivot,
			position
		);
		return view.overlaps(raster.bounds);
	}
	return false;
}

void animation_group::render(
	udx state,
	udx frame,
	udx variation,
	const mirror_type& mirror,
	const chroma& color,
	const glm::vec2& scale,
	const glm::vec2& position,
	const rect& view,
	renderer& rdr
) const {
	if (texture_ and state < sequences_.size()) {
		auto& sequence = sequences_[state];
		const animation_raster raster = sequence.raster_with(
			frame,
			variation,
			mirror,
			scale,
			position
		);
		if (view.overlaps(raster.bounds)) {
			const rect quad = sequence.quad_with(frame, variation);
			auto& list = rdr.query(
				priority_type::automatic,
				blending_type::alpha,
				pipeline_type::sprite
			);
			list.batch_sprite(raster.points, quad, *texture_, mirror, color);
		}
	}
}

bool animation_group::visible(
	udx state,
	udx frame,
	udx variation,
	const mirror_type& mirror,
	const glm::vec2& scale,
	const glm::vec2& position,
	const rect& view
) const {
	if (texture_ and state < sequences_.size()) {
		auto& sequence = sequences_[state];
		const animation_raster raster = sequence.raster_with(
			frame,
			variation,
			mirror,
			scale,
			position
		);
		return view.overlaps(raster.bounds);
	}
	return false;
}

void animation_group::render(
	bool& invalidated,
	udx state,
	udx frame,
	udx variation,
	const glm::vec2& position,
	renderer& rdr
) const {
	if (texture_ and state < sequences_.size()) {
		auto& list = rdr.query(
			priority_type::deferred,
			blending_type::alpha,
			pipeline_type::sprite
		);
		if (invalidated) {
			invalidated = false;

			auto& sequence = sequences_[state];
			const glm::vec2 dimensions = sequence.dimensions();
			const glm::vec2 origin = sequence.origin_with(frame, variation, {});
			const rect quad = sequence.quad_with(frame, variation);
			list.batch_sprite(position - origin, dimensions, quad, *texture_);
		} else {
			list.skip(display_list::QUAD);
		}
	}
}

void animation_group::load(const std::string& path) {
	if (!sequences_.empty()) {
		spdlog::warn("Tried to overwrite animation!");
		return;
	}

	const auto file = vfs::buffer_json(path);
	if (file.empty()) {
		spdlog::error("Failed to load animation from {}!", path);
		return;
	}

	glm::vec2 dimensions {};
	if (file.contains(MATERIAL_ENTRY) and file[MATERIAL_ENTRY].is_string()) {
		texture_ = vfs::find_material(file[MATERIAL_ENTRY].get<std::string>());
		dimensions = texture_->dimensions();
	}

	for (auto&& anim : file[ANIMATIONS_ENTRY]) {
		glm::vec2 starts {};
		if (
			anim.contains(STARTS_ENTRY) and
			anim[STARTS_ENTRY].is_array() and
			anim[STARTS_ENTRY].size() >= 2 and
			anim[STARTS_ENTRY][0].is_number() and
			anim[STARTS_ENTRY][1].is_number()
		) {
			starts = {
				anim[STARTS_ENTRY][0].get<r32>(),
				anim[STARTS_ENTRY][1].get<r32>()
			};
		}

		glm::vec2 vksize {};
		if (
			anim.contains(VKSIZE_ENTRY) and
			anim[VKSIZE_ENTRY].is_array() and
			anim[VKSIZE_ENTRY].size() >= 2 and
			anim[VKSIZE_ENTRY][0].is_number() and
			anim[VKSIZE_ENTRY][1].is_number()
		) {
			vksize = {
				anim[VKSIZE_ENTRY][0].get<r32>(),
				anim[VKSIZE_ENTRY][1].get<r32>()
			};
			vksize = glm::abs(vksize);
		}

		r64 tdelay = 0.0;
		if (
			anim.contains(TDELAY_ENTRY) and
			anim[TDELAY_ENTRY].is_number_float()
		) {
			tdelay = anim[TDELAY_ENTRY].get<r64>();
			tdelay = glm::abs(tdelay);
		}

		bool repeat = true;
		if (
			anim.contains(REPEAT_ENTRY) and
			anim[REPEAT_ENTRY].is_boolean()
		) {
			repeat = anim[REPEAT_ENTRY].get<bool>();
		}

		bool reflect = false;
		if (
			anim.contains(REFLECT_ENTRY) and
			anim[REFLECT_ENTRY].is_boolean()
		) {
			reflect = anim[REFLECT_ENTRY].get<bool>();
		}

		udx predict = 0;
		if (
			anim.contains(FRAMES_ENTRY) and
			anim[FRAMES_ENTRY].is_array() and
			anim[FRAMES_ENTRY].size() > 0 and
			anim[FRAMES_ENTRY][0].is_array()
		) {
			predict = anim[FRAMES_ENTRY][0].size();
		}

		if (predict == 0) {
			spdlog::error("Failed to load animation frames from {}!", path);
			return;
		}

		auto& sequence = sequences_.emplace_back(
			vksize,
			konst::SECONDS_TO_NANOSECONDS(tdelay),
			predict,
			repeat,
			reflect
		);

		if (
			anim.contains(ACTION_ENTRY) and
			anim[ACTION_ENTRY].is_array() and
			anim[ACTION_ENTRY].size() > 0
		) {
			for (auto&& action : anim[ACTION_ENTRY]) {
				glm::vec2 point {};
				if (
					action.is_array() and
					action.size() >= 2 and
					action[0].is_number() and
					action[1].is_number()
				) {
					point = {
						action[0].get<r32>(),
						action[1].get<r32>()
					};
				}
				sequence.append(point);
			}
		}

		for (auto&& frames : anim[FRAMES_ENTRY]) {
			for (auto&& rule : frames) {
				glm::vec4 points {};
				if (rule.is_array()) {
					const glm::length_t total = glm::min(
						points.length(),
						as<glm::length_t>(rule.size())
					);
					for (glm::length_t idx = 0; idx < total; ++idx) {
						if (rule[as<udx>(idx)].is_number()) {
							points[idx] = rule[as<udx>(idx)].get<r32>();
						}
					}
				}
				sequence.append(starts, points);
			}
		}
	}
}

bool animation_group::finished(udx state, udx frame, i64 timer) const {
	if (state < sequences_.size()) {
		return sequences_[state].finished(frame, timer);
	}
	return false;
}

glm::vec2 animation_group::origin(udx state, udx frame, udx variation, const mirror_type& mirror) const {
	if (state < sequences_.size()) {
		return sequences_[state].origin_with(frame, variation, mirror);
	}
	return {};
}

glm::vec2 animation_group::action_point(udx state, udx variation, const mirror_type& mirror) const {
	if (state < sequences_.size()) {
		return sequences_[state].action_point_with(variation, mirror);
	}
	return {};
}

#include <spdlog/spdlog.h>
#include <apostellein/cast.hpp>

#include "./barrier.hpp"
#include "../video/material.hpp"
#include "../x2d/renderer.hpp"

namespace {
	constexpr i32 HIGHEST_MAXIMUM = 16;
	constexpr i32 WRAPPING_INDEX = 7;
}

void gui::barrier::build(
	const glm::vec2& position,
	const rect& raster,
	i32 current,
	i32 maximum,
	i32 indent,
	const material* texture
) {
	invalidated_ = true;
	position_ = position;
	raster_ = raster;
	texture_ = texture;
	this->set(current, maximum, indent);
}

void gui::barrier::fix() {
	this->generate_quads_(
		as<udx>(current_),
		as<udx>(maximum_),
		false
	);
}

void gui::barrier::render(renderer& rdr) const {
	if (texture_ and !vertices_.empty()) {
		auto& list = rdr.query(
			priority_type::deferred,
			blending_type::alpha,
			pipeline_type::sprite
		);
		if (invalidated_) {
			invalidated_ = false;
			list.upload(vertices_);
		} else {
			list.skip(vertices_.size());
		}
	}
}

void gui::barrier::set(i32 current, i32 maximum, i32 indent) {
	// sanity checks
	if (current <= 0 or maximum <= 0) {
		spdlog::error("Barrier GUI values cannot be less than or equal to zero!");
		return;
	}
	if (current > maximum) {
		spdlog::error("Barrier GUI value must be less than or equal to maximum!");
		return;
	}
	if (maximum > HIGHEST_MAXIMUM) {
		spdlog::warn("Barrier GUI maximum will be truncated to {} units!", HIGHEST_MAXIMUM);
		maximum = HIGHEST_MAXIMUM;
	}
	bool changed = false;
	bool resized = false;
	if (maximum_ != maximum) {
		maximum_ = maximum;
		changed = true;
		resized = true;
	}
	if (current_ != current) {
		current_ = current;
		changed = true;
	}
	if (indent_ != indent) {
		indent_ = indent;
		changed = true;
	}
	if (changed) {
		this->generate_quads_(as<udx>(current_), as<udx>(maximum_), resized);
	}
}

void gui::barrier::generate_quads_(udx current_units, udx maximum_units, bool resized) {
	if (texture_) {
		invalidated_ = true;
		if (resized) {
			vertices_.resize(maximum_units * display_list::QUAD);
		}
		glm::vec2 pos = position_;
		const glm::vec2 ind { 0.0f, raster_.h * as<r32>(indent_) };
		const glm::vec2 off = texture_->offset();
		const auto atlas = texture_->atlas();
		for (udx idx = 0; idx < maximum_units; ++idx) {
			glm::vec2 uvs = raster_.left_top();
			if (current_units > 0 and idx <= (current_units - 1)) {
				uvs.x += raster_.w;
			}

			auto vtx = &vertices_[idx * display_list::QUAD];
			vtx[0].position = pos;
			vtx[0].index = 0;
			vtx[0].uvs = (uvs + ind + off) / material::MAXIMUM_DIMENSIONS;
			vtx[0].atlas = atlas;
			vtx[0].color = chroma::WHITE();

			vtx[1].position = { pos.x, pos.y + raster_.h };
			vtx[1].index = 0;
			vtx[1].uvs = glm::vec2(uvs.x + off.x + ind.x, uvs.y + off.y + ind.y + raster_.h) / material::MAXIMUM_DIMENSIONS;
			vtx[1].atlas = atlas;
			vtx[1].color = chroma::WHITE();

			vtx[2].position = { pos.x + raster_.w, pos.y };
			vtx[2].index = 0;
			vtx[2].uvs = glm::vec2(uvs.x + off.x + ind.x + raster_.w, uvs.y + off.y + ind.y) / material::MAXIMUM_DIMENSIONS;
			vtx[2].atlas = atlas;
			vtx[2].color = chroma::WHITE();

			vtx[3].position = pos + raster_.dimensions();
			vtx[3].index = 0;
			vtx[3].uvs = (uvs + off + ind + raster_.dimensions()) / material::MAXIMUM_DIMENSIONS;
			vtx[3].atlas = atlas;
			vtx[3].color = chroma::WHITE();

			if (idx != WRAPPING_INDEX) {
				pos.x += raster_.w;
			} else {
				pos.x = position_.x;
				pos.y += raster_.h;
			}
		}
	}
}

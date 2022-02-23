#include <spdlog/spdlog.h>

#include "./display-list.hpp"
#include "./mirror-type.hpp"
#include "../video/opengl.hpp"
#include "../video/material.hpp"

namespace {
	udx simulate_parallax_(const glm::vec2& raster, const glm::vec2& shift, const rect& view) {
		udx result = 0;
		for (r32 y = view.y + shift.y - raster.y; y < view.bottom(); y += raster.y) {
			for (r32 x = view.x + shift.x - raster.x; x < view.right(); x += raster.x) {
				result += display_list::QUAD;
			}
		}
		return result;
	}
}

void display_list::batch_blank(const rect& raster, const chroma& color) {
	this->batch_begin_(display_list::QUAD);

	const auto index = priority_ == priority_type::deferred ? 0 : 1;

	auto vtx = quads_->at<vtx_blank>(length_);
	vtx[0].position = raster.left_top();
	vtx[0].index = index;
	vtx[0].color = color;
	vtx[1].position = raster.left_bottom();
	vtx[1].index = index;
	vtx[1].color = color;
	vtx[2].position = raster.right_top();
	vtx[2].index = index;
	vtx[2].color = color;
	vtx[3].position = raster.right_bottom();
	vtx[3].index = index;
	vtx[3].color = color;

	this->batch_end_();
}

void display_list::batch_sprite(
	const glm::vec2& position,
	const glm::vec2& raster,
	const rect& uvs,
	const material& texture
) {
	this->batch_begin_(display_list::QUAD);

	const auto index = priority_ != priority_type::deferred ? 1 : 0;
	const glm::vec2 off = texture.offset();
	const auto atlas = texture.atlas();

	auto vtx = quads_->at<vtx_sprite>(length_);
	vtx[0].position = position;
	vtx[0].index = index;
	vtx[0].uvs = (uvs.left_top() + off) / material::MAXIMUM_DIMENSIONS;
	vtx[0].atlas = atlas;
	vtx[0].color = chroma::WHITE();
	vtx[1].position = { position.x, raster.y + position.y };
	vtx[1].index = index;
	vtx[1].uvs = (uvs.left_bottom() + off) / material::MAXIMUM_DIMENSIONS;
	vtx[1].atlas = atlas;
	vtx[1].color = chroma::WHITE();
	vtx[2].position = { raster.x + position.x, position.y };
	vtx[2].index = index;
	vtx[2].uvs = (uvs.right_top() + off) / material::MAXIMUM_DIMENSIONS;
	vtx[2].atlas = atlas;
	vtx[2].color = chroma::WHITE();
	vtx[3].position = raster + position;
	vtx[3].index = index;
	vtx[3].uvs = (uvs.right_bottom() + off) / material::MAXIMUM_DIMENSIONS;
	vtx[3].atlas = atlas;
	vtx[3].color = chroma::WHITE();

	this->batch_end_();
}

void display_list::batch_sprite(
	const std::array<glm::vec2, 4>& raster,
	const rect& quad,
	const material& texture,
	const mirror_type& mirror,
	const chroma& color
) {
	this->batch_begin_(display_list::QUAD);

	const auto index = priority_ != priority_type::deferred ? 1 : 0;
	const glm::vec2 off = texture.offset();
	const auto atlas = texture.atlas();

	auto vtx = quads_->at<vtx_sprite>(length_);
	vtx[0].position = raster[0];
	vtx[0].index = index;
	vtx[0].uvs = (quad.left_top() + off) / material::MAXIMUM_DIMENSIONS;
	vtx[0].atlas = atlas;
	vtx[0].color = color;
	vtx[1].position = raster[1];
	vtx[1].index = index;
	vtx[1].uvs = (quad.left_bottom() + off) / material::MAXIMUM_DIMENSIONS;
	vtx[1].atlas = atlas;
	vtx[1].color = color;
	vtx[2].position = raster[2];
	vtx[2].index = index;
	vtx[2].uvs = (quad.right_top() + off) / material::MAXIMUM_DIMENSIONS;
	vtx[2].atlas = atlas;
	vtx[2].color = color;
	vtx[3].position = raster[3];
	vtx[3].index = index;
	vtx[3].uvs = (quad.right_bottom() + off) / material::MAXIMUM_DIMENSIONS;
	vtx[3].atlas = atlas;
	vtx[3].color = color;

	if (mirror.horizontally) {
		std::swap(vtx[0].uvs.x, vtx[3].uvs.x);
		std::swap(vtx[1].uvs.x, vtx[2].uvs.x);
	}
	if (mirror.vertically) {
		std::swap(vtx[0].uvs.y, vtx[3].uvs.y);
		std::swap(vtx[1].uvs.y, vtx[2].uvs.y);
	}

	this->batch_end_();
}

void display_list::batch_parallax(
	const rect& view,
	const glm::vec2& shift,
	const glm::vec2& raster,
	const material& texture
) {
	this->batch_begin_(simulate_parallax_(raster, shift, view));

	const glm::vec2 dim = raster / material::MAXIMUM_DIMENSIONS;
	const glm::vec2 off = texture.offset() / material::MAXIMUM_DIMENSIONS;
	const auto atlas = texture.atlas();

	udx idx = 0;
	for (r32 y = view.y + shift.y - raster.y; y < view.bottom(); y += raster.y) {
		for (r32 x = view.x + shift.x - raster.x; x < view.right(); x += raster.x) {
			auto vtx = quads_->at<vtx_sprite>(length_ + idx);
			vtx[0].position = { x, y };
			vtx[0].index = 1;
			vtx[0].uvs = off;
			vtx[0].atlas = atlas;
			vtx[0].color = chroma::WHITE();
			vtx[1].position = { x, raster.y };
			vtx[1].index = 1;
			vtx[1].uvs = { off.x, off.y + dim.y };
			vtx[1].atlas = atlas;
			vtx[1].color = chroma::WHITE();
			vtx[2].position = { raster.x, y };
			vtx[2].index = 1;
			vtx[2].uvs = { off.x + dim.x, off.y };
			vtx[2].atlas = atlas;
			vtx[2].color = chroma::WHITE();
			vtx[3].position = { x + raster.x, y + raster.y };
			vtx[3].index = 1;
			vtx[3].uvs = off + dim;
			vtx[3].atlas = atlas;
			vtx[3].color = chroma::WHITE();
			idx += display_list::QUAD;
		}
	}
	this->batch_end_();
}

void display_list::skip(udx count) {
	length_ += count;
	stored_ = 0;
}

void display_list::flush(const shader_program& program) {
	visible_ = length_ > 0;
	if (visible_) {
		quads_->draw(program, length_);
	}
	length_ = 0;
}

void display_list::batch_begin_(udx count) {
	if (stored_ > 0) {
		this->batch_end_();
		spdlog::warn("Previous display list batch was never properly ended!");
	}
	if ((length_ + count) > quads_->length()) {
		spdlog::warn("This quad buffer is out of available vertices!");
	} else {
		stored_ = count;
	}
}

void display_list::batch_end_() {
	if (stored_ > 0) {
		length_ += stored_;
		stored_ = 0;
	} else {
		spdlog::warn("Previous display list batch was never properly started!");
	}
}

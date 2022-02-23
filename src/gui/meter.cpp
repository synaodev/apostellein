#include <apostellein/konst.hpp>
#include <apostellein/cast.hpp>

#include "./meter.hpp"
#include "../x2d/renderer.hpp"

void gui::meter::build(
	const glm::vec2& position,
	const glm::vec2& dimensions,
	i32 current,
	i32 maximum,
	const glm::vec2& frame_position,
	udx frame_state,
	const animation_group* frame_file
) {
	position_ = position;
	dimensions_ = dimensions;
	raster_ = {
		position_,
		dimensions_
	};
	current_ = current;
	maximum_ = maximum;
	frame_.build(
		frame_position,
		frame_state, 0,
		frame_file
	);
}

void gui::meter::clear() {
	this->invalidate();
	current_ = 0;
	maximum_ = 0;
	raster_ = {
		position_,
		dimensions_
	};
}

void gui::meter::update(i64 delta) {
	if (maximum_ > 0) {
		frame_.update(delta);
	}
}

void gui::meter::render(renderer& rdr) const {
	if (maximum_ > 0) {
		auto& list = rdr.query(
			priority_type::deferred,
			blending_type::alpha,
			pipeline_type::blank
		);
		if (invalidated_) {
			invalidated_ = false;
			list.batch_blank(raster_, chroma::WHITE());
		} else {
			list.skip(display_list::QUAD);
		}
		frame_.render(rdr);
	}
}

void gui::meter::set(i32 current, i32 maximum) {
	if (current <= 0 or maximum <= 0) {
		this->clear();
	} else if (current_ != current or maximum_ != maximum) {
		current_ = current;
		maximum_ = maximum;
		const auto ratio = as<r32>(current_) / as<r32>(maximum_);
		raster_.h = glm::round(ratio * dimensions_.y);
		raster_.y = position_.y + raster_.h;
		invalidated_ = true;
	}
}

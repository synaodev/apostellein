#include "./provision.hpp"

#include <apostellein/cast.hpp>

void gui::provision::build(
	const glm::vec2& frame_position,
	udx frame_state,
	const animation_group* frame_file,
	const glm::vec2& item_position,
	const animation_group* item_file,
	const glm::vec2& amount_position,
	const rect& amount_raster,
	const material* amount_texture
) {
	slot_ = {};
	frame_.build(
		frame_position,
		frame_state, 0,
		frame_file
	);
	item_.build(
		item_position,
		0, 0,
		item_file
	);
	amount_.build(
		true,
		amount_position,
		amount_raster,
		0, 0,
		amount_texture
	);
}

void gui::provision::render(renderer& rdr) const {
	if (slot_.type > 0) {
		frame_.render(rdr);
		item_.render(rdr);
		if (slot_.count > 1 or slot_.weapon) {
			amount_.render(rdr);
		}
	}
}

void gui::provision::set(const item_slot& value) {
	if (slot_ != value) {
		slot_ = value;
		if (slot_.type > 0) {
			item_.frame(as<udx>(slot_.type) - 1);
		}
		if (slot_.count > 0) {
			amount_.set(slot_.count);
		}
	}
}

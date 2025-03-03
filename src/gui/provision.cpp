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
	const texture_2d* amount_texture
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

void gui::provision::render(renderer_2d& renderer) const {
	if (slot_.type > 0) {
		frame_.render(renderer);
		item_.render(renderer);
		if (slot_.count > 1 or slot_.weapon) {
			amount_.render(renderer);
		}
	}
}

void gui::provision::set(const item_slot& value) {
	if (slot_ != value) {
		slot_ = value;
		if (slot_.type > 0) {
			item_.frame(cast<udx>(slot_.type) - 1);
		}
		if (slot_.count > 0) {
			amount_.set(slot_.count);
		}
	}
}

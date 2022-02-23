#include <spdlog/spdlog.h>
#include <apostellein/cast.hpp>

#include "./element.hpp"

namespace {
	constexpr udx FALLBACK_MODULO = 2;
}

void gui::element::build(
	udx index,
	udx modulo,
	const glm::vec2& start,
	const glm::vec2& spacing,
	const animation_group* item_file,
	bool amount_backwards,
	const glm::vec2& amount_offset,
	const rect& amount_raster,
	const material* amount_texture
) {
	slot_ = {};
	if (modulo == 0 or modulo == 1) {
		spdlog::error("Inventory element modulo cannot be lower than 2!");
		modulo = FALLBACK_MODULO;
	}
	const glm::vec2 position {
		start.x + as<r32>(index % modulo) * spacing.x,
		start.y + as<r32>(index / modulo) * spacing.y
	};
	item_.build(
		position,
		0, 0,
		item_file
	);
	amount_.build(
		amount_backwards,
		position + amount_offset,
		amount_raster,
		0, 0,
		amount_texture
	);
}

void gui::element::render(renderer& rdr) const {
	if (slot_.type > 0) {
		item_.render(rdr);
		if (slot_.count > 1 or slot_.weapon) {
			amount_.render(rdr);
		}
	}
}

void gui::element::set(const item_slot& value) {
	if (slot_ != value) {
		slot_ = value;
		if (slot_.type > 0) {
			item_.frame(as<udx>(slot_.type) - 1);
		}
		if (slot_.count > 0) {
			amount_.set(slot_.count);
		}
	}
	item_.invalidate();
	amount_.invalidate();
}

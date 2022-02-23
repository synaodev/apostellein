#include <spdlog/spdlog.h>
#include <apostellein/konst.hpp>

#include "./headsup.hpp"
#include "../hw/vfs.hpp"
#include "../ctrl/controller.hpp"
#include "../util/id-table.hpp"
#include "../x2d/bitmap-font.hpp"

namespace {
	constexpr udx TITLE_FONT = 2;
	constexpr glm::vec2 TITLE_POSITION { 240.0f, 36.0f };
	constexpr chroma TITLE_BACK_COLOR { 0x3FU, 0x3FU, 0xFFU, 0xFFU };
	constexpr glm::vec2 INDICATOR_POSITION { 2.0f, 2.0f };
	constexpr glm::vec2 POISON_POSITION { 10.0f, 5.0f };
	constexpr rect POISON_RASTER { 56.0f, 0.0f, 8.0f, 9.0f };
	constexpr i32 POISON_ZEROS = 3;
	constexpr glm::vec2 BARRIER_POSITION { 47.0f, 2.0f };
	constexpr rect BARRIER_RASTER { 45.0f, 0.0f, 6.0f, 8.0f };
	constexpr i32 BARRIER_INITIAL_UNITS = 2;
	constexpr glm::vec2 OXYGEN_POSITION { 2.0f, 18.0f };
	constexpr rect OXYGEN_RASTER { 56.0f, 18.0f, 8.0f, 10.0f };
	constexpr i32 MAXIMUM_OXYGEN = 100;
	constexpr glm::vec2 PROVISION_FRAME_POSITION { 440.0f, 2.0f };
	constexpr udx PROVISION_FRAME_STATE = 2;
	constexpr glm::vec2 PROVISION_ITEM_POSITION { 443.0f, 5.0f };
	constexpr glm::vec2 PROVISION_COUNT_POSITION { 470.0f, 25.0f };
	constexpr rect PROVISION_RASTER { 56.0f, 9.0f, 8.0f, 9.0f };
	constexpr glm::vec2 METER_POSITION { 302.0f, 142.0f };
	constexpr glm::vec2 METER_DIMENSIONS { 6.0f, 94.0f };
	constexpr glm::vec2 METER_FRAME_POSITION { 311.0f, 143.0f };
	constexpr udx METER_FRAME_STATE = 3;
}

void headsup::build() {
	title_.build(
		TITLE_POSITION, {},
		TITLE_BACK_COLOR, chroma::WHITE(),
		vfs::find_font(TITLE_FONT),
		[](udx index) { return vfs::find_font(index); }
	);
	auto heads_file = vfs::find_animation(anim::Heads);
	auto items_file = vfs::find_animation(anim::Items);
	auto counter_texture = vfs::find_material(img::Heads);
	indicator_.build(
		INDICATOR_POSITION,
		0, 0,
		heads_file
	);
	poison_.build(
		false,
		POISON_POSITION,
		POISON_RASTER,
		0, POISON_ZEROS,
		counter_texture
	);
	barrier_.build(
		BARRIER_POSITION,
		BARRIER_RASTER,
		BARRIER_INITIAL_UNITS,
		BARRIER_INITIAL_UNITS, 0,
		counter_texture
	);
	oxygen_.build(
		false,
		OXYGEN_POSITION,
		OXYGEN_RASTER,
		MAXIMUM_OXYGEN, 0,
		counter_texture
	);
	oxygen_.visible(false);
	provision_.build(
		PROVISION_FRAME_POSITION,
		PROVISION_FRAME_STATE,
		heads_file,
		PROVISION_ITEM_POSITION,
		items_file,
		PROVISION_COUNT_POSITION,
		PROVISION_RASTER,
		counter_texture
	);
	meter_.build(
		METER_POSITION,
		METER_DIMENSIONS,
		0, 0,
		METER_FRAME_POSITION,
		METER_FRAME_STATE,
		heads_file
	);
	graphic_.build(true);
	fader_.clear();
}

void headsup::fix() {
	auto font = vfs::find_font(TITLE_FONT);
	title_.fix(font);
	indicator_.fix();
	poison_.fix();
	barrier_.fix();
	oxygen_.fix();
	provision_.fix();
	meter_.fix();
	graphic_.fix();
}

void headsup::handle(const controller& ctl) {
	if (ctl.provision_valid()) {
		const item_slot item = ctl.provisioned_item();
		provision_.set(item);
	} else {
		provision_.set();
	}
	fader_.handle();
}

void headsup::render(r32 ratio, renderer& rdr, const controller& ctl) const {
	title_.render(rdr);
	if (!ctl.state().locked) {
		indicator_.render(rdr);
		poison_.render(rdr);
		barrier_.render(rdr);
		oxygen_.render(rdr);
		provision_.render(rdr);
		meter_.render(rdr);
		graphic_.render(rdr);
	} else {
		indicator_.invalidate();
		poison_.invalidate();
		barrier_.invalidate();
		oxygen_.invalidate();
		provision_.invalidate();
		meter_.invalidate();
		graphic_.invalidate();
	}
	fader_.render(ratio, rdr);
}

void headsup::set(const headsup_params& params) {
	indicator_.variation(params.strafing ? 1 : 0);
	indicator_.frame(params.indication);
	poison_.set(params.poison);
	barrier_.set(
		params.current_barrier,
		params.maximum_barrier,
		params.indication
	);
	this->oxygen_.visible(params.current_oxygen < params.maximum_oxygen);
	this->oxygen_.set(params.current_oxygen);
}

void headsup::push_message(
	bool centered,
	const glm::vec2& position,
	udx index,
	const std::string& words
) {
	title_.push_message(
		centered,
		position,
		index,
		vfs::find_font(index),
		words
	);
}

void headsup::forward_message(
	bool centered,
	const glm::vec2& position,
	udx index,
	std::u32string&& words
) {
	title_.forward_message(
		centered,
		position,
		index,
		vfs::find_font(index),
		std::move(words)
	);
}

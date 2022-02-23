#include <spdlog/spdlog.h>
#include <apostellein/konst.hpp>
#include <apostellein/cast.hpp>

#include "./dialogue.hpp"
#include "./headsup.hpp"
#include "./inventory.hpp"
#include "../hw/audio.hpp"
#include "../hw/vfs.hpp"
#include "../util/buttons.hpp"
#include "../util/id-table.hpp"
#include "../x2d/bitmap-font.hpp"
#include "../x2d/renderer.hpp"

namespace {
	constexpr udx STATE_ARROW = 1;
	constexpr i64 FASTEST_DELAY = konst::NANOSECONDS_PER_TICK();
	constexpr i64 DEFAULT_DELAY = FASTEST_DELAY * 3;
	constexpr i64 SLOWEST_DELAY = DEFAULT_DELAY * 2;
	constexpr glm::vec2 RASTER_HIGH { 112.0f, 8.0f };
	constexpr glm::vec2 RASTER_LOW { 112.0f, 206.0f };
	constexpr glm::vec2 RASTER_DIMENSIONS { 256.0f, 56.0f };
	constexpr glm::vec2 FACES_OFFSET { 10.0f, 4.0f };
	constexpr glm::vec2 ARROW_OFFSET { 5.0f, 10.0f };
	constexpr glm::vec2 TEXT_OFFSET_NORMAL { 10.0f, 6.0f };
	constexpr glm::vec2 TEXT_OFFSET_FACEBOX { 68.0f, 6.0f };
}

void dialogue::build() {
	invalidated_ = true;
	flags_._raw = {};
	cursor_ = 0;
	choices_ = 0;
	timer_ = 0;
	delay_ = DEFAULT_DELAY;
	raster_ = {
		RASTER_HIGH,
		RASTER_DIMENSIONS
	};
	text_.build(
		RASTER_HIGH + TEXT_OFFSET_NORMAL, {},
		chroma::WHITE(),
		vfs::find_font(0), {}
	);
	faces_.build(
		RASTER_HIGH + FACES_OFFSET, 0, 0,
		vfs::find_animation(anim::Faces)
	);
	arrow_.build(
		RASTER_HIGH + ARROW_OFFSET,
		STATE_ARROW, 0,
		vfs::find_animation(anim::Heads)
	);
}

void dialogue::fix() {
	auto font = vfs::find_font(0);
	faces_.fix();
	arrow_.fix();
	text_.fix(font);
	invalidated_ = true;
}

void dialogue::handle(const buttons& bts, headsup& hud, const inventory& ivt) {
	if (flags_.textbox) {
		if (!text_.finished()) {
			if (flags_.sound) {
				flags_.sound = false;
				text_.handle();
				audio::play(sfx::Text, 7);
			}
			if (!flags_.delay) {
				delay_ = bts.holding.cancel ?
					FASTEST_DELAY :
					DEFAULT_DELAY;
			}
		} else if (flags_.question) {
			const glm::vec2 fd = text_.font_dimensions();
			if (bts.pressed.up) {
				if (cursor_ > 0) {
					--cursor_;
					arrow_.transform(0.0f, -fd.y);
				} else {
					cursor_ = choices_;
					arrow_.transform(0.0f, fd.y * as<r32>(choices_));
				}
				audio::play(sfx::Select, 0);
			} else if (bts.pressed.down) {
				if (cursor_ < choices_) {
					++cursor_;
					arrow_.transform(0.0f, fd.y);
				} else {
					cursor_ = 0;
					arrow_.transform(0.0f, -fd.y * as<r32>(choices_));
				}
				audio::play(sfx::Select, 0);
			} else if (bts.pressed.confirm) {
				flags_.question = false;
				choices_ = 0;
				audio::play(sfx::TitleBeg, 0);
			}
		} else {
			flags_.writing = false;
		}
	}
	if (invalidated_ and flags_.textbox) {
		hud.clear_title();
		if (ivt.active()) {
			ivt.invalidate();
		}
	}
}

void dialogue::update(i64 delta) {
	if (flags_.textbox) {
		timer_ += delta;
		if (timer_ >= delay_) {
			timer_ %= delay_;
			if (!text_.finished()) {
				flags_.writing = true;
				flags_.sound = true;
			}
		}
		if (flags_.facebox) {
			if (!text_.finished()) {
				faces_.update(delta);
			} else {
				faces_.frame(0);
			}
		}
		if (flags_.question) {
			arrow_.update(delta);
		}
	}
}

void dialogue::render(renderer& rdr) const {
	if (flags_.textbox) {
		text_.render(rdr);
		if (flags_.facebox) {
			faces_.render(rdr);
		} else {
			faces_.invalidate();
		}
		if (flags_.question) {
			arrow_.render(rdr);
		} else {
			arrow_.invalidate();
		}
		auto& list = rdr.query(
			priority_type::deferred,
			blending_type::alpha,
			pipeline_type::blank
		);
		if (invalidated_) {
			invalidated_ = false;
			list.batch_blank(raster_, chroma::TRANSLUCENT());
		} else {
			list.skip(display_list::QUAD);
		}
	}
}

void dialogue::open_textbox_high() {
	invalidated_ = true;
	flags_.textbox = true;
	cursor_ = 0;
	choices_ = 0;
	raster_ = {
		RASTER_HIGH,
		RASTER_DIMENSIONS
	};
	text_.position(
		flags_.facebox ?
		RASTER_HIGH + TEXT_OFFSET_FACEBOX :
		RASTER_HIGH + TEXT_OFFSET_NORMAL
	);
	faces_.position(RASTER_HIGH + FACES_OFFSET);
	arrow_.position(RASTER_HIGH + ARROW_OFFSET);
}

void dialogue::open_textbox_low() {
	invalidated_ = true;
	flags_.textbox = true;
	cursor_ = 0;
	choices_ = 0;
	raster_ = {
		RASTER_LOW,
		RASTER_DIMENSIONS
	};
	text_.position(
		flags_.facebox ?
		RASTER_LOW + TEXT_OFFSET_FACEBOX :
		RASTER_LOW + TEXT_OFFSET_NORMAL
	);
	faces_.position(RASTER_LOW + FACES_OFFSET);
	arrow_.position(RASTER_LOW + ARROW_OFFSET);
}

void dialogue::close_textbox() {
	invalidated_ = true;
	flags_._raw = {};
	cursor_ = 0;
	choices_ = 0;
	timer_ = 0;
	delay_ = DEFAULT_DELAY;
	text_.clear();
	text_.color(chroma::WHITE());
	text_.position(raster_.position() + TEXT_OFFSET_NORMAL);
	faces_.state(0);
	faces_.variation(0);
}

void dialogue::open_facebox(udx state, udx variation) {
	flags_.facebox = true;
	cursor_ = 0;
	choices_ = 0;
	const glm::vec2 position = raster_.position();
	text_.position(position + TEXT_OFFSET_FACEBOX);
	faces_.state(state);
	faces_.variation(variation);
	faces_.position(position + FACES_OFFSET);
	arrow_.position(position + ARROW_OFFSET);
}

void dialogue::close_facebox() {
	flags_.facebox = false;
	cursor_ = 0;
	choices_ = 0;
	const glm::vec2 position = raster_.position();
	text_.position(position + TEXT_OFFSET_NORMAL);
	faces_.state(0);
	faces_.variation(0);
	faces_.position(position + FACES_OFFSET);
	arrow_.position(position + ARROW_OFFSET);
}

void dialogue::custom_delay(r32 value) {
	flags_.delay = true;
	delay_ = glm::clamp(
		konst::SECONDS_TO_NANOSECONDS(value),
		SLOWEST_DELAY,
		FASTEST_DELAY
	);
}

void dialogue::reset_delay() {
	flags_.delay = false;
	delay_ = DEFAULT_DELAY;
}

void dialogue::color_text(i32 r, i32 g, i32 b) {
	r = glm::clamp(r, 0, 255);
	g = glm::clamp(r, 0, 255);
	b = glm::clamp(r, 0, 255);
	const chroma color {
		as<byte>(r),
		as<byte>(g),
		as<byte>(b),
		0xFFU
	};
	text_.color(color);
}

void dialogue::forward_question(std::u32string&& string, udx choices) {
	flags_.question = true;
	flags_.writing = false;
	cursor_ = 0;
	choices_ = choices;
	text_.forward(std::move(string));
}

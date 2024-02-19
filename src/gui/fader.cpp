#include <apostellein/konst.hpp>
#include <spdlog/spdlog.h>

#include "./fader.hpp"
#include "../x2d/renderer.hpp"

namespace {
	constexpr r32 INCREMENTATION = 12.0f;
}

void gui::fader::clear() {
	type_ = fade_type::done_out;
	current_ = konst::WINDOW_DIMENSIONS<r32>();
	previous_ = konst::WINDOW_DIMENSIONS<r32>();
}

void gui::fader::handle() {
	switch (type_) {
		case fade_type::done_in:
		case fade_type::done_out: {
			break;
		}
		case fade_type::moving_in: {
			current_.y -= INCREMENTATION;
			if (current_.y < 0.0f) {
				type_ = fade_type::done_in;
				current_.y = 0.0f;
			}
			break;
		}
		case fade_type::moving_out: {
			current_.y += INCREMENTATION;
			if (current_.y > konst::WINDOW_HEIGHT<r32>()) {
				type_ = fade_type::done_out;
				current_.y = konst::WINDOW_HEIGHT<r32>();
			}
			break;
		}
	}
}

void gui::fader::render(r32 ratio, renderer& rdr) const {
	if (this->visible()) {
		auto& list = rdr.query(
			priority_type::deferred,
			blending_type::alpha,
			pipeline_type::blank
		);
		const glm::vec2 dimensions = konst::INTERPOLATE(
			previous_,
			current_,
			ratio
		);
		list.batch_blank(dimensions, chroma::BASE());
	}
}

void gui::fader::fade_in() {
	type_ = fade_type::moving_in;
	current_.y = konst::WINDOW_HEIGHT<r32>();
}

void gui::fader::fade_out() {
	type_ = fade_type::moving_out;
	current_.y = 0.0f;
}

#include <spdlog/spdlog.h>
#include <apostellein/konst.hpp>

#include "./title.hpp"

namespace {
	constexpr udx MAXIMUM_MESSAGES = 8;
	constexpr i64 DEFAULT_TIMER = konst::SECONDS_TO_NANOSECONDS(2.0);
	constexpr i64 FRONT_TIME = konst::SECONDS_TO_NANOSECONDS(0.075);
}

void gui::title::build(
	const glm::vec2& position,
	const glm::vec2& origin,
	const chroma& back_color,
	const chroma& front_color,
	const bitmap_font* font,
	gui::title::callback recalibrate
) {
	back_text_.build(
		position,
		origin,
		back_color,
		font, {}
	);
	front_text_.build(
		position,
		origin + 1.0f,
		front_color,
		font, {}
	);
	recalibrate_ = recalibrate;
}

void gui::title::clear() {
	timer_ = 0;
	back_text_.clear();
	front_text_.clear();
	messages_.clear();
	indices_.clear();
}

void gui::title::fix(const bitmap_font* font) {
	back_text_.fix(font);
	front_text_.fix(font);
	if (recalibrate_) {
		for (udx idx = 0; idx < messages_.size() and idx < indices_.size(); ++idx) {
			font = recalibrate_(indices_[idx]);
			messages_[idx].fix(font);
		}
	}
}

void gui::title::render(renderer& rdr) const {
	if (timer_ > 0) {
		back_text_.render(rdr);
		if (timer_ >= FRONT_TIME) {
			front_text_.render(rdr);
		}
	}
	for (auto&& msg : messages_) {
		msg.render(rdr);
	}
}

void gui::title::push_message(
	bool centered,
	const glm::vec2& position,
	udx index,
	const bitmap_font* font,
	const std::string& words
) {
	if (messages_.size() < MAXIMUM_MESSAGES) {
		auto& recent = messages_.emplace_back();
		recent.build(
			position, {},
			chroma::WHITE(),
			font, words
		);
		if (centered) {
			const rect bounds = recent.bounds();
			recent.origin(bounds.w / 2.0f, 0.0f);
		}
	} else {
		spdlog::warn("Too many title messages!");
	}
	indices_.push_back(index);
}

void gui::title::forward_message(
	bool centered,
	const glm::vec2& position,
	udx index,
	const bitmap_font* font,
	std::u32string&& words
) {
	if (messages_.size() < MAXIMUM_MESSAGES) {
		auto& recent = messages_.emplace_back();
		recent.build(
			position, {},
			chroma::WHITE(),
			font, {}
		);
		recent.forward(std::move(words));
		if (centered) {
			const rect bounds = recent.bounds();
			recent.origin(bounds.w / 2.0f, 0.0f);
		}
	} else {
		spdlog::warn("Too many title messages!");
	}
	indices_.push_back(index);
}

i64 gui::title::default_timer_() {
	return DEFAULT_TIMER;
}

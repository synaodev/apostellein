#include <spdlog/spdlog.h>
#include <apostellein/konst.hpp>

#include "./overlay.hpp"
#include "./widget-detail.hpp"
#include "./headsup.hpp"
#include "./dialogue.hpp"
#include "../ctrl/controller.hpp"
#include "../hw/audio.hpp"
#include "../hw/vfs.hpp"
#include "../util/buttons.hpp"
#include "../x2d/renderer.hpp"

namespace {
	constexpr udx MAXIMUM_WIDGETS = 4;
}

void overlay::fix() {
	if (!widgets_.empty()) {
		auto font = vfs::find_font(0);
		for (auto&& wgt : widgets_) {
			wgt->fix(font);
		}
	}
	invalidated_ = true;
}

void overlay::handle(buttons& bts, controller& ctl, headsup& hud) {
	if (release_) {
		release_ = false;
		widgets_.clear();
	} else if (!widgets_.empty()) {
		auto& wgt = *widgets_.back();
		if (!wgt.ready()) {
			auto font = vfs::find_font(0);
			wgt.build(font, ctl, *this);
			hud.clear_title();
		}
		wgt.handle(bts, ctl, *this, hud);
		if (release_) {
			release_ = false;
			widgets_.clear();
		} else if (!wgt.active()) {
			widgets_.pop_back();
			if (!widgets_.empty()) {
				auto& next = *widgets_.back();
				next.invalidate();
			}
		}
	} else if (!ctl.state().locked) {
		if (bts.pressed.options) {
			this->push(widget_type::option);
		}
	}
}

void overlay::render(renderer& rdr) const {
	if (!widgets_.empty()) {
		widgets_.back()->render(rdr);
		auto& list = rdr.query(
			priority_type::deferred,
			blending_type::alpha,
			pipeline_type::blank
		);
		if (invalidated_) {
			invalidated_ = false;
			list.batch_blank(
				konst::WINDOW_DIMENSIONS<r32>(),
				chroma::TRANSLUCENT()
			);
		} else {
			list.skip(display_list::QUAD);
		}
	}
}

void overlay::push(widget_type type) {
	invalidated_ = true;
	if (widgets_.size() >= MAXIMUM_WIDGETS) {
		spdlog::error("Too many widgets in the menu stack!");
		return;
	}
	switch (type) {
	case widget_type::option:
		widgets_.emplace_back(std::make_unique<option_widget>());
		break;
	case widget_type::input:
		widgets_.emplace_back(std::make_unique<input_widget>());
		break;
	case widget_type::video:
		widgets_.emplace_back(std::make_unique<video_widget>());
		break;
	case widget_type::audio:
		widgets_.emplace_back(std::make_unique<audio_widget>());
		break;
	case widget_type::language:
		widgets_.emplace_back(std::make_unique<language_widget>());
		break;
	case widget_type::record:
		widgets_.emplace_back(std::make_unique<profile_widget>(widget_type::record));
		break;
	case widget_type::resume:
		widgets_.emplace_back(std::make_unique<profile_widget>(widget_type::resume));
		break;
	default:
		spdlog::warn("Invalid widget type! This code should not run!");
		break;
	}
}

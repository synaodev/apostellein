#include <spdlog/spdlog.h>
#include <apostellein/konst.hpp>
#include <apostellein/cast.hpp>

#include "./widget-detail.hpp"
#include "./overlay.hpp"
#include "./headsup.hpp"
#include "../hw/vfs.hpp"
#include "../hw/audio.hpp"
#include "../hw/music.hpp"
#include "../hw/input.hpp"
#include "../hw/video.hpp"
#include "../ctrl/controller.hpp"
#include "../util/buttons.hpp"
#include "../util/config-file.hpp"
#include "../util/id-table.hpp"
#include "../x2d/bitmap-font.hpp"

namespace {
	constexpr glm::vec2 DEFAULT_POSITION { 4.0f, 2.0f };
	constexpr r32 TAB_SPACE = 4.0f;
	constexpr r32 SMALL_TAB_SPACE = 3.0f;
	constexpr r32 LARGE_TAB_SPACE = SMALL_TAB_SPACE * 2.0f;
	constexpr udx STATE_ARROW = 1;
	constexpr udx OPTION_OPTIONS = 6;
	constexpr udx PROFILE_OPTIONS = controller::MAXIMUM_PROFILES - 1;
	constexpr udx INPUT_OPTIONS_LEFT = as<udx>(button_name::LAST_ORDINAL);
	constexpr udx INPUT_OPTIONS_RIGHT = as<udx>(button_name::LAST_BUTTON);
	constexpr glm::vec2 INPUT_POSITION_LEFT { 3.0f, 16.0f };
	constexpr glm::vec2 INPUT_POSITION_RIGHT { 175.0f, 16.0f };
	constexpr udx VIDEO_OPTIONS = 2;
	constexpr udx AUDIO_OPTIONS = 1;
	constexpr udx LANGUAGE_OPTIONS = 9;

	constexpr char MAIN_ENTRY[] = "Main";
	constexpr char OPTION_ENTRY[] = "Option";
	constexpr char PROFILE_ENTRY[] = "Profile";
	constexpr char INPUT_ENTRY[] = "Input";
	constexpr char VIDEO_ENTRY[] = "Video";
	constexpr char AUDIO_ENTRY[] = "Audio";
	constexpr char LANGUAGE_ENTRY[] = "Language";
}

void option_widget::build(const bitmap_font* font, controller& ctl, overlay&) {
	flags_.ready = true;
	text_.build(
		DEFAULT_POSITION, {},
		chroma::WHITE(),
		font,
		vfs::i18n_from(OPTION_ENTRY, 0, 7)
	);
	const glm::vec2 fd = text_.font_dimensions();
	arrow_.build(
		{ fd.x, TAB_SPACE + DEFAULT_POSITION.y + (fd.y * 2.0f) },
		STATE_ARROW, 0,
		vfs::find_animation(anim::Heads)
	);
	ctl.freeze();
	audio::play(sfx::Inven, 0);
}

void option_widget::handle(buttons& bts, controller& ctl, overlay& ovl, headsup& hud) {
	bool reboot = false;
	const glm::vec2 fd = text_.font_dimensions();
	if (bts.pressed.up) {
		if (cursor_ > 0) {
			--cursor_;
			arrow_.transform(0.0f, -fd.y);
		} else {
			cursor_ = OPTION_OPTIONS;
			arrow_.transform(0.0f, fd.y * as<r32>(cursor_));
		}
		audio::play(sfx::Select, 0);
	} else if (bts.pressed.down) {
		if (cursor_ < OPTION_OPTIONS) {
			++cursor_;
			arrow_.transform(0.0f, fd.y);
		} else {
			cursor_ = 0;
			arrow_.transform(0.0f, -fd.y * as<r32>(OPTION_OPTIONS));
		}
		audio::play(sfx::Select, 0);
	} else if (bts.pressed.confirm) {
		switch (cursor_) {
		case 0: // Return
			flags_.active = false;
			break;
		case 1: // Input
			ovl.push(widget_type::input);
			audio::play(sfx::Inven, 0);
			break;
		case 2: // Video
			ovl.push(widget_type::video);
			audio::play(sfx::Inven, 0);
			break;
		case 3: // Audio
			ovl.push(widget_type::audio);
			audio::play(sfx::Inven, 0);
			break;
		case 4: // Language
			ovl.push(widget_type::language);
			audio::play(sfx::Inven, 0);
			break;
		case 5: // Reboot
			reboot = true;
			flags_.active = false;
			break;
		case 6: // Quit
			ctl.quit();
			break;
		default:
			break;
		}
	} else if (bts.pressed.cancel or bts.pressed.options) {
		flags_.active = false;
	}
	if (!flags_.active) {
		input::zero(bts);
		if (reboot) {
			hud.fade_out();
			ovl.clear();
			ctl.boot();
			audio::play(sfx::TitleBeg, 0);
		} else {
			ctl.unlock();
			audio::play(sfx::Inven, 0);
		}
	}
}

void input_widget::build(const bitmap_font* font, controller&, overlay&) {
	flags_.ready = true;
	const glm::vec2 fd = font->glyph_dimensions();
	header_.build(
		DEFAULT_POSITION, {},
		chroma::WHITE(),
		font, vfs::i18n_at(INPUT_ENTRY, 0)
	);
	left_text_.build(
		DEFAULT_POSITION + INPUT_POSITION_LEFT, {},
		chroma::WHITE(),
		font, {}
	);
	this->setup_left_text_();
	right_text_.build(
		DEFAULT_POSITION + INPUT_POSITION_RIGHT, {},
		chroma::WHITE(),
		font, {}
	);
	this->setup_right_text_();
	arrow_.build(
		{}, STATE_ARROW,
		0, vfs::find_animation(anim::Heads)
	);
	const glm::vec2 position = left_text_.position();
	arrow_.position(
		position.x + (INPUT_POSITION_LEFT.x - SMALL_TAB_SPACE),
		(fd.y * 2.0f) - LARGE_TAB_SPACE
	);
}

void input_widget::handle(buttons& bts, controller&, overlay&, headsup&) {
	const glm::vec2 fd = left_text_.font_dimensions();
	if (waiting_) {
		flash_ = !flash_;
		if (input::valid_stored_code()) {
			auto code = input::receive_stored_code();
			if (left_side_) {
				input::swap_keyboard_bindings(code, as<u32>(cursor_));
				this->setup_left_text_();
			} else {
				input::swap_joystick_bindings(code, as<u32>(cursor_));
				this->setup_right_text_();
			}
			waiting_ = false;
			flash_ = false;
			audio::play(sfx::TitleBeg, 0);
			input::zero(bts);
		} else if (!input::joystick_attached() and !left_side_) {
			waiting_ = false;
			flash_ = false;
			input::stop_listening();
			audio::play(sfx::Inven, 0);
		}
	} else if (bts.pressed.up) {
		if (cursor_ > 0) {
			--cursor_;
			arrow_.transform(0.0f, -fd.y);
		} else {
			cursor_ = left_side_ ?
				INPUT_OPTIONS_LEFT :
				INPUT_OPTIONS_RIGHT;
			arrow_.transform(0.0f, fd.y * as<r32>(cursor_));
		}
		audio::play(sfx::Select, 0);
	} else if (bts.pressed.down) {
		auto options = left_side_ ?
			INPUT_OPTIONS_LEFT :
			INPUT_OPTIONS_RIGHT;
		if (cursor_ < options) {
			++cursor_;
			arrow_.transform(0.0f, fd.y);
		} else {
			cursor_ = 0;
			arrow_.transform(0.0f, -fd.y * as<r32>(options));
		}
		audio::play(sfx::Select, 0);
	} else if (bts.pressed.right and left_side_) {
		left_side_ = false;
		if (cursor_ > INPUT_OPTIONS_RIGHT) {
			cursor_ = INPUT_OPTIONS_RIGHT;
			const glm::vec2 position = arrow_.position();
			arrow_.position(
				position.x + (INPUT_POSITION_RIGHT.x - SMALL_TAB_SPACE),
				(as<r32>(INPUT_OPTIONS_RIGHT) * fd.y) +
				((fd.y * 2.0f) - LARGE_TAB_SPACE)
			);
		} else {
			arrow_.transform(INPUT_POSITION_RIGHT.x - SMALL_TAB_SPACE, 0.0f);
		}
		audio::play(sfx::Select, 0);
	} else if (bts.pressed.left and !left_side_) {
		left_side_ = true;
		arrow_.transform(-INPUT_POSITION_RIGHT.x + SMALL_TAB_SPACE, 0.0f);
		audio::play(sfx::Select, 0);
	} else if (bts.pressed.confirm) {
		if (left_side_) {
			waiting_ = true;
			flash_ = false;
			input::listen_to_keyboard();
			audio::play(sfx::Inven, 0);
		} else if (input::joystick_attached()) {
			waiting_ = true;
			flash_ = false;
			input::listen_to_joystick();
			audio::play(sfx::Inven, 0);
		}
	} else if (bts.pressed.cancel or bts.pressed.options) {
		flags_.active = false;
	}
	if (!flags_.active) {
		input::zero(bts);
		audio::play(sfx::Inven, 0);
	}
}

void input_widget::setup_left_text_() {
	fmt::memory_buffer out {};
	for (auto iter = button_name::FIRST_BUTTON; iter <= button_name::LAST_ORDINAL; ++iter) {
		out.append(vfs::i18n_at(INPUT_ENTRY, as<udx>(iter) + 1));
		out.append(input::keyboard_string(as<u32>(iter)));
	}
	left_text_.replace(fmt::to_string(out));
	right_text_.invalidate();
}

void input_widget::setup_right_text_() {
	fmt::memory_buffer out {};
	for (auto iter = button_name::FIRST_BUTTON; iter <= button_name::LAST_BUTTON; ++iter) {
		out.append(vfs::i18n_at(INPUT_ENTRY, as<udx>(iter) + 1));
		out.append(input::joystick_string(as<u32>(iter)));
	}
	right_text_.replace(fmt::to_string(out));
	left_text_.invalidate();
}

void video_widget::build(const bitmap_font* font, controller&, overlay&) {
	flags_.ready = true;
	text_.build(
		DEFAULT_POSITION, {},
		chroma::WHITE(),
		font, {}
	);
	const glm::vec2 fd = text_.font_dimensions();
	arrow_.build(
		{ fd.x, TAB_SPACE + DEFAULT_POSITION.y + (fd.y * 2.0f) },
		STATE_ARROW, 0,
		vfs::find_animation(anim::Heads)
	);
	this->setup_text_();
}

void video_widget::handle(buttons& bts, controller&, overlay&, headsup&) {
	const glm::vec2 fd = text_.font_dimensions();
	if (bts.pressed.up) {
		if (cursor_ > 0) {
			--cursor_;
			arrow_.transform(0.0f, -fd.y);
		} else {
			cursor_ = VIDEO_OPTIONS;
			arrow_.transform(0.0f, fd.y * as<r32>(cursor_));
		}
		audio::play(sfx::Select, 0);
	} else if (bts.pressed.down) {
		if (cursor_ < VIDEO_OPTIONS) {
			++cursor_;
			arrow_.transform(0.0f, fd.y);
		} else {
			cursor_ = 0;
			arrow_.transform(0.0f, -fd.y * as<r32>(VIDEO_OPTIONS));
		}
		audio::play(sfx::Select, 0);
	} else if (bts.pressed.right or bts.pressed.left) {
		switch (cursor_) {
			case 0: {
				bool value = !video::full_screen();
				video::full_screen(value);
				break;
			}
			case 1: {
				i32 value = video::scaling();
				if (bts.pressed.right) {
					++value;
				} else {
					--value;
				}
				video::scaling(value);
				break;
			}
			case 2: {
				bool value = !video::vertical_sync();
				video::vertical_sync(value);
				break;
			}
			default:
				break;
		}
		this->setup_text_();
	} else if (bts.pressed.cancel or bts.pressed.options) {
		flags_.active = false;
	}
	if (!flags_.active) {
		input::zero(bts);
		audio::play(sfx::Inven, 0);
	}
}

void video_widget::setup_text_() {
	const std::string YES = vfs::i18n_at(MAIN_ENTRY, 1);
	const std::string NO = vfs::i18n_at(MAIN_ENTRY, 2);
	fmt::memory_buffer out {};
	out.append(vfs::i18n_from(VIDEO_ENTRY, 0, 1));
	if (video::full_screen()) {
		out.append(YES);
	} else {
		out.append(NO);
	}
	fmt::format_to(
		std::back_inserter(out),
		"\n{}{}\n{}",
		vfs::i18n_at(VIDEO_ENTRY, 2),
		video::scaling(),
		vfs::i18n_at(VIDEO_ENTRY, 3)
	);
	if (video::vertical_sync()) {
		out.append(YES);
	} else {
		out.append(NO);
	}
	text_.replace(fmt::to_string(out));
}

void audio_widget::build(const bitmap_font* font, controller&, overlay&) {
	flags_.ready = true;
	text_.build(
		DEFAULT_POSITION, {},
		chroma::WHITE(),
		font,
		{}
	);
	const glm::vec2 fd = text_.font_dimensions();
	arrow_.build(
		{ fd.x, TAB_SPACE + DEFAULT_POSITION.y + (fd.y * 2.0f) },
		STATE_ARROW, 0,
		vfs::find_animation(anim::Heads)
	);
	this->setup_text_();
}

void audio_widget::handle(buttons& bts, controller&, overlay&, headsup&) {
	const glm::vec2 fd = text_.font_dimensions();
	if (bts.pressed.up) {
		if (cursor_ > 0) {
			--cursor_;
			arrow_.transform(0.0f, -fd.y);
		} else {
			cursor_ = AUDIO_OPTIONS;
			arrow_.transform(0.0f, fd.y * as<r32>(cursor_));
		}
		audio::play(sfx::Select, 0);
	} else if (bts.pressed.down) {
		if (cursor_ < AUDIO_OPTIONS) {
			++cursor_;
			arrow_.transform(0.0f, fd.y);
		} else {
			cursor_ = 0;
			arrow_.transform(0.0f, -fd.y * as<r32>(AUDIO_OPTIONS));
		}
		audio::play(sfx::Select, 0);
	} else if (bts.holding.right or bts.holding.left) {
		switch (cursor_) {
			case 0: {
				r32 value = audio::volume();
				if (bts.holding.right) {
					value += 0.01f;
				} else {
					value -= 0.01f;
				}
				audio::volume(value);
				break;
			}
			case 1: {
				r32 value = music::volume();
				if (bts.holding.right) {
					value += 0.01f;
				} else {
					value -= 0.01f;
				}
				music::volume(value);
				break;
			}
			default:
				break;
		}
		this->setup_text_();
	} else if (bts.pressed.cancel or bts.pressed.options) {
		flags_.active = false;
	}
	if (!flags_.active) {
		input::zero(bts);
		audio::play(sfx::Inven, 0);
	}
}

void audio_widget::setup_text_() {
	fmt::memory_buffer out {};
	fmt::format_to(
		std::back_inserter(out),
		"{}{}\n{}{}",
		vfs::i18n_from(AUDIO_ENTRY, 0, 1),
		audio::volume(),
		vfs::i18n_at(AUDIO_ENTRY, 2),
		music::volume()
	);
	text_.replace(fmt::to_string(out));
}

void language_widget::build(const bitmap_font* font, controller&, overlay&) {
	languages_ = vfs::list_languages();
	if (!languages_.empty()) {
		flags_.ready = true;
		last_ = LANGUAGE_OPTIONS;
		text_.build(
			DEFAULT_POSITION, {},
			chroma::WHITE(),
			font, {}
		);
		const glm::vec2 fd = text_.font_dimensions();
		arrow_.build(
			{ fd.x, TAB_SPACE + DEFAULT_POSITION.y + (fd.y * 2.0f) },
			STATE_ARROW, 0,
			vfs::find_animation(anim::Heads)
		);
		this->setup_text_();
	}
}

void language_widget::handle(buttons& bts, controller& ctl, overlay& ovl, headsup& hud) {
	bool selection = false;
	const glm::vec2 fd = text_.font_dimensions();
	if (bts.pressed.up) {
		if (cursor_ > 0) {
			--cursor_;
			if (cursor_ < first_) {
				--first_;
				--last_;
				this->setup_text_();
			} else {
				arrow_.transform(0.0f, -fd.y);
			}
		} else {
			cursor_ = languages_.size() - 1;
			arrow_.position(
				fd.x,
				TAB_SPACE + DEFAULT_POSITION.y + (fd.y * 2.0f)
			);
			if (cursor_ > LANGUAGE_OPTIONS) {
				first_ = (cursor_ + 1) - LANGUAGE_OPTIONS;
				last_ = cursor_ + 1;
				arrow_.transform(0.0f, fd.y * as<r32>(LANGUAGE_OPTIONS - 1));
			} else {
				first_ = 0;
				last_ = LANGUAGE_OPTIONS;
				arrow_.transform(0.0f, fd.y * as<r32>(cursor_));
			}
			this->setup_text_();
		}
		audio::play(sfx::Select, 0);
	} else if (bts.pressed.down) {
		if (cursor_ < (languages_.size() - 1)) {
			++cursor_;
			if (cursor_ >= last_) {
				++first_;
				++last_;
				this->setup_text_();
			} else {
				arrow_.transform(0.0f, fd.y);
			}
		} else {
			cursor_ = 0;
			first_ = 0;
			last_ = LANGUAGE_OPTIONS;
			arrow_.position(
				fd.x,
				TAB_SPACE + DEFAULT_POSITION.y + (fd.y * 2.0f)
			);
			this->setup_text_();
		}
		audio::play(sfx::Select, 0);
	} else if (bts.pressed.confirm) {
		flags_.active = false;
		selection = true;
	} else if (bts.pressed.cancel or bts.pressed.options) {
		flags_.active = false;
	}
	if (!flags_.active) {
		input::zero(bts);
		if (selection) {
			ctl.language(languages_[cursor_]);
			ovl.clear();
			hud.fade_out();
			audio::play(sfx::TitleBeg, 0);
		} else {
			audio::play(sfx::Inven, 0);
		}
	}
}

void language_widget::setup_text_() {
	fmt::memory_buffer out {};
	out.append(vfs::i18n_at(LANGUAGE_ENTRY, 0));
	for (udx idx = first_; idx < last_ and idx < languages_.size(); ++idx) {
		fmt::format_to(std::back_inserter(out), "\t {}\n", languages_[idx]);
	}
	text_.replace(fmt::to_string(out));
}

void profile_widget::build(const bitmap_font* font, controller& ctl, overlay&) {
	if (flags_.active) {
		flags_.ready = true;
		cursor_ = ctl.profile();
		text_.build(
			DEFAULT_POSITION, {},
			chroma::WHITE(),
			font, {}
		);
		const glm::vec2 fd = text_.font_dimensions();
		arrow_.build(
			{ fd.x, TAB_SPACE + DEFAULT_POSITION.y + (fd.y * 2.0f) },
			STATE_ARROW, 0,
			vfs::find_animation(anim::Heads)
		);
		arrow_.transform(0.0f, as<r32>(cursor_) * fd.y);
		this->setup_text_();
		ctl.freeze();
	} else {
		spdlog::error("Profile widget has an invalid sub-type assigned to it!");
	}
}

void profile_widget::handle(buttons& bts, controller& ctl, overlay& ovl, headsup& hud) {
	const glm::vec2 fd = text_.font_dimensions();
	if (bts.pressed.up) {
		if (cursor_ > 0) {
			--cursor_;
			arrow_.transform(0.0f, -fd.y);
		} else {
			cursor_ = PROFILE_OPTIONS;
			arrow_.transform(0.0f, fd.y * as<r32>(cursor_));
		}
		audio::play(sfx::Select, 0);
	} else if (bts.pressed.down) {
		if (cursor_ < PROFILE_OPTIONS) {
			++cursor_;
			arrow_.transform(0.0f, fd.y);
		} else {
			cursor_ = 0;
			arrow_.transform(0.0f, -fd.y * as<r32>(PROFILE_OPTIONS));
		}
		audio::play(sfx::Select, 0);
	} else if (bts.pressed.confirm) {
		flags_.active = false;
		if (type_ == widget_type::record) {
			saving_ = true;
		} else {
			loading_ = true;
		}
		ctl.profile(cursor_);
		audio::play(sfx::TitleBeg, 0);
	} else if (bts.pressed.cancel) {
		flags_.active = false;
	}
	if (!flags_.active) {
		input::zero(bts);
		if (saving_) {
			ctl.save();
			ctl.unlock();
		} else if (loading_) {
			hud.fade_out();
			ovl.clear();
			ctl.load();
		} else {
			ctl.unlock();
		}
	}
}

void profile_widget::setup_text_() {
	fmt::memory_buffer out {};
	if (type_ == widget_type::record) {
		out.append(vfs::i18n_at(PROFILE_ENTRY, 0));
	} else {
		out.append(vfs::i18n_at(PROFILE_ENTRY, 1));
	}
	const std::string figure = vfs::i18n_at(PROFILE_ENTRY, 2);
	auto iter = std::back_inserter(out);
	for (udx idx = 0; idx < controller::MAXIMUM_PROFILES; ++idx) {
		fmt::format_to(iter, figure, idx + 1);
	}
	text_.replace(fmt::to_string(out));
}

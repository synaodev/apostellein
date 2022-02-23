#pragma once

#include "./widget-interface.hpp"
#include "../gui/text.hpp"
#include "../gui/scheme.hpp"

struct option_widget : public widget_interface {
	~option_widget() = default;
public:
	void build(const bitmap_font* font, controller& ctl, overlay& ovl) override;
	void invalidate() const override {
		if (flags_.ready and flags_.active) {
			text_.invalidate();
			arrow_.invalidate();
		}
	}
	void fix(const bitmap_font* font) override {
		text_.fix(font);
		arrow_.fix();
	}
	void handle(buttons& bts, controller& ctl, overlay& ovl, headsup& hud) override;
	void update(i64 delta) override {
		if (flags_.ready and flags_.active) {
			arrow_.update(delta);
		}
	}
	void render(renderer& rdr) const override {
		if (flags_.ready and flags_.active) {
			text_.render(rdr);
			arrow_.render(rdr);
		}
	}
private:
	udx cursor_ {};
	gui::text text_ {};
	gui::scheme arrow_ {};
};

struct input_widget : public widget_interface {
	~input_widget() = default;
public:
	void build(const bitmap_font* font, controller& ctl, overlay& ovl) override;
	void invalidate() const override {
		if (flags_.ready and flags_.active) {
			header_.invalidate();
			left_text_.invalidate();
			right_text_.invalidate();
			arrow_.invalidate();
		}
	}
	void fix(const bitmap_font* font) override {
		header_.fix(font);
		left_text_.fix(font);
		right_text_.fix(font);
		arrow_.fix();
	}
	void handle(buttons& bts, controller& ctl, overlay& ovl, headsup& hud) override;
	void update(i64 delta) override {
		if (flags_.ready and flags_.active) {
			arrow_.update(delta);
		}
	}
	void render(renderer& rdr) const override {
		if (flags_.ready and flags_.active) {
			header_.render(rdr);
			left_text_.render(rdr);
			right_text_.render(rdr);
			if (waiting_) {
				if (flash_) {
					arrow_.invalidate();
				} else {
					arrow_.render(rdr);
				}
			} else {
				arrow_.render(rdr);
			}
		}
	}
private:
	void setup_left_text_();
	void setup_right_text_();
	bool left_side_ { true };
	bool waiting_ {};
	bool flash_ {};
	udx cursor_ {};
	gui::text header_ {};
	gui::text left_text_ {};
	gui::text right_text_ {};
	gui::scheme arrow_ {};
};

struct video_widget : public widget_interface {
	~video_widget() = default;
public:
	void build(const bitmap_font* font, controller& ctl, overlay& ovl) override;
	void invalidate() const override {
		if (flags_.ready and flags_.active) {
			text_.invalidate();
			arrow_.invalidate();
		}
	}
	void fix(const bitmap_font* font) override {
		text_.fix(font);
		arrow_.fix();
	}
	void handle(buttons& bts, controller& ctl, overlay& ovl, headsup& hud) override;
	void update(i64 delta) override {
		if (flags_.ready and flags_.active) {
			arrow_.update(delta);
		}
	}
	void render(renderer& rdr) const override {
		if (flags_.ready and flags_.active) {
			text_.render(rdr);
			arrow_.render(rdr);
		}
	}
private:
	void setup_text_();
	udx cursor_ {};
	gui::text text_ {};
	gui::scheme arrow_ {};
};

struct audio_widget : public widget_interface {
	~audio_widget() = default;
public:
	void build(const bitmap_font* font, controller& ctl, overlay& ovl) override;
	void invalidate() const override {
		if (flags_.ready and flags_.active) {
			text_.invalidate();
			arrow_.invalidate();
		}
	}
	void fix(const bitmap_font* font) override {
		text_.fix(font);
		arrow_.fix();
	}
	void handle(buttons& bts, controller& ctl, overlay& ovl, headsup& hud) override;
	void update(i64 delta) override {
		if (flags_.ready and flags_.active) {
			arrow_.update(delta);
		}
	}
	void render(renderer& rdr) const override {
		if (flags_.ready and flags_.active) {
			text_.render(rdr);
			arrow_.render(rdr);
		}
	}
private:
	void setup_text_();
	udx cursor_ {};
	gui::text text_ {};
	gui::scheme arrow_ {};
};

struct language_widget : public widget_interface {
	~language_widget() = default;
public:
	void build(const bitmap_font* font, controller& ctl, overlay& ovl) override;
	void invalidate() const override {
		if (flags_.ready and flags_.active) {
			text_.invalidate();
			arrow_.invalidate();
		}
	}
	void fix(const bitmap_font* font) override {
		text_.fix(font);
		arrow_.fix();
	}
	void handle(buttons& bts, controller& ctl, overlay& ovl, headsup& hud) override;
	void update(i64 delta) override {
		if (flags_.ready and flags_.active) {
			arrow_.update(delta);
		}
	}
	void render(renderer& rdr) const override {
		if (flags_.ready and flags_.active) {
			text_.render(rdr);
			arrow_.render(rdr);
		}
	}
private:
	void setup_text_();
	std::vector<std::string> languages_ {};
	udx cursor_ {};
	udx first_ {};
	udx last_ {};
	gui::text text_ {};
	gui::scheme arrow_ {};
};

struct profile_widget : public widget_interface {
	profile_widget(widget_type type) noexcept : type_{ type } {
		if (type != widget_type::record and type != widget_type::resume) {
			flags_.active = false;
		}
	}
	~profile_widget() = default;
public:
	void build(const bitmap_font* font, controller& ctl, overlay& ovl) override;
	void invalidate() const override {
		if (flags_.ready and flags_.active) {
			text_.invalidate();
			arrow_.invalidate();
		}
	}
	void fix(const bitmap_font* font) override {
		text_.fix(font);
		arrow_.fix();
	}
	void handle(buttons& bts, controller& ctl, overlay& ovl, headsup& hud) override;
	void update(i64 delta) override {
		if (flags_.ready and flags_.active) {
			arrow_.update(delta);
		}
	}
	void render(renderer& rdr) const override {
		if (flags_.ready and flags_.active) {
			text_.render(rdr);
			arrow_.render(rdr);
		}
	}
private:
	void setup_text_();
	widget_type type_ { widget_type::option };
	bool loading_ {};
	bool saving_ {};
	udx cursor_ {};
	gui::text text_ {};
	gui::scheme arrow_ {};
};

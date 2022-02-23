#pragma once

#include <apostellein/struct.hpp>

struct config_file;
struct bitmap_font;
struct renderer;
struct buttons;
struct controller;

enum class widget_type {
	option,
	input,
	video,
	audio,
	language,
	record,
	resume
};

struct overlay;
struct headsup;

struct widget_interface : public not_copyable {
	virtual ~widget_interface() = default;
public:
	virtual void build(const bitmap_font* font, controller& ctl, overlay& ovl) = 0;
	virtual void invalidate() const = 0;
	virtual void fix(const bitmap_font* font) = 0;
	virtual void handle(buttons& bts, controller& ctl, overlay& ovl, headsup& hud) = 0;
	virtual void update(i64 delta) = 0;
	virtual void render(renderer& rdr) const = 0;
	bool ready() const { return flags_.ready; }
	bool active() const { return flags_.active; }
protected:
	union {
		bitfield_raw<u32> _raw { 1 << 1 };
		bitfield_index<u32, 0> ready;
		bitfield_index<u32, 1> active;
	} flags_ {};
};

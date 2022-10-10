#pragma once

#include "../gui/text.hpp"
#include "../gui/scheme.hpp"

struct buttons;
struct overlay;
struct headsup;
struct inventory;

struct dialogue {
public:
	static constexpr u32 MAXIMUM_CHOICES = 4;
	void build();
	void clear() { this->close_textbox(); }
	void invalidate() const { invalidated_ = true; }
	void fix();
	void handle(const buttons& bts, headsup& hud, const inventory& ivt);
	void update(i64 delta);
	void render(renderer& rdr) const;
	void open_textbox_high();
	void open_textbox_low();
	template<typename T>
	void append_textbox(const std::basic_string<T>& words) {
		text_.append(words, false);
	}
	void clear_textbox() { text_.clear(); }
	void close_textbox();
	void open_facebox(udx state, udx variation);
	void close_facebox();
	void custom_delay(r32 value);
	void reset_delay();
	void color_text(i32 r, i32 g, i32 b);
	void forward_question(std::u32string&& string, udx choices);
	bool textbox_open() const { return flags_.textbox; }
	bool has_question() const { return flags_.question; }
	bool writing() const { return flags_.writing; }
	udx answer() const { return cursor_ + 1; }
private:
	mutable bool invalidated_ {};
	union {
		bitfield_raw<u32> _raw {};
		bitfield_index<u32, 0> textbox;
		bitfield_index<u32, 1> facebox;
		bitfield_index<u32, 2> question;
		bitfield_index<u32, 3> writing;
		bitfield_index<u32, 4> delay;
		bitfield_index<u32, 5> sound;
	} flags_ {};
	udx cursor_ {};
	udx choices_ {};
	i64 timer_ {};
	i64 delay_ {};
	rect raster_ {};
	gui::text text_ {};
	gui::scheme faces_ {};
	gui::scheme arrow_ {};
};

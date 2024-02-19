#pragma once

#include "../gui/barrier.hpp"
#include "../gui/counter.hpp"
#include "../gui/fader.hpp"
#include "../gui/graphic.hpp"
#include "../gui/meter.hpp"
#include "../gui/provision.hpp"
#include "../gui/scheme.hpp"
#include "../gui/title.hpp"

struct overlay;
struct dialogue;
struct inventory;
struct kernel;
struct controller;

struct headsup_params {
public:
	bool strafing {};
	i32 indication {};
	i32 current_barrier {};
	i32 maximum_barrier {};
	i32 poison {};
	i32 current_oxygen {};
	i32 maximum_oxygen {};
};

struct headsup {
public:
	void build();
	void clear() {
		indicator_.variation(0);
		meter_.clear();
		graphic_.clear();
		fader_.clear();
	}
	void invalidate() const { title_.invalidate(); }
	void fix();
	void prepare() { fader_.prepare(); }
	void handle(const controller& ctl);
	void update(i64 delta) {
		title_.update(delta);
		indicator_.update(delta);
		meter_.update(delta);
	}
	void render(r32 ratio, renderer& rdr, const controller& ctl) const;
	void set(const headsup_params& params);
	void clear_title() { title_.clear(); }
	template<typename T>
	void display_title(const std::basic_string<T>& words) {
		title_.display(words);
	}
	void push_message(
		bool centered,
		const glm::vec2& position,
		udx index,
		const std::string& words
	);
	void forward_message(
		bool centered,
		const glm::vec2& position,
		udx index,
		std::u32string&& words
	);
	void show_graphic(const std::string& name, const glm::vec2& position) {
		graphic_.set(name);
		graphic_.position(position);
	}
	void hide_graphic() { graphic_.clear(); }
	void fade_out() { fader_.fade_out(); }
	void fade_in() { fader_.fade_in(); }
	void meter(i32 current, i32 maximum) { meter_.set(current, maximum); }
	udx indication() const { return this->indicator_.variation(); }
	bool fader_visible() const { return fader_.visible(); }
	bool fader_moving() const { return fader_.moving(); }
	bool fader_finished() const { return fader_.finished(); }
private:
	gui::title title_ {};
	gui::scheme indicator_ {};
	gui::counter poison_ {};
	gui::barrier barrier_ {};
	gui::counter oxygen_ {};
	gui::provision provision_ {};
	gui::meter meter_ {};
	gui::graphic graphic_ {};
	gui::fader fader_ {};
};

#pragma once

#include "../gui/element.hpp"

struct buttons;
struct overlay;
struct headsup;
struct dialogue;
struct kernel;
struct controller;

struct inventory {
public:
	void build();
	void clear();
	void invalidate() const { invalidated_ = true; }
	void fix() {
		for (auto&& elm : elements_) {
			elm.fix();
		}
		invalidated_ = true;
	}
	void handle(
		const buttons& bts,
		controller& ctl,
		kernel& knl,
		const overlay& ovl,
		headsup& hud,
		const dialogue& dlg
	);
	void render(renderer& rdr) const;
	bool active() const { return active_; }
private:
	mutable bool invalidated_ {};
	bool active_ {};
	udx provision_ {};
	rect cursor_raster_ {};
	rect provision_raster_ {};
	std::vector<gui::element> elements_ {};
};

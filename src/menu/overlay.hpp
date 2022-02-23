#pragma once

#include <memory>
#include <vector>

#include "./widget-interface.hpp"

struct headsup;
struct dialogue;
struct controller;

struct overlay {
public:
	void reset() {
		invalidated_ = true;
		release_ = false;
		widgets_.clear();
	}
	void invalidate() const { invalidated_ = true; }
	void fix();
	void handle(buttons& bts, controller& ctl, headsup& hud);
	void update(i64 delta) {
		if (!widgets_.empty()) {
			auto& wgt = *widgets_.back();
			wgt.update(delta);
		}
	}
	void render(renderer& rdr) const;
	void push(widget_type type);
	void clear() {
		if (!widgets_.empty()) {
			release_ = true;
		}
	}
	bool empty() const { return widgets_.empty(); }
	void load() { this->push(widget_type::resume); }
	void save() { this->push(widget_type::record); }
private:
	mutable bool invalidated_ {};
	bool release_ {};
	std::vector<std::unique_ptr<widget_interface> > widgets_ {};
};

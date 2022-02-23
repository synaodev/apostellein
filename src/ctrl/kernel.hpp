#pragma once

#include <string>
#include <map>
#include <sol/sol.hpp>
#include <apostellein/def.hpp>

struct buttons;
struct overlay;
struct headsup;
struct dialogue;
struct inventory;
struct environment;
struct player;
struct camera;
struct controller;

struct kernel {
public:
	bool build(
		controller& ctl,
		overlay& ovl,
		headsup& hud,
		dialogue& dlg,
		camera& cam,
		player& plr,
		environment& env
	);
	void clear();
	void handle(
		const buttons& bts,
		controller& ctl,
		const overlay& ovl,
		const headsup& hud,
		dialogue& dlg,
		const inventory& ivt
	);
	void update(i64 delta) {
		if (waiting_) {
			if (timer_ -= delta; timer_ <= 0) {
				timer_ = 0;
				running_ = true;
				waiting_ = false;
				stalling_ = false;
			}
		}
	}
	bool load(const std::string& name);
	void reload();
	void push(i32 id, const std::string& name);
	bool run_death(u32 type);
	bool run_inventory(u32 type);
	bool run_event(i32 id);
	bool run_symbol(const std::string& symbol);
	bool run_transfer(const controller& ctl);
	bool running() const { return running_; }
	bool changed() {
		bool result = changed_;
		changed_ = false;
		return result;
	}
	udx kilobytes() const { return engine_.memory_used() / 1024; }
	std::vector<std::string> symbols() const;
private:
	void execute_(const sol::function& event);
	void setup_api_(
		controller& ctl,
		overlay& ovl,
		headsup& hud,
		dialogue& dlg,
		camera& cam,
		player& plr,
		environment& env
	);
	sol::state engine_ {};
	sol::coroutine resume_ {};
	sol::optional<u32> param_ {};
	std::map<i32, sol::function> events_ {};
	std::string module_ {};
	i64 timer_ {};
	bool running_ {};
	bool waiting_ {};
	bool stalling_ {};
	bool faulted_ {};
	bool changed_ {};
};

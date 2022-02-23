#include <spdlog/spdlog.h>
#include <apostellein/konst.hpp>

#include "./kernel.hpp"
#include "./controller.hpp"
#include "../menu/overlay.hpp"
#include "../menu/headsup.hpp"
#include "../menu/dialogue.hpp"
#include "../menu/inventory.hpp"
#include "../field/environment.hpp"
#include "../field/player.hpp"
#include "../field/camera.hpp"
#include "../hw/audio.hpp"
#include "../hw/music.hpp"
#include "../hw/vfs.hpp"
#include "../hw/rng.hpp"
#include "../util/buttons.hpp"
#include "../video/material.hpp"

namespace {
	constexpr char INIT_MODULE[] = "init";
	constexpr char BOOT_SYMBOL[] = "boot";
	constexpr char MAIN_SYMBOL[] = "main";
	constexpr char DEATH_SYMBOL[] = "death";
	constexpr char INVENTORY_SYMBOL[] = "inventory";
}

bool kernel::build(
	controller& ctl,
	overlay& ovl,
	headsup& hud,
	dialogue& dlg,
	camera& cam,
	player& plr,
	environment& env
) {
	// initialize modules
	engine_.open_libraries(
		sol::lib::base,
		sol::lib::package,
		sol::lib::coroutine
	);
	this->setup_api_(
		ctl, ovl, hud, dlg,
		cam, plr, env
	);

	// load global module
	const std::string source = vfs::buffer_string(vfs::global_script_path(INIT_MODULE));
	if (!engine_.require_script(INIT_MODULE, source).valid()) {
		spdlog::critical("Loading global module failed!");
		return false;
	}
	return true;
}

void kernel::clear() {
	param_ = sol::nullopt;
	events_.clear();
	module_.clear();
	timer_ = 0;
	running_ = false;
	waiting_ = false;
	stalling_ = false;
}

void kernel::handle(
	const buttons& bts,
	controller& ctl,
	const overlay& ovl,
	const headsup& hud,
	dialogue& dlg,
	const inventory& ivt
) {
	if (running_ and !faulted_) {
		if (!hud.fader_moving() and !dlg.has_question()) {
			if (stalling_) {
				if (!dlg.writing() and bts.pressed.confirm) {
					timer_ = 0;
					waiting_ = false;
					stalling_ = false;
				}
			} else if (!waiting_) {
				const auto result = param_.take().map_or_else(
					[this](u32 param) { return this->resume_(param); },
					[this] { return this->resume_(); }
				);
				switch (const auto status = result.status()) {
					case sol::call_status::ok: {
						if (ovl.empty() and !ivt.active()) {
							ctl.unlock();
						}
						ovl.invalidate();
						dlg.close_textbox();
						ivt.invalidate();
						timer_ = 0;
						running_ = false;
						waiting_ = false;
						stalling_ = false;
						break;
					}
					case sol::call_status::yielded: {
						break;
					}
					default: {
						auto as_chars = [](const sol::call_status& s) {
							switch (s) {
							case sol::call_status::runtime: return "runtime";
							case sol::call_status::syntax: return "syntax";
							case sol::call_status::memory: return "memory";
							case sol::call_status::gc: return "garbage collection";
							case sol::call_status::handler: return "handler";
							case sol::call_status::file: return "file";
							default: break;
							}
							return "unknown";
						};
						{
							const sol::error error = result;
							spdlog::critical(
								"Event aborted due to {} error! Lua Exception: {}",
								as_chars(status),
								error.what()
							);
						}
						if (ovl.empty() and !ivt.active()) {
							ctl.freeze();
						}
						faulted_ = true;
						break;
					}
				}
			}
		}
	}
}

bool kernel::load(const std::string& name) {
	const std::string source = vfs::buffer_string(vfs::local_script_path(name));
	if (!engine_.require_script(name, source).valid()) {
		spdlog::error("Loading module \"{}\" failed!", name);
		return false;
	}
	return true;
}

void kernel::reload() {
	// TODO: re-write this so it's marginally less insane
	const auto references = [this] {
		std::map<i32, std::string> result {};
		for (auto&& [key, value] : this->engine_[this->module_].get<sol::table>()) {
			for (auto&& [id, event] : this->events_) {
				if (value.as<sol::function>() == event) {
					result[id] = key.as<std::string>();
				}
			}
		}
		return result;
	}();
	events_.clear();

	// no turning back
	engine_["package"]["loaded"][module_] = sol::lua_nil;

	if (!this->load(module_)) {
		// don't bother backing anything up
		// if something goes wrong, just bail
		const std::string message = fmt::format("Reloading \"{}.lua\" failed! Recovery is impossible!", module_);
		spdlog::critical(message);
		throw std::runtime_error(message);
	}
	for (auto&& [id, symbol] : references) {
		if (const sol::optional<sol::function> maybe = engine_[module_][symbol]; maybe) {
			events_[id] = *maybe;
		} else {
			// this situation also merits giving up
			const std::string message = fmt::format(
				"A referenced event from \"{}.lua\" is missing! Recovery is impossible!",
				module_
			);
			spdlog::critical(message);
			throw std::runtime_error(message);
		}
	}
}

void kernel::push(i32 id, const std::string& name) {
	if (id <= 0) {
		spdlog::error("Events must be assigned an ID greater than zero!");
		return;
	}
	if (events_.find(id) != events_.end()) {
		spdlog::warn("Event-ID mapping will be overwritten!");
	}
	if (auto event = engine_[module_][name].get<sol::function>(); event.valid()) {
		events_[id] = event;
	} else if (event = engine_[INIT_MODULE][name].get<sol::function>(); event.valid()) {
		events_[id] = event;
	} else {
		spdlog::error("There are no events called \"{}\"! Triggering it will fail!", name);
	}
}

bool kernel::run_death(u32 type) {
	if (!running_) {
		if (const auto event = engine_[INIT_MODULE][DEATH_SYMBOL].get<sol::function>(); event.valid()) {
			param_ = type;
			this->execute_(event);
			return true;
		}
	}
	spdlog::error("Couldn't run death event!");
	return false;
}

bool kernel::run_inventory(u32 type) {
	if (!running_) {
		if (const auto event = engine_[INIT_MODULE][INVENTORY_SYMBOL].get<sol::function>(); event.valid()) {
			param_ = type;
			this->execute_(event);
			return true;
		}
	}
	spdlog::error("Couldn't run inventory event!");
	return false;
}

bool kernel::run_event(i32 id) {
	if (!running_ and id > 0) {
		if (auto iter = events_.find(id); iter != events_.end()) {
			this->execute_(iter->second);
			return true;
		}
	}
	spdlog::error("Couldn't run aktor event!");
	return false;
}

bool kernel::run_symbol(const std::string& symbol) {
	if (!running_) {
		const auto event = [this, &symbol]() -> sol::object {
			for (auto&& [key, value] : this->engine_[module_].get<sol::table>()) {
				if (key.as<std::string>() == symbol) {
					return value.as<sol::function>();
				}
			}
			return sol::make_object(this->engine_.lua_state(), sol::lua_nil);
		}();
		if (event != sol::lua_nil) {
			this->execute_(event);
			return true;
		}
	}
	spdlog::error("Couldn't run aktor event from symbol!");
	return false;
}

bool kernel::run_transfer(const controller& ctl) {
	changed_ = true;
	if (!running_) {
		auto& state = ctl.state();
		if (state.init) {
			if (const auto event = engine_[INIT_MODULE][BOOT_SYMBOL].get<sol::function>(); event.valid()) {
				this->execute_(event);
				return true;
			}
		} else if (state.importing) {
			module_ = ctl.field();
			if (const auto event = engine_[module_][ctl.event()].get<sol::function>(); event.valid()) {
				this->execute_(event);
				return true;
			}
		} else {
			module_ = ctl.field();
			if (const auto event = engine_[module_][MAIN_SYMBOL].get<sol::function>(); event.valid()) {
				this->execute_(event);
				return true;
			}
		}
	}
	spdlog::error("Couldn't run transfer event!");
	return false;
}

std::vector<std::string> kernel::symbols() const {
	std::vector<std::string> result {};
	engine_[module_].get<sol::table>().for_each([&result](sol::object key, sol::object) {
		result.push_back(key.as<std::string>());
	});
	return result;
}

void kernel::execute_(const sol::function& event) {
	if (!resume_) {
		resume_ = event;
		timer_ = 0;
		running_ = true;
		waiting_ = false;
		stalling_ = false;
	} else {
		spdlog::error("Previous event has not finished!");
	}
}

void kernel::setup_api_(
	controller& ctl,
	overlay& ovl,
	headsup& hud,
	dialogue& dlg,
	camera& cam,
	player& plr,
	environment& env
) {
	{
		auto tbl = engine_["sys"].get_or_create<sol::table>();

		tbl.set_function("log", [](std::string string) {
			spdlog::info(string);
		});
		tbl.set_function("stall", sol::yielding([this] {
			this->running_ = true;
			this->waiting_ = false;
			this->stalling_ = true;
			this->timer_ = 0;
		}));
		tbl.set_function("wait", sol::yielding([this](r64 seconds) {
			this->running_ = true;
			this->waiting_ = true;
			this->stalling_ = false;
			this->timer_ = konst::SECONDS_TO_NANOSECONDS(seconds);
		}));
		tbl.set_function("link", [this](std::string field, std::string event) -> sol::object {
			if (!this->load(field)) {
				return sol::make_object(this->engine_.lua_state(), sol::lua_nil);
			}
			if (const sol::optional<sol::function> maybe = this->engine_[field][event]; !maybe) {
				return sol::make_object(this->engine_.lua_state(), sol::lua_nil);
			}
			return this->engine_.create_table_with(
				"field", field,
				"event", event
			);
		});
		tbl.set_function("call", [&ctl](sol::table data, i32 id) {
			const sol::optional<std::string> maybe_field = data["field"];
			const sol::optional<std::string> maybe_event = data["event"];
			if (maybe_field and maybe_event) {
				ctl.transfer(*maybe_field, *maybe_event, id);
			}
		});
		tbl.set_function("transfer", [&ctl](std::string field, i32 id) {
			ctl.transfer(field, id);
		});
		tbl.set_function("lock", [&ctl] {
			ctl.lock();
		});
		tbl.set_function("unlock", [&ctl] {
			ctl.unlock();
		});
		tbl.set_function("freeze", [&ctl] {
			ctl.freeze();
		});
		tbl.set_function("boot", [&ctl] {
			ctl.boot();
		});
		tbl.set_function("quit", [&ctl] {
			ctl.quit();
		});
		tbl.set_function("load", [&ctl] {
			ctl.load();
		});
		tbl.set_function("save", [&ctl] {
			ctl.save();
		});
		tbl.set_function("flag", [&ctl](udx bit, bool value) {
			ctl.flag_at(bit, value);
		});
		tbl.set_function("flags", [&ctl](udx first, udx last, bool value) {
			ctl.flags_from(first, last, value);
		});
		tbl.set_function("flag_at", [&ctl](udx bit) {
			return ctl.flag_at(bit);
		});
		tbl.set_function("query_flags", [&ctl](udx first, udx last) {
			return ctl.query_flags(first, last);
		});
		tbl.set_function("push_item", [&ctl](i32 type, i32 count) {
			return ctl.push_item(type, count);
		});
		tbl.set_function("add_item", [&ctl](i32 type, i32 count) {
			return ctl.add_item(type, count);
		});
		tbl.set_function("remove_item", [&ctl](i32 type) {
			return ctl.remove_item(type);
		});
		tbl.set_function("subtract_item", [&ctl](i32 type, i32 count) {
			return ctl.subtract_item(type, count);
		});
		tbl.set_function("count_item", [&ctl](i32 type) {
			return ctl.count_items(type);
		});
		tbl.set_function("limit_item", [&ctl](i32 type, i32 limit) {
			return ctl.limit_item(type, limit);
		});
		tbl.set_function("weaponize_item", [&ctl](i32 type, bool weapon) {
			return ctl.weaponize_item(type, weapon);
		});
		tbl.set_function("i18n", [](std::string segment, udx index) {
			return vfs::i18n_at(segment, index - 1);
		});
		tbl.set_function("i18n_size", [](std::string segment) {
			return vfs::i18n_size(segment);
		});
		tbl.set_function("material", [](std::string name) {
			auto ptr = vfs::find_material(name);
			if (ptr) {
				return ptr->valid();
			}
			return false;
		});
	}
	{
		auto tbl = engine_["hud"].get_or_create<sol::table>();

		tbl.set_function("save", [&ovl] {
			ovl.save();
		});
		tbl.set_function("load", [&ovl] {
			ovl.load();
		});
		tbl.set_function("fade_in", sol::yielding([&hud] {
			hud.fade_in();
		}));
		tbl.set_function("fade_out", sol::yielding([&hud] {
			hud.fade_out();
		}));
		tbl.set_function("show", [&hud](std::string name, r32 x, r32 y) {
			hud.show_graphic(name, { x, y });
		});
		tbl.set_function("hide", [&hud] {
			hud.hide_graphic();
		});
		tbl.set_function("title", [&hud](std::u32string words) {
			hud.display_title(words);
		});
		tbl.set_function("erase", [&hud] {
			hud.clear_title();
		});
		tbl.set_function("message", [&hud](bool centered, r32 x, r32 y, udx font, std::u32string words) {
			hud.forward_message(centered, { x, y }, font - 1, std::move(words));
		});
		tbl.set_function("low", [&dlg] {
			dlg.open_textbox_low();
		});
		tbl.set_function("high", [&dlg] {
			dlg.open_textbox_high();
		});
		tbl.set_function("close", [&dlg] {
			dlg.close_textbox();
		});
		tbl.set_function("clear", [&dlg] {
			dlg.clear_textbox();
		});
		tbl.set_function("say", [&dlg](std::u32string words) {
			dlg.append_textbox(words);
		});
		tbl.set_function("color", [&dlg](i32 r, i32 g, i32 b) {
			dlg.color_text(r, g, b);
		});
		tbl.set_function("face", [&dlg](udx state, udx variation) {
			dlg.open_facebox(state - 1, variation - 1);
		});
		tbl.set_function("delay", [&dlg](r32 delay) {
			dlg.custom_delay(delay);
		});
		tbl.set_function("internal_ask", [&dlg](sol::table array) {
			if (array.empty()) {
				spdlog::error("Question array is empty!");
				return;
			}
			if (array.size() > dialogue::MAXIMUM_CHOICES) {
				spdlog::error("Question array must be 1-4 in length!");
				return;
			}
			std::u32string question {};
			std::for_each(array.begin(), array.end(), [&question](const auto& element) {
				auto&& [_, value] = element;
				if (value.template is<std::string>()) {
					const auto string = fmt::format("   {}\n", value.template as<std::string>());
					gui::unicode(string, question);
				}
			});
			dlg.forward_question(std::move(question), array.size() - 1);
		});
		tbl.set_function("internal_answer", [&dlg] {
			return dlg.answer();
		});
		engine_.script(
			"hud.ask = function(data)\n"
				"hud.internal_ask(data);\n"
				"coroutine.yield();\n"
				"return hud.internal_answer();\n"
			"end"
		);
	}
	{
		auto tbl = engine_["sfx"].get_or_create<sol::table>();

		tbl.set_function("sound", [](std::string id) {
			audio::play(id);
		});
		tbl.set_function("clear", [] {
			music::clear();
		});
		tbl.set_function("load", [](std::string title) {
			return music::load(title);
		});
		tbl.set_function("play", [](r32 start, r32 fade) {
			return music::play(start, fade);
		});
		tbl.set_function("pause", [] {
			music::pause();
		});
		tbl.set_function("fade_out", [](r32 seconds) {
			music::fade(seconds);
		});
		tbl.set_function("resume", [](r32 fade) {
			music::resume(fade);
		});
		tbl.set_function("loop", [](bool loop) {
			music::loop(loop);
		});
		tbl.set_function("playing", [] {
			return music::playing();
		});
		tbl.set_function("looping", [] {
			return music::looping();
		});
	}
	{
		auto tbl = engine_["env"].get_or_create<sol::table>();

		tbl.set_function("spawn", [&env](std::string name, r32 x, r32 y, i32 id) {
			return env.try_spawn(name, x, y, id);
		});
		tbl.set_function("kill", [&env](i32 id) {
			env.kill(id);
		});
		tbl.set_function("animate", [&env](i32 id, udx state, udx variation) {
			env.animate(id, state - 1, variation - 1);
		});
		tbl.set_function("think", [&env](i32 id, u32 state) {
			env.think(id, state - 1);
		});
		tbl.set_function("event", [this, &env](i32 id, sol::function event) {
			if (env.search(id) != entt::null) {
				this->events_[id] = event;
			}
		});
		tbl.set_function("fight", [this, &env](i32 id, sol::function event) {
			if (env.fight(id)) {
				this->events_[id] = event;
			}
		});
		tbl.set_function("still", [&env](i32 id) {
			return env.still(id);
		});
	}
	{
		auto tbl = engine_["plr"].get_or_create<sol::table>();

		tbl.set_function("visible", [&plr, &env](bool visible){
			plr.visible(env, visible);
		});
		tbl.set_function("animate", [&plr, &env](udx state, u32 facing) {
			plr.animate(env, state - 1, facing);
		});
		tbl.set_function("teleport", [&plr, &env](r32 x, r32 y) {
			plr.teleport(env, { x, y });
		});
		tbl.set_function("restore", [&plr, &env](i32 amount) {
			plr.add_barrier(env, amount);
		});
		tbl.set_function("barrier", [&plr, &env](i32 amount) {
			plr.boost_barrier(env, amount);
		});
		tbl.set_function("poison", [&plr, &env](i32 amount) {
			plr.change_poison(env, amount);
		});
		tbl.set_function("equip", [&plr](u32 mask, bool value) {
			plr.change_equipment(mask, value);
		});
	}
	{
		auto tbl = engine_["cam"].get_or_create<sol::table>();

		tbl.set_function("quake", [&cam](r32 power) {
			cam.quake(power);
		});
		tbl.set_function("limited_quake", [&cam](r32 power, i32 ticks) {
			cam.quake(power, ticks);
		});
		tbl.set_function("follow", [&cam](i32 id) {
			cam.follow(id);
		});
	}
}

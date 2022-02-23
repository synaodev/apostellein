#include <nlohmann/json.hpp>
#include <tmxlite/Map.hpp>
#include <tmxlite/TileLayer.hpp>
#include <tmxlite/ImageLayer.hpp>
#include <tmxlite/ObjectGroup.hpp>
#include <spdlog/spdlog.h>
#include <apostellein/konst.hpp>

#include "./runtime.hpp"
#include "../ecs/aktor.hpp"
#include "../ecs/health.hpp"
#include "../hw/audio.hpp"
#include "../hw/input.hpp"
#include "../hw/video.hpp"
#include "../hw/vfs.hpp"
#include "../util/buttons.hpp"
#include "../video/material.hpp"
#include "../x2d/renderer.hpp"

namespace {
	constexpr char PROFILE_NAME[] = "profile-";
}

bool runtime::build(const config_file& cfg, const renderer& rdr) {
	if (!dbr_.build(cfg, rdr)) {
		return false;
	}
	ctl_.build();
	if (!knl_.build(ctl_, ovl_, hud_, dlg_, cam_, plr_, env_)) {
		return false;
	}
	ovl_.reset();
	hud_.build();
	dlg_.build();
	ivt_.build();
	cam_.clear();
	if (!plr_.build(env_)) {
		return false;
	}
	env_.build();
	map_.clear();

	this->boot_();
	return true;
}

void runtime::handle(udx ticks, activity_type& aty, buttons& bts) {
	// debugger
	dbr_.handle(bts, *this);
	// before handling
	hud_.prepare();
	cam_.prepare();
	env_.prepare();
	map_.prepare();
	// handle accumulated ticks
	for (; ticks > 0; --ticks) {
		auto& state = ctl_.state();
		if (hud_.fader_finished()) {
			if (state.language) {
				const std::string language = ctl_.language();
				if (!vfs::try_language(language)) {
					spdlog::error("Failed to switch language to \"{}\" during runtime!", language);
				} else {
					spdlog::info("Switched language to \"{}\" during runtime!", language);
				}
			}
			if (state.booting) {
				this->boot_();
			}
			if (state.loading) {
				this->load_();
			}
			if (state.transfering and !this->transfer_()) {
				ctl_.quit();
			}
		}
		if (state.saving) {
			this->save_();
		}
		if (state.quitting) {
			aty = activity_type::quitting;
		}
		ctl_.handle();
		knl_.handle(bts, ctl_, ovl_, hud_, dlg_, ivt_);
		ovl_.handle(bts, ctl_, hud_);
		hud_.handle(ctl_);
		dlg_.handle(bts, hud_, ivt_);
		ivt_.handle(bts, ctl_, knl_, ovl_, hud_, dlg_);
		if (!state.frozen) {
			cam_.handle(plr_, env_);
			plr_.handle(bts, ctl_, knl_, hud_, env_, map_);
			env_.handle(knl_, hud_, cam_, plr_, map_);
			map_.handle(cam_.view());
		}
		bts.clear();
	}
	// recalibrate virtual texture
	if (material::recalibrate()) {
		ovl_.fix();
		hud_.fix();
		dlg_.fix();
		ivt_.fix();
		map_.fix();
	}
}

void runtime::update(i64 delta) {
	knl_.update(delta);
	ovl_.update(delta);
	hud_.update(delta);
	dlg_.update(delta);
	if (!ctl_.state().frozen) {
		env_.update(delta);
	}
	dbr_.update(delta);
}

void runtime::render(r32 ratio, renderer& rdr) const {
	ovl_.render(rdr);
	hud_.render(ratio, rdr, ctl_);
	dlg_.render(rdr);
	ivt_.render(rdr);
	if (!hud_.fader_finished()) {
		const rect view = cam_.view(ratio);
		map_.render(ratio, view, rdr);
		env_.render(ratio, view, rdr);
		map_.render(rdr);
	}
	rdr.flush(cam_.matrix(ratio));
	dbr_.flush();
}

void runtime::boot_() {
	ctl_.clear();
	knl_.clear();
	ovl_.clear();
	hud_.clear();
	dlg_.clear();
	ivt_.clear();
	plr_.clear(env_);
	knl_.run_transfer(ctl_);
}

bool runtime::transfer_() {
	auto& field = ctl_.field();
	ctl_.lock();
	knl_.clear();
	ovl_.clear();
	hud_.invalidate();
	cam_.clear();
	env_.clear();
	map_.clear();
	if (knl_.load(field)) {
		knl_.run_transfer(ctl_);
	} else {
		spdlog::critical("Couldn't load next field's scripts!");
		ctl_.finish();
		return false;
	}
	const std::string path = vfs::field_path(field);
	tmx::Map desc {};
	if (!desc.load(path)) {
		spdlog::critical("Couldn't load next field's description!");
		ctl_.finish();
		return false;
	}
	const rect bounds = tmx_convert::rect_to_rect(desc);
	cam_.limit(bounds);
	map_.load_properties(desc, bounds);
	for (auto&& layer : desc.getLayers()) {
		switch (layer->getType()) {
		case tmx::Layer::Type::Tile:
			map_.load_tiles(layer->getLayerAs<tmx::TileLayer>());
			break;
		case tmx::Layer::Type::Object:
			env_.load(layer->getLayerAs<tmx::ObjectGroup>(), ctl_, knl_);
			break;
		case tmx::Layer::Type::Image:
			map_.load_parallax(layer->getLayerAs<tmx::ImageLayer>());
			break;
		default:
			break;
		}
	}
	plr_.transfer(ctl_.id(), cam_, env_);
	ctl_.finish();
	return true;
}

bool runtime::save_() {
	const std::string path = vfs::save_path(PROFILE_NAME, ctl_.profile());
	if (path.empty()) {
		spdlog::error("Couldn't create save directory!");
		ctl_.close();
		return false;
	}
	auto file = nlohmann::json::object();
	plr_.write(file, env_);
	ctl_.write(file);
	if (!vfs::dump_json(file, path)) {
		ctl_.close();
		spdlog::error("Couldn't save current state!");
		return false;
	}
	spdlog::info("Save successful.");
	ctl_.close();
	return true;
}

bool runtime::load_() {
	const std::string path = vfs::save_path(PROFILE_NAME, ctl_.profile());
	const auto file = vfs::buffer_json(path);
	if (file.empty()) {
		spdlog::error("Couldn't load current state!");
		ctl_.close();
		ctl_.boot();
		return false;
	}
	plr_.read(file, env_);
	ctl_.read(file);
	knl_.clear();
	ovl_.clear();
	hud_.clear();
	dlg_.clear();
	ivt_.clear();
	spdlog::info("Load successful.");
	ctl_.close();
	return true;
}

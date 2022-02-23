#include <algorithm>
#include <spdlog/spdlog.h>
#include <tmxlite/ObjectGroup.hpp>

#include "./environment.hpp"
#include "./player.hpp"
#include "../ai/particles.hpp"
#include "../ecs/aktor.hpp"
#include "../ecs/sprite.hpp"
#include "../ecs/kinematics.hpp"
#include "../ecs/health.hpp"
#include "../ecs/liquid.hpp"
#include "../ctrl/kernel.hpp"
#include "../ctrl/controller.hpp"

namespace {
	constexpr char AKTOR_TYPE[] = "aktor";
	constexpr char WATER_TYPE[] = "water";
	constexpr char DETER_PROPERTY[] = "deter";
	constexpr char EVENT_PROPERTY[] = "event";
	constexpr char FLAGS_PROPERTY[] = "flags";
	constexpr char ID_PROPERTY[] = "id";

	void props_to_aktor_(
		const std::vector<tmx::Property>& properties,
		udx& deter,
		std::string& event,
		u32& flags,
		i32& id
	) {
		for (auto&& prop : properties) {
			auto& name = prop.getName();
			if (name == DETER_PROPERTY) {
				deter = tmx_convert::prop_to_udx(prop);
			}
			if (name == EVENT_PROPERTY) {
				event = tmx_convert::prop_to_string(prop);
			}
			if (name == FLAGS_PROPERTY) {
				flags = tmx_convert::prop_to_uint(prop);
			}
			if (name == ID_PROPERTY) {
				id = tmx_convert::prop_to_int(prop);
			}
		}
	}
}

void environment::build() {
	ecs::thinker_ctor_table_builder::build(ctors_);
	registry_.on_construct<ecs::sprite>()
		.connect<&environment::redraw>(*this);
}

void environment::prepare() {
	ecs::sprite::prepare(*this);
}

void environment::handle(kernel& knl, headsup& hud, camera& cam, player& plr, const tile_map& map) {
	ecs::thinker::handle(knl, cam, plr, *this);
	ecs::kinematics::handle(*this, map);
	ecs::health::handle(knl, hud, plr, *this);
	ecs::liquid::handle(*this);
	ecs::sprite::handle(*this);
	if (!spawns_.empty()) {
		for (auto&& info : spawns_) {
			this->create_(info);
		}
		spawns_.clear();
	}
	if (redraw_) {
		redraw_ = false;
		registry_.sort<ecs::sprite>(
			[](const auto& lhv, const auto& rhv) { return lhv.layer < rhv.layer; },
			entt::std_sort {}
		);
	}
}

void environment::update(i64 delta) {
	ecs::sprite::update(delta, *this);
}

void environment::render(r32 ratio, const rect& view, renderer& rdr) const {
	ecs::sprite::render(ratio, view, rdr, *this);
	ecs::liquid::render(view, rdr, *this);
	// ecs::location::render(view, rdr, *this);
	// ecs::kinematics::render(rdr, *this);
}

void environment::clear() {
	if (const auto aktors = this->slice<ecs::aktor>(); !aktors.empty()) {
		registry_.destroy(aktors.begin(), aktors.end());
	}
	if (const auto liquids = this->slice<ecs::liquid>(); !liquids.empty()) {
		registry_.destroy(liquids.begin(), liquids.end());
	}
	spawns_.clear();
}

entt::entity environment::search(const entt::hashed_string& type) const {
	const auto view = this->slice<ecs::aktor>();
	const auto end = view.end();
	const auto iter = std::find_if(view.begin(), end, [this, &type](entt::entity e) {
		return this->get<ecs::aktor>(e).type == type;
	});
	if (iter != end) {
		return *iter;
	}
	return entt::null;
}

entt::entity environment::search(i32 id) const {
	const auto view = this->slice<ecs::trigger>();
	const auto end = view.end();
	const auto iter = std::find_if(view.begin(), end, [this, id](entt::entity e) {
		return this->get<ecs::trigger>(e).id == id;
	});
	if (iter != end) {
		return *iter;
	}
	return entt::null;
}

void environment::load(const tmx::ObjectGroup& data, const controller& ctl, kernel& knl) {
	auto& objects = data.getObjects();
	for (auto&& obj : objects) {
		if (auto& type = obj.getType(); type == AKTOR_TYPE) {
			udx deter = 0;
			std::string event {};
			u32 flags = 0;
			i32 id = 0;
			props_to_aktor_(
				obj.getProperties(),
				deter, event,
				flags, id
			);
			if (ctl.flag_at(deter) != ecs::trigger::will_deter(flags)) {
				continue;
			}
			auto& name = obj.getName();
			const glm::vec2 position = tmx_convert::vec2_to_vec2(obj.getPosition());
			if (!this->create_(name, position, flags, id)) {
				continue;
			}
			if (id > 0) {
				knl.push(id, event);
			}
		} else if (type == WATER_TYPE) {
			const rect hitbox = tmx_convert::rect_to_rect(obj.getAABB());
			auto e = this->allocate();
			this->emplace<ecs::liquid>(e, hitbox);
		}
	}
}

udx environment::visible(r32 ratio, const rect& view) const {
	return ecs::sprite::visible(ratio, view, *this);
}

void environment::kill(i32 id) {
	const auto view = this->slice<ecs::trigger>();
	auto pred = [&view, id](entt::entity e) {
		return view.get<ecs::trigger>(e).id == id;
	};
	const auto end = view.end();
	while (1) {
		if (const auto iter = std::find_if(view.begin(), end, pred); iter != end) {
			registry_.destroy(*iter);
		} else {
			break;
		}
	}
}

void environment::smoke(const glm::vec2& position, udx count) {
	while (count > 0) {
		this->spawn(ai::smoke, position);
		--count;
	}
}

void environment::shrapnel(const glm::vec2& position, udx count) {
	while (count > 0) {
		this->spawn(ai::shrapnel, position);
		--count;
	}
}

void environment::animate(i32 id, udx state, udx variation) {
	this->slice<ecs::trigger>().each(
	[this, id, state, variation](const entt::entity e, ecs::trigger& trg) {
		if (trg.id == id and this->has<ecs::sprite>(e)) {
			auto& spt = this->get<ecs::sprite>(e);
			spt.state(state);
			spt.variation = variation;
		}
	});
}

void environment::think(i32 id, u32 state) {
	this->slice<ecs::trigger>().each(
	[this, id, state](const entt::entity e, ecs::trigger& trg) {
		if (trg.id == id and this->has<ecs::thinker>(e)) {
			auto& thk = this->get<ecs::thinker>(e);
			thk.state = state;
		}
	});
}

bool environment::fight(i32 id) {
	if (auto e = this->search(id); e != entt::null) {
		auto& trg = this->get<ecs::trigger>(e);
		trg.flags.interaction = false;
		trg.flags.destruction = true;

		auto& hel = this->emplace<ecs::health>(e);
		hel.clear();
		hel.flags.boss_fight = true;

		return true;
	}
	return false;
}

bool environment::still(i32 id) const {
	if (auto e = this->search(id); e != entt::null) {
		if (this->has<ecs::kinematics>(e)) {
			auto& kin = this->get<ecs::kinematics>(e);
			if (kin.velocity.x != 0.0f and !kin.hori_sides()) {
				return false;
			}
			if (kin.velocity.y != 0.0f and !kin.vert_sides()) {
				return false;
			}
		}
		return true;
	}
	return false;
}

bool environment::create_(const spawn_info& info) {
	if (const auto iter = ctors_.find(info.type.value()); iter != ctors_.end()) {
		auto e = this->allocate();
		registry_.emplace<ecs::aktor>(e, info.type);
		registry_.emplace<ecs::location>(e, info.position);
		if (info.velocity.x != 0.0f or info.velocity.y != 0.0f) {
			registry_.emplace<ecs::kinematics>(e, info.velocity);
		}
		if (info.id > 0) {
			registry_.emplace<ecs::trigger>(e, info.id, info.mask);
		}
		iter->second(e, *this);
		return true;
	}
	spdlog::error("Couldn't spawn aktor: \"{}\"!", info.type.data());
	return false;
}

bool environment::create_(const std::string& name, const glm::vec2& position, u32 flags, i32 id) {
	const entt::hashed_string entry { name.c_str() };
	if (const auto iter = ctors_.find(entry.value()); iter != ctors_.end()) {
		// allocate
		auto e = this->allocate();
		registry_.emplace<ecs::aktor>(e, entry);
		registry_.emplace<ecs::location>(e, position);
		if (id > 0) {
			registry_.emplace<ecs::trigger>(e, id, flags);
		}
		// construct
		iter->second(e, *this);
		// aftermath
		if (ecs::trigger::will_face_left(flags) and this->has<ecs::sprite>(e)) {
			auto& spt = this->get<ecs::sprite>(e);
			spt.mirror.horizontally = true;
		}
		return true;
	}
	spdlog::error("Couldn't spawn aktor: \"{}\"!", name);
	return false;
}

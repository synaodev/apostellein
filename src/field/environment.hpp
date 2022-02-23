#pragma once

#include <entt/entity/registry.hpp>

#include "../ecs/thinker.hpp"
#include "../util/tmx-convert.hpp"

struct rect;
struct renderer;
struct headsup;
struct controller;
struct kernel;
struct tile_map;

struct spawn_info {
	spawn_info() noexcept = default;
	spawn_info(
		const entt::hashed_string& _type,
		const glm::vec2& _position
	) noexcept : type{ _type }, position{ _position } {}
	spawn_info(
		const entt::hashed_string& _type,
		const glm::vec2& _position,
		const glm::vec2& _velocity
	) noexcept : type{ _type }, position{ _position }, velocity{ _velocity } {}
	spawn_info(
		const entt::hashed_string& _type,
		const glm::vec2& _position,
		i32 _id
	) noexcept : type{ _type }, position{ _position }, id{ _id } {}
	spawn_info(
		const entt::hashed_string& _type,
		const glm::vec2& _position,
		const glm::vec2& _velocity,
		i32 _id
	) noexcept : type{ _type }, position{ _position }, velocity{ _velocity }, id{ _id } {}
	spawn_info(
		const entt::hashed_string& _type,
		const glm::vec2& _position,
		const glm::vec2& _velocity,
		i32 _id,
		u32 _mask
	) noexcept : type{ _type }, position{ _position }, velocity{ _velocity }, id{ _id }, mask{ _mask } {}

	entt::hashed_string type {};
	glm::vec2 position {};
	glm::vec2 velocity {};
	i32 id {};
	u32 mask {};
};

struct environment {
public:
	void build();
	void clear();
	void prepare();
	void handle(kernel& knl, headsup& hud, camera& cam, player& plr, const tile_map& map);
	void update(i64 delta);
	void render(r32 ratio, const rect& view, renderer& rdr) const;
	entt::entity search(const entt::hashed_string& type) const;
	entt::entity search(i32 id) const;
	entt::entity allocate() { return registry_.create(); }
	void load(const tmx::ObjectGroup& data, const controller& ctl, kernel& knl);
	bool valid(entt::entity e) const { return registry_.valid(e); }
	udx length() const { return registry_.size(); }
	udx alive() const { return registry_.alive(); }
	udx visible(r32 ratio, const rect& view) const;
	template<typename... T> auto slice() { return registry_.view<T...>(); }
	template<typename... T> auto slice() const { return registry_.view<T...>(); }
	template<typename... T> bool has(entt::entity e) const { return registry_.all_of<T...>(e); }
	template<typename... T> bool any(entt::entity e) const { return registry_.any_of<T...>(e); }
	template<typename... T> decltype(auto) get(entt::entity e) { return registry_.get<T...>(e); }
	template<typename... T> decltype(auto) get(entt::entity e) const { return registry_.get<T...>(e); }
	template<typename T, typename... Args>
	decltype(auto) emplace(entt::entity e, Args&& ...args) {
		return registry_.get_or_emplace<T>(e, std::forward<Args>(args)...);
	}
	void dispose(entt::entity e) {
		if (e != entt::null) {
			registry_.destroy(e);
		}
	}
	void kill(i32 id);
	void smoke(const glm::vec2& position, udx count);
	void shrapnel(const glm::vec2& position, udx count);
	void animate(i32 id, udx state, udx variation);
	void think(i32 id, u32 state);
	bool fight(i32 id);
	bool still(i32 id) const;
	void redraw() { redraw_ = true; }
	void spawn(const spawn_info& info) { spawns_.push_back(info); }
	void spawn(
		const entt::hashed_string& type,
		const glm::vec2& position
	) { spawns_.emplace_back(type, position); }
	void spawn(
		const entt::hashed_string& type,
		const glm::vec2& position,
		const glm::vec2& velocity
	) { spawns_.emplace_back(type, position, velocity); }
	bool try_spawn(const std::string& name, r32 x, r32 y, i32 id) {
		const entt::hashed_string type { name.c_str() };
		if (auto it = ctors_.find(type.value()); it != ctors_.end()) {
			const glm::vec2 position { x, y };
			spawns_.emplace_back(type, position, id);
			return true;
		}
		return false;
	}
private:
	bool create_(const spawn_info& info);
	bool create_(const std::string& name, const glm::vec2& position, u32 flags, i32 id);
	bool redraw_ {};
	entt::registry registry_ {};
	std::vector<spawn_info> spawns_ {};
	ecs::thinker_ctor_table ctors_ {};
};

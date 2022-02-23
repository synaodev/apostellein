#pragma once

#include <entt/core/hashed_string.hpp>
#include <entt/entity/fwd.hpp>
#include <entt/entity/entity.hpp>
#include <apostellein/rect.hpp>

struct renderer;
struct environment;

namespace ecs {
	struct location;

	struct submersible {
		submersible() noexcept = default;
		submersible(
			const entt::hashed_string& _particle,
			const entt::hashed_string& _sound
		) noexcept : particle{ _particle }, sound{ _sound } {}

		entt::entity entity { entt::null };
		entt::hashed_string particle {};
		entt::hashed_string sound {};
	};

	struct liquid {
		liquid() noexcept = default;
		liquid(const rect& _hitbox) noexcept :
			hitbox{ _hitbox } {}

		rect hitbox {};
	public:
		static void handle(
			environment& env,
			const ecs::location& loc,
			ecs::submersible& sub
		);
		static void handle(environment& env);
		static void render(const rect& view, renderer& rdr, const environment& env);
	};
}

#pragma once

#include <apostellein/struct.hpp>

struct kernel;
struct headsup;
struct player;
struct environment;

namespace ecs {
	struct location;

	struct health {
		health() noexcept = default;
		health(i32 _current, i32 _maximum, i32 _poison, i32 _damage) noexcept :
			current{ _current },
			maximum{ _maximum },
			poison{ _poison },
			damage{ _damage } {}
		union {
			bitfield_raw<u32> _raw {};
			bitfield_index<u32, 0> hurt;
			bitfield_index<u32, 1> attacking;
			bitfield_index<u32, 2> apostolic;
			bitfield_index<u32, 3> invincible;
			bitfield_index<u32, 4> deflectable;
			bitfield_index<u32, 5> hookable;
			bitfield_index<u32, 6> hooked;
			bitfield_index<u32, 7> deadly;
			bitfield_index<u32, 8> once_more;
			bitfield_index<u32, 9> boss_fight;
		} flags {};
		i32 current { 1 };
		i32 maximum { 1 };
		i32 poison {};
		i32 damage {};
	public:
		void clear() {
			flags._raw = {};
			current = 1;
			maximum = 1;
			damage = 0;
			poison = 0;
		}
		static bool attack(
			const ecs::location& this_location,
			ecs::location& that_location,
			const ecs::health& this_health,
			ecs::health& that_health
		);
		static void handle(
			kernel& knl,
			headsup& hud,
			player& plr,
			environment& env
		);
	};
}

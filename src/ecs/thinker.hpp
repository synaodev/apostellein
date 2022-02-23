#pragma once

#include <vector>
#include <unordered_map>
#include <entt/entity/fwd.hpp>
#include <apostellein/struct.hpp>

struct kernel;
struct camera;
struct player;
struct environment;

namespace ecs {
	struct thinker {
		using procedure = void(*)(entt::entity, kernel&, camera&, player&, environment&);
		thinker() noexcept = default;
		thinker(procedure _func) noexcept :
			func{ _func } {}

		u32 state {};
		procedure func {};
	public:
		static void handle(kernel& knl, camera& cam, player& plr, environment& env);
	};

	using thinker_ctor = void(*)(entt::entity, environment&);
	using thinker_ctor_table = std::unordered_map<entt::id_type, thinker_ctor>;
	using thinker_ctor_table_callback = void(*)(thinker_ctor_table&);

	struct thinker_ctor_table_builder : public not_moveable {
		thinker_ctor_table_builder() = delete;
		thinker_ctor_table_builder(thinker_ctor_table_callback func) {
			auto& callbacks = thinker_ctor_table_builder::callbacks_();
			callbacks.push_back(func);
		}
	public:
		static void build(thinker_ctor_table& table) {
			auto& callbacks = thinker_ctor_table_builder::callbacks_();
			for (auto&& func : callbacks) {
				func(table);
			}
			callbacks.clear();
			callbacks.shrink_to_fit();
		}
	private:
		static std::vector<thinker_ctor_table_callback>& callbacks_() {
			static std::vector<thinker_ctor_table_callback> callbacks {};
			return callbacks;
		}
	};
}

#define APOSTELLEIN_THINKER_TABLE(SYMBOL) \
	static void SYMBOL##_build_ctor_table(ecs::thinker_ctor_table& table); \
	static const ecs::thinker_ctor_table_builder SYMBOL##_ctor_table_builder { SYMBOL##_build_ctor_table }; \
	static void SYMBOL##_build_ctor_table(ecs::thinker_ctor_table& table) \

#define APOSTELLEIN_THINKER_ENTRY(AKTOR, ENTRY) table[AKTOR] = ENTRY

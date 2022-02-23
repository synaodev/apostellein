#pragma once

#include "./counter.hpp"
#include "./scheme.hpp"
#include "../util/item-slot.hpp"

namespace gui {
	struct element : public not_copyable {
	public:
		void build(
			udx index,
			udx modulo,
			const glm::vec2& start,
			const glm::vec2& spacing,
			const animation_group* item_file,
			bool amount_backwards,
			const glm::vec2& amount_offset,
			const rect& amount_raster,
			const material* amount_texture
		);
		void invalidate() const {
			item_.invalidate();
			amount_.invalidate();
		}
		void fix() {
			item_.fix();
			amount_.fix();
		}
		void render(renderer& rdr) const;
		void set(const item_slot& value);
		const item_slot& get() const { return slot_; }
	private:
		item_slot slot_ {};
		gui::scheme item_ {};
		gui::counter amount_ {};
	};
}

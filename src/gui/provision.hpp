#pragma once

#include "./counter.hpp"
#include "./scheme.hpp"
#include "../util/item-slot.hpp"

struct rect;

namespace gui {
	struct provision : public not_copyable {
	public:
		void build(
			const glm::vec2& frame_position,
			udx frame_state,
			const animation_group* frame_file,
			const glm::vec2& item_position,
			const animation_group* item_file,
			const glm::vec2& amount_position,
			const rect& amount_raster,
			const material* amount_texture
		);
		void invalidate() const {
			frame_.invalidate();
			item_.invalidate();
			amount_.invalidate();
		}
		void fix() {
			frame_.fix();
			item_.fix();
			amount_.fix();
		}
		void render(renderer& rdr) const;
		void set(const item_slot& value);
		void set() { slot_ = {}; }
		const item_slot& get() const { return slot_; }
	private:
		item_slot slot_ {};
		gui::scheme frame_ {};
		gui::scheme item_ {};
		gui::counter amount_ {};
	};
}

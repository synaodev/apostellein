#pragma once

#include <apostellein/rect.hpp>

#include "./scheme.hpp"

namespace gui {
	struct meter {
		void build(
			const glm::vec2& position,
			const glm::vec2& dimensions,
			i32 current,
			i32 maximum,
			const glm::vec2& frame_position,
			udx frame_state,
			const animation_group* frame_file
		);
		void clear();
		void invalidate() const {
			invalidated_ = true;
			frame_.invalidate();
		}
		void fix() {
			invalidated_ = true;
			frame_.fix();
		}
		void update(i64 delta);
		void render(renderer& rdr) const;
		void set(i32 current, i32 maximum);
		i32 get() const { return current_; }
	private:
		mutable bool invalidated_ {};
		glm::vec2 position_ {};
		glm::vec2 dimensions_ {};
		rect raster_ {};
		i32 current_ {};
		i32 maximum_ {};
		gui::scheme frame_ {};
	};
}

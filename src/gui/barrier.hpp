#pragma once

#include <apostellein/rect.hpp>

#include "../video/vertex.hpp"

struct texture_2d;
struct renderer_2d;

namespace gui {
	struct barrier : public not_copyable {
	public:
		void build(
			const glm::vec2& position,
			const rect& raster,
			i32 current,
			i32 maximum,
			i32 indent,
			const texture_2d* texture
		);
		// void invalidate() const { invalidated_ = true; }
		void fix();
		void render(renderer_2d& renderer) const;
		void set(i32 current, i32 maximum, i32 indent);
		i32 get() const { return current_; }
	private:
		void generate_quads_(udx current_units, udx maximum_units, bool resized);
		// mutable bool invalidated_ {};
		glm::vec2 position_ {};
		rect raster_ {};
		i32 current_ {};
		i32 maximum_ {};
		i32 indent_ {};
		const texture_2d* texture_ {};
		std::vector<vtx_sprite> vertices_ {};
	};
}

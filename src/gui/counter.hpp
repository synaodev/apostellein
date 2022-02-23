#pragma once

#include <apostellein/rect.hpp>

#include "../video/vertex.hpp"

struct material;
struct renderer;

namespace gui {
	struct counter : public not_copyable {
	public:
		void build(
			bool backwards,
			const glm::vec2& position,
			const rect& raster,
			i32 number,
			i32 zeroes,
			const material* texture
		);
		void invalidate() const { invalidated_ = true; }
		void fix() { this->generate_quads_(); }
		void render(renderer& rdr) const;
		void set(i32 value) {
			if (number_ != value) {
				number_ = value;
				invalidated_ = true;
				this->generate_quads_();
			}
		}
		void visible(bool value) {
			if (visible_ != value) {
				visible_ = value;
				invalidated_ = true;
			}
		}
		i32 get() const { return number_; }
		bool visible() const { return visible_; }
	private:
		static constexpr i32 MAGIC_NUMBER = -45313;
		void generate_quads_();
		mutable bool invalidated_ {};
		bool backwards_ {};
		bool visible_ { true };
		glm::vec2 position_ {};
		rect raster_ {};
		i32 number_ { MAGIC_NUMBER };
		i32 zeroes_ {};
		std::vector<i32> digits_ {};
		const material* texture_ {};
		std::vector<vtx_sprite> vertices_ {};
	};
}

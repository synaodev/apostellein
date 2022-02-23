#pragma once

#include <apostellein/def.hpp>
#include <glm/vec2.hpp>

struct renderer;

namespace gui {
	enum class fade_type {
		done_in,
		done_out,
		moving_in,
		moving_out
	};
	struct fader {
		void clear();
		void prepare() {
			if (this->visible()) {
				previous_ = current_;
			}
		}
		void handle();
		void render(r32 ratio, renderer& rdr) const;
		void fade_in();
		void fade_out();
		bool finished() const { return type_ == fade_type::done_out; }
		bool moving() const { return type_ == fade_type::moving_in or type_ == fade_type::moving_out; }
		bool visible() const { return type_ != fade_type::done_in; }
	private:
		fade_type type_ { fade_type::done_out };
		glm::vec2 previous_ {};
		glm::vec2 current_ {};
	};
}

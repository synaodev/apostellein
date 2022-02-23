#pragma once

#include <string>
#include <apostellein/rect.hpp>
#include <apostellein/struct.hpp>

struct material;
struct renderer;

namespace gui {
	struct graphic : public not_copyable {
		~graphic() { this->clear(); }
	public:
		void build(bool transient) {
			transient_ = transient;
			this->clear();
		}
		void clear();
		void invalidate() const { invalidated_ = true; }
		void fix() const { this->invalidate(); }
		void render(renderer& rdr) const;
		void set(const std::string& name);
		void position(const glm::vec2& value) {
			if (position_ != value) {
				invalidated_ = true;
				position_ = value;
			}
		}
		const glm::vec2& position() const { return position_; }
		bool valid() const { return picture_ != nullptr; }
	private:
		mutable bool invalidated_ {};
		bool transient_ {};
		glm::vec2 position_ {};
		const material* picture_ {};
	};
}

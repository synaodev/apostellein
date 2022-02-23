#include <glm/gtc/constants.hpp>

#include "./scheme.hpp"
#include "../x2d/animation-group.hpp"

void gui::scheme::build(
	const glm::vec2& position,
	udx state,
	udx variation,
	const animation_group* file
) {
	invalidated_ = true;
	position_ = position;
	state_ = state;
	variation_ = variation;
	file_ = file;
}

void gui::scheme::update(i64 delta) {
	if (file_) {
		file_->update(
			delta,
			invalidated_,
			state_,
			timer_,
			frame_
		);
	}
}

void gui::scheme::render(renderer& rdr) const {
	if (file_) {
		file_->render(
			invalidated_,
			state_,
			frame_,
			variation_,
			position_,
			rdr
		);
	}
}

bool gui::scheme::finished() const {
	if (file_) {
		return file_->finished(state_, frame_, timer_);
	}
	return true;
}

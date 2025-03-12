#include <spdlog/spdlog.h>

#include "./graphic.hpp"
#include "../hw/vfs.hpp"
#include "../video/texture-2d.hpp"
#include "../x2d/renderer-2d.hpp"

void gui::graphic::clear() {
	// invalidated_ = true;
	position_ = {};
	if (transient_ and picture_) {
		vfs::clear_texture(picture_);
	}
	picture_ = nullptr;
}

void gui::graphic::render(renderer_2d& renderer) const {
	if (picture_) {
		auto& list = renderer.query(
			priority_type::deferred,
			blending_type::alpha,
			pipeline_type::sprite
		);
		// if (invalidated_) {
			// invalidated_ = false;
			const rect quad { picture_->dimensions() };
			list.batch_sprite(quad, quad, *picture_);
		// } else {
			// list.skip(display_list::QUAD);
		// }
	}
}

void gui::graphic::set(const std::string& name) {
	this->clear();
	picture_ = vfs::find_texture(name);
	if (!picture_ or !picture_->valid()) {
		spdlog::error("Couldn't load picture named \"{}\"!", name);
		return;
	}
}

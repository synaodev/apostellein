#include <spdlog/spdlog.h>

#include "./graphic.hpp"
#include "../hw/vfs.hpp"
#include "../video/material.hpp"
#include "../x2d/renderer.hpp"

void gui::graphic::clear() {
	invalidated_ = true;
	position_ = {};
	if (transient_ and picture_) {
		vfs::clear_material(picture_);
	}
	picture_ = nullptr;
}

void gui::graphic::render(renderer& rdr) const {
	if (picture_) {
		auto& list = rdr.query(
			priority_type::deferred,
			blending_type::alpha,
			pipeline_type::sprite
		);
		if (invalidated_) {
			invalidated_ = false;
			const rect quad { picture_->dimensions() };
			list.batch_sprite(quad, quad, *picture_);
		} else {
			list.skip(display_list::QUAD);
		}
	}
}

void gui::graphic::set(const std::string& name) {
	this->clear();
	picture_ = vfs::find_material(name);
	if (!picture_ or !picture_->valid()) {
		spdlog::error("Couldn't load picture named \"{}\"!", name);
		return;
	}
}

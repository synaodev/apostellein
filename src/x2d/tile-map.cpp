#include <spdlog/spdlog.h>
#include <tmxlite/Map.hpp>
#include <tmxlite/TileLayer.hpp>
#include <tmxlite/ImageLayer.hpp>
#include <apostellein/konst.hpp>
#include <apostellein/cast.hpp>

#include "./tile-map.hpp"
#include "./renderer.hpp"
#include "../hw/vfs.hpp"
#include "../video/material.hpp"

namespace {
	constexpr char COLLIDABLE_PROPERTY[] = "collide";
	constexpr char PRIORITY_PROPERTY[] = "priority";
	constexpr char SCROLL_X_PROPERTY[] = "scroll.x";
	constexpr char SCROLL_Y_PROPERTY[] = "scroll.y";
	constexpr i32 SCREEN_WIDTH = (konst::WINDOW_WIDTH<i32>() / konst::TILE<i32>()) + 1;
	constexpr i32 SCREEN_HEIGHT = (konst::WINDOW_HEIGHT<i32>() / konst::TILE<i32>()) + 1;
	constexpr glm::ivec2 INVALID_TILE { ~0 };

	bool rounding_compare_(const rect& lhr, const rect& rhr) {
		const glm::ivec4 lhv { lhr.x, lhr.y, lhr.w, lhr.h };
		const glm::ivec4 rhv { rhr.x, rhr.y, rhr.w, rhr.h };
		return lhv == rhv;
	}
}

tile_layer::tile_layer(const glm::ivec2& dimensions) {
	tiles_.resize(as<udx>(dimensions.x) * as<udx>(dimensions.y));
	vertices_.resize(
		as<udx>(SCREEN_WIDTH) *
		as<udx>(SCREEN_HEIGHT) *
		display_list::QUAD
	);
}

void tile_layer::build(const tmx::TileLayer& data, std::vector<u32>& attributes, const std::vector<u32>& key) {
	for (auto&& prop : data.getProperties()) {
		auto& name = prop.getName();
		if (name == COLLIDABLE_PROPERTY) {
			if (tmx_convert::prop_to_bool(prop)) {
				collidable_ = true;
				foreground_ = true;
			}
		} else if (name == PRIORITY_PROPERTY) {
			foreground_ = tmx_convert::prop_to_bool(prop);
		}
	}

	auto& array = data.getTiles();
	attributes.resize(array.size());

	for (udx idx = 0; idx < array.size(); ++idx) {
		if (const auto type = as<i32>(array[idx].ID) - 1; type >= 0) {
			tiles_[idx] = {
				type % konst::TILE<i32>(),
				type / konst::TILE<i32>()
			};
			if (collidable_) {
				attributes[idx]  = key[as<udx>(type)];
			}
		} else {
			tiles_[idx] = INVALID_TILE;
		}
	}
}

void tile_layer::handle(
	const glm::ivec2& first,
	const glm::ivec2& last,
	const glm::ivec2& dimensions,
	const material* texture
) {
	const glm::ivec2 diff = last - first;
	const auto range = (
		as<udx>(diff.x) *
		as<udx>(diff.y) *
		display_list::QUAD
	);
	if (range > vertices_.size()) {
		vertices_.resize(range);
	}
	indices_ = 0;
	glm::vec2 pos = glm::vec2(first) * konst::TILE<r32>();
	glm::vec2 uvs {};
	const glm::vec2 off = texture->offset();
	const auto atlas = texture->atlas();
	for (i32 y = first.y; y < last.y; ++y) {
		for (i32 x = first.x; x < last.x; ++x) {
			auto& tile = tiles_[
				as<udx>(x) +
				as<udx>(y) *
				as<udx>(dimensions.x)
			];
			if (tile.x >= 0 and tile.y >= 0) {
				uvs = glm::vec2(tile * konst::TILE<i32>());

				auto vtx = &vertices_[indices_ * display_list::QUAD];
				vtx[0].position = pos;
				vtx[0].index = 1;
				vtx[0].uvs = (uvs + off) / material::MAXIMUM_DIMENSIONS;
				vtx[0].atlas = atlas;
				vtx[0].color = chroma::WHITE();

				vtx[1].position = { pos.x, pos.y + konst::TILE<r32>() };
				vtx[1].index = 1;
				vtx[1].uvs = glm::vec2(uvs.x + off.x, uvs.y + off.y + konst::TILE<r32>()) / material::MAXIMUM_DIMENSIONS;
				vtx[1].atlas = atlas;
				vtx[1].color = chroma::WHITE();

				vtx[2].position = { pos.x + konst::TILE<r32>(), pos.y };
				vtx[2].index = 1;
				vtx[2].uvs = glm::vec2(uvs.x + off.x + konst::TILE<r32>(), uvs.y + off.y) / material::MAXIMUM_DIMENSIONS;
				vtx[2].atlas = atlas;
				vtx[2].color = chroma::WHITE();

				vtx[3].position = pos + konst::TILE<r32>();
				vtx[3].index = 1;
				vtx[3].uvs = (uvs + off + konst::TILE<r32>()) / material::MAXIMUM_DIMENSIONS;
				vtx[3].atlas = atlas;
				vtx[3].color = chroma::WHITE();

				++indices_;
			}
			pos.x += konst::TILE<r32>();
		}
		pos.x = as<r32>(first.x * konst::TILE<i32>());
		pos.y += konst::TILE<r32>();
	}
}

void tile_layer::render(renderer& rdr) const {
	auto& list = rdr.query(
		priority_type::automatic,
		blending_type::alpha,
		pipeline_type::sprite
	);
	list.upload(vertices_, indices_ * display_list::QUAD);
}

void tile_parallax::build(const tmx::ImageLayer& data, const material* background) {
	for (auto&& prop : data.getProperties()) {
		auto& name = prop.getName();
		if (name == SCROLL_X_PROPERTY) {
			scrolling_.x = tmx_convert::prop_to_real(prop);
		} else if (name == SCROLL_Y_PROPERTY) {
			scrolling_.y = tmx_convert::prop_to_real(prop);
		}
	}
	if (background) {
		background_ = background;
		raster_ = background->dimensions();
	}
}

void tile_parallax::prepare() {
	previous_ = current_;
}

void tile_parallax::handle(const rect& view) {
	const glm::vec2 next = glm::mod(
		view.position() * -scrolling_,
		raster_
	);
	if (current_ != next) {
		current_ = next;
	}
}

void tile_parallax::render(r32 ratio, const rect& view, renderer& rdr) const {
	if (background_) {
		auto& list = rdr.query(
			priority_type::automatic,
			blending_type::alpha,
			pipeline_type::sprite
		);
		const glm::vec2 position = konst::INTERPOLATE(
			previous_,
			current_,
			ratio
		);
		list.batch_parallax(view, position, raster_, *background_);
	}
}

void tile_map::clear() {
	invalidated_ = true;
	dimensions_ = {};
	attributes_.clear();
	key_.clear();
	previous_ = {};
	texture_ = nullptr;
	parallaxes_.clear();
	layers_.clear();
}

void tile_map::load_properties(const tmx::Map& data, const rect& bounds) {
	invalidated_ = true;
	dimensions_ = {
		glm::max(
			as<i32>(bounds.w) / konst::TILE<i32>(),
			SCREEN_WIDTH
		),
		glm::max(
			as<i32>(bounds.h) / konst::TILE<i32>(),
			SCREEN_HEIGHT
		)
	};
	auto& tilesets = data.getTilesets();
	if (!tilesets.empty()) {
		const std::string name = tmx_convert::path_to_name(tilesets[0].getImagePath());
		texture_ = vfs::find_material(name);

		const std::string path = vfs::key_path(name);
		key_ = vfs::buffer_uints(path);
	}
}

void tile_map::load_tiles(const tmx::TileLayer& data) {
	invalidated_ = true;

	if (!key_.empty()) {
		auto& recent = layers_.emplace_back(dimensions_);
		recent.build(data, attributes_, key_);
	} else {
		spdlog::warn("Can't load any tiles without loading tile attribute key!");
	}
}

void tile_map::load_parallax(const tmx::ImageLayer& data) {
	invalidated_ = true;

	const std::string name = tmx_convert::path_to_name(data.getImagePath());
	auto background_ = vfs::find_material(name);

	auto& recent = parallaxes_.emplace_back();
	recent.build(data, background_);
}

void tile_map::prepare() {
	for (auto&& pllx : parallaxes_) {
		pllx.prepare();
	}
}

void tile_map::handle(const rect& view, bool force) {
	for (auto&& pllx : parallaxes_) {
		pllx.handle(view);
	}
	if (!rounding_compare_(previous_, view) or force) {
		invalidated_ = true;
		previous_ = view;
		const glm::ivec2 first {
			glm::max(konst::TILE_FLOOR(view.x), 0),
			glm::max(konst::TILE_FLOOR(view.y), 0)
		};
		const glm::ivec2 last {
			glm::min(konst::TILE_CEILING(view.right() + konst::TILE<r32>()), dimensions_.x),
			glm::min(konst::TILE_CEILING(view.bottom() + konst::TILE<r32>()), dimensions_.y)
		};
		for (auto&& layer : layers_) {
			layer.handle(first, last, dimensions_, texture_);
		}
	}
}

void tile_map::render(r32 ratio, const rect& view, renderer& rdr) const {
	for (auto&& pllx : parallaxes_) {
		pllx.render(ratio, view, rdr);
	}
	for (auto&& layer : layers_) {
		if (!layer.foreground()) {
			layer.render(rdr);
		}
	}
}

void tile_map::render(renderer& rdr) const {
	for (auto&& layer : layers_) {
		if (layer.foreground()) {
			layer.render(rdr);
		}
	}
}

tile_type tile_map::tile(i32 x, i32 y) const {
	static const tile_type OUT_OF_BOUNDS = []{
		tile_type r {};
		r.flags.out_of_bounds = true;
		return r;
	}();
	if (x >= 0 and y >= 0 and x < dimensions_.x and y < dimensions_.y) {
		const auto idx = (
			as<udx>(x) +
			as<udx>(y) *
			as<udx>(dimensions_.x)
		);
		if (idx < attributes_.size()) {
			return attributes_[idx];
		}
	} else if (y > (dimensions_.y + 1)) {
		return OUT_OF_BOUNDS;
	}
	return {};
}

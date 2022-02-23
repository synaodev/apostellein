#pragma once

#include <memory>
#include <apostellein/rect.hpp>

#include "./priority-type.hpp"
#include "./tile-type.hpp"
#include "../util/tmx-convert.hpp"
#include "../video/vertex.hpp"

struct material;
struct renderer;

struct tile_layer : public not_copyable {
	tile_layer() noexcept = default;
	tile_layer(const glm::ivec2& dimensions);
	tile_layer(tile_layer&& that) noexcept {
		*this = std::move(that);
	}
	tile_layer& operator=(tile_layer&& that) noexcept {
		if (this != &that) {
			collidable_ = that.collidable_;
			that.collidable_ = false;
			foreground_ = that.foreground_;
			that.foreground_ = false;
			indices_ = that.indices_;
			that.indices_ = 0;
			tiles_ = std::move(that.tiles_);
			that.tiles_.clear();
			vertices_ = std::move(that.vertices_);
			that.vertices_.clear();
		}
		return *this;
	}
public:
	void build(const tmx::TileLayer& data, std::vector<u32>& attributes, const std::vector<u32>& key);
	void handle(const glm::ivec2& first, const glm::ivec2& last, const glm::ivec2& dimensions, const material* texture);
	void render(renderer& rdr) const;
	bool foreground() const { return foreground_; }
private:
	bool collidable_ {};
	bool foreground_ {};
	udx indices_ {};
	std::vector<glm::ivec2> tiles_ {};
	std::vector<vtx_sprite> vertices_ {};
};

struct tile_parallax : public not_copyable {
	tile_parallax() noexcept = default;
	tile_parallax(tile_parallax&& that) noexcept {
		*this = std::move(that);
	}
	tile_parallax& operator=(tile_parallax&& that) noexcept {
		if (this != &that) {
			previous_ = that.previous_;
			that.previous_ = {};
			current_ = that.current_;
			that.current_ = {};
			scrolling_ = that.scrolling_;
			that.scrolling_ = {};
			raster_ = that.raster_;
			that.raster_ = {};
			background_ = that.background_;
			that.background_ = nullptr;
		}
		return *this;
	}
public:
	void build(const tmx::ImageLayer& data, const material* background);
	void prepare();
	void handle(const rect& view);
	void render(r32 ratio, const rect& view, renderer& rdr) const;
private:
	glm::vec2 previous_ {};
	glm::vec2 current_ {};
	glm::vec2 scrolling_ {};
	glm::vec2 raster_ {};
	const material* background_ {};
};

struct tile_map {
public:
	void load_properties(const tmx::Map& data, const rect& bounds);
	void load_tiles(const tmx::TileLayer& data);
	void load_parallax(const tmx::ImageLayer& data);
	void clear();
	void fix() { this->handle(previous_, true); }
	void prepare();
	void handle(const rect& view, bool force = false);
	// background
	void render(r32 ratio, const rect& view, renderer& rdr) const;
	// foreground
	void render(renderer& rdr) const;
	const glm::ivec2& dimensions() const { return dimensions_; }
	tile_type tile(i32 x, i32 y) const;
	tile_type tile(const glm::ivec2& index) const { return this->tile(index.x, index.y); }
private:
	bool invalidated_ {};
	glm::ivec2 dimensions_ {};
	std::vector<u32> attributes_ {};
	std::vector<u32> key_ {};
	rect previous_ {};
	const material* texture_ {};
	std::vector<tile_parallax> parallaxes_ {};
	std::vector<tile_layer> layers_ {};
};

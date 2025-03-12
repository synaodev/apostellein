#include <memory>
#include <optional>
#include <array>
#include <set>
#include <spdlog/spdlog.h>
#include <apostellein/cast.hpp>

#define STB_RECT_PACK_IMPLEMENTATION
#include <stb_rect_pack.h>

#include "./texture-2d.hpp"
#include "./opengl.hpp"

namespace {
	constexpr u32 DEFAULT_FORMAT = GL_RGBA2;
	constexpr i32 DEFAULT_MIPMAP = 4;
	constexpr i32 FURTHER_HEIGHT = 1 << 30;
	constexpr udx TOTAL_SEGMENTS =
		cast<udx>(image_file::MAXIMUM_LENGTH / image_file::MINIMUM_LENGTH) *
		cast<udx>(image_file::MAXIMUM_LENGTH / image_file::MINIMUM_LENGTH);
}

struct virtual_texture : public not_moveable {
	virtual_texture() {
		context_.width = image_file::MAXIMUM_LENGTH;
		context_.height = image_file::MAXIMUM_LENGTH;
		context_.init_mode = STBRP__INIT_skyline;
		context_.heuristic = STBRP_HEURISTIC_Skyline_default;
		context_.num_nodes = cast<i32>(TOTAL_SEGMENTS);
		context_.align = (context_.width + context_.num_nodes - 1) / context_.num_nodes;
		nodes_.resize(TOTAL_SEGMENTS);

		glCheck(glCreateTextures(GL_TEXTURE_2D, 1, &handle_));
		glCheck(glTextureStorage2D(
			handle_,
			DEFAULT_MIPMAP,
			DEFAULT_FORMAT,
			image_file::MAXIMUM_LENGTH,
			image_file::MAXIMUM_LENGTH
		));
		glCheck(glTextureParameteri(handle_, GL_TEXTURE_WRAP_S, GL_REPEAT));
		glCheck(glTextureParameteri(handle_, GL_TEXTURE_WRAP_T, GL_REPEAT));
		glCheck(glTextureParameteri(handle_, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
		glCheck(glTextureParameteri(handle_, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
		glCheck(glBindTextureUnit(0, handle_));
	}
	~virtual_texture() {
		if (handle_ != 0) {
			glCheck(glDeleteTextures(1, &handle_));
		}
	}
public:
	static i32 generate_id() {
		static i32 id_ = 0;
		return ++id_;
	}
	void reset() {
		context_.active_head = &context_.extra[0];
		context_.free_head = nodes_.data();
		context_.extra[0].x = 0;
		context_.extra[0].y = 0;
		context_.extra[0].next = &context_.extra[1];
		context_.extra[1].x = image_file::MAXIMUM_LENGTH;
		context_.extra[1].y = FURTHER_HEIGHT;
		context_.extra[1].next = nullptr;
		for (udx it = 0; it < TOTAL_SEGMENTS - 1; ++it) {
			nodes_[it].x = 0;
			nodes_[it].y = 0;
			nodes_[it].next = &nodes_[it + 1];
		}
		nodes_.back() = stbrp_node{};
	}
	bool append(const glm::ivec2& dimensions, i32& id) {
		this->reset();
		invalidated = true;
		id = virtual_texture::generate_id();
		// Find viable space
		spaces_.push_back({
			id, // id
			cast<stbrp_coord>(dimensions.x), // w
			cast<stbrp_coord>(dimensions.y), // h
			0, 0, // x, y
			0 // was_packed
		});
		if (stbrp_pack_rects(
			&context_,
			spaces_.data(),
			cast<i32>(spaces_.size())
		)) {
			return true;
		}
		// if rect-packing failed, rollback
		spaces_.erase(
			std::remove_if(
				spaces_.begin(),
				spaces_.end(),
				[&id](const stbrp_rect& space) { return space.id == id; }
			),
			spaces_.end()
		);
		if (!stbrp_pack_rects(
			&context_,
			spaces_.data(),
			cast<i32>(spaces_.size())
		)) {
			spdlog::critical("Failed to rollback virtual texture layer!");
		}
		return false;
	}
	std::optional<stbrp_rect> remember(i32 id) const {
		for (auto&& s : spaces_) {
			if (s.id == id and s.was_packed) {
				return s;
			}
		}
		return std::nullopt;
	}
	void recalibrate() {
		for (auto&& iter : cache) {
			if (const auto space = this->remember(iter->id()); space) {
				glCheck(glTextureSubImage2D(
					handle_, 0,
					space->x, space->y,
					space->w, space->h,
					GL_RGBA, GL_UNSIGNED_BYTE,
					iter->pixels()
				));
				iter->offset(space->x, space->y);
			} else {
				throw std::runtime_error("Virtual texture layer cannot remember offsets!");
			}
		}
		glCheck(glGenerateTextureMipmap(handle_));
		invalidated = false;
	}
	bool invalidated {};
	std::set<texture_2d*> cache {};
private:
	stbrp_context context_ {};
	std::vector<stbrp_rect> spaces_ {};
	std::vector<stbrp_node> nodes_ {};
	u32 handle_ {};
};

static std::unique_ptr<virtual_texture> vtp_ {};

void texture_2d::load(image_file image) {
	if (!image.valid()) {
		spdlog::error("Texture image is invalid!");
		return;
	}
	if (!vtp_) {
		vtp_ = std::make_unique<virtual_texture>();
	}
	if (vtp_->cache.find(this) != vtp_->cache.end()) {
		spdlog::error("This texture was almost overwritten! Texture ID: {}", id_);
		return;
	}
	dimensions_ = image.dimensions();
	if (!vtp_->append(dimensions_, id_)) {
		spdlog::critical("Ran out of texture space!");
		this->destroy();
		return;
	}
	image_ = std::move(image);
	vtp_->cache.insert(this);
}

void texture_2d::destroy() {
	if (vtp_) {
		vtp_->cache.erase(this);
		if (vtp_->cache.empty()) {
			vtp_.reset();
		}
	}
	id_ = 0;
	dimensions_ = {};
	offset_ = {};
	if (image_.valid()) {
		image_.clear();
	}
}

i32 texture_2d::binding() {
	return 0;
}

bool texture_2d::recalibrate() {
	if (vtp_ and vtp_->invalidated) {
		vtp_->recalibrate();
		return true;
	}
	return false;
}

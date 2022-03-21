#include <memory>
#include <optional>
#include <array>
#include <set>
#include <spdlog/spdlog.h>
#include <apostellein/cast.hpp>

#define STB_RECT_PACK_IMPLEMENTATION
#include <stb_rect_pack.h>

#include "./material.hpp"
#include "./opengl.hpp"

namespace {
	constexpr u32 DEFAULT_FORMAT = GL_RGBA4;
	constexpr i32 DEFAULT_LAYERS = 4;
	constexpr i32 DEFAULT_MIPMAP = 1;
	constexpr i32 TOTAL_SEGMENTS =
		(image_file::MAXIMUM_LENGTH / image_file::MINIMUM_LENGTH) *
		(image_file::MAXIMUM_LENGTH / image_file::MINIMUM_LENGTH);
}

struct virtual_texture_layer : public not_moveable {
	virtual_texture_layer() {
		nodes.resize(as<udx>(TOTAL_SEGMENTS));
		this->reset();
	}
	void reset() {
		std::fill(
			nodes.begin(),
			nodes.end(),
			stbrp_node{}
		);
		stbrp_init_target(
			&context,
			image_file::MAXIMUM_LENGTH,
			image_file::MAXIMUM_LENGTH,
			nodes.data(),
			TOTAL_SEGMENTS
		);
		stbrp_setup_allow_out_of_mem(&context, 1);
		if (!spaces.empty()) {
			if (!stbrp_pack_rects(
				&context,
				spaces.data(),
				as<i32>(spaces.size())
			)) {
				spdlog::critical("Failed to rollback virtual texture layer!");
			}
		}
	}

	stbrp_context context {};
	std::vector<stbrp_rect> spaces {};
	std::vector<stbrp_node> nodes {};
};

struct virtual_texture : public not_moveable {
	virtual_texture() {
		if (ogl::direct_state_available()) {
			glCheck(glCreateTextures(GL_TEXTURE_2D_ARRAY, 1, &handle_));
			glCheck(glTextureStorage3D(
				handle_,
				DEFAULT_MIPMAP,
				DEFAULT_FORMAT,
				image_file::MAXIMUM_LENGTH,
				image_file::MAXIMUM_LENGTH,
				DEFAULT_LAYERS
			));
			glCheck(glTextureParameteri(handle_, GL_TEXTURE_WRAP_S, GL_REPEAT));
			glCheck(glTextureParameteri(handle_, GL_TEXTURE_WRAP_T, GL_REPEAT));
			glCheck(glTextureParameteri(handle_, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE));
			glCheck(glTextureParameteri(handle_, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
			glCheck(glTextureParameteri(handle_, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
			glCheck(glBindTextureUnit(0, handle_));
		} else  {
			glCheck(glGenTextures(1, &handle_));
			glCheck(glActiveTexture(GL_TEXTURE0));
			glCheck(glBindTexture(GL_TEXTURE_2D_ARRAY, handle_));
			if (ogl::texture_storage_available()) {
				glCheck(glTexStorage3D(
					GL_TEXTURE_2D_ARRAY,
					DEFAULT_MIPMAP,
					DEFAULT_FORMAT,
					image_file::MAXIMUM_LENGTH,
					image_file::MAXIMUM_LENGTH,
					DEFAULT_LAYERS
				));
			} else {
				glCheck(glTexImage3D(
					GL_TEXTURE_2D_ARRAY, 0,
					DEFAULT_FORMAT,
					image_file::MAXIMUM_LENGTH,
					image_file::MAXIMUM_LENGTH,
					DEFAULT_LAYERS,
					0, GL_RGBA, GL_UNSIGNED_BYTE,
					nullptr
				));
			}
			glCheck(glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_REPEAT));
			glCheck(glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_REPEAT));
			glCheck(glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE));
			glCheck(glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
			glCheck(glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
		}
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
	bool append(const glm::ivec2& dimensions, i32& id, i32& atlas) {
		invalidated = true;
		id = virtual_texture::generate_id();
		// Find viable space
		for (auto&& layer : layers_) {
			layer.spaces.push_back({
				id, // id
				as<stbrp_coord>(dimensions.x), // w
				as<stbrp_coord>(dimensions.y), // h
				0, 0, // x, y
				0 // was_packed
			});
			if (stbrp_pack_rects(
				&layer.context,
				layer.spaces.data(),
				as<i32>(layer.spaces.size())
			)) {
				const auto diff = std::distance(layers_.data(), &layer);
				atlas = as<i32>(diff);
				return true;
			}
			// if rect-packing failed, rollback
			layer.spaces.erase(
				std::remove_if(
					layer.spaces.begin(),
					layer.spaces.end(),
					[&id](const stbrp_rect& space) { return space.id == id; }
				),
				layer.spaces.end()
			);
			layer.reset();
		}
		return false;
	}
	std::optional<stbrp_rect> remember(i32 id, i32& atlas) const {
		for (auto&& layer : layers_) {
			for (auto&& space : layer.spaces) {
				if (space.id == id and space.was_packed) {
					const auto diff = std::distance(layers_.data(), &layer);
					atlas = as<i32>(diff);
					return space;
				}
			}
		}
		return std::nullopt;
	}
	void recalibrate() {
		const auto glReceiveTexture = ogl::direct_state_available() ?
			glTextureSubImage3D :
			glTexSubImage3D;
		const auto target = ogl::direct_state_available() ?
			handle_ :
			GL_TEXTURE_2D_ARRAY;
		for (auto&& iter : cache) {
			i32 atlas = 0;
			if (const auto space = this->remember(iter->id(), atlas); space) {
				glCheck(glBindBuffer(
					GL_PIXEL_UNPACK_BUFFER,
					iter->buffer()
				));
				glCheck(glReceiveTexture(
					target, 0,
					space->x, space->y, atlas,
					space->w, space->h, 1,
					GL_RGBA, GL_UNSIGNED_BYTE,
					nullptr
				));
				iter->offset(atlas, space->x, space->y);
			} else {
				throw std::runtime_error("Virtual texture layer cannot remember atlases or offsets!");
			}
		}
		glCheck(glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0));
		invalidated = false;
	}
	bool invalidated {};
	std::set<material*> cache {};
private:
	std::array<virtual_texture_layer, DEFAULT_LAYERS> layers_ {};
	u32 handle_ {};
};

static std::unique_ptr<virtual_texture> vtp_ {};

void material::load(image_file image) {
	if (!image.valid()) {
		spdlog::error("Material image is invalid!");
		return;
	}
	if (!vtp_) {
		vtp_ = std::make_unique<virtual_texture>();
	}
	if (vtp_->cache.find(this) != vtp_->cache.end()) {
		spdlog::error("This material was almost overwritten! Material ID: {}", id_);
		return;
	}
	dimensions_ = image.dimensions();
	if (!vtp_->append(dimensions_, id_, atlas_)) {
		spdlog::critical("Ran out of texture space!");
		this->destroy();
		return;
	}
	if (ogl::direct_state_available()) {
		glCheck(glCreateBuffers(1, &buffer_));
		glCheck(glNamedBufferStorage(
			buffer_,
			image.length(),
			image.pixels(), 0
		));
	} else {
		glCheck(glGenBuffers(1, &buffer_));
		glCheck(glBindBuffer(GL_PIXEL_UNPACK_BUFFER, buffer_));
		if (ogl::buffer_storage_available()) {
			glCheck(glBufferStorage(
				GL_PIXEL_UNPACK_BUFFER,
				image.length(),
				image.pixels(), 0
			));
		} else {
			glCheck(glBufferData(
				GL_PIXEL_UNPACK_BUFFER,
				image.length(),
				image.pixels(),
				GL_STATIC_DRAW
			));
		}
		glCheck(glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0));
	}
	vtp_->cache.insert(this);
}

void material::destroy() {
	if (vtp_) {
		vtp_->cache.erase(this);
		if (vtp_->cache.empty()) {
			vtp_.reset();
		}
	}
	id_ = 0;
	atlas_ = 0;
	dimensions_ = {};
	offset_ = {};
	if (buffer_ != 0) {
		glCheck(glDeleteBuffers(1, &buffer_));
		buffer_ = 0;
	}
}

i32 material::binding() {
	return 0;
}

bool material::recalibrate() {
	if (vtp_ and vtp_->invalidated) {
		vtp_->recalibrate();
		return true;
	}
	return false;
}

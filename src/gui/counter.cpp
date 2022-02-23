#include <array>
#include <algorithm>
#include <glm/exponential.hpp>
#include <apostellein/cast.hpp>

#include "./counter.hpp"
#include "../video/material.hpp"
#include "../x2d/renderer.hpp"

namespace {
	constexpr i32 BASE_TEN_RADIX = 10;
	constexpr i32 MINUS_DIGIT = 10;
	i32 quick_power_of_ten_(i32 exponent) {
		static constexpr std::array POWERS_LIST {
			1,
			10,
			100,
			1000,
			10000,
			100000,
			1000000,
			10000000,
			100000000,
			1000000000
		};
		static constexpr i32 POWERS_SIZE = as<i32>(POWERS_LIST.size());
		if (exponent >= 0 and exponent < POWERS_SIZE) {
			return POWERS_LIST[exponent];
		}
		return as<i32>(glm::pow(10, exponent));
	}
}

void gui::counter::build(
	bool backwards,
	const glm::vec2& position,
	const rect& raster,
	i32 number,
	i32 zeroes,
	const material* texture
) {
	invalidated_ = true;
	backwards_ = backwards;
	position_ = position;
	raster_ = raster;
	zeroes_ = zeroes;
	texture_ = texture;
	this->set(number);
}

void gui::counter::render(renderer& rdr) const {
	if (visible_ and !digits_.empty() and !vertices_.empty()) {
		auto& list = rdr.query(
			priority_type::deferred,
			blending_type::alpha,
			pipeline_type::sprite
		);
		if (invalidated_) {
			invalidated_ = false;
			list.upload(vertices_, digits_.size() * display_list::QUAD);
		} else {
			list.skip(digits_.size() * display_list::QUAD);
		}
	}
}

void gui::counter::generate_quads_() {
	invalidated_ = true;
	digits_.clear();
	if (texture_) {
		i32 value = number_;
		if (value < 0) {
			digits_.push_back(MINUS_DIGIT);
			value = -value;
		}
		do {
			digits_.push_back(value % BASE_TEN_RADIX);
			value /= BASE_TEN_RADIX;
		} while (value != 0);
		if (zeroes_ > 0) {
			for (i32 power = number_ != 0 ? 0 : 1; power < zeroes_; ++power) {
				if (number_ < quick_power_of_ten_(power)) {
					digits_.push_back(0);
				}
			}
		}
		if (digits_.size() * display_list::QUAD > vertices_.size()) {
			vertices_.resize(digits_.size() * display_list::QUAD);
		}

		const glm::vec2 off = texture_->offset();
		const auto atlas = texture_->atlas();
		glm::vec2 pos = position_;
		udx idx = 0;

		auto generator = [
			this, &idx, &pos, &off, &atlas
		](const i32& digit) {
			const glm::vec2 uvs {
				this->raster_.x + as<r32>(digit) * this->raster_.w,
				this->raster_.y
			};

			auto vtx = &this->vertices_[idx * display_list::QUAD];
			vtx[0].position = pos;
			vtx[0].index = 0;
			vtx[0].uvs = (uvs + off) / material::MAXIMUM_DIMENSIONS;
			vtx[0].atlas = atlas;
			vtx[0].color = chroma::WHITE();

			vtx[1].position = { pos.x, pos.y + this->raster_.h };
			vtx[1].index = 0;
			vtx[1].uvs = glm::vec2(uvs.x + off.x, uvs.y + off.y + this->raster_.h) / material::MAXIMUM_DIMENSIONS;
			vtx[1].atlas = atlas;
			vtx[1].color = chroma::WHITE();

			vtx[2].position = { pos.x + this->raster_.w, pos.y };
			vtx[2].index = 0;
			vtx[2].uvs = glm::vec2(uvs.x + off.x + this->raster_.w, uvs.y + off.y) / material::MAXIMUM_DIMENSIONS;
			vtx[2].atlas = atlas;
			vtx[2].color = chroma::WHITE();

			vtx[3].position = pos + this->raster_.dimensions();
			vtx[3].index = 0;
			vtx[3].uvs = (uvs + off + this->raster_.dimensions()) / material::MAXIMUM_DIMENSIONS;
			vtx[3].atlas = atlas;
			vtx[3].color = chroma::WHITE();

			pos.x += (this->backwards_ ?
				-this->raster_.w :
				this->raster_.w
			);
			++idx;
		};

		if (backwards_) {
			std::for_each(digits_.begin(), digits_.end(), generator);
		} else {
			std::for_each(digits_.rbegin(), digits_.rend(), generator);
		}
	}
}

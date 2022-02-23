#pragma once

#include <memory>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include "./const-buffer.hpp"

struct light_param {
	constexpr light_param() noexcept = default;
	constexpr light_param(
		const glm::vec3& _position,
		r32 _diameter,
		const glm::vec3& _attenuation,
		const glm::vec4& _color
	) noexcept :
		position{ _position },
		diameter{ _diameter },
		attenuation{ _attenuation },
		color{ _color } {}
public:
	glm::vec3 position {};
	r32 diameter {};
	glm::vec3 attenuation {};
	r32 unused {};
	glm::vec4 color { 1.0f };
};

struct light_buffer : public const_buffer {
	light_buffer() noexcept = default;
	light_buffer(light_buffer&& that) noexcept {
		*this = std::move(that);
	}
	light_buffer& operator=(light_buffer&& that) noexcept {
		if (this != &that) {
			handle_ = that.handle_;
			that.handle_ = 0;
			binding_ = that.binding_;
			that.binding_ = 0;
			drawable_ = that.drawable_;
			that.drawable_ = 0;
			staging_ = std::move(that.staging_);
		}
		return *this;
	}
	~light_buffer() = default;
public:
	static udx maximum();
	light_param& next() {
		++drawable_;
		assert(drawable_ < light_buffer::maximum());
		return staging_[drawable_];
	}
	bool flush(i32 scaling);
	udx length() const override;
private:
	udx drawable_ {};
	std::unique_ptr<light_param[]> staging_ {};
};

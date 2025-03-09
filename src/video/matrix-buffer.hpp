#pragma once

#include <glm/mat4x4.hpp>

#include "./const-buffer.hpp"

struct matrix_buffer : public const_buffer {
	matrix_buffer() noexcept = default;
	matrix_buffer(matrix_buffer&& that) noexcept {
		*this = std::move(that);
	}
	matrix_buffer& operator=(matrix_buffer&& that) noexcept {
		if (this != &that) {
			handle_ = that.handle_;
			that.handle_ = 0;
			binding_ = that.binding_;
			that.binding_ = 0;
			cached_ = that.cached_;
			that.cached_ = glm::mat4{ 1.0f };
		}
		return *this;
	}
	~matrix_buffer() = default;
public:
	static udx maximum();
	udx length() const override;
	bool projection(const glm::mat4& value);
	bool viewport(const glm::mat4& value);
private:
	glm::mat4 cached_ { 1.0f };
};

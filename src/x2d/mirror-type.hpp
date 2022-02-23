#pragma once

#include <apostellein/def.hpp>

struct mirror_type {
	constexpr mirror_type() noexcept = default;
	constexpr mirror_type(
		bool _horizontally,
		bool _vertically
	) noexcept : horizontally{ _horizontally }, vertically{ _vertically } {}

	bool horizontally {};
	bool vertically {};
public:
	constexpr operator bool() const noexcept {
		return horizontally or vertically;
	}
};

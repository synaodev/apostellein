#pragma once

#include <type_traits>
#include <apostellein/def.hpp>

template<typename R, typename T>
constexpr R as(const T value) noexcept {
	if constexpr (std::is_enum<T>::value) {
		static_assert(std::is_enum<T>::value and std::is_same<std::underlying_type_t<T>, R>::value);
	} else {
		static_assert(std::is_arithmetic<T>::value and std::is_arithmetic<R>::value);
	}
	return static_cast<R>(value);
}

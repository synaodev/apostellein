#pragma once

#include <glm/vec2.hpp>
#include <glm/common.hpp>
#include <glm/geometric.hpp>
#include <apostellein/def.hpp>

namespace konst {
	// dimensions
	template<typename T>
	constexpr T WINDOW_WIDTH() noexcept { return static_cast<T>(480); }
	template<typename T>
	constexpr T WINDOW_HEIGHT() noexcept { return static_cast<T>(270); }
	template<typename T>
	constexpr glm::vec<2, T, glm::packed_highp> WINDOW_DIMENSIONS() noexcept {
		return { WINDOW_WIDTH<T>(), WINDOW_HEIGHT<T>() };
	}

	// tiles
	template<typename T>
	constexpr T TILE() noexcept { return static_cast<T>(16); }
	template<typename T>
	constexpr T HALF_TILE() noexcept { return TILE<T>() / static_cast<T>(2); }
	template<typename T>
	constexpr T QUARTER_TILE() noexcept { return HALF_TILE<T>() / static_cast<T>(2); }
	template<typename T>
	constexpr glm::vec<2, T, glm::packed_highp> TILE_DIMENSIONS() noexcept {
		return { TILE<T>(), TILE<T>() };
	}
	inline i32 TILE_ROUND(r32 value) noexcept {
		return static_cast<i32>(value) / konst::TILE<i32>();
	}
	inline i32 TILE_CEILING(r32 value) noexcept {
		return static_cast<i32>(glm::ceil(value / konst::TILE<r32>()));
	}
	inline i32 TILE_FLOOR(r32 value) noexcept {
		return static_cast<i32>(glm::floor(value / konst::TILE<r32>()));
	}
	inline r32 TILE_ALIGN(i32 value) noexcept {
		return static_cast<r32>(value * konst::TILE<i32>());
	}

	// interpolate
	template<typename T>
	inline glm::vec<2, T, glm::packed_highp> INTERPOLATE(
		const glm::vec<2, T, glm::packed_highp>& back,
		const glm::vec<2, T, glm::packed_highp>& front,
		const T& ratio
	) {
		static_assert(std::is_floating_point<T>::value);
		if (back == front or glm::distance(back, front) >= TILE<T>()) {
			return front;
		}
		return glm::mix(back, front, ratio);
	}

	// seconds
	template<typename T>
	constexpr T ZERO() noexcept { return static_cast<T>(0); }
	template<typename T>
	constexpr T ONE() noexcept { return static_cast<T>(1); }
	template<typename T>
	constexpr T BILLION() noexcept { return static_cast<T>(1'000'000'000); }
	template<typename T>
	constexpr T TICK() noexcept { return static_cast<T>(60); }
	template<typename T>
	constexpr T INVERSE_TICK() noexcept { return ONE<T>() / TICK<T>(); }
	template<typename T = r64>
	constexpr i64 SECONDS_TO_NANOSECONDS(T seconds) noexcept {
		static_assert(std::is_floating_point<T>::value);
		return static_cast<i64>(seconds * BILLION<T>());
	}
	template<typename T = r64>
	constexpr T NANOSECONDS_TO_SECONDS(i64 nanoseconds) noexcept {
		static_assert(std::is_floating_point<T>::value);
		return static_cast<T>(nanoseconds) / BILLION<T>();
	}
	template<typename T = r64>
	constexpr i64 NANOSECONDS_PER_TICK() noexcept {
		static_assert(std::is_floating_point<T>::value);
		return SECONDS_TO_NANOSECONDS<T>(INVERSE_TICK<T>());
	}
	template<typename T = r64>
	constexpr T FRAMES_PER_SECOND(i64 nanoseconds, i64 frames) noexcept {
		if (const T seconds = NANOSECONDS_TO_SECONDS<T>(nanoseconds); seconds > ZERO<T>()) {
			return static_cast<T>(frames) / seconds;
		}
		return ZERO<T>();
	}

	// sizes
	constexpr udx KILOBYTE = 1024;
	constexpr udx MEGABYTE = KILOBYTE * KILOBYTE;
	constexpr udx MAXIMUM_BYTES = MEGABYTE * 10;
	constexpr udx MAXIMUM_SINKS = 10;

	// strings
	constexpr char APPLICATION[] = "Apostéllein";
	constexpr char GRAPHICS[] = "OpenGL";
	constexpr char ORGANIZATION[] = "studio-synao";
	constexpr char IDENTIFIER[] = "a59323db-3294-41e4-a375-9d05595db2e9";
	constexpr char PATTERN[] = "[%X.%e] %^[%l] %v%$";
	constexpr char VERSION[] = "0.0.0.0";
	constexpr char ARCHITECTURE[] =
#if defined(APOSTELLEIN_MACHINE_64BIT)
	"64-bit"
#else
	"32-bit"
#endif
	;
	constexpr char PLATFORM[] =
#if defined(APOSTELLEIN_PLATFORM_WINDOWS)
		"Windows"
#elif defined(APOSTELLEIN_PLATFORM_MACOS)
		"MacOS"
#elif defined(APOSTELLEIN_PLATFORM_LINUX)
		"Linux"
#elif defined(APOSTELLEIN_PLATFORM_FREEBSD)
		"FreeBSD"
#elif defined(APOSTELLEIN_PLATFORM_OPENBSD)
		"OpenBSD"
#elif defined(APOSTELLEIN_PLATFORM_NETBSD)
		"NetBSD"
#elif defined(APOSTELLEIN_PLATFORM_DRAGONFLY)
		"Dragonfly"
#endif
	;
	constexpr char COMPILER[] =
#if defined(APOSTELLEIN_COMPILER_MSVC)
		"MSVC"
#elif defined(APOSTELLEIN_COMPILER_LLVM)
		"Clang"
#elif defined(APOSTELLEIN_COMPILER_GNUC)
		"GCC"
#endif
	;
	constexpr char TOOLCHAIN[] =
#if defined(APOSTELLEIN_TOOLCHAIN_LLVM)
		"LLVM"
#elif defined(APOSTELLEIN_TOOLCHAIN_APPLECLANG)
		"Apple"
#elif defined(APOSTELLEIN_TOOLCHAIN_GNUC)
		"GNU"
#elif defined(APOSTELLEIN_TOOLCHAIN_MSVC)
		"MSBuild"
#endif
	;
	constexpr char BUILD_TYPE[] =
#if !defined(NDEBUG)
	"Debug"
#else
	"Release"
#endif
	;

	// conditions
	constexpr bool DEBUG =
#if !defined(NDEBUG)
	true
#else
	false
#endif
	;
	constexpr bool IMGUI =
#if defined(APOSTELLEIN_IMGUI_DEBUGGER)
	true
#else
	false
#endif
	;
}

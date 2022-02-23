#pragma once

#include <apostellein/struct.hpp>

struct tile_type {
	constexpr tile_type() noexcept = default;
	constexpr tile_type(u32 value) noexcept { flags._raw = value; }
	constexpr tile_type& operator=(u32 value) noexcept {
		flags._raw = value;
		return *this;
	}
	union {
		bitfield_raw<u32> _raw {};
		bitfield_index<u32, 0> block;
		bitfield_index<u32, 1> sloped;
		bitfield_index<u32, 2> positive;
		bitfield_index<u32, 3> negative;
		bitfield_index<u32, 4> floor;
		bitfield_index<u32, 5> ceiling;
		bitfield_index<u32, 6> low;
		bitfield_index<u32, 7> high;
		bitfield_index<u32, 8> harmful;
		bitfield_index<u32, 9> out_of_bounds;
		bitfield_index<u32, 10> fall_through;
		bitfield_index<u32, 11> hookable;
	} flags {};
public:
	constexpr bool any() const noexcept { return flags._raw.any(); }
	constexpr u32 slope() const noexcept {
		if (flags.sloped) {
			if (flags.positive) {
				if (flags.floor) {
					if (flags.high) return 1;
					if (flags.low) return 2;
				}
				if (flags.ceiling) {
					if (flags.low) return 7;
					if (flags.high) return 8;
				}
			}
			if (flags.negative) {
				if (flags.floor) {
					if (flags.low) return 3;
					if (flags.high) return 4;
				}
				if (flags.ceiling) {
					if (flags.high) return 5;
					if (flags.low) return 6;
				}
			}
		}
		return 0;
	}
};

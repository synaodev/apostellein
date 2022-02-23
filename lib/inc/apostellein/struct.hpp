#pragma once

#include <apostellein/def.hpp>

struct not_copyable {
	not_copyable() noexcept = default;
	not_copyable(const not_copyable&) = delete;
	not_copyable& operator=(const not_copyable&) = delete;
	not_copyable(not_copyable&&) noexcept = default;
	not_copyable& operator=(not_copyable&&) noexcept = default;
	~not_copyable() = default;
};

struct not_moveable {
public:
	not_moveable() noexcept = default;
	not_moveable(const not_moveable&) = delete;
	not_moveable& operator=(const not_moveable&) = delete;
	not_moveable(not_moveable&&) = delete;
	not_moveable& operator=(not_moveable&&) = delete;
	~not_moveable() = default;
};

template<typename T>
struct bitfield_raw {
	constexpr bitfield_raw() noexcept = default;
	constexpr bitfield_raw(const T& mask) noexcept :
		value_{ mask } {}
	constexpr bitfield_raw& operator=(const T& mask) noexcept {
		value_ = mask;
		return *this;
	}
public:
	constexpr void set(T index, bool value) noexcept {
		if (value) {
			value_ |= (static_cast<T>(1) << index);
		} else {
			value_ &= ~(static_cast<T>(1) << index);
		}
	}
	constexpr bool get(T index) const noexcept {
		return value_ & (static_cast<T>(1) << index);
	}
	constexpr bool any() const noexcept { return value_ != 0; }
	constexpr T value() const noexcept { return value_; }
private:
	T value_ {};
};

template<typename T, udx Index>
struct bitfield_index {
	static_assert(Index < sizeof(T) * 8);
	static constexpr T BITMASK = static_cast<T>(1) << Index;

	constexpr bitfield_index() noexcept = default;
	constexpr bitfield_index(bool value) noexcept { *this = value; }
	constexpr bitfield_index& operator=(bool value) noexcept {
		if (value) {
			value_ |= BITMASK;
		} else {
			value_ &= ~BITMASK;
		}
		return *this;
	}
public:
	constexpr bool value() const noexcept { return value_ & BITMASK; }
	constexpr operator bool() const noexcept { return this->value(); }
private:
	T value_ {};
};

struct chroma {
	constexpr chroma() noexcept = default;
	constexpr chroma(byte _r, byte _g, byte _b) noexcept :
		r{ _r }, g{ _g }, b{ _b } {}
	constexpr chroma(byte _r, byte _g, byte _b, byte _a) noexcept :
		r{ _r }, g{ _g }, b{ _b }, a{ _a } {}
	constexpr chroma(u32 value) noexcept :
		r{ static_cast<byte>((value & 0xFF000000U) >> 24) },
		g{ static_cast<byte>((value & 0x00FF0000U) >> 16) },
		b{ static_cast<byte>((value & 0x0000FF00U) >> 8) },
		a{ static_cast<byte>((value & 0x000000FFU) >> 0) } {}
	constexpr chroma& operator=(u32 value) noexcept {
		r = static_cast<byte>((value & 0xFF000000U) >> 24);
		g = static_cast<byte>((value & 0x00FF0000U) >> 16);
		b = static_cast<byte>((value & 0x0000FF00U) >> 8);
		a = static_cast<byte>((value & 0x000000FFU) >> 0);
		return *this;
	}

	byte r {};
	byte g {};
	byte b {};
	byte a {};
public:
	static constexpr chroma TRANSLUCENT() { return { 0x00U, 0x00U, 0x00U, 0x7FU }; }
	static constexpr chroma WHITE() { return { 0xFFU, 0xFFU, 0xFFU, 0xFFU }; }
	static constexpr chroma BASE() { return { 0x00U, 0x00U, 0x1FU, 0xFFU }; }

	constexpr bool operator==(const chroma& that) noexcept {
		return (
			this->r == that.r and
			this->g == that.g and
			this->b == that.b and
			this->a == that.a
		);
	}
	constexpr bool operator!=(const chroma& that) noexcept {
		return !(*this == that);
	}
};

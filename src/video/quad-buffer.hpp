#pragma once

#include <cassert>
#include <cstring>
#include <memory>

#include "./vertex.hpp"

struct index_buffer;
struct shader_program;

struct quad_buffer : public not_moveable {
	quad_buffer(const index_buffer& indices, const vertex_format& format);
	virtual ~quad_buffer() = default;
public:
	static std::unique_ptr<quad_buffer> allocate(
		const index_buffer& indices,
		const vertex_format& format,
		bool streaming
	);
	template<typename T = udx>
	static constexpr T INDICES_TO_VERTICES(udx count) noexcept {
		return static_cast<T>((count / 6) * 4);
	}
	template<typename T = udx>
	static constexpr T VERTICES_TO_INDICES(udx count) noexcept {
		return static_cast<T>((count * 6) / 4);
	}
	template<typename T = udx>
	static constexpr T QUADS_TO_INDICES(udx count) noexcept {
		const auto vertices = count * 4;
		return VERTICES_TO_INDICES<T>(vertices);
	}
	template<typename V>
	V* at(udx index) {
		static_assert(std::is_base_of<vtx_type, V>::value);
		assert(format_.id == V::id());
		assert(index < length_);
		return reinterpret_cast<V*>(this->staging(index));
	}
	template<typename V>
	void copy(const std::vector<V>& source, udx index, udx count) {
		static_assert(std::is_base_of<vtx_type, V>::value);
		assert(format_.id == V::id());
		assert((index + count) < length_);
#if defined(APOSTELLEIN_PLATFORM_WINDOWS)
		std::memcpy(
			this->staging(index),
			source.data(),
			count * sizeof(V)
		);
#else
		std::copy(
			source.begin(),
			source.begin() + count,
			reinterpret_cast<V*>(this->staging(index))
		);
#endif
	}
	udx length() const { return length_; }
	virtual bool draw(const shader_program& program, udx count) noexcept = 0;
	virtual bool valid() const noexcept = 0;
protected:
	virtual char* staging(udx index) noexcept = 0;
	vertex_format format_ {};
	udx length_ {};
};

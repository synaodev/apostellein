#pragma once

#include <vector>
#include <glm/vec2.hpp>
#include <apostellein/struct.hpp>

struct vtx_type {
	constexpr vtx_type() noexcept = default;
protected:
	static u32 generate_id() {
		static u32 id_ = 0;
		return ++id_;
	}
};

template<typename V>
struct vertex_template : public vtx_type {
	constexpr vertex_template() noexcept = default;
public:
	static u32 id() {
		static u32 id_ = vtx_type::generate_id();
		return id_;
	}
};

struct vertex_format {
	constexpr vertex_format() noexcept = default;

	u32 id {};
	udx size {};
	void(*detail)(u32 handle) {};
public:
	constexpr operator bool() const {
		return (
			id > 0 and
			size > 0 and
			detail != nullptr
		);
	}
	constexpr bool operator==(const vertex_format& that) const {
		return this->id == that.id;
	}
	constexpr bool operator!=(const vertex_format& that) const {
		return !(*this == that);
	}
	static vertex_format from(const std::vector<u32>& types);
	static vertex_format from(u32 id);
};

struct vtx_light : public vertex_template<vtx_light> {
	constexpr vtx_light() noexcept = default;

	glm::vec2 position {};
};

struct vtx_blank : public vertex_template<vtx_blank> {
	constexpr vtx_blank() noexcept = default;

	glm::vec2 position {};
	i32 index {};
	chroma color { chroma::WHITE() };
};

struct vtx_sprite : public vertex_template<vtx_sprite> {
	constexpr vtx_sprite() noexcept = default;

	glm::vec2 position {};
	i32 index {};
	glm::vec2 uvs {};
	r32 atlas {};
	chroma color { chroma::WHITE() };
};

#include <array>
#include <spdlog/spdlog.h>
#include <apostellein/konst.hpp>
#include <apostellein/cast.hpp>

#include "./vertex.hpp"
#include "./opengl.hpp"

namespace {
	template<typename A, typename B>
	struct address_offset_calculator {
		static constexpr B detail {};
		static constexpr udx value(A B::*member) noexcept {
			return
				reinterpret_cast<udx>(&(address_offset_calculator<A, B>::detail.*member)) -
				reinterpret_cast<udx>(&(address_offset_calculator<A, B>::detail));
		}
	};
	template<typename A, typename B>
	constexpr B address_offset_calculator<A, B>::detail;
	template<typename A, typename B>
	constexpr u32 INTEGRAL_OFFSET(A B::*member) noexcept {
		return as<u32>(address_offset_calculator<A, B>::value(member));
	}
	template<typename A, typename B>
	constexpr const void* ADDRESS_OFFSET(A B::*member) noexcept {
		return reinterpret_cast<const void*>(address_offset_calculator<A, B>::value(member));
	}
	constexpr std::array LIGHT_TYPES {
		as<u32>(GL_FLOAT_VEC2)
	};
	constexpr std::array BLANK_TYPES {
		as<u32>(GL_FLOAT_VEC2),
		as<u32>(GL_INT),
		as<u32>(GL_FLOAT_VEC4)
	};
	constexpr std::array SPRITE_TYPES {
		as<u32>(GL_FLOAT_VEC2),
		as<u32>(GL_INT),
		as<u32>(GL_FLOAT_VEC3),
		as<u32>(GL_FLOAT_VEC4)
	};
}

vertex_format vertex_format::from(const std::vector<u32>& types) {
	auto compare = [](const auto& lhv, const auto& rhv) {
		return std::equal(lhv.begin(), lhv.end(), rhv.begin(), rhv.end());
	};
	if (compare(types, LIGHT_TYPES)) {
		return vertex_format::from(vtx_light::id());
	} else if (compare(types, BLANK_TYPES)) {
		return vertex_format::from(vtx_blank::id());
	} else if (compare(types, SPRITE_TYPES)) {
		return vertex_format::from(vtx_sprite::id());
	}
	return {};
}

vertex_format vertex_format::from(u32 id) {
	vertex_format result {};
	if (id == vtx_light::id()) {
		result.id = vtx_light::id();
		result.size = sizeof(vtx_light);
		result.detail = [](u32 handle) {
			if (ogl::direct_state_available()) {
				glCheck(glEnableVertexArrayAttrib(handle, 0));
				glCheck(glVertexArrayAttribFormat(
					handle, 0, 2, GL_FLOAT, GL_FALSE,
					INTEGRAL_OFFSET(&vtx_light::position)
				));
				glCheck(glVertexArrayAttribBinding(handle, 0, 0));
			} else {
				glCheck(glEnableVertexAttribArray(0));
				glCheck(glVertexAttribPointer(
					0, 2, GL_FLOAT,
					GL_FALSE, sizeof(vtx_light),
					ADDRESS_OFFSET(&vtx_light::position)
				));
			}
		};
	} else if (id == vtx_blank::id()) {
		result.id = vtx_blank::id();
		result.size = sizeof(vtx_blank);
		result.detail = [](u32 handle) {
			if (ogl::direct_state_available()) {
				glCheck(glEnableVertexArrayAttrib(handle, 0));
				glCheck(glEnableVertexArrayAttrib(handle, 1));
				glCheck(glEnableVertexArrayAttrib(handle, 2));
				glCheck(glVertexArrayAttribFormat(
					handle, 0, 2, GL_FLOAT, GL_FALSE,
					INTEGRAL_OFFSET(&vtx_blank::position)
				));
				glCheck(glVertexArrayAttribIFormat(
					handle, 1, 1, GL_INT,
					INTEGRAL_OFFSET(&vtx_blank::index)
				));
				glCheck(glVertexArrayAttribFormat(
					handle, 2, 4, GL_UNSIGNED_BYTE, GL_TRUE,
					INTEGRAL_OFFSET(&vtx_blank::color)
				));
				glCheck(glVertexArrayAttribBinding(handle, 0, 0));
				glCheck(glVertexArrayAttribBinding(handle, 1, 0));
				glCheck(glVertexArrayAttribBinding(handle, 2, 0));
			} else {
				glCheck(glEnableVertexAttribArray(0));
				glCheck(glEnableVertexAttribArray(1));
				glCheck(glEnableVertexAttribArray(2));
				glCheck(glVertexAttribPointer(
					0, 2, GL_FLOAT,
					GL_FALSE, sizeof(vtx_blank),
					ADDRESS_OFFSET(&vtx_blank::position)
				));
				glCheck(glVertexAttribIPointer(
					1, 1, GL_INT,
					sizeof(vtx_blank),
					ADDRESS_OFFSET(&vtx_blank::index)
				));
				glCheck(glVertexAttribPointer(
					2, 4, GL_UNSIGNED_BYTE,
					GL_TRUE, sizeof(vtx_blank),
					ADDRESS_OFFSET(&vtx_blank::color)
				));
			}
		};
	} else if (id == vtx_sprite::id()) {
		result.id = vtx_sprite::id();
		result.size = sizeof(vtx_sprite);
		result.detail = [](u32 handle) {
			if (ogl::direct_state_available()) {
				glCheck(glEnableVertexArrayAttrib(handle, 0));
				glCheck(glEnableVertexArrayAttrib(handle, 1));
				glCheck(glEnableVertexArrayAttrib(handle, 2));
				glCheck(glEnableVertexArrayAttrib(handle, 3));
				glCheck(glVertexArrayAttribFormat(
					handle, 0, 2, GL_FLOAT, GL_FALSE,
					INTEGRAL_OFFSET(&vtx_sprite::position)
				));
				glCheck(glVertexArrayAttribIFormat(
					handle, 1, 1, GL_INT,
					INTEGRAL_OFFSET(&vtx_sprite::index)
				));
				glCheck(glVertexArrayAttribFormat(
					handle, 2, 3, GL_FLOAT, GL_FALSE,
					INTEGRAL_OFFSET(&vtx_sprite::uvs)
				));
				glCheck(glVertexArrayAttribFormat(
					handle, 3, 4, GL_UNSIGNED_BYTE, GL_TRUE,
					INTEGRAL_OFFSET(&vtx_sprite::color)
				));
				glCheck(glVertexArrayAttribBinding(handle, 0, 0));
				glCheck(glVertexArrayAttribBinding(handle, 1, 0));
				glCheck(glVertexArrayAttribBinding(handle, 2, 0));
				glCheck(glVertexArrayAttribBinding(handle, 3, 0));
			} else {
				glCheck(glEnableVertexAttribArray(0));
				glCheck(glEnableVertexAttribArray(1));
				glCheck(glEnableVertexAttribArray(2));
				glCheck(glEnableVertexAttribArray(3));
				glCheck(glVertexAttribPointer(
					0, 2, GL_FLOAT,
					GL_FALSE, sizeof(vtx_sprite),
					ADDRESS_OFFSET(&vtx_sprite::position)
				));
				glCheck(glVertexAttribIPointer(
					1, 1, GL_INT,
					sizeof(vtx_sprite),
					ADDRESS_OFFSET(&vtx_sprite::index)
				));
				glCheck(glVertexAttribPointer(
					2, 3, GL_FLOAT,
					GL_FALSE, sizeof(vtx_sprite),
					ADDRESS_OFFSET(&vtx_sprite::uvs)
				));
				glCheck(glVertexAttribPointer(
					3, 4, GL_UNSIGNED_BYTE,
					GL_TRUE, sizeof(vtx_sprite),
					ADDRESS_OFFSET(&vtx_sprite::color)
				));
			}
		};
	}
	if (!result) {
		spdlog::critical("Vertex declaration was generated incorrectly!");
	}
	return result;
}

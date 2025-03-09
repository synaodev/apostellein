#include <algorithm>
#include <limits>
#include <spdlog/spdlog.h>
#include <glm/gtc/matrix_transform.hpp>
#include <apostellein/konst.hpp>
#include <apostellein/cast.hpp>

#include "./renderer-2d.hpp"
#include "./pipeline-source.hpp"
#include "../video/opengl.hpp"
#include "../video/texture-2d.hpp"
#include "../video/swap-chain.hpp"

namespace {
	constexpr udx MAXIMUM_QUAD_INDICES = quad_buffer::QUADS_TO_INDICES(1024);
	constexpr udx MAXIMUM_PRIORITIES = cast<udx>(priority_type::deferred) + 1;
	constexpr udx MAXIMUM_BLENDINGS = cast<udx>(blending_type::multiply) + 1;
	// constexpr udx MAXIMUM_PIPELINES = cast<udx>(pipeline_type::light) + 1;
	constexpr udx MAXIMUM_PIPELINES = cast<udx>(pipeline_type::glyph) + 1;
	constexpr udx MAXIMUM_LISTS = MAXIMUM_PRIORITIES * MAXIMUM_BLENDINGS * MAXIMUM_PIPELINES;
}

bool renderer_2d::build() {
	if (!indices_.quads(MAXIMUM_QUAD_INDICES)) {
		spdlog::critical("Couldn't setup global index buffer!");
		return false;
	}

	if (!matrices_.create(0)) {
		spdlog::critical("Couldn't setup matrix buffer!");
		return false;
	} else {
		matrices_.projection(
			glm::ortho(
				0.0f, konst::WINDOW_WIDTH<r32>(),
				konst::WINDOW_HEIGHT<r32>(), 0.0f
			)
		);
	}

	// if (!lights_.create(1)) {
	// 	spdlog::critical("Couldn't setup light buffer!");
	// 	return false;
	// }

	shader_object blank_vertex_object {};
	if (!blank_vertex_object.create(
		pipeline_source::blank_vertex_code(),
		shader_stage::vertex
	)) {
		spdlog::critical("Compilation of \"blank\" vertex object failed!");
		return false;
	}
	shader_object blank_pixel_object {};
	if (!blank_pixel_object.create(
		pipeline_source::blank_pixel_code(),
		shader_stage::pixel
	)) {
		spdlog::critical("Compilation of \"blank\" pixel object failed!");
		return false;
	}
	shader_object sprite_vertex_object {};
	if (!sprite_vertex_object.create(
		pipeline_source::sprite_vertex_code(),
		shader_stage::vertex
	)) {
		spdlog::critical("Compilation of \"sprite\" vertex object failed!");
		return false;
	}
	shader_object sprite_pixel_object {};
	if (!sprite_pixel_object.create(
		pipeline_source::sprite_pixel_code(),
		shader_stage::pixel
	)) {
		spdlog::critical("Compilation of \"sprite\" pixel object failed!");
		return false;
	}
	shader_object glyph_vertex_object {};
	if (!glyph_vertex_object.create(
		pipeline_source::glyph_vertex_code(),
		shader_stage::vertex
	)) {
		spdlog::critical("Compilation of \"glyph\" vertex object failed!");
		return false;
	}
	shader_object glyph_pixel_object {};
	if (!glyph_pixel_object.create(
		pipeline_source::glyph_pixel_code(),
		shader_stage::pixel
	)) {
		spdlog::critical("Compilation of \"glyph\" pixel object failed!");
		return false;
	}
	// shader_object light_vertex_object {};
	// if (!light_vertex_object.create(
	// 	pipeline_source::light_vertex_code(),
	// 	shader_stage::vertex
	// )) {
	// 	spdlog::critical("Compilation of \"light\" vertex object failed!");
	// 	return false;
	// }
	// shader_object light_pixel_object {};
	// if (!light_pixel_object.create(
	// 	pipeline_source::light_pixel_code(),
	// 	shader_stage::pixel
	// )) {
	// 	spdlog::critical("Compilation of \"light\" pixel object failed!");
	// 	return false;
	// }

	programs_.resize(MAXIMUM_PIPELINES);

	auto& blank_program = programs_[cast<udx>(pipeline_type::blank)];
	if (!blank_program.create(blank_vertex_object, blank_pixel_object)) {
		spdlog::critical("\"blank\" program creation failed!");
		return false;
	}

	auto& sprite_program = programs_[cast<udx>(pipeline_type::sprite)];
	if (!sprite_program.create(sprite_vertex_object, sprite_pixel_object)) {
		spdlog::critical("\"sprite\" program creation failed!");
		return false;
	}

	auto& glyph_program = programs_[cast<udx>(pipeline_type::glyph)];
	if (!glyph_program.create(glyph_vertex_object, glyph_pixel_object)) {
		spdlog::critical("\"glyph\" program creation failed!");
		return false;
	}

	// auto& light_program = programs_[as<udx>(pipeline_type::light)];
	// if (!light_program.create(light_vertex_object, light_pixel_object)) {
	// 	spdlog::critical("\"light\" program creation failed!");
	// 	return false;
	// }

	swap_chain::reset();

	return true;
}

void renderer_2d::flush(const glm::mat4& viewport) {
	swap_chain::clear(color_type::TRANSLUCENT());

	matrices_.viewport(viewport);

	for (auto&& list : lists_) {
		if (list.visible()) {
			swap_chain::blend(list.blending());
			const auto index = cast<udx>(list.pipeline());
			list.flush(programs_[index]);
		}
	}
}

display_list& renderer_2d::query(priority_type priority, blending_type blending, pipeline_type pipeline) {
	for (auto&& list : lists_) {
		if (list.matches(priority, blending, pipeline)) {
			return list;
		}
	}
	if (lists_.size() == MAXIMUM_LISTS) {
		const std::string message = fmt::format("Display list count is currently {}!", lists_.size());
		spdlog::critical(message);
		throw std::runtime_error(message);
	}
	auto quads = quad_buffer::allocate(
		indices_,
		programs_[cast<udx>(pipeline)].format(),
		priority != priority_type::deferred
	);
	lists_.emplace_back(
		priority,
		blending,
		pipeline,
		std::move(quads)
	);
	std::sort(lists_.begin(), lists_.end());
	return this->query(priority, blending, pipeline);
}

udx renderer_2d::calculate_visible_lists_() const {
	return cast<udx>(std::count_if(
		lists_.begin(),
		lists_.end(),
		[](const auto& list) { return list.visible(); }
	));
}

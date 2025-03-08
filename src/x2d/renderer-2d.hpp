#pragma once

#include <glm/mat4x4.hpp>

#include "./display-list.hpp"
#include "../video/matrix-buffer.hpp"
#include "../video/light-buffer.hpp"
#include "../video/index-buffer.hpp"
#include "../video/shader.hpp"

struct renderer_report {
	udx visible_lists {};
	udx all_lists {};
};

struct renderer_2d {
public:
	bool build();
	void clear() { lists_.clear(); }
	void flush(const glm::mat4& viewport);
	display_list& query(priority_type priority, blending_type blending, pipeline_type pipeline);
	auto reporter() const {
		return [this] {
			return std::make_pair(
				this->calculate_visible_lists_(),
				this->lists_.size()
			);
		};
	}
private:
	udx calculate_visible_lists_() const;
	matrix_buffer matrices_ {};
	// light_buffer lights_ {};
	blending_type blending_ { blending_type::alpha };
	std::vector<shader_program> programs_ {};
	std::vector<display_list> lists_ {};
	index_buffer indices_ {};
};

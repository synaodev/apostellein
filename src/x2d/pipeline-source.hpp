#pragma once

#include <string>
#include <vector>

namespace pipeline_source {
	constexpr char MATRIX_BUFFER_NAME[] = "matrix_buffer";
	constexpr char LIGHT_BUFFER_NAME[] = "light_buffer";
	constexpr char SAMPLER_ARRAY_NAME[] = "sampler_array";
	constexpr char FRAME_BUFFER_NAME[] = "frame_buffer";

	std::string directive();
	std::string matrix_buffer();
	std::string sampler_array();
	std::string light_buffer();
	std::string frame_buffer();
	std::string blank_vertex_code();
	std::string blank_pixel_code();
	std::string sprite_vertex_code();
	std::string sprite_pixel_code();
	std::string glyph_vertex_code();
	std::string glyph_pixel_code();
	std::string light_vertex_code();
	std::string light_pixel_code();
}

#include <fmt/format.h>

#include "./pipeline-source.hpp"
#include "../video/light-buffer.hpp"
#include "../video/matrix-buffer.hpp"
#include "../video/opengl.hpp"

// public
std::string pipeline_source::directive() {
	return "#version 450 core\n";
}

std::string pipeline_source::matrix_buffer() {
	return fmt::format(
		"layout(std140, binding = 0) uniform matrix_buffer {{\n"
			"\tmat4 viewports[{}];\n"
		"}};",
		matrix_buffer::maximum()
	);
}

std::string pipeline_source::sampler_array() {
	return "layout(binding = 0) uniform sampler2D sampler_array;";
}

std::string pipeline_source::light_buffer() {
	return fmt::format(
		"struct Light {{\n"
			"\tvec4 position;\n" // xyz = position, w = diameter
			"\tvec4 attenuation;\n" // xyz = attenuation, w = unused
			"\tvec4 color;\n"
		"}};\n"
		"layout(std140, binding = 1) uniform light_buffer {{\n"
			"\tvec4 scaling;\n" // xy = dimensions, zw = resolution
			"\tuint count;\n"
			"\tLight lights[{}];\n"
		"}};",
		light_buffer::maximum()
	);
}

std::string pipeline_source::frame_buffer() {
	// TODO: This will not be bound to "0" in any serious use case.
	return "layout(binding = 0) uniform sampler2D frame_buffer;";
}

static constexpr char SOURCE_BLANK_VERTEX[] = R"({}{}
layout(location = 0) in vec2 position;
layout(location = 1) in int index;
layout(location = 2) in vec4 color;
out PS {{
	vec4 color;
}} ps;
void main() {{
	gl_Position = viewports[index] * vec4(position, 0.0f, 1.0f);
	ps.color = color;
}})";

std::string pipeline_source::blank_vertex_code() {
	return fmt::format(
		SOURCE_BLANK_VERTEX,
		pipeline_source::directive(),
		pipeline_source::matrix_buffer()
	);
}

static constexpr char SOURCE_BLANK_PIXEL[] = R"({}
in PS {{
	vec4 color;
}} ps;
layout(location = 0) out vec4 pixel;
void main() {{
	pixel = ps.color;
}})";

std::string pipeline_source::blank_pixel_code() {
	return fmt::format(
		SOURCE_BLANK_PIXEL,
		pipeline_source::directive()
	);
}

static constexpr char SOURCE_SPRITE_VERTEX[] = R"({}{}
layout(location = 0) in vec2 position;
layout(location = 1) in int index;
layout(location = 2) in vec2 uvs;
layout(location = 3) in vec4 color;
out PS {{
	vec2 uvs;
	vec4 color;
}} ps;
void main() {{
	gl_Position = viewports[index] * vec4(position, 0.0f, 1.0f);
	ps.uvs = uvs;
	ps.color = color;
}})";

std::string pipeline_source::sprite_vertex_code() {
	return fmt::format(
		SOURCE_SPRITE_VERTEX,
		pipeline_source::directive(),
		pipeline_source::matrix_buffer()
	);
}

static constexpr char SOURCE_SPRITE_PIXEL[] = R"({}{}
in PS {{
	vec2 uvs;
	vec4 color;
}} ps;
layout(location = 0) out vec4 pixel;
void main() {{
	pixel = ps.color * texture(sampler_array, ps.uvs);
}})";

std::string pipeline_source::sprite_pixel_code() {
	return fmt::format(
		SOURCE_SPRITE_PIXEL,
		pipeline_source::directive(),
		pipeline_source::sampler_array()
	);
}

static constexpr char SOURCE_GLYPH_VERTEX[] = R"({}{}
layout(location = 0) in vec2 position;
layout(location = 1) in int index;
layout(location = 2) in vec2 uvs;
layout(location = 3) in vec4 color;
out PS {{
	flat int index;
	vec2 uvs;
	vec4 color;
}} ps;
void main() {{
	gl_Position = viewports[0] * vec4(position, 0.0f, 1.0f);
	ps.index = index;
	ps.uvs = uvs;
	ps.color = color;
}})";

std::string pipeline_source::glyph_vertex_code() {
	return fmt::format(
		SOURCE_GLYPH_VERTEX,
		pipeline_source::directive(),
		pipeline_source::matrix_buffer()
	);
}

static constexpr char SOURCE_GLYPH_PIXEL[] = R"({}{}
in PS {{
	flat int index;
	vec2 uvs;
	vec4 color;
}} ps;
layout(location = 0) out vec4 pixel;
void main() {{
	vec4 table = texture(sampler_array, ps.uvs);
	pixel = ps.color * (1.0f - table[ps.index]);
}})";

std::string pipeline_source::glyph_pixel_code() {
	return fmt::format(
		SOURCE_GLYPH_PIXEL,
		pipeline_source::directive(),
		pipeline_source::sampler_array()
	);
}

static constexpr char SOURCE_LIGHT_VERTEX[] = R"({}{}
layout(location = 0) in vec2 position;
void main() {{
	gl_Position = viewports[0] * vec4(position, 0.0f, 1.0f);
}})";

std::string pipeline_source::light_vertex_code() {
	return fmt::format(
		SOURCE_LIGHT_VERTEX,
		pipeline_source::directive(),
		pipeline_source::matrix_buffer()
	);
}

static constexpr char SOURCE_LIGHT_PIXEL[] = R"({}{}{}{}
layout(location = 0) out vec4 pixel;
void main() {{
	vec2 screen = gl_FragCoord.xy / scaling.xy;
	vec4 color = texture(frame_buffer, screen);
	pixel = color;
	for (uint i = 0U; i < count; ++i) {{
		vec4 position = viewports[1] * vec4(lights[i].position.xy, 0.0f, 1.0f) * 0.5f + 0.5f;
		vec3 direction = vec3(
			vec2(position.xy - screen) / vec2(lights[i].position.w / scaling.zw),
			lights[i].position.z
		);
		float distance = length(direction);
		float attenuation = 1.0f / (
			(lights[i].attenuation.x) +
			(distance * lights[i].attenuation.y) +
			(distance * distance * lights[i].attenuation.z)
		);
		vec3 lambert = lights[i].color.rgb * lights[i].color.a;
		pixel.rgb += (color.rgb * lambert * attenuation);
	}}
}})";

std::string pipeline_source::light_pixel_code() {
	return fmt::format(
		SOURCE_LIGHT_PIXEL,
		pipeline_source::directive(),
		pipeline_source::matrix_buffer(),
		pipeline_source::light_buffer(),
		pipeline_source::frame_buffer()
	);
}

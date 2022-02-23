#pragma once

#include <vector>
#include <string>
#include <apostellein/rect.hpp>

namespace tmx {
	class Map;
	class Layer;
	class TileLayer;
	class ImageLayer;
	class ObjectGroup;
	class Property;

	template<typename T>
	struct Vector2;
	using Vector2f = Vector2<float>;

	template<typename T>
	struct Rectangle;
	using FloatRect = Rectangle<float>;
}

namespace tmx_convert {
	bool prop_to_bool(const tmx::Property& property);
	udx prop_to_udx(const tmx::Property& property);
	u32 prop_to_uint(const tmx::Property& property);
	i32 prop_to_int(const tmx::Property& property);
	r32 prop_to_real(const tmx::Property& property);
	glm::vec2 vec2_to_vec2(const tmx::Vector2f& position);
	std::string prop_to_string(const tmx::Property& property);
	std::string prop_to_path(const tmx::Property& property);
	std::string path_to_name(const std::string& path);
	rect rect_to_rect(const tmx::FloatRect& rectangle);
	rect rect_to_rect(const tmx::Map& map);
}

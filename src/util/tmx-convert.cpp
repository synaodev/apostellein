#include "./tmx-convert.hpp"

#include <tmxlite/Map.hpp>
#include <apostellein/cast.hpp>

bool tmx_convert::prop_to_bool(const tmx::Property& property) {
	if (property.getType() == tmx::Property::Type::Boolean) {
		return property.getBoolValue();
	}
	return false;
}

udx tmx_convert::prop_to_udx(const tmx::Property& property) {
	if (property.getType() == tmx::Property::Type::Int) {
		return as<udx>(property.getIntValue());
	} else if (property.getType() == tmx::Property::Type::String) {
		if (auto& value = property.getStringValue(); !value.empty()) {
			if constexpr (sizeof(udx) == 8) {
				return std::stoull(value, nullptr, 0);
			} else {
				return std::stoul(value, nullptr, 0);
			}
		}
	}
	return 0;
}

u32 tmx_convert::prop_to_uint(const tmx::Property& property) {
	if (property.getType() == tmx::Property::Type::Int) {
		return as<u32>(property.getIntValue());
	} else if (property.getType() == tmx::Property::Type::String) {
		if (auto& value = property.getStringValue(); !value.empty()) {
			return as<u32>(std::stoul(value, nullptr, 0));
		}
	}
	return 0;
}

i32 tmx_convert::prop_to_int(const tmx::Property& property) {
	if (property.getType() == tmx::Property::Type::Int) {
		return property.getIntValue();
	} else if (property.getType() == tmx::Property::Type::String) {
		if (auto& value = property.getStringValue(); !value.empty()) {
			return std::stoi(value, nullptr, 0);
		}
	}
	return 0;
}

r32 tmx_convert::prop_to_real(const tmx::Property& property) {
	if (property.getType() == tmx::Property::Type::Float) {
		return property.getFloatValue();
	} else if (property.getType() == tmx::Property::Type::String) {
		if (auto& value = property.getStringValue(); !value.empty()) {
			return std::stof(value);
		}
	}
	return 0.0f;
}

glm::vec2 tmx_convert::vec2_to_vec2(const tmx::Vector2f& position) {
	return { position.x, position.y };
}

std::string tmx_convert::prop_to_string(const tmx::Property& property) {
	if (property.getType() == tmx::Property::Type::String) {
		return property.getStringValue();
	}
	return {};
}

std::string tmx_convert::prop_to_path(const tmx::Property& property) {
	if (property.getType() == tmx::Property::Type::File) {
		if (auto& value = property.getFileValue(); !value.empty()) {
			return tmx_convert::path_to_name(value);
		}
	}
	return {};
}

std::string tmx_convert::path_to_name(const std::string& path) {
	const auto sep = path.find_last_of('/');
	if (sep == std::string::npos) {
		return path;
	}
	const auto dot = path.find_last_of('.');
	if (dot == std::string::npos) {
		return path;
	}
	return path.substr(sep + 1, dot - (sep + 1));
}

rect tmx_convert::rect_to_rect(const tmx::FloatRect& rectangle) {
	return {
		rectangle.left,
		rectangle.top,
		rectangle.width,
		rectangle.height
	};
}

rect tmx_convert::rect_to_rect(const tmx::Map& map) {
	return rect_to_rect(map.getBounds());
}

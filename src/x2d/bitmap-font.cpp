#include <cstring>
#include <spdlog/spdlog.h>
#include <glm/common.hpp>
#include <nlohmann/json.hpp>
#include <apostellein/cast.hpp>

#include "./bitmap-font.hpp"
#include "../video/material.hpp"
#include "../hw/vfs.hpp"

namespace {
	const bitmap_glyph NULL_GLYPH {};

	constexpr char FONT_ENTRY[] = "font";
	constexpr char COMMON_ENTRY[] = "common";
	constexpr char CHARS_ENTRY[] = "chars";
	constexpr char CHAR_ENTRY[] = "char";
	constexpr char PAGES_ENTRY[] = "pages";
	constexpr char PAGE_ENTRY[] = "page";
	constexpr char BASE_ENTRY[] = "-base";
	constexpr char LINE_HEIGHT_ENTRY[] = "-lineHeight";
	constexpr char ID_ENTRY[] = "-id";
	constexpr char X_ENTRY[] = "-x";
	constexpr char Y_ENTRY[] = "-y";
	constexpr char WIDTH_ENTRY[] = "-width";
	constexpr char HEIGHT_ENTRY[] = "-height";
	constexpr char X_OFFSET_ENTRY[] = "-xoffset";
	constexpr char Y_OFFSET_ENTRY[] = "-yoffset";
	constexpr char X_ADVANCE_ENTRY[] = "-xadvance";
	constexpr char CHANNEL_ENTRY[] = "-chnl";
	constexpr char KERNINGS_ENTRY[] = "kernings";
	constexpr char KERNING_ENTRY[] = "kerning";
	constexpr char FIRST_ENTRY[] = "-first";
	constexpr char SECOND_ENTRY[] = "-second";
	constexpr char AMOUNT_ENTRY[] = "-amount";
	constexpr char FILE_ENTRY[] = "-file";
}

void bitmap_font::load(const std::string& route, const std::string& path) {
	auto parse_code = [](const auto& entry, const auto& key) {
		if (entry.contains(key)) {
			if (entry[key].is_string()) {
				const auto value = entry[key].template get<std::string>();
				const auto integer = as<u32>(std::stoul(value, nullptr, 0));
				return static_cast<char32_t>(integer);
			} else if (entry[key].is_number_unsigned()) {
				return static_cast<char32_t>(entry[key].template get<u32>());
			}
		}
		return U'\0';
	};
	auto parse_float = [](const auto& entry, const auto& key) {
		if (entry.contains(key)) {
			if (entry[key].is_string()) {
				return std::stof(entry[key].template get<std::string>());
			} else if (entry[key].is_number()) {
				return entry[key].template get<r32>();
			}
		}
		return 0.0f;
	};
	auto parse_channel = [](const auto& entry, const auto& key) {
		i32 encoded = 0;
		if (entry.contains(key)) {
			if (entry[key].is_string()) {
				encoded = std::stoi(entry[key].template get<std::string>());
			} else if (entry[key].is_number_integer()) {
				encoded = entry[key].template get<i32>();
			}
		}
		switch (encoded) {
			case 1: return 2; // b
			case 2: return 1; // g
			case 4: return 0; // r
			case 8: return 3; // a
			default: break;
		}
		return 0;
	};

	if (!glyphs_.empty()) {
		spdlog::warn("Tried to overwrite font!");
		return;
	}

	auto file = vfs::buffer_json(path);
	if (file.empty()) {
		spdlog::error("Failed to load font from \"{}\"!", path);
		return;
	}

	if (!file.contains(FONT_ENTRY)) {
		spdlog::error("Font data doesn't seem to exist in file from \"{}\"!", path);
		return;
	}

	auto& font = file[FONT_ENTRY];
	if (
		font.contains(COMMON_ENTRY) and
		font[COMMON_ENTRY].contains(BASE_ENTRY) and
		font[COMMON_ENTRY].contains(LINE_HEIGHT_ENTRY)
	) {
		auto& common = font[COMMON_ENTRY];
		dimensions_.x = parse_float(common, BASE_ENTRY);
		dimensions_.y = parse_float(common, LINE_HEIGHT_ENTRY);
	}
	if (dimensions_.x <= 0.0f or dimensions_.y <= 0.0f) {
		spdlog::warn("Invalid glyph dimensions in font from file \"{}\"!", path);
		return;
	}

	if (
		font.contains(CHARS_ENTRY) and
		font[CHARS_ENTRY].contains(CHAR_ENTRY) and
		font[CHARS_ENTRY][CHAR_ENTRY].is_array() and
		font[CHARS_ENTRY][CHAR_ENTRY].size() > 0
	) {
		for (auto&& entry : font[CHARS_ENTRY][CHAR_ENTRY]) {
			const char32_t code = parse_code(entry, ID_ENTRY);
			const bitmap_glyph glyph {
				parse_float(entry, X_ENTRY),
				parse_float(entry, Y_ENTRY),
				parse_float(entry, WIDTH_ENTRY),
				parse_float(entry, HEIGHT_ENTRY),
				parse_float(entry, X_OFFSET_ENTRY),
				parse_float(entry, Y_OFFSET_ENTRY),
				parse_float(entry, X_ADVANCE_ENTRY),
				parse_channel(entry, CHANNEL_ENTRY)
			};
			glyphs_[code] = glyph;
		}
	} else {
		spdlog::error("Glyph data missing in font from file \"{}\"!", path);
	}

	if (
		font.contains(KERNINGS_ENTRY) and
		font[KERNINGS_ENTRY].contains(KERNING_ENTRY) and
		font[KERNINGS_ENTRY][KERNING_ENTRY].is_array() and
		font[KERNINGS_ENTRY][KERNING_ENTRY].size() > 0
	) {
		for (auto&& entry : font[KERNINGS_ENTRY][KERNING_ENTRY]) {
			const auto key = std::make_pair(
				parse_code(entry, FIRST_ENTRY),
				parse_code(entry, SECOND_ENTRY)
			);
			kernings_[key] = parse_float(entry, AMOUNT_ENTRY);
		}
	}

	if (
		font.contains(PAGES_ENTRY) and
		font[PAGES_ENTRY].contains(PAGE_ENTRY) and
		font[PAGES_ENTRY][PAGE_ENTRY].contains(FILE_ENTRY) and
		font[PAGES_ENTRY][PAGE_ENTRY][FILE_ENTRY].is_string()
	) {
		const std::string name = font[PAGES_ENTRY][PAGE_ENTRY][FILE_ENTRY].get<std::string>();
		texture_ = vfs::find_material(name, route);
	} else {
		spdlog::error("Couldn't find texture name in font file \"{}\"!", path);
	}
}

void bitmap_font::destroy() {
	glyphs_.clear();
	kernings_.clear();
	dimensions_ = {};
	if (texture_) {
		vfs::clear_material(texture_);
		texture_ = nullptr;
	}
}

bool bitmap_font::valid() const {
	return (
		texture_ and
		texture_->valid()
	);
}

const bitmap_glyph& bitmap_font::glyph(char32_t code_point) const {
	auto iter = glyphs_.find(code_point);
	if (iter == glyphs_.end()) {
		return NULL_GLYPH;
	}
	return iter->second;
}

r32 bitmap_font::kerning(char32_t first, char32_t second) const {
	if (first == U'\0' or second == U'\0') {
		return 0.0f;
	}
	auto iter = kernings_.find(std::make_pair(first, second));
	if (iter == kernings_.end()) {
		return 0.0f;
	}
	return iter->second;
}

glm::vec2 bitmap_font::texture_dimensions() const {
	if (texture_) {
		return texture_->dimensions();
	}
	return {};
}

glm::vec2 bitmap_font::texture_offset() const {
	if (texture_) {
		return texture_->offset();
	}
	return {};
}

r32 bitmap_font::atlas() const {
	if (texture_) {
		return texture_->atlas();
	}
	return 0.0f;
}

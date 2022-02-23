#include <algorithm>
#include <array>
#include <fstream>
#include <memory>
#include <unordered_map>
#include <filesystem>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <fmt/chrono.h>
#include <SDL2/SDL_filesystem.h>
#include <SDL2/SDL_error.h>
#include <apostellein/konst.hpp>

#include "./vfs.hpp"
#include "../audio/noise-buffer.hpp"
#include "../video/material.hpp"
#include "../util/config-file.hpp"
#include "../util/message-box.hpp"
#include "../x2d/bitmap-font.hpp"
#include "../x2d/animation-group.hpp"

namespace vfs_route {
	constexpr char DATA[] = "data";
	constexpr char INIT[] = "init";
	constexpr char SAVE[] = "save";
	constexpr char LOG[] = "log";
	constexpr char ANIM[] = "anim";
	constexpr char EVENT[] = "event";
	constexpr char FIELD[] = "field";
	constexpr char FONT[] = "font";
	constexpr char I18N[] = "i18n";
	constexpr char IMAGE[] = "image";
	constexpr char KEY[] = "key";
	constexpr char NOISE[] = "noise";
	constexpr char TUNE[] = "tune";
}

namespace vfs_ext {
	constexpr char TMX[] = ".tmx";
	constexpr char JSON[] = ".json";
	constexpr char PNG[] = ".png";
	constexpr char WAV[] = ".wav";
	constexpr char ATTR[] = ".attr";
	constexpr char PTCOP[] = ".ptcop";
	constexpr char LUA[] = ".lua";
	constexpr char LOG[] = ".log";
}

namespace {
	constexpr char CONFIG_NAME[] = "config";
	constexpr char FONT_ENTRY[] = "Font";
	constexpr char EVENT_ENTRY[] = "Event";
	constexpr udx MAXIMUM_FONTS = 4;
}

// private
namespace vfs {
	// types
	using i18n_entry = std::vector<std::string>;
	// driver
	struct driver {
	public:
		config_file* config {};
		bool logging { false };
		std::filesystem::path root_directory {};
		std::filesystem::path personal_directory {};
		std::unordered_map<std::string, i18n_entry> i18n {};
		std::unordered_map<entt::id_type, noise_buffer> noises {};
		std::unordered_map<std::string, material> materials {};
		std::unordered_map<std::string, bitmap_font> fonts {};
		std::unordered_map<entt::id_type, animation_group> animations {};
	};
	std::unique_ptr<driver> drv_ {};

	// functions
	std::string application_name_() {
		std::string name = konst::APPLICATION;
		name[0] = std::tolower(name[0], std::locale{});
		return name;
	}

	std::filesystem::path working_directory_() {
		std::error_code code;
		const std::filesystem::path path = std::filesystem::current_path(code);
		if (code) {
			const std::string message = fmt::format(
				"Failed to find working directory! System Error: {}",
				code.message()
			);
			message_box::error(message);
			return {};
		}
		return path;
	}

	std::filesystem::path executable_directory_() {
		auto base = SDL_GetBasePath();
		if (!base) {
			const std::string message = fmt::format(
				"Failed to find executable directory! SDL Error: {}",
				SDL_GetError()
			);
			message_box::error(message);
			return {};
		}
		const std::filesystem::path result { base };
		SDL_free(base);
		return result;
	}

	std::filesystem::path personal_directory_() {
		const std::string name = vfs::application_name_();
		auto pref = SDL_GetPrefPath(konst::ORGANIZATION, name.c_str());
		if (!pref) {
			const std::string message = fmt::format(
				"Failed to find personal directory! SDL Error: {}",
				SDL_GetError()
			);
			message_box::error(message);
			return {};
		}
		const std::filesystem::path result { pref };
		SDL_free(pref);
		return result;
	}

	std::vector<std::string> list_normal_files_(const std::filesystem::path& directory) {
		std::vector<std::string> result {};
		for (auto&& file : std::filesystem::directory_iterator(directory)) {
			std::error_code code;
			if (file.is_regular_file(code)) {
				if (const auto& path = file.path(); path.has_extension()) {
					result.push_back(path.stem().string());
				}
			}
		}
		return result;
	}

	bool directory_exists_(const std::filesystem::path& path, bool alert) {
		std::error_code code;
		if (!std::filesystem::is_directory(path, code)) {
			if (alert) {
				spdlog::error(
					"\"{}\" isn't a valid directory! System Error: {}",
					path.string(),
					code.message()
				);
			}
			return false;
		}
		return true;
	}

	bool validate_mount_(const std::filesystem::path& root, bool alert) {
		if (!vfs::directory_exists_(root, alert)) {
			return false;
		}
		constexpr std::array routes {
			vfs_route::ANIM, vfs_route::EVENT,
			vfs_route::FIELD, vfs_route::FONT,
			vfs_route::I18N, vfs_route::IMAGE,
			vfs_route::NOISE, vfs_route::KEY,
			vfs_route::TUNE
		};
		const auto errors = std::count_if(
			routes.begin(), routes.end(),
			[&root](const auto& route) {
				std::error_code code;
				return !std::filesystem::is_directory(root / route, code);
			}
		);
		return errors == 0;
	}

	bool mount_(const std::filesystem::path& root) {
		if (!root.empty()) {
			if (vfs::validate_mount_(root, false)) {
				drv_->root_directory = root;
				return true;
			}
			const std::filesystem::path again = root / vfs_route::DATA;
			if (vfs::validate_mount_(again, false)) {
				drv_->root_directory = again;
				return true;
			} else {
				spdlog::warn(
					"Couldn\'t mount filesystem at \"{}\" or \"{}\"!",
					root.string(),
					again.string()
				);
			}
		}
		const std::filesystem::path working_root =
			vfs::working_directory_() /
			vfs_route::DATA;
		if (vfs::validate_mount_(working_root, false)) {
			drv_->root_directory = working_root;
			return true;
		}
		const std::filesystem::path executable_root =
			vfs::executable_directory_() /
			vfs_route::DATA;
		if (vfs::validate_mount_(executable_root, true)) {
			drv_->root_directory = executable_root;
			return true;
		}
		return false;
	}

	bool init_(const std::filesystem::path& root, config_file& cfg) {
		// Create driver
		if (drv_) {
			if (drv_->logging) {
				spdlog::critical("Virtual file system already has active driver!");
			}
			return false;
		}
		drv_ = std::make_unique<driver>();

		// Setup personal directory
		drv_->personal_directory = vfs::personal_directory_();
		if (drv_->personal_directory.empty()) {
			return false;
		}
		// Load config file
		{
			const std::string path = vfs::init_path(CONFIG_NAME);
			if (auto file = vfs::buffer_json(path); !cfg.load(file)) {
				cfg.create();
			}
			drv_->config = &cfg;
		}
		// Setup logging
		{
			std::vector<spdlog::sink_ptr> sinks {};
			if constexpr (konst::DEBUG) {
				sinks.push_back(std::make_shared<spdlog::sinks::stdout_color_sink_st>());
			}
			if (cfg.logging()) {
				sinks.push_back(std::make_shared<spdlog::sinks::rotating_file_sink_st>(
					vfs::log_path(konst::APPLICATION),
					konst::MAXIMUM_BYTES,
					konst::MAXIMUM_SINKS,
					true
				));
			}
			if (!sinks.empty()) {
				auto logger = std::make_shared<spdlog::logger>(
					konst::APPLICATION,
					sinks.begin(),
					sinks.end()
				);
				if (!logger) {
					message_box::error("Failed to initialize logger!");
					return false;
				}
				spdlog::set_default_logger(logger);
				spdlog::set_pattern(konst::PATTERN);
			} else {
				spdlog::set_level(spdlog::level::off);
			}
			drv_->logging = true;
		}
		// Setup root directory
		if (!vfs::mount_(root)) {
			message_box::error("Failed to mount virtual file system!");
			return false;
		}
		// Load language
		if (!vfs::try_language()) {
			message_box::error("Couldn't load language from config file!");
			return false;
		}
		return true;
	}

	void drop_() {
		if (drv_) {
			if (drv_->config) {
				const std::string path = vfs::init_path(CONFIG_NAME);
				if (std::ofstream ofs {
					vfs::init_path(CONFIG_NAME),
					std::ios::binary
				}; ofs.is_open()) {
					const std::string output = drv_->config->dump();
					ofs.write(output.data(), output.size());
				} else if (drv_->logging) {
					spdlog::error("Failed to write config to file: {}!", path);
				}
			}
			drv_.reset();
		}
	}

	guard::guard(const std::string& path) {
		std::filesystem::path root { path };
		if (root.is_relative()) {
			std::error_code code;
			root = std::filesystem::weakly_canonical(root, code);
			if (code) {
				root.clear();
			}
		}
		if (vfs::init_(root, config_)) {
			ready_ = true;
		} else {
			vfs::drop_();
		}
	}

	guard::~guard() {
		if (ready_) {
			vfs::drop_();
		}
	}
}

// public
bool vfs::try_language() {
	if (!drv_ or !drv_->config) {
		return false;
	}
	const std::string language = drv_->config->language();
	return vfs::try_language(language);
}

bool vfs::try_language(const std::string& name) {
	if (!drv_) {
		return false;
	}
	const std::filesystem::path path =
		drv_->root_directory /
		vfs_route::I18N /
		(name + vfs_ext::JSON);
	std::ifstream ifs { path, std::ios::binary };
	if (!ifs.is_open()) {
		spdlog::error("Couldn't load language file: {}", path.string());
		return false;
	}
	std::unordered_map<std::string, i18n_entry> result {};
	auto file = nlohmann::json::parse(ifs);
	for (auto iter = file.begin(); iter != file.end(); ++iter) {
		auto& entry = result[iter.key()];
		for (auto&& value : iter.value()) {
			if (value.is_string()) {
				entry.push_back(value.get<std::string>());
			}
		}
	}
	drv_->config->language(name);
	drv_->i18n = std::move(result);
	drv_->fonts.clear();
	return true;
}

bool vfs::directory_exists(const std::string& path) {
	bool alert = drv_ ?
		drv_->logging :
		false;
	return vfs::directory_exists_(path, alert);
}

bool vfs::create_directory(const std::string& path) {
	if (vfs::directory_exists_(path, false)) {
		return true;
	}
	std::error_code code;
	if (!std::filesystem::create_directory(path, code)) {
		if (drv_->logging) {
			spdlog::error(
				"Failed to create directory \"{}\"! System Error: {}",
				path,
				code.message()
			);
		}
		return false;
	}
	return true;
}

std::string vfs::init_path(const std::string& name) {
	if (!drv_) {
		return {};
	}
	const std::filesystem::path directory =
		drv_->personal_directory /
		vfs_route::INIT;
	if (const std::string dir = directory.string(); !vfs::create_directory(dir)) {
		return {};
	}
	const std::filesystem::path result =
		directory /
		(name + vfs_ext::JSON);
	return result.string();
}

std::string vfs::save_path(const std::string& name, udx profile) {
	const std::filesystem::path directory =
		drv_->personal_directory /
		vfs_route::SAVE;
	if (const std::string dir = directory.string(); !vfs::create_directory(dir)) {
		return {};
	}
	const std::filesystem::path result =
		directory /
		(name + std::to_string(profile) + vfs_ext::JSON);
	return result.string();
}

std::string vfs::log_path(const std::string& name) {
	if (!drv_) {
		return {};
	}
	static const std::time_t time = std::time(nullptr);
	const std::string file = fmt::format(
		"{}.{:%Y-%m-%d_%H-%M-%S}{}", name,
		fmt::localtime(time),
		vfs_ext::LOG
	);
	const std::filesystem::path result =
		drv_->personal_directory /
		vfs_route::LOG /
		file;
	return result.string();
}

std::string vfs::tune_path(const std::string& name) {
	if (!drv_) {
		return {};
	}
	const std::filesystem::path result =
		drv_->root_directory /
		vfs_route::TUNE /
		(name + vfs_ext::PTCOP);
	return result.string();
}

std::string vfs::global_script_path(const std::string& name) {
	if (!drv_) {
		return {};
	}
	const std::filesystem::path result =
		drv_->root_directory /
		vfs_route::EVENT /
		(name + vfs_ext::LUA);
	return result.string();
}

std::string vfs::local_script_path(const std::string& name) {
	if (!drv_) {
		return {};
	}
	const std::filesystem::path result =
		drv_->root_directory /
		vfs_route::EVENT /
		vfs::i18n_at(EVENT_ENTRY, 0) /
		(name + vfs_ext::LUA);
	return result.string();
}

std::string vfs::field_path(const std::string& name) {
	if (!drv_) {
		return {};
	}
	const std::filesystem::path result =
		drv_->root_directory /
		vfs_route::FIELD /
		(name + vfs_ext::TMX);
	return result.string();
}

std::string vfs::key_path(const std::string& name) {
	if (!drv_) {
		return {};
	}
	const std::filesystem::path result =
		drv_->root_directory /
		vfs_route::KEY /
		(name + vfs_ext::ATTR);
	return result.string();
}

std::string vfs::image_path(const std::string& name) {
	if (!drv_) {
		return {};
	}
	const std::filesystem::path result =
		drv_->root_directory /
		vfs_route::IMAGE /
		(name + vfs_ext::PNG);
	return result.string();
}

std::vector<std::string> vfs::list_languages() {
	if (!drv_) {
		return {};
	}
	const std::filesystem::path directory =
		drv_->root_directory /
		vfs_route::I18N;
	std::vector<std::string> languages = vfs::list_normal_files_(directory);
	const std::locale locale {};
	for (auto&& lang : languages) {
		lang[0] = std::toupper(lang[0], locale);
	}
	std::sort(languages.begin(), languages.end());
	return languages;
}

std::vector<std::string> vfs::list_fields() {
	if (!drv_) {
		return {};
	}
	return vfs::list_normal_files_(drv_->root_directory / vfs_route::FIELD);
}

std::vector<std::string> vfs::list_keys() {
	if (!drv_) {
		return {};
	}
	return vfs::list_normal_files_(drv_->root_directory / vfs_route::KEY);
}

std::string vfs::buffer_string(const std::string& path) {
	std::ifstream ifs { path, std::ios::binary };
	if (ifs.is_open()) {
		ifs.seekg(0, std::ios::end);
		const auto length = static_cast<udx>(ifs.tellg());
		if (length > 0) {
			ifs.seekg(0, std::ios::beg);
			std::string buffer {};
			buffer.resize(length);
			ifs.read(buffer.data(), length);
			return buffer;
		}
	}
	spdlog::warn("Failed to open file: {}!", path);
	return {};
}

std::vector<char> vfs::buffer_chars(const std::string& path) {
	std::ifstream ifs { path, std::ios::binary };
	if (ifs.is_open()) {
		ifs.seekg(0, std::ios::end);
		const auto length = static_cast<udx>(ifs.tellg());
		if (length > 0) {
			ifs.seekg(0, std::ios::beg);
			std::vector<char> buffer {};
			buffer.resize(length);
			ifs.read(buffer.data(), length);
			return buffer;
		}
	}
	spdlog::warn("Failed to open file: {}!", path);
	return {};
}

std::vector<byte> vfs::buffer_bytes(const std::string& path) {
	std::ifstream ifs { path, std::ios::binary };
	if (ifs.is_open()) {
		ifs.seekg(0, std::ios::end);
		const auto length = static_cast<udx>(ifs.tellg());
		if (length > 0) {
			ifs.seekg(0, std::ios::beg);
			std::vector<byte> buffer {};
			buffer.resize(length);
			ifs.read(reinterpret_cast<char*>(buffer.data()), length);
			return buffer;
		}
	}
	spdlog::warn("Failed to open file: {}!", path);
	return {};
}

std::vector<u32> vfs::buffer_uints(const std::string& path) {
	std::ifstream ifs { path, std::ios::binary };
	if (ifs.is_open()) {
		ifs.seekg(0, std::ios::end);
		const auto length = static_cast<udx>(ifs.tellg());
		if (length > 0) {
			ifs.seekg(0, std::ios::beg);
			std::vector<u32> buffer {};
			buffer.resize(length / sizeof(u32));
			ifs.read(reinterpret_cast<char*>(buffer.data()), length);
			return buffer;
		}
	}
	spdlog::warn("Failed to open file: {}!", path);
	return {};
}

image_file vfs::buffer_image(const std::string& path) {
	const std::vector<byte> buffer = vfs::buffer_bytes(path);
	if (buffer.empty()) {
		return {};
	}
	image_file result {};
	if (!result.load(buffer)) {
		spdlog::error("Failed to load image from {}!", path);
	}
	return result;
}

nlohmann::json vfs::buffer_json(const std::string& path) {
	std::ifstream ifs { path, std::ios::binary };
	if (!ifs.is_open()) {
		if (drv_->logging) {
			spdlog::error("Failed to open json file: {}!", path);
		}
		return {};
	}
	auto result = nlohmann::json::parse(
		ifs, // stream
		nullptr, // parser callback
		false, // no exceptions
		true // ignore comments
	);
	return result;
}

bool vfs::dump_json(const nlohmann::json& file, const std::string& path) {
	std::ofstream ofs { path, std::ios::binary };
	if (!ofs.is_open()) {
		spdlog::error("Failed to dump JSON file: {}!", path);
		return false;
	}
	const std::string output = file.dump(
		1, // indent
		'\t', // indent char
		true, // ensure ascii
		nlohmann::detail::error_handler_t::ignore // ignore UTF-8 errors
	);
	ofs.write(output.data(), output.size());
	return true;
}

std::string vfs::i18n_from(const std::string& segment, udx first, udx last) {
	if (!drv_) {
		return {};
	}
	auto iter = drv_->i18n.find(segment);
	if (iter == drv_->i18n.end()) {
		return {};
	}
	const auto length = [&first, &last](const i18n_entry& entry) {
		udx result = 0;
		if (first < entry.size() and last < entry.size()) {
			for (udx idx = first; idx <= last; ++idx) {
				result += entry[idx].size();
			}
		}
		return result;
	}(iter->second);
	if (length == 0) {
		return {};
	}

	std::string result {};
	result.reserve(length);
	for (udx idx = first; idx <= last; ++idx) {
		result += iter->second[idx];
	}
	return result;
}

std::string vfs::i18n_at(const std::string& segment, udx index) {
	if (!drv_) {
		return {};
	}
	auto iter = drv_->i18n.find(segment);
	if (iter == drv_->i18n.end()) {
		spdlog::error("I18N segment \"{}\" doesn't exist!", segment);
		return {};
	}
	if (index >= iter->second.size()) {
		spdlog::error("I18N segment \"\" doesn't have data at index {}!", index);
		return {};
	}
	return iter->second[index];
}

udx vfs::i18n_size(const std::string& segment) {
	if (!drv_) {
		return 0;
	}
	auto iter = drv_->i18n.find(segment);
	if (iter == drv_->i18n.end()) {
		spdlog::error("I18N segment \"{}\" doesn't exist!", segment);
		return 0;
	}
	return iter->second.size();
}

void vfs::clear_noises() {
	if (drv_) {
		drv_->noises.clear();
	}
}

void vfs::clear_materials() {
	if (drv_) {
		drv_->materials.clear();
	}
}

void vfs::clear_material(const material* handle) {
	if (!drv_) {
		return;
	}
	auto iter = drv_->materials.begin();
	auto end = drv_->materials.end();
	while (iter != end) {
		auto& [_, ref] = *iter;
		if (handle == &ref) {
			break;
		}
		++iter;
	}
	if (iter != end) {
		drv_->materials.erase(iter);
	}
}

void vfs::clear_fonts() {
	if (drv_) {
		drv_->fonts.clear();
	}
}

void vfs::clear_animations() {
	if (drv_) {
		drv_->animations.clear();
	}
}

const noise_buffer* vfs::find_noise(const entt::hashed_string& entry) {
	if (!drv_) {
		return nullptr;
	}
	auto iter = drv_->noises.find(entry.value());
	if (iter == drv_->noises.end()) {
		auto& ref = drv_->noises[entry.value()];
		const std::string name { entry.data() };
		const std::filesystem::path path =
			drv_->root_directory /
			vfs_route::NOISE /
			(name + vfs_ext::WAV);
		ref.load(path.string());
		return &ref;
	}
	return std::addressof(iter->second);
}

const noise_buffer* vfs::find_noise(const std::string& name) {
	if (!drv_) {
		return nullptr;
	}
	const entt::hashed_string entry { name.c_str() };
	auto iter = drv_->noises.find(entry.value());
	if (iter == drv_->noises.end()) {
		auto& ref = drv_->noises[entry.value()];
		const std::filesystem::path path =
			drv_->root_directory /
			vfs_route::NOISE /
			(name + vfs_ext::WAV);
		ref.load(path.string());
		return &ref;
	}
	return std::addressof(iter->second);
}

const material* vfs::find_material(const std::string& name) {
	if (!drv_) {
		return nullptr;
	}
	auto iter = drv_->materials.find(name);
	if (iter == drv_->materials.end()) {
		auto& ref = drv_->materials[name];
		const std::filesystem::path path =
			drv_->root_directory /
			vfs_route::IMAGE /
			(name + vfs_ext::PNG);
		auto image = vfs::buffer_image(path.string());
		ref.load(std::move(image));
		return &ref;
	}
	return std::addressof(iter->second);
}

const material* vfs::find_material(const std::string& name, const std::string& route) {
	if (!drv_) {
		return nullptr;
	}
	auto iter = drv_->materials.find(name);
	if (iter == drv_->materials.end()) {
		auto& ref = drv_->materials[name];
		const std::filesystem::path path =
			drv_->root_directory /
			route /
			(name + vfs_ext::PNG);
		auto image = vfs::buffer_image(path.string());
		ref.load(std::move(image));
		return &ref;
	}
	return std::addressof(iter->second);
}

const bitmap_font* vfs::find_font(const std::string& name) {
	if (!drv_) {
		return nullptr;
	}
	auto iter = drv_->fonts.find(name);
	if (iter == drv_->fonts.end()) {
		auto& ref = drv_->fonts[name];
		const std::filesystem::path path =
			drv_->root_directory /
			vfs_route::FONT /
			(name + vfs_ext::JSON);
		ref.load(vfs_route::FONT, path.string());
		return &ref;
	}
	return std::addressof(iter->second);
}

const bitmap_font* vfs::find_font(udx index) {
	if (!drv_) {
		return nullptr;
	}
	if (index >= MAXIMUM_FONTS) {
		spdlog::error("Font index cannot be higher than {}!", MAXIMUM_FONTS);
		return nullptr;
	}
	auto iter = drv_->i18n.find(FONT_ENTRY);
	if (iter == drv_->i18n.end()) {
		spdlog::critical(
			"Couldn't find \"Fonts\" section in {}'s language data!",
			drv_->config->language()
		);
		return nullptr;
	}
	auto& entry = drv_->i18n[FONT_ENTRY];
	if (index >= entry.size()) {
		spdlog::error(
			"Couldn't find font #{} in {}'s language data!",
			index,
			drv_->config->language()
		);
		return nullptr;
	}
	return vfs::find_font(entry[index]);
}

const animation_group* vfs::find_animation(const entt::hashed_string& entry) {
	if (!drv_) {
		return nullptr;
	}
	auto iter = drv_->animations.find(entry.value());
	if (iter == drv_->animations.end()) {
		auto& ref = drv_->animations[entry.value()];
		const std::string name { entry.data() };
		const std::filesystem::path path =
			drv_->root_directory /
			vfs_route::ANIM /
			(name + vfs_ext::JSON);
		ref.load(path.string());
		return &ref;
	}
	return std::addressof(iter->second);
}

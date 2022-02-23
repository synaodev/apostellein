#pragma once

#include <vector>
#include <string>
#include <entt/core/hashed_string.hpp>

#include "../util/config-file.hpp"
#include "../util/image-file.hpp"

struct noise_buffer;
struct material;
struct bitmap_font;
struct animation_group;

namespace vfs {
	// static functions
	bool try_language();
	bool try_language(const std::string& name);
	bool directory_exists(const std::string& path);
	bool create_directory(const std::string& path);
	std::string init_path(const std::string& name);
	std::string save_path(const std::string& name, udx profile);
	std::string log_path(const std::string& name);
	std::string tune_path(const std::string& name);
	std::string global_script_path(const std::string& name);
	std::string local_script_path(const std::string& name);
	std::string field_path(const std::string& name);
	std::string key_path(const std::string& name);
	std::string image_path(const std::string& name);
	std::vector<std::string> list_languages();
	std::vector<std::string> list_fields();
	std::vector<std::string> list_keys();
	std::string buffer_string(const std::string& path);
	std::vector<char> buffer_chars(const std::string& path);
	std::vector<byte> buffer_bytes(const std::string& path);
	std::vector<u32> buffer_uints(const std::string& path);
	image_file buffer_image(const std::string& path);
	nlohmann::json buffer_json(const std::string& path);
	bool dump_json(const nlohmann::json& file, const std::string& path);
	void clear_noises();
	void clear_materials();
	void clear_material(const material* handle);
	void clear_fonts();
	void clear_animations();
	std::string i18n_from(const std::string& segment, udx first, udx last);
	std::string i18n_at(const std::string& segment, udx index);
	udx i18n_size(const std::string& segment);
	const noise_buffer* find_noise(const entt::hashed_string& entry);
	const noise_buffer* find_noise(const std::string& name);
	const material* find_material(const std::string& name);
	const material* find_material(const std::string& name, const std::string& route);
	const bitmap_font* find_font(const std::string& name);
	const bitmap_font* find_font(udx index);
	const animation_group* find_animation(const entt::hashed_string& entry);
	// Init-Guard
	struct guard : public not_moveable {
		guard(const std::string& path);
		~guard();
	public:
		operator bool() const { return ready_ and config_.valid(); }
		config_file& config() { return config_; }
	private:
		bool ready_ {};
		config_file config_ {};
	};
}

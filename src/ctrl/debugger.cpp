#if defined(APOSTELLEIN_IMGUI_DEBUGGER)

#include <SDL2/SDL.h>
#include <spdlog/spdlog.h>
#include <imgui/imgui.h>
#include <imgui/imgui_impl_opengl3.h>
#include <imgui/imgui_impl_sdl.h>
#include <apostellein/konst.hpp>
#include <apostellein/cast.hpp>

#include "./debugger.hpp"
#include "./runtime.hpp"
#include "../ecs/aktor.hpp"
#include "../ecs/kinematics.hpp"
#include "../hw/video.hpp"
#include "../hw/input.hpp"
#include "../hw/vfs.hpp"
#include "../util/buttons.hpp"
#include "../util/config-file.hpp"
#include "../video/opengl.hpp"
#include "../x2d/pipeline-source.hpp"
#include "../x2d/renderer.hpp"

namespace {
	const ImVec2 MAIN_POSITION() { return { 20.0f, 20.0f }; }
	const ImVec2 MAIN_DIMENSIONS() { return { 400.0f, 300.0f }; }
	const ImVec2 FIELD_POSITION() { return { 80.0f, 80.0f }; }
	const ImVec2 FIELD_DIMENSIONS() { return { 240.0f, 280.0f }; }
	const ImVec2 EVENT_POSITION() { return { 120.0f, 80.0f }; }
	const ImVec2 EVENT_DIMENSIONS() { return { 240.0f, 280.0f }; }
	const ImVec2 FLAGS_POSITION() { return { 100.0f, 80.0f }; }
	const ImVec2 FLAGS_DIMENSIONS() { return { 240.0f, 280.0f }; }

	constexpr i32 DEFAULT_LIST_LENGTH = 10;
	constexpr i64 DEFAULT_FRAME_DELAY = konst::SECONDS_TO_NANOSECONDS(0.1);
	constexpr i64 DEFAULT_FADING_TIME = konst::SECONDS_TO_NANOSECONDS(0.5);
	constexpr r32 DEFAULT_INTERPOLATION = 0.5f;
}

debugger::~debugger() {
	if (window_) {
		ImGui_ImplOpenGL3_Shutdown();
		ImGui_ImplSDL2_Shutdown();
		ImGui::DestroyContext();
	}
}

bool debugger::build(const config_file& cfg, const renderer& rdr) {
	if (cfg.debugger()) {
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGui::StyleColorsDark();
		auto [window, context] = video::pointers();
		if (!ImGui_ImplSDL2_InitForOpenGL(
			reinterpret_cast<SDL_Window*>(window),
			reinterpret_cast<SDL_GLContext>(context)
		)) {
			spdlog::critical("Failed to initialize ImGui SDL2!");
			ImGui::DestroyContext();
			return false;
		}
		const std::string directive = pipeline_source::directive();
		if (!ImGui_ImplOpenGL3_Init(directive.c_str())) {
			spdlog::critical("Failed to initialize ImGui OpenGL 3! GLSL Version: \"{}\"", directive);
			ImGui_ImplSDL2_Shutdown();
			ImGui::DestroyContext();
			return false;
		}
		input::callback([](const void* ptr) {
			return ImGui_ImplSDL2_ProcessEvent(reinterpret_cast<const SDL_Event*>(ptr));
		});
		window_ = window;
		rdr_ = &rdr;
	}
	return true;
}

void debugger::handle(const buttons& bts, runtime& state) {
	if (window_ and rdr_) {
		if (bts.pressed.debugger) {
			visible_ = !visible_;
		}
		if (visible_) {
			// init frame
			ImGui_ImplOpenGL3_NewFrame();
			ImGui_ImplSDL2_NewFrame(reinterpret_cast<SDL_Window*>(window_));
			ImGui::NewFrame();
			// render ui
			this->ui_(state);
			// draw frame
			ImGui::Render();
		}
	}
}

void debugger::update(i64 delta) {
	if (visible_) {
		timer_ += delta;
		if (fading_ > 0) {
			if (fading_ -= delta; fading_ <= 0) {
				fading_ = 0;
			}
		}
		++frames_;
	}
}

void debugger::flush() const {
	if (visible_) {
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
	}
}

void debugger::ui_(runtime& state) {
	static bool fields_visible = false;
	static bool events_visible = false;
	static bool flags_visible = false;
	static bool aktors_visible = false;

	// debugger
	ImGui::SetNextWindowCollapsed(false, ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowPos(MAIN_POSITION(), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize(MAIN_DIMENSIONS(), ImGuiCond_FirstUseEver);
	if (ImGui::Begin("Debugger", nullptr, ImGuiWindowFlags_NoResize)) {
		static r64 fps = 0.0;
		if (timer_ >= DEFAULT_FRAME_DELAY) {
			fps = konst::FRAMES_PER_SECOND<r64>(timer_, frames_);
			timer_ = 0;
			frames_ = 0;
		}
		{
			const std::string text = fmt::format(
				"FPS: {:.2f}",
				fps
			);
			ImGui::TextUnformatted(text.c_str());
		}
		{
			const std::string text = fmt::format(
				"Lua Memory: {} KB",
				state.knl_.kilobytes()
			);
			ImGui::TextUnformatted(text.c_str());
		}
		{
			const std::string text = fmt::format(
				"Available Video Memory: {} KB",
				ogl::memory_available()
			);
			ImGui::TextUnformatted(text.c_str());
		}
		{
			const std::string text = fmt::format(
				"Aktors: {}",
				state.env_.alive()
			);
			ImGui::TextUnformatted(text.c_str());
		}
		{
			const std::string text = fmt::format(
				"Display Lists: {}/{}",
				rdr_->visible_lists(),
				rdr_->all_lists()
			);
			ImGui::TextUnformatted(text.c_str());
		}
		{
			const rect view = state.cam_.view(DEFAULT_INTERPOLATION);
			const std::string text = fmt::format(
				"Visible Sprites: {}",
				state.env_.visible(DEFAULT_INTERPOLATION, view)
			);
			ImGui::TextUnformatted(text.c_str());
		}

		// buttons
		ImGui::Separator();
		if (ImGui::Button("Fields")) {
			fields_visible = !fields_visible;
		}
		ImGui::SameLine();
		if (ImGui::Button("Events")) {
			events_visible = !events_visible;
		}
		ImGui::SameLine();
		if (ImGui::Button("Flags")) {
			flags_visible = !flags_visible;
		}
		ImGui::SameLine();
		if (ImGui::Button("Aktors")) {
			aktors_visible = !aktors_visible;
		}
		if (state.ctl_.state().frozen) {
			fields_visible = false;
			events_visible = false;
			flags_visible = false;
			aktors_visible = false;
		}
	}
	ImGui::End();

	// fields
	if (fields_visible) {
		ImGui::SetNextWindowCollapsed(false, ImGuiCond_Appearing);
		ImGui::SetNextWindowPos(FIELD_POSITION(), ImGuiCond_Appearing);
		ImGui::SetNextWindowSize(FIELD_DIMENSIONS(), ImGuiCond_Appearing);
		if (ImGui::Begin("Fields")) {
			static std::vector<std::string> fields = vfs::list_fields();
			static i32 size = as<i32>(fields.size());
			static i32 selected = 0;

			ImGui::PushItemWidth(-1.0f);
			ImGui::ListBox("", &selected,
				[](void* data, i32 index, const char** output) {
					if (!data or !output) {
						return false;
					}
					auto& list = *reinterpret_cast<std::vector<std::string>*>(data);
					*output = list.at(index).c_str();
					return true;
				},
				&fields, size,
				DEFAULT_LIST_LENGTH
			);
			if (selected < size) {
				static i32 id = 0;

				ImGui::TextUnformatted("Aktor ID");
				ImGui::InputInt("Aktor ID", &id);
				if (id <= 0) {
					ImGui::BeginDisabled();
					ImGui::Button("Transfer");
					ImGui::EndDisabled();
				} else if (ImGui::Button("Transfer")) {
					state.ctl_.freeze();
					state.hud_.fade_out();
					state.ctl_.transfer(fields[as<udx>(selected)], id);
				}
			}
		}
		ImGui::End();
	}

	// events
	if (events_visible) {
		ImGui::SetNextWindowCollapsed(false, ImGuiCond_Appearing);
		ImGui::SetNextWindowPos(EVENT_POSITION(), ImGuiCond_Appearing);
		ImGui::SetNextWindowSize(EVENT_DIMENSIONS(), ImGuiCond_Appearing);
		if (ImGui::Begin("Events")) {
			static std::vector<std::string> symbols {};
			static i32 size = 0;
			static i32 selected = 0;

			if (state.knl_.changed()) {
				symbols = state.knl_.symbols();
				size = as<i32>(symbols.size());
				selected = 0;
			}

			if (state.knl_.running()) {
				ImGui::BeginDisabled();
				ImGui::Button("Reload");
				ImGui::EndDisabled();
			} else if (ImGui::Button("Reload")) {
				state.knl_.reload();
				fading_ = DEFAULT_FADING_TIME;
			}
			if (fading_ > 0) {
				ImGui::SameLine();
				ImGui::TextUnformatted("Success!");
			}

			ImGui::Separator();
			ImGui::PushItemWidth(-1.0f);
			ImGui::ListBox("", &selected,
				[](void* data, i32 index, const char** output) {
					if (!data or !output) {
						return false;
					}
					auto& list = *reinterpret_cast<std::vector<std::string>*>(data);
					*output = list.at(index).c_str();
					return true;
				},
				&symbols, size,
				DEFAULT_LIST_LENGTH
			);
			if (selected >= size or state.knl_.running()) {
				ImGui::BeginDisabled();
				ImGui::Button("Execute");
				ImGui::EndDisabled();
			} else if (ImGui::Button("Execute")) {
				state.knl_.run_symbol(symbols[as<udx>(selected)]);
			}
		}
		ImGui::End();
	}

	// flags
	if (flags_visible) {
		ImGui::SetNextWindowCollapsed(false, ImGuiCond_Appearing);
		ImGui::SetNextWindowPos(FLAGS_POSITION(), ImGuiCond_Appearing);
		ImGui::SetNextWindowSize(FLAGS_DIMENSIONS(), ImGuiCond_Appearing);
		if (ImGui::Button("Flags")) {

		}
		ImGui::End();
	}
}

#endif

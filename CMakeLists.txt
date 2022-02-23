cmake_minimum_required (VERSION 3.15)

# Check/alter compiler options
set (CMAKE_USER_MAKE_RULES_OVERRIDE "${CMAKE_CURRENT_LIST_DIR}/cmake/MakeRules.cmake")

# Linux specific options
if (UNIX AND NOT APPLE)
	set (APOSTELLEIN_CCACHE ON CACHE BOOL "Use ccache?")
	set (APOSTELLEIN_GOLD ON CACHE BOOL "Use gold linker?")
	if (APOSTELLEIN_CCACHE)
		find_program (CCACHE_PROGRAM "ccache")
		if (CCACHE_PROGRAM)
			set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ${CCACHE_PROGRAM})
		endif ()
	endif ()
else ()
	if (APOSTELLEIN_CCACHE)
		set (APOSTELLEIN_CCACHE OFF)
		message ("Only able to use ccache on Linux. Sorry!")
	endif ()
	if (APOSTELLEIN_GOLD)
		set (APOSTELLEIN_GOLD OFF)
		message ("Only able to use gold linker on Linux. Sorry!")
	endif ()
endif ()

# C++20 or C++17 (the default)
set (APOSTELLEIN_STANDARD_2020 OFF CACHE BOOL "Use C++20 instead of C++17?")

# Precompiled headers
set (APOSTELLEIN_PRECOMPILED_HEADERS OFF CACHE BOOL "Use precompiled headers?")

# Esoteric warnings
if (NOT MSVC)
	set (APOSTELLEIN_ESOTERIC_WARNINGS OFF CACHE BOOL "Esoteric warnings?")
elseif (APOSTELLEIN_ESOTERIC_WARNINGS)
	set (APOSTELLEIN_ESOTERIC_WARNINGS OFF)
	message ("Only able to enable esoteric warnings on gcc and clang. Sorry!")
endif ()

# Clang sanitizer
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
	set (APOSTELLEIN_SANITIZER OFF CACHE BOOL "Use clang's sanitizer for undefined behavior?")
	if (APOSTELLEIN_GOLD)
		message (FATAL_ERROR "Cannot use gold linker while also enabling clang's sanitizer!")
	endif ()
elseif (APOSTELLEIN_SANITIZER)
	set (APOSTELLEIN_SANITIZER OFF)
	message ("Only able to enable sanitizer on clang. Sorry!")
endif ()

# PVS Studio static analysis
set (APOSTELLEIN_PVS_STUDIO OFF CACHE BOOL "Generate static analysis?")
set (APOSTELLEIN_DIAGNOSTIC_MODE "GA:1,2" CACHE STRING "What kind of diagnostics?")

# Debug/release options
if ("${CMAKE_BUILD_TYPE}" STREQUAL "Debug" OR "${CMAKE_BUILD_TYPE}" STREQUAL "RelWithDebInfo")
	if (NOT DEFINED APOSTELLEIN_OPENGL_LOGGING)
		message ("Defaulting to logging OpenGL errors on debug builds...")
	endif ()
	if (NOT DEFINED APOSTELLEIN_OPENAL_LOGGING)
		message ("Defaulting to logging OpenAL errors on debug builds...")
	endif ()
	set (APOSTELLEIN_OPENGL_LOGGING ON CACHE BOOL "Log OpenGL errors?")
	set (APOSTELLEIN_OPENAL_LOGGING ON CACHE BOOL "Log OpenAL errors?")
else ()
	set (APOSTELLEIN_OPENGL_LOGGING OFF CACHE BOOL "Log OpenGL errors?")
	set (APOSTELLEIN_OPENAL_LOGGING OFF CACHE BOOL "Log OpenAL errors?")
endif ()

# ImGui debugger
set (APOSTELLEIN_IMGUI_DEBUGGER ON CACHE BOOL "ImGui debugger?")

# Fast math
set (APOSTELLEIN_FAST_MATH OFF CACHE BOOL "Fast math?")

# Unsafe lua
set (APOSTELLEIN_UNSAFE_LUA OFF CACHE BOOL "Unsafe lua?")

# Project definition
project (apostellein)

# Basic settings
set (APOSTELLEIN_VERSION_MAJOR 0)
set (APOSTELLEIN_VERSION_MINOR 0)
set (APOSTELLEIN_VERSION_PATCH 0)
set (APOSTELLEIN_VERSION_TWEAK 0)

if (APOSTELLEIN_STANDARD_2020)
	set (CMAKE_CXX_STANDARD 20)
else ()
	set (CMAKE_CXX_STANDARD 17)
endif ()

set (CMAKE_C_STANDARD 11)
set (CMAKE_C_STANDARD_REQUIRED ON)
set (CMAKE_C_EXTENSIONS OFF)
set (CMAKE_CXX_STANDARD_REQUIRED ON)
set (CMAKE_CXX_EXTENSIONS OFF)
set (CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Target
add_executable (apostellein)

# Configure
include ("${CMAKE_CURRENT_LIST_DIR}/config.cmake")

# Sources
if (WIN32)
	target_sources (apostellein PRIVATE
		"res/win32/resource.h"
		"res/win32/apostellein.rc"
	)
endif ()

if (APOSTELLEIN_PRECOMPILED_HEADERS)
	target_precompile_headers (apostellein PRIVATE "src/stdafx.hpp")
endif ()

target_sources (apostellein PRIVATE
	"src/init.cpp"
	"src/ai/common.cpp"
	"src/ai/friends.cpp"
	"src/ai/particles.cpp"
	"src/ai/weapons.cpp"
	"src/audio/noise-buffer.cpp"
	"src/audio/openal.cpp"
	"src/audio/speaker.cpp"
	"src/ctrl/controller.cpp"
	"src/ctrl/debugger.cpp"
	"src/ctrl/kernel.cpp"
	"src/ctrl/runtime.cpp"
	"src/ecs/aktor.cpp"
	"src/ecs/collision.cpp"
	"src/ecs/health.cpp"
	"src/ecs/kinematics.cpp"
	"src/ecs/liquid.cpp"
	"src/ecs/sprite.cpp"
	"src/ecs/thinker.cpp"
	"src/field/camera.cpp"
	"src/field/environment.cpp"
	"src/field/player.cpp"
	"src/gui/barrier.cpp"
	"src/gui/counter.cpp"
	"src/gui/element.cpp"
	"src/gui/fader.cpp"
	"src/gui/graphic.cpp"
	"src/gui/meter.cpp"
	"src/gui/provision.cpp"
	"src/gui/scheme.cpp"
	"src/gui/text.cpp"
	"src/gui/title.cpp"
	"src/hw/audio.cpp"
	"src/hw/input.cpp"
	"src/hw/music.cpp"
	"src/hw/rng.cpp"
	"src/hw/vfs.cpp"
	"src/hw/video.cpp"
	"src/menu/dialogue.cpp"
	"src/menu/headsup.cpp"
	"src/menu/inventory.cpp"
	"src/menu/overlay.cpp"
	"src/menu/widget-detail.cpp"
	"src/util/config-file.cpp"
	"src/util/image-file.cpp"
	"src/util/message-box.cpp"
	"src/util/tmx-convert.cpp"
	"src/video/const-buffer.cpp"
	"src/video/frame-buffer.cpp"
	"src/video/index-buffer.cpp"
	"src/video/light-buffer.cpp"
	"src/video/material.cpp"
	"src/video/matrix-buffer.cpp"
	"src/video/opengl.cpp"
	"src/video/quad-buffer.cpp"
	"src/video/shader.cpp"
	"src/video/swap-chain.cpp"
	"src/video/vertex.cpp"
	"src/x2d/animation-group.cpp"
	"src/x2d/bitmap-font.cpp"
	"src/x2d/display-list.cpp"
	"src/x2d/pipeline-source.cpp"
	"src/x2d/renderer.cpp"
	"src/x2d/tile-map.cpp"

	"lib/src/glad/gl.cpp"
	"lib/src/imgui/imgui_draw.cpp"
	"lib/src/imgui/imgui_impl_opengl3.cpp"
	"lib/src/imgui/imgui_impl_sdl.cpp"
	"lib/src/imgui/imgui_tables.cpp"
	"lib/src/imgui/imgui_widgets.cpp"
	"lib/src/imgui/imgui.cpp"
	"lib/src/pxtone/pxtnDelay.cpp"
	"lib/src/pxtone/pxtnDescriptor.cpp"
	"lib/src/pxtone/pxtnError.cpp"
	"lib/src/pxtone/pxtnEvelist.cpp"
	"lib/src/pxtone/pxtnMaster.cpp"
	"lib/src/pxtone/pxtnMem.cpp"
	"lib/src/pxtone/pxtnOverDrive.cpp"
	"lib/src/pxtone/pxtnPulse_Frequency.cpp"
	"lib/src/pxtone/pxtnPulse_Noise.cpp"
	"lib/src/pxtone/pxtnPulse_NoiseBuilder.cpp"
	"lib/src/pxtone/pxtnPulse_Oggv.cpp"
	"lib/src/pxtone/pxtnPulse_Oscillator.cpp"
	"lib/src/pxtone/pxtnPulse_PCM.cpp"
	"lib/src/pxtone/pxtnService.cpp"
	"lib/src/pxtone/pxtnService_moo.cpp"
	"lib/src/pxtone/pxtnText.cpp"
	"lib/src/pxtone/pxtnUnit.cpp"
	"lib/src/pxtone/pxtnWoice.cpp"
	"lib/src/pxtone/pxtnWoice_io.cpp"
	"lib/src/pxtone/pxtnWoicePTV.cpp"
	"lib/src/pxtone/pxtoneNoise.cpp"
)

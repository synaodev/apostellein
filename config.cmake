cmake_minimum_required (VERSION 3.15)

# Platforms
if (CYGWIN)
	message (FATAL_ERROR "Cygwin is not supported!")
endif ()

if (WIN32)
	target_compile_definitions (apostellein PRIVATE
		"-DWIN32_LEAN_AND_MEAN"
		"-D_CRT_SECURE_NO_WARNINGS"
		"-DUNICODE"
		"-D_UNICODE"
		"-DNOMINMAX"
	)
else ()
	find_package (Threads REQUIRED)
	target_link_libraries (apostellein PRIVATE
		Threads::Threads
		"${CMAKE_DL_LIBS}"
	)
endif ()

if (MSVC)
	target_compile_options (apostellein PRIVATE
		"/W4"
		"/GR-"
		"/Zc:__cplusplus"
	)
elseif ()
	target_compile_options (apostellein PRIVATE
		"-Wall"
		"-Wextra"
		"-Wpedantic"
		"-fno-rtti"
	)
endif ()

# Options
if (APOSTELLEIN_GOLD)
	target_link_options (apostellein PRIVATE "-fuse-ld=gold")
endif ()

if (APOSTELLEIN_ESOTERIC_WARNINGS)
	target_compile_options (apostellein PRIVATE
		"-Wshadow"
		"-Wpointer-arith"
		"-Wcast-qual"
		"-Wcast-align"
		"-Wdouble-promotion"
		"-Wformat=2"
	)
	if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
		target_compile_options (apostellein PRIVATE
			"-fno-common"
			"-Wstrict-aliasing=1"
			"-Wstrict-overflow=2"
			"-Warray-bounds=2"
			"-Wundef"
		)
	elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
		target_compile_options (apostellein PRIVATE
			"-Wnon-virtual-dtor"
			"-Woverloaded-virtual"
			"-Wnull-dereference"
		)
	endif ()
endif ()

if (APOSTELLEIN_SANITIZER)
	target_link_libraries (apostellein PRIVATE "-fsanitize=undefined")
	target_compile_options (apostellein PRIVATE "-fsanitize=undefined")
endif ()

if (APOSTELLEIN_PVS_STUDIO)
	include ("${PROJECT_SOURCE_DIR}/cmake/StaticAnalysis.cmake")
	if (MSVC)
		pvs_studio_add_target (
			TARGET "apostellein.analysis" ALL
			OUTPUT FORMAT errorfile
			ANALYZE apostellein
			MODE "${APOSTELLEIN_DIAGNOSTIC_MODE}"
			LOG "apostellein.errorfile"
		)
	else ()
		pvs_studio_add_target (
			TARGET "apostellein.analysis" ALL
			OUTPUT FORMAT errorfile
			ANALYZE apostellein
			MODE "${APOSTELLEIN_DIAGNOSTIC_MODE}"
			LOG "apostellein.errorfile"
			COMPILE_COMMANDS "${CMAKE_CURRENT_BINARY_DIR}/compile_commands.json"
		)
	endif ()
endif ()

if (APOSTELLEIN_IMGUI_DEBUGGER)
	target_compile_definitions (apostellein PRIVATE "-DAPOSTELLEIN_IMGUI_DEBUGGER")
endif ()

if (APOSTELLEIN_FAST_MATH)
	if (MSVC)
		target_compile_options (apostellein PRIVATE "/fp:fast")
	else ()
		target_compile_options (apostellein PRIVATE "-ffast-math")
	endif ()
endif ()

if (NOT APOSTELLEIN_UNSAFE_LUA)
	target_compile_definitions (apostellein PRIVATE "-DSOL_ALL_SAFETIES_ON=1")
endif ()

if (APOSTELLEIN_OPENGL_LOGGING)
	target_compile_definitions (apostellein PRIVATE "-DAPOSTELLEIN_OPENGL_LOGGING")
endif ()

if (APOSTELLEIN_OPENAL_LOGGING)
	target_compile_definitions (apostellein PRIVATE "-DAPOSTELLEIN_OPENAL_LOGGING")
endif ()

if (CMAKE_SIZEOF_VOID_P EQUAL 8)
	target_compile_definitions (apostellein PRIVATE "-DAPOSTELLEIN_MACHINE_64BIT")
elseif (NOT CMAKE_SIZEOF_VOID_P EQUAL 4)
	message (FATAL_ERROR "Cannot determine size of void pointer!")
endif ()

# Packages
if (EXISTS "${CMAKE_BINARY_DIR}/conanbuildinfo.cmake")
	set (CONAN_TOOLCHAIN OFF)
    include ("${CMAKE_BINARY_DIR}/conanbuildinfo.cmake")
	conan_basic_setup ()
	target_compile_definitions (apostellein PRIVATE ${CONAN_DEFINES})
	target_include_directories (apostellein PRIVATE ${CONAN_INCLUDE_DIRS})
	target_link_directories (apostellein PRIVATE ${CONAN_LIB_DIRS})
	target_link_libraries (apostellein PRIVATE ${CONAN_LIBS})
else ()
	find_package (fmt CONFIG REQUIRED)
	find_package (spdlog CONFIG REQUIRED)
	find_package (nlohmann_json CONFIG REQUIRED)
	find_package (glm CONFIG REQUIRED)
	find_package (EnTT CONFIG REQUIRED)
	find_package (sol2 CONFIG REQUIRED)

	find_package (Lua REQUIRED)
	if (NOT LUA_FOUND)
		message (FATAL_ERROR "Could not find Lua!")
	elseif ("${LUA_VERSION_MAJOR}" LESS 5 OR "${LUA_VERSION_MINOR}" LESS 4)
		message (FATAL_ERROR "Could not find Lua version 5.4 or above!")
	endif ()

	include ("${PROJECT_SOURCE_DIR}/cmake/FindTmxlite.cmake")
	if (TMXLITE_NOTFOUND)
		message (FATAL_ERROR "Could not find Tmxlite!")
	endif ()

	find_path (STB_INCLUDE_DIR "stb_image.h")

	target_link_libraries (apostellein PRIVATE
		fmt::fmt
		spdlog::spdlog
		nlohmann_json::nlohmann_json
		glm::glm
		EnTT::EnTT
		sol2::sol2
		"${LUA_LIBRARIES}"
		"${TMXLITE_LIBRARY}"
	)
	target_include_directories (apostellein PRIVATE
		"${LUA_INCLUDE_DIR}"
		"${TMXLITE_INCLUDE_DIR}"
		"${STB_INCLUDE_DIR}"
	)

	if (VCPKG_TOOLCHAIN)
		target_compile_definitions (apostellein PRIVATE "-DAPOSTELLEIN_VCPKG_LIBRARIES")
		find_package (OpenAL CONFIG REQUIRED)
		target_link_libraries (apostellein PRIVATE OpenAL::OpenAL)
		find_package (SDL2 CONFIG REQUIRED)
		target_link_libraries (apostellein PRIVATE SDL2::SDL2)
		if (NOT APPLE)
			target_link_libraries (apostellein PRIVATE SDL2::SDL2main)
		endif ()
	else ()
		find_package(OpenAL REQUIRED)
		if (NOT OPENAL_FOUND)
			message (FATAL_ERROR "Could not find OpenAL!")
		endif ()
		target_link_libraries (apostellein PRIVATE "${OPENAL_LIBRARY}")
		target_include_directories (apostellein PRIVATE "${OPENAL_INCLUDE_DIR}")

		include ("${PROJECT_SOURCE_DIR}/cmake/FindSDL2.cmake")
		if (NOT SDL2_FOUND OR NOT SDL2MAIN_FOUND)
			message (FATAL_ERROR "Could not find SDL2!")
		endif ()
		target_link_libraries (apostellein PRIVATE "${SDL2_LIBRARIES}")
		target_include_directories (apostellein PRIVATE "${SDL2_INCLUDE_DIR}")
	endif ()

endif ()

# General
target_include_directories (apostellein PRIVATE "${PROJECT_SOURCE_DIR}/lib/inc")
target_compile_definitions (apostellein PRIVATE
	"-DGLM_FORCE_XYZW_ONLY"
	"-DIMGUI_IMPL_OPENGL_LOADER_CUSTOM"
)

# Apostéllein
This is the current official repository for Apostéllein. I work on this game in my spare time, and there is no planned release date. It will be ready when it is ready.
## Dependencies
- [fmt](https://github.com/fmtlib/fmt) (>= 7.1.3)
- [spdlog](https://github.com/gabime/spdlog) (>= 1.8.1)
- [OpenAL-Soft](https://github.com/kcat/openal-soft) (>= 1.19.1)
- [sol2](https://github.com/ThePhD/sol2) (>= 3.2.3)
- [lua](https://www.lua.org) (>= 5.4.0)
- [glm](https://github.com/g-truc/glm) (== 0.9.9.8)
- [EnTT](https://github.com/skypjack/entt) (>= 3.8.0)
- [SDL2](https://www.libsdl.org/download-2.0.php) (>= 2.0.14)
- [Nlohmann JSON](https://github.com/nlohmann/json) (>= 3.9.1)
- [stb](https://github.com/nothings/stb) (any)
- [Tmxlite](https://github.com/fallahn/tmxlite) (included)
- [Glad](https://glad.dav1d.de) (included)
- [ImGui](https://github.com/ocornut/imgui) (included)
- [Pxtone](https://pxtone.org/developer) (included)
## Compiling From Source
- Notes:
  - CMake version must be at least 3.15.
  - C++ compiler must fully support at least C++17 and C11.
  - OpenGL driver must support at least a 3.1 core profile.
  - 32-bit builds are infrequently tested, but they should work fine.
  - The Windows version compiles with MSVC, Clang, and MinGW. Cygwin environment is not supported.
  - Cross-compiling the Windows version from Linux will be officially supported at some point.
  - The MacOS version compiles only with AppleClang currently. I do plan to support GCC and Clang, eventually.
  - Once Apple decides to drop OpenGL entirely, I will consider hiring somebody to write a [Metal](https://developer.apple.com/metal/) version of the renderer.
- Linux:
  - If you don't want to use your package manager, use [conan](https://github.com/conan-io/conan) or [vcpkg](https://github.com/microsoft/vcpkg).
  - Use your package manager to install OpenGL utilities, fmt, spdlog, OpenAL-Soft, lua, glm, SDL2, and Nlohmann JSON.
    - Apt: `apt-get install libgl1-mesa-dev mesa-utils libfmt-dev libspdlog-dev libopenal-dev liblua5.4-dev libglm-dev libsdl2-dev nlohmann-json3-dev`
    - Pacman: `pacman -S mesa fmt spdlog openal lua glm sdl2 nlohmann-json`
    - Yum: `yum install epel-release && yum install mesa-libGL-devel fmt-devel spdlog-devel openal-soft-devel lua-devel glm-devel SDL2-devel json-devel`
  - For sol2 and EnTT: build and install from source (using cmake, ideally).
  - For stb: clone the repo. Then, move `stb_image.h` and `stb_rect_pack.h` to `/usr/local/include/` so cmake can find them.
  - Move into the `build` directory.
  - Run `cmake ..` and `cmake --build .`.
- Conan:
  - Move into the `build` directory.
  - Run `conan install ..`.
  - On MacOS you will need to add the argument `--build=missing`.
  - Run `cmake ..` and `cmake --build .`.
- Vcpkg:
  - Install these dependencies: `fmt spdlog openal-soft sol2 lua glm entt sdl2 nlohmann-json stb`.
  - Occasionally the `x64-mingw-dynamic` triplet fails to build OpenAL-Soft. I'm currently looking for a way to make this more consistent.
  - Move into the `build` directory.
  - Run `cmake ..`.
  - If you're not running Windows, you will need to add the argument `-DCMAKE_TOOLCHAIN_FILE="<vcpkg-root>/scripts/buildsystems/vcpkg.cmake"`.
  - Run `cmake --build .`.

## Thank You
- [Daisuke Amaya](https://en.wikipedia.org/wiki/Daisuke_Amaya):
  The developer of Cave Story, Kero Blaster, and Pxtone Collage. If not for his work, I might have done something else with my life, and this project would not exist.
- [Christopher Hebert](https://github.com/chebert):
  The creator of an extraordinary video series called [Reconstructing Cave Story](https://youtube.com/playlist?list=PL006xsVEsbKjSKBmLu1clo85yLrwjY67X). Apostéllein's tilemap collision system is heavily based on his tilemap collision system.
- [SFML Team](https://github.com/sfml):
  The glCheck() and alCheck() macros all come from the [SFML library](https://github.com/sfml/sfml).
- [Mikola Lysenko](https://github.com/mikolalysenko):
  The tilemap raycast implementation borrows from [this 3D implementation](https://github.com/mikolalysenko/voxel-raycast).
- [Caitlin Shaw](http://nxengine.sourceforge.net/):
  The global "thinker table" is generated using some pre-processor magic that takes advantage of class constructors. This idea comes from the rather unorthodox "InitList" system in [NxEngine](https://github.com/nxengine/nxengine-evo/), which cleverly initializes the AI function table across several different source files.
- [PVS Studio](https://pvs-studio.com/en/pvs-studio/):
  This static analyzer was extrordinarily helpful for catching errors that the compilers didn't warn me about. If you're starting a new C/C++ project in the near future, you should consider using as part of your build pipeline.

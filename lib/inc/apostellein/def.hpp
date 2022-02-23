#pragma once

#if defined(_WIN32)
	#define APOSTELLEIN_PLATFORM_WINDOWS
#elif defined(__APPLE__) && defined(__MACH__)
	#include <TargetConditionals.h>
	#if defined(TARGET_OS_MAC)
		#define APOSTELLEIN_PLATFORM_MACOS
		#define APOSTELLEIN_POSIX_COMPLIANT
	#else
		#error "Apple platform is not supported!"
	#endif
#elif defined(__unix__)
	#define APOSTELLEIN_POSIX_COMPLIANT
	#if defined(__linux__)
		#define APOSTELLEIN_PLATFORM_LINUX
	#elif defined(__bsdi__)
		#if defined(__FreeBSD__)
			#define APOSTELLEIN_PLATFORM_FREEBSD
		#elif defined(__OpenBSD__)
			#define APOSTELLEIN_PLATFORM_OPENBSD
		#elif defined(__NetBSD__)
			#define APOSTELLEIN_PLATFORM_NETBSD
		#elif defined(__DragonFly__)
			#define APOSTELLEIN_PLATFORM_DRAGONFLY
		#else
			#error "Unknown BSD platform is not supported!"
		#endif
	#elif defined(__CYGWIN__)
		#error "Cygwin is not supported!"
	#else
		#error "Unknown unix-like platform is not supported!"
	#endif
#else
	#error "Unknown platform is not supported!"
#endif

#if defined(__clang__)
	#define APOSTELLEIN_COMPILER_LLVM
	#if defined(_MSC_VER)
		#define APOSTELLEIN_TOOLCHAIN_MSVC
	#elif defined(__apple_build_version__)
		#define APOSTELLEIN_TOOLCHAIN_APPLECLANG
	#else
		#define APOSTELLEIN_TOOLCHAIN_LLVM
	#endif
#elif defined(__GNUC__)
	#define APOSTELLEIN_COMPILER_GNUC
	#define APOSTELLEIN_TOOLCHAIN_GNUC
#elif defined(_MSC_VER)
	#define APOSTELLEIN_COMPILER_MSVC
	#define APOSTELLEIN_TOOLCHAIN_MSVC
	#include <ciso646>
#else
	#error "Target compiler and toolchain are not supported!"
#endif

// primitives
using byte = unsigned char;
using i16 = short;
using u16 = unsigned short;
using i32 = int;
using u32 = unsigned int;
using i64 = long long;
using u64 = unsigned long long;
using udx = decltype(sizeof(0));
using r32 = float;
using r64 = double;

static_assert(sizeof(i32) == sizeof(r32));
static_assert(sizeof(i64) == sizeof(r64));

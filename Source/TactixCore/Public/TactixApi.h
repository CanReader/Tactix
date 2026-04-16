// Copyright Sleak Software. All Rights Reserved.
//
// TactixApi.h — DLL-export macros and common attributes for the Tactix core
// libraries. This file is intentionally dependency-free so the foundation
// headers can be consumed both inside Unreal Engine (UBT-driven) and inside a
// standalone CMake/GTest harness (for unit testing the engine-agnostic core).
//
// When compiled inside UE, UBT defines TACTIXCORE_API / TACTIXSYSTEMS_API as
// either `__declspec(dllexport)` / `__declspec(dllimport)` (Windows) or the
// visibility("default") attribute (Clang/GCC). When compiled standalone, the
// macros are not defined — we fall back to empty definitions here so the
// headers still compile for static-linked tests.

#pragma once

// ---- Module API macros (UBT-aware) ----------------------------------------

#ifndef TACTIXCORE_API
	#define TACTIXCORE_API
#endif

#ifndef TACTIXSYSTEMS_API
	#define TACTIXSYSTEMS_API
#endif

#ifndef TACTIXUE_API
	#define TACTIXUE_API
#endif

#ifndef TACTIXDEBUG_API
	#define TACTIXDEBUG_API
#endif

// ---- Compiler attributes --------------------------------------------------

#if defined(_MSC_VER)
	#define TACTIX_FORCEINLINE __forceinline
	#define TACTIX_NOINLINE    __declspec(noinline)
#else
	#define TACTIX_FORCEINLINE inline __attribute__((always_inline))
	#define TACTIX_NOINLINE    __attribute__((noinline))
#endif

#define TACTIX_NODISCARD [[nodiscard]]

// Expands to `assert(x)` in debug, no-op in shipping. We deliberately avoid
// <cassert> so the foundation has no OS runtime coupling; the unit-test build
// redefines TACTIX_ASSERT if it wants GTest-style assertions.
#ifndef TACTIX_ASSERT
	#if defined(TACTIX_DEBUG) && TACTIX_DEBUG
		#include <cstdio>
		#include <cstdlib>
		#define TACTIX_ASSERT(x) do { if (!(x)) { std::fprintf(stderr, \
			"TACTIX_ASSERT(%s) failed at %s:%d\n", #x, __FILE__, __LINE__); \
			std::abort(); } } while (0)
	#else
		#define TACTIX_ASSERT(x) ((void)0)
	#endif
#endif

namespace Tactix
{
	inline constexpr int kVersionMajor = 0;
	inline constexpr int kVersionMinor = 1;
	inline constexpr int kVersionPatch = 0;
}

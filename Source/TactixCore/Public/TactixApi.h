// Copyright Sleak Software. All Rights Reserved.

/**
 * @file TactixApi.h
 * @brief Export macros, compiler attributes and version constants shared by
 *        every Tactix module.
 *
 * This header has no dependencies on purpose. The foundation headers include it
 * and nothing else from the engine, which is what lets TactixCore compile in two
 * very different worlds: inside Unreal (driven by UBT) and inside the standalone
 * CMake/GTest harness that exercises the engine-agnostic code.
 *
 * The trick is that every `TACTIX*_API` macro is defined defensively. Under UBT
 * the build system has already defined them (to `__declspec(dllexport/import)`
 * on Windows, or a `visibility("default")` attribute under Clang/GCC) before
 * this header is reached, so the `#ifndef` guards do nothing. In the standalone
 * test build nobody defines them, so they collapse to empty and the same headers
 * link statically without complaint.
 *
 * @note Keep this file free of `#include`s beyond the C++ standard library.
 *       Pulling in anything engine-side here would defeat the whole point.
 */

#pragma once

/**
 * @defgroup TactixExportMacros Module export macros
 * @brief Per-module linkage decorators, UBT-aware with an empty fallback.
 *
 * Decorate a public type or free function with the macro of the module it lives
 * in (e.g. a class declared in TactixSystems uses @c TACTIXSYSTEMS_API). When
 * the module is built as a shared object the symbol is exported; when something
 * else links against it the symbol is imported; in the static test build the
 * macro vanishes.
 * @{
 */

/** @brief Linkage decorator for symbols defined in the TactixCore module. */
#ifndef TACTIXCORE_API
	#define TACTIXCORE_API
#endif

/** @brief Linkage decorator for symbols defined in the TactixSystems module. */
#ifndef TACTIXSYSTEMS_API
	#define TACTIXSYSTEMS_API
#endif

/** @brief Linkage decorator for symbols defined in the TactixUE module. */
#ifndef TACTIXUE_API
	#define TACTIXUE_API
#endif

/** @brief Linkage decorator for symbols defined in the TactixDebug module. */
#ifndef TACTIXDEBUG_API
	#define TACTIXDEBUG_API
#endif

/** @} */

/**
 * @defgroup TactixCompilerAttributes Compiler attributes
 * @brief Portable spellings of inlining and attribute hints.
 * @{
 */

#if defined(_MSC_VER)
	/** @brief Force the compiler to inline regardless of its own heuristics. */
	#define TACTIX_FORCEINLINE __forceinline
	/** @brief Forbid inlining, e.g. to keep a cold path out of a hot caller. */
	#define TACTIX_NOINLINE    __declspec(noinline)
#else
	#define TACTIX_FORCEINLINE inline __attribute__((always_inline))
	#define TACTIX_NOINLINE    __attribute__((noinline))
#endif

/**
 * @brief `[[nodiscard]]` applied to accessors and allocators whose result must
 *        not be silently dropped (a leaked pool handle, an ignored Alloc, etc.).
 */
#define TACTIX_NODISCARD [[nodiscard]]

/** @} */

/**
 * @def TACTIX_ASSERT
 * @brief Debug-only invariant check; compiles to nothing in shipping.
 *
 * Deliberately implemented without `<cassert>` so the foundation carries no
 * coupling to the platform's assert handler. When @c TACTIX_DEBUG is set the
 * macro prints the failed expression with file and line and calls
 * `std::abort()`. The GTest harness is free to `#define` its own version before
 * including any Tactix header to route failures into the test framework instead.
 *
 * @param x Expression expected to be true. Evaluated exactly once in debug, not
 *          at all in shipping, so it must be free of side effects you rely on.
 */
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
	/// @brief Major version. Bumped on a breaking change to the public API.
	inline constexpr int kVersionMajor = 0;
	/// @brief Minor version. Bumped when features are added compatibly.
	inline constexpr int kVersionMinor = 1;
	/// @brief Patch version. Bumped for fixes that don't touch the API.
	inline constexpr int kVersionPatch = 0;
}

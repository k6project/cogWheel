#pragma once

#ifdef CORE_DEBUG
#define DEBUG_BUILD
#endif

#ifdef MSC_VER
#define debugBreak() __debugbreak()
#define FORCE_INLINE __forceinline
#else
#define debugBreak() __builtin_debugtrap()
#define FORCE_INLINE static inline __attribute__((always_inline))
#endif

#ifdef DEBUG_BUILD
#define ENSURE(c) if(!(c)) debugBreak();
#else
#define ENSURE(c) !(c);
#endif

#ifndef COMMON_COMPILER_H
#define COMMON_COMPILER_H

/**
 * Compiler intrinsics and portability macros.
 */

/**
 * Hints that a condition is likely to be true.
 *
 * Helps the compiler generate better branch layouts by placing the
 * likely path inline. No-op on compilers without __builtin_expect.
 *
 * @param x Condition expression
 * @return  The boolean value of x
 */
#if defined(__GNUC__) || defined(__clang__)
    #define LIKELY(x)   __builtin_expect(!!(x), 1)
#else
    #define LIKELY(x)   (x)
#endif

/**
 * Hints that a condition is unlikely to be true.
 *
 * Helps the compiler generate better branch layouts by placing the
 * unlikely path out-of-line. No-op on compilers without __builtin_expect.
 *
 * @param x Condition expression
 * @return  The boolean value of x
 */
#if defined(__GNUC__) || defined(__clang__)
    #define UNLIKELY(x) __builtin_expect(!!(x), 0)
#else
    #define UNLIKELY(x) (x)
#endif

/**
 * Forces a function to be inlined.
 *
 * Use for small, performance-critical functions where inlining
 * eliminates call overhead. No-op on compilers without the attribute.
 */
#if defined(__GNUC__) || defined(__clang__)
    #define ALWAYS_INLINE static inline __attribute__((always_inline))
#else
    #define ALWAYS_INLINE static inline
#endif

#endif // COMMON_COMPILER_H

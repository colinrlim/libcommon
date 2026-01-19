#ifndef COMMON_BUFCONV_H
#define COMMON_BUFCONV_H

#include <common/internal/conv_impl.h>
#include <common/compiler.h>

#define BUFCONV_RESULT_SUCCESS              CONV_RESULT_SUCCESS
#define BUFCONV_RESULT_INVALID_CHARACTER    CONV_RESULT_INVALID_CHARACTER

// Internal macros for generating conversion logic
#define BUFCONV_INT(ptr, n, out, on_return)                                                 \
    CONV_INT_IMPL(ptr[i], out, size_t i = 0, on_return, (i < n), (i < n && ptr[i]), i++)    \

#define BUFCONV_FLOAT(ptr, n, out, on_return, T, suffix)                                                \
    CONV_FLOAT_IMPL(ptr[i], out, size_t i = 0, on_return, (i < n), (i < n && ptr[i]), i++, T, suffix)   \

#define BUFCONV_SIGNED(OP, ptr, n, out)                         \
    do {                                                        \
        const u8 is_neg = (*ptr == '-' && ++ptr && n--);        \
        BUFCONV_##OP(ptr, n, out, if (is_neg) *out = -(*out));  \
    } while (0)                                                 \

#define BUFCONV_INT_UNSIGNED(ptr, n, out)       BUFCONV_INT(ptr, n, out,)
#define BUFCONV_INT_SIGNED(ptr, n, out)         BUFCONV_SIGNED(INT, ptr, n, out)

#define BUFCONV_FLOAT32(ptr, n, out, on_return) BUFCONV_FLOAT(ptr, n, out, on_return, f32, f)
#define BUFCONV_FLOAT32_UNSIGNED(ptr, n, out)   BUFCONV_FLOAT32(ptr, n, out,)
#define BUFCONV_FLOAT32_SIGNED(ptr, n, out)     BUFCONV_SIGNED(FLOAT32, ptr, n, out)

#define BUFCONV_FLOAT64(ptr, n, out, on_return) BUFCONV_FLOAT(ptr, n, out, on_return, f64, 0)
#define BUFCONV_FLOAT64_UNSIGNED(ptr, n, out)   BUFCONV_FLOAT64(ptr, n, out,)
#define BUFCONV_FLOAT64_SIGNED(ptr, n, out)     BUFCONV_SIGNED(FLOAT64, ptr, n, out)

// --------------------------------------------------------------------------
// Unsigned Integer Conversions
// --------------------------------------------------------------------------

/**
 * Converts a buffer to an unsigned integer (u8).
 *
 * @param ptr Input buffer (does not require null-termination)
 * @param n   Length of the buffer in bytes
 * @param out Pointer to store the result
 * @return BUFCONV_RESULT_SUCCESS or BUFCONV_RESULT_INVALID_CHARACTER
 *
 * @warning **NO OVERFLOW CHECK:** The caller must ensure the input buffer
 * represents a value that fits within u8 (0-255). Larger values will
 * wrap around silently.
 */
ALWAYS_INLINE int bufconv_u8(const char* ptr, size_t n, u8* out) { BUFCONV_INT_UNSIGNED(ptr, n, out); }

/**
 * Converts a buffer to an unsigned integer (u16).
 * @see bufconv_u8 for warnings regarding overflow.
 */
ALWAYS_INLINE int bufconv_u16(const char* ptr, size_t n, u16* out) { BUFCONV_INT_UNSIGNED(ptr, n, out); }

/**
 * Converts a buffer to an unsigned integer (u32).
 * @see bufconv_u8 for warnings regarding overflow.
 */
ALWAYS_INLINE int bufconv_u32(const char* ptr, size_t n, u32* out) { BUFCONV_INT_UNSIGNED(ptr, n, out); }

/**
 * Converts a buffer to an unsigned integer (u64).
 * @see bufconv_u8 for warnings regarding overflow.
 */
ALWAYS_INLINE int bufconv_u64(const char* ptr, size_t n, u64* out) { BUFCONV_INT_UNSIGNED(ptr, n, out); }


// --------------------------------------------------------------------------
// Signed Integer Conversions
// --------------------------------------------------------------------------

/**
 * Converts a buffer to a signed integer (i8).
 * Supports negative numbers if the buffer starts with '-'.
 *
 * @param ptr Input buffer (does not require null-termination)
 * @param n   Length of the buffer in bytes
 * @param out Pointer to store the result
 * @return BUFCONV_RESULT_SUCCESS or BUFCONV_RESULT_INVALID_CHARACTER
 *
 * @warning **NO OVERFLOW CHECK:** The caller must ensure the input buffer
 * fits within the range of i8 (-128 to 127).
 */
ALWAYS_INLINE int bufconv_i8(const char* ptr, size_t n, i8* out) { BUFCONV_INT_SIGNED(ptr, n, out); }

/**
 * Converts a buffer to a signed integer (i16).
 * @see bufconv_i8 for warnings regarding overflow.
 */
ALWAYS_INLINE int bufconv_i16(const char* ptr, size_t n, i16* out) { BUFCONV_INT_SIGNED(ptr, n, out); }

/**
 * Converts a buffer to a signed integer (i32).
 * @see bufconv_i8 for warnings regarding overflow.
 */
ALWAYS_INLINE int bufconv_i32(const char* ptr, size_t n, i32* out) { BUFCONV_INT_SIGNED(ptr, n, out); }

/**
 * Converts a buffer to a signed integer (i64).
 * @see bufconv_i8 for warnings regarding overflow.
 */
ALWAYS_INLINE int bufconv_i64(const char* ptr, size_t n, i64* out) { BUFCONV_INT_SIGNED(ptr, n, out); }


// --------------------------------------------------------------------------
// Floating Point Conversions
// --------------------------------------------------------------------------

/**
 * Converts a buffer to an unsigned float (f32).
 * Parses "123", "123.456", etc. Does not handle scientific notation.
 *
 * @param ptr Input buffer (does not require null-termination)
 * @param n   Length of the buffer in bytes
 * @param out Pointer to store the result
 * @return BUFCONV_RESULT_SUCCESS or BUFCONV_RESULT_INVALID_CHARACTER
 * @warning **NO OVERFLOW/PRECISION CHECK:** Caller is responsible for bounds.
 */
ALWAYS_INLINE int bufconv_f32_unsigned(const char* ptr, size_t n, f32* out) { BUFCONV_FLOAT32_UNSIGNED(ptr, n, out); }

/**
 * Converts a buffer to an unsigned double (f64).
 * @see bufconv_f32_unsigned
 */
ALWAYS_INLINE int bufconv_f64_unsigned(const char* ptr, size_t n, f64* out) { BUFCONV_FLOAT64_UNSIGNED(ptr, n, out); }

/**
 * Converts a buffer to a signed float (f32).
 * Supports negative numbers if the buffer starts with '-'.
 *
 * @param ptr Input buffer (does not require null-termination)
 * @param n   Length of the buffer in bytes
 * @param out Pointer to store the result
 * @return BUFCONV_RESULT_SUCCESS or BUFCONV_RESULT_INVALID_CHARACTER
 * @warning **NO OVERFLOW/PRECISION CHECK:** Caller is responsible for bounds.
 */
ALWAYS_INLINE int bufconv_f32_signed(const char* ptr, size_t n, f32* out) { BUFCONV_FLOAT32_SIGNED(ptr, n, out); }

/**
 * Converts a buffer to a signed double (f64).
 * @see bufconv_f32_signed
 */
ALWAYS_INLINE int bufconv_f64_signed(const char* ptr, size_t n, f64* out) { BUFCONV_FLOAT64_SIGNED(ptr, n, out); }

#endif // COMMON_BUFCONV_H
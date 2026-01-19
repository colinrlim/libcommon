#ifndef COMMON_STRCONV_H
#define COMMON_STRCONV_H

#include <common/internal/conv_impl.h>
#include <common/compiler.h>

#define STRCONV_RESULT_SUCCESS              CONV_RESULT_SUCCESS
#define STRCONV_RESULT_INVALID_CHARACTER    CONV_RESULT_INVALID_CHARACTER

// Internal macros for generating conversion logic
#define STRCONV_INT(str, out, on_return)                        \
    CONV_INT_IMPL(*str, out,, on_return, (1), (*str), str++)    \

#define STRCONV_FLOAT(str, out, on_return, T, suffix)                       \
    CONV_FLOAT_IMPL(*str, out,, on_return, (1), (*str), str++, T, suffix)   \

#define STRCONV_SIGNED(OP, str, out)                            \
    do {                                                        \
        const u8 is_neg = (*str == '-' && ++str);               \
        STRCONV_##OP(str, out, if (is_neg) *out = -(*out));     \
    } while (0)                                                 \

#define STRCONV_INT_UNSIGNED(str, out)          STRCONV_INT(str, out,)
#define STRCONV_INT_SIGNED(str, out)            STRCONV_SIGNED(INT, str, out)

#define STRCONV_FLOAT32(str, out, on_return)    STRCONV_FLOAT(str, out, on_return, f32, f)
#define STRCONV_FLOAT32_UNSIGNED(str, out)      STRCONV_FLOAT32(str, out,)
#define STRCONV_FLOAT32_SIGNED(str, out)        STRCONV_SIGNED(FLOAT32, str, out)

#define STRCONV_FLOAT64(str, out, on_return)    STRCONV_FLOAT(str, out, on_return, f64, 0)
#define STRCONV_FLOAT64_UNSIGNED(str, out)      STRCONV_FLOAT64(str, out,)
#define STRCONV_FLOAT64_SIGNED(str, out)        STRCONV_SIGNED(FLOAT64, str, out)

// --------------------------------------------------------------------------
// Unsigned Integer Conversions
// --------------------------------------------------------------------------

/**
 * Converts a string to an unsigned integer (u8).
 *
 * @param str Input string (must be null-terminated)
 * @param out Pointer to store the result
 * @return STRCONV_RESULT_SUCCESS or STRCONV_RESULT_INVALID_CHARACTER
 *
 * @warning **NO OVERFLOW CHECK:** The caller must ensure the input string
 * represents a value that fits within u8 (0-255). Larger values will
 * wrap around silently.
 */
ALWAYS_INLINE int strconv_u8(const char* str, u8* out) { STRCONV_INT_UNSIGNED(str, out); }

/**
 * Converts a string to an unsigned integer (u16).
 * @see strconv_u8 for warnings regarding overflow.
 */
ALWAYS_INLINE int strconv_u16(const char* str, u16* out) { STRCONV_INT_UNSIGNED(str, out); }

/**
 * Converts a string to an unsigned integer (u32).
 * @see strconv_u8 for warnings regarding overflow.
 */
ALWAYS_INLINE int strconv_u32(const char* str, u32* out) { STRCONV_INT_UNSIGNED(str, out); }

/**
 * Converts a string to an unsigned integer (u64).
 * @see strconv_u8 for warnings regarding overflow.
 */
ALWAYS_INLINE int strconv_u64(const char* str, u64* out) { STRCONV_INT_UNSIGNED(str, out); }


// --------------------------------------------------------------------------
// Signed Integer Conversions
// --------------------------------------------------------------------------

/**
 * Converts a string to a signed integer (i8).
 * Supports negative numbers if the string starts with '-'.
 *
 * @param str Input string (must be null-terminated)
 * @param out Pointer to store the result
 * @return STRCONV_RESULT_SUCCESS or STRCONV_RESULT_INVALID_CHARACTER
 *
 * @warning **NO OVERFLOW CHECK:** The caller must ensure the input string
 * fits within the range of i8 (-128 to 127).
 */
ALWAYS_INLINE int strconv_i8(const char* str, i8* out) { STRCONV_INT_SIGNED(str, out); }

/**
 * Converts a string to a signed integer (i16).
 * @see strconv_i8 for warnings regarding overflow.
 */
ALWAYS_INLINE int strconv_i16(const char* str, i16* out) { STRCONV_INT_SIGNED(str, out); }

/**
 * Converts a string to a signed integer (i32).
 * @see strconv_i8 for warnings regarding overflow.
 */
ALWAYS_INLINE int strconv_i32(const char* str, i32* out) { STRCONV_INT_SIGNED(str, out); }

/**
 * Converts a string to a signed integer (i64).
 * @see strconv_i8 for warnings regarding overflow.
 */
ALWAYS_INLINE int strconv_i64(const char* str, i64* out) { STRCONV_INT_SIGNED(str, out); }


// --------------------------------------------------------------------------
// Floating Point Conversions
// --------------------------------------------------------------------------

/**
 * Converts a string to an unsigned float (f32).
 * Parses "123", "123.456", etc. Does not handle scientific notation.
 *
 * @param str Input string (must be null-terminated)
 * @param out Pointer to store the result
 * @return STRCONV_RESULT_SUCCESS or STRCONV_RESULT_INVALID_CHARACTER
 * @warning **NO OVERFLOW/PRECISION CHECK:** Caller is responsible for bounds.
 */
ALWAYS_INLINE int strconv_f32_unsigned(const char* str, f32* out) { STRCONV_FLOAT32_UNSIGNED(str, out); }

/**
 * Converts a string to an unsigned double (f64).
 * @see strconv_f32_unsigned
 */
ALWAYS_INLINE int strconv_f64_unsigned(const char* str, f64* out) { STRCONV_FLOAT64_UNSIGNED(str, out); }

/**
 * Converts a string to a signed float (f32).
 * Supports negative numbers if the string starts with '-'.
 *
 * @param str Input string (must be null-terminated)
 * @param out Pointer to store the result
 * @return STRCONV_RESULT_SUCCESS or STRCONV_RESULT_INVALID_CHARACTER
 * @warning **NO OVERFLOW/PRECISION CHECK:** Caller is responsible for bounds.
 */
ALWAYS_INLINE int strconv_f32_signed(const char* str, f32* out) { STRCONV_FLOAT32_SIGNED(str, out); }

/**
 * Converts a string to a signed double (f64).
 * @see strconv_f32_signed
 */
ALWAYS_INLINE int strconv_f64_signed(const char* str, f64* out) { STRCONV_FLOAT64_SIGNED(str, out); }

#endif // COMMON_STRCONV_H
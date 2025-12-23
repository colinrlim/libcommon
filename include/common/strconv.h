#ifndef COMMON_STRCONV_H
#define COMMON_STRCONV_H

#include <common/types.h>

/**
 * Result codes for string conversion operations.
 */
typedef enum STRCONV_RESULT {
    STRCONV_RESULT_SUCCESS              = 0, ///< Parsing completed successfully
    STRCONV_RESULT_INVALID_CHARACTER    = -1 ///< Parsing stopped due to a non-digit character
} STRCONV_RESULT;

// Internal macros for generating conversion logic
#define STRCONV_INT_UNCHECKED(str, out, on_return)              \
    do { *out = 0; while (*str)                                 \
            *out = (*out << 3) + (*out << 1) + (*str++ & 0xF);  \
        on_return;                                              \
        return STRCONV_RESULT_SUCCESS;                          \
    } while (0)                                                 \

#define STRCONV_INT(str, out, on_return)                        \
    do { *out = 0;                                              \
        while (1) {                                             \
            const u8 c = (const u8)(*str - '0');                \
            if (c > 9) break;                                   \
            *out = (*out * 10) + c;                             \
            str++;                                              \
        }                                                       \
        on_return;                                              \
        return *str ? STRCONV_RESULT_INVALID_CHARACTER          \
            : STRCONV_RESULT_SUCCESS;                           \
    } while (0)                                                 \

#define STRCONV_FLOAT(str, out, on_return, T, suffix)           \
    STRCONV_INT(str, out, {                                     \
        if (*str == '.') {                                      \
            str++;                                              \
            T divisor = 10.##suffix;                            \
            while (1) {                                         \
                const u8 c = (const u8)(*str - '0');            \
                if (c > 9) break;                               \
                *out += c / divisor;                            \
                divisor *= 10.##suffix;                         \
                str++;                                          \
            }                                                   \
        }                                                       \
        on_return;                                              \
    })                                                          \

#define STRCONV_SIGNED(OP, str, out)                            \
    do {                                                        \
        u8 is_neg = (*str == '-' && str++);                     \
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
static inline int __attribute__((always_inline)) strconv_u8(const char* str, u8* out) { STRCONV_INT_UNSIGNED(str, out); }

/**
 * Converts a string to an unsigned integer (u16).
 * @see strconv_u8 for warnings regarding overflow.
 */
static inline int __attribute__((always_inline)) strconv_u16(const char* str, u16* out) { STRCONV_INT_UNSIGNED(str, out); }

/**
 * Converts a string to an unsigned integer (u32).
 * @see strconv_u8 for warnings regarding overflow.
 */
static inline int __attribute__((always_inline)) strconv_u32(const char* str, u32* out) { STRCONV_INT_UNSIGNED(str, out); }

/**
 * Converts a string to an unsigned integer (u64).
 * @see strconv_u8 for warnings regarding overflow.
 */
static inline int __attribute__((always_inline)) strconv_u64(const char* str, u64* out) { STRCONV_INT_UNSIGNED(str, out); }


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
static inline int __attribute__((always_inline)) strconv_i8(const char* str, i8* out) { STRCONV_INT_SIGNED(str, out); }

/**
 * Converts a string to a signed integer (i16).
 * @see strconv_i8 for warnings regarding overflow.
 */
static inline int __attribute__((always_inline)) strconv_i16(const char* str, i16* out) { STRCONV_INT_SIGNED(str, out); }

/**
 * Converts a string to a signed integer (i32).
 * @see strconv_i8 for warnings regarding overflow.
 */
static inline int __attribute__((always_inline)) strconv_i32(const char* str, i32* out) { STRCONV_INT_SIGNED(str, out); }

/**
 * Converts a string to a signed integer (i64).
 * @see strconv_i8 for warnings regarding overflow.
 */
static inline int __attribute__((always_inline)) strconv_i64(const char* str, i64* out) { STRCONV_INT_SIGNED(str, out); }


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
static inline int __attribute__((always_inline)) strconv_f32_unsigned(const char* str, f32* out) { STRCONV_FLOAT32_UNSIGNED(str, out); }

/**
 * Converts a string to an unsigned double (f64).
 * @see strconv_f32_unsigned
 */
static inline int __attribute__((always_inline)) strconv_f64_unsigned(const char* str, f64* out) { STRCONV_FLOAT64_UNSIGNED(str, out); }

/**
 * Converts a string to a signed float (f32).
 * Supports negative numbers if the string starts with '-'.
 *
 * @param str Input string (must be null-terminated)
 * @param out Pointer to store the result
 * @return STRCONV_RESULT_SUCCESS or STRCONV_RESULT_INVALID_CHARACTER
 * @warning **NO OVERFLOW/PRECISION CHECK:** Caller is responsible for bounds.
 */
static inline int __attribute__((always_inline)) strconv_f32_signed(const char* str, f32* out) { STRCONV_FLOAT32_SIGNED(str, out); }

/**
 * Converts a string to a signed double (f64).
 * @see strconv_f32_signed
 */
static inline int __attribute__((always_inline)) strconv_f64_signed(const char* str, f64* out) { STRCONV_FLOAT64_SIGNED(str, out); }

#endif // COMMON_STRCONV_H
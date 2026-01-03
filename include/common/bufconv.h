#ifndef COMMON_BUFCONV_H
#define COMMON_BUFCONV_H

#include <common/types.h>

/**
 * Result codes for buffer conversion operations.
 */
typedef enum BUFCONV_RESULT {
    BUFCONV_RESULT_SUCCESS              = 0, ///< Parsing completed successfully
    BUFCONV_RESULT_INVALID_CHARACTER    = -1 ///< Parsing stopped due to a non-digit character
} BUFCONV_RESULT;

// Internal macros for generating conversion logic
#define BUFCONV_INT(ptr, n, out, on_return)                 \
    do {                                                    \
        size_t i = 0;                                       \
        *out = 0;                                           \
        while (i < n) {                                     \
            const u8 c = (const u8)(ptr[i] - '0');          \
            if (c > 9) break;                               \
            *out = (*out * 10) + c;                         \
            i++;                                            \
        }                                                   \
        on_return;                                          \
        return (i == n || !ptr[i]) ? BUFCONV_RESULT_SUCCESS \
            : BUFCONV_RESULT_INVALID_CHARACTER;             \
    } while (0)                                             \

#define BUFCONV_FLOAT(ptr, n, out, on_return, T, suffix)    \
    BUFCONV_INT(ptr, n, out, {                              \
        if (ptr[i] == '.') {                                \
            i++;                                            \
            T divisor = 10.##suffix;                        \
            while (i < n) {                                 \
                const u8 c = (const u8)(ptr[i] - '0');      \
                if (c > 9) break;                           \
                *out += c / divisor;                        \
                divisor *= 10.##suffix;                     \
                i++;                                        \
            }                                               \
        }                                                   \
        on_return;                                          \
    })                                                      \

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
static inline int __attribute__((always_inline)) bufconv_u8(const char* ptr, size_t n, u8* out) { BUFCONV_INT_UNSIGNED(ptr, n, out); }

/**
 * Converts a buffer to an unsigned integer (u16).
 * @see bufconv_u8 for warnings regarding overflow.
 */
static inline int __attribute__((always_inline)) bufconv_u16(const char* ptr, size_t n, u16* out) { BUFCONV_INT_UNSIGNED(ptr, n, out); }

/**
 * Converts a buffer to an unsigned integer (u32).
 * @see bufconv_u8 for warnings regarding overflow.
 */
static inline int __attribute__((always_inline)) bufconv_u32(const char* ptr, size_t n, u32* out) { BUFCONV_INT_UNSIGNED(ptr, n, out); }

/**
 * Converts a buffer to an unsigned integer (u64).
 * @see bufconv_u8 for warnings regarding overflow.
 */
static inline int __attribute__((always_inline)) bufconv_u64(const char* ptr, size_t n, u64* out) { BUFCONV_INT_UNSIGNED(ptr, n, out); }


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
static inline int __attribute__((always_inline)) bufconv_i8(const char* ptr, size_t n, i8* out) { BUFCONV_INT_SIGNED(ptr, n, out); }

/**
 * Converts a buffer to a signed integer (i16).
 * @see bufconv_i8 for warnings regarding overflow.
 */
static inline int __attribute__((always_inline)) bufconv_i16(const char* ptr, size_t n, i16* out) { BUFCONV_INT_SIGNED(ptr, n, out); }

/**
 * Converts a buffer to a signed integer (i32).
 * @see bufconv_i8 for warnings regarding overflow.
 */
static inline int __attribute__((always_inline)) bufconv_i32(const char* ptr, size_t n, i32* out) { BUFCONV_INT_SIGNED(ptr, n, out); }

/**
 * Converts a buffer to a signed integer (i64).
 * @see bufconv_i8 for warnings regarding overflow.
 */
static inline int __attribute__((always_inline)) bufconv_i64(const char* ptr, size_t n, i64* out) { BUFCONV_INT_SIGNED(ptr, n, out); }


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
static inline int __attribute__((always_inline)) bufconv_f32_unsigned(const char* ptr, size_t n, f32* out) { BUFCONV_FLOAT32_UNSIGNED(ptr, n, out); }

/**
 * Converts a buffer to an unsigned double (f64).
 * @see bufconv_f32_unsigned
 */
static inline int __attribute__((always_inline)) bufconv_f64_unsigned(const char* ptr, size_t n, f64* out) { BUFCONV_FLOAT64_UNSIGNED(ptr, n, out); }

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
static inline int __attribute__((always_inline)) bufconv_f32_signed(const char* ptr, size_t n, f32* out) { BUFCONV_FLOAT32_SIGNED(ptr, n, out); }

/**
 * Converts a buffer to a signed double (f64).
 * @see bufconv_f32_signed
 */
static inline int __attribute__((always_inline)) bufconv_f64_signed(const char* ptr, size_t n, f64* out) { BUFCONV_FLOAT64_SIGNED(ptr, n, out); }

#endif // COMMON_BUFCONV_H
#ifndef COMMON_ASCII_MASK_H
#define COMMON_ASCII_MASK_H

#include <stdint.h>

/**
 * A lightweight 128-bit bitmap representing a set of Standard ASCII characters.
 * This structure uses two 64-bit integers to map characters 0-127.
 * - chunks[0]: Characters 0x00 - 0x3F (0 - 63)
 * - chunks[1]: Characters 0x40 - 0x7F (64 - 127)
 * @warning This mask supports Standard ASCII (0-127) only. 
 * Attempting to add Extended ASCII characters (128-255) will cause a buffer overflow.
 */
typedef struct {
    uint64_t chunks[2];
} ascii_mask;

/**
 * Adds a single character to the mask.
 * @param mask  Pointer to the ascii_mask to modify
 * @param c     The character to add (Must be < 128)
 */
static inline void __attribute__((always_inline)) ascii_mask_add_char(
        ascii_mask* const restrict  mask,
        const uint8_t               c) {
    mask->chunks[c >> 6] |= (1ULL << (c & 63));
}

/**
 * Adds an inclusive range of characters to the mask.
 * Efficiently sets a block of bits without looping. It handles cases where
 * the range fits within a single 64-bit chunk or crosses the boundary 
 * between chunks[0] and chunks[1].
 *
 * @param mask  Pointer to the ascii_mask to modify
 * @param start The starting character of the range (inclusive)
 * @param end   The ending character of the range (inclusive)
 */
static inline void __attribute__((always_inline)) ascii_mask_add_range(
        ascii_mask* const restrict  mask,
        const uint8_t               start,
        const uint8_t               end) {
    // Chunk indices (divide by 64)
    const uint8_t start_chunk   = start >> 6;
    const uint8_t end_chunk     = end   >> 6;

    const uint64_t start_mask   = (~0ULL << (start & 63)); 
    const uint64_t end_mask     = (~0ULL >> (63 - (end & 63)));

    if (start_chunk == end_chunk) { // Range is entirely within one chunk (e.g., 'a'-'z' are both in chunk 1)
        mask->chunks[start_chunk] |= (start_mask & end_mask);
    } else {                        // Range crosses the boundary (e.g., '?' to 'z')
        mask->chunks[start_chunk] |= start_mask;
        mask->chunks[end_chunk]   |= end_mask;
    }
}

/**
 * Checks if a character exists in the mask.
 * @param mask  Pointer to the ascii_mask to query
 * @param c     The character to check
 * @return 1 if the character is present, 0 otherwise
 */
static inline int __attribute__((always_inline)) ascii_mask_check(
        const ascii_mask*   const restrict  mask,
        const uint8_t                       c) {
    return (mask->chunks[c >> 6] >> (c & 63)) & 1;
}

// =========================================================
// 1. General Character Classes (ctype.h)
// =========================================================

/**
 * Standard printable characters (ASCII 32-126) plus whitespace (\t, \n, \r).
 * Useful for sanitizing BIOS TTY logs.
 */
#define ASCII_MASK_PRINTABLE    ((ascii_mask){{ 0xffffffff00002600ULL, 0x7fffffffffffffffULL }})

/**
 * Control characters (0-31) and DEL (127).
 * Useful for rejecting invalid inputs or detecting non-printable garbage.
 */
#define ASCII_MASK_CONTROL      ((ascii_mask){{ 0x00000000ffffffffULL, 0x8000000000000000ULL }})

/**
 * Standard whitespace characters: Space (32), \t (9), \n (10), \v (11), \f (12), \r (13).
 * Essential for parsing and tokenization.
 */
#define ASCII_MASK_WHITESPACE   ((ascii_mask){{ 0x0000000100003e00ULL, 0x0000000000000000ULL }})

/**
 * Alphanumeric characters (0-9, a-z, A-Z).
 * High-performance replacement for isalnum().
 */
#define ASCII_MASK_ALPHANUMERIC ((ascii_mask){{ 0x03ff000000000000ULL, 0x07fffffe07fffffeULL }})

// =========================================================
// 2. Numeric & Data Parsing
// =========================================================

/**
 * Decimal digits (0-9).
 * Subset of Hex/Alphanumeric.
 */
#define ASCII_MASK_DIGITS       ((ascii_mask){{ 0x03ff000000000000ULL, 0x0000000000000000ULL }})

/**
 * Hexadecimal digits (0-9, a-f, A-F).
 * Useful for parsing cheat codes or immediate values.
 */
#define ASCII_MASK_HEX          ((ascii_mask){{ 0x03ff000000000000ULL, 0x0000007e0000007eULL }})

// =========================================================
// 3. Syntax & Identifiers
// =========================================================

/**
 * C-style Identifier characters (0-9, a-z, A-Z, '_').
 * Standard for config keys, symbol names, and variable tokens.
 */
#define ASCII_MASK_IDENTIFIER   ((ascii_mask){{ 0x03ff000000000000ULL, 0x07fffffe87fffffeULL }})

// =========================================================
// 4. Filesystem & Domain Specific
// =========================================================

/**
 * Path separators: Forward slash '/' and Backslash '\'.
 * Useful for cross-platform path normalization.
 */
#define ASCII_MASK_PATH_SEPS    ((ascii_mask){{ 0x0000800000000000ULL, 0x0000000010000000ULL }})

/**
 * ISO 9660 Level 1 filename characters (A-Z, 0-9, '_', '.').
 * Strict filter for PS1 CD-ROM filesystem entries.
 */
#define ASCII_MASK_ISO_9660     ((ascii_mask){{ 0x03ff400000000000ULL, 0x0000000087fffffeULL }})

#endif // COMMON_ASCII_MASK_H
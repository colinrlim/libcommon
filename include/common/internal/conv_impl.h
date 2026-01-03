#ifndef COMMON_INTERNAL_CONV_IMPL_H
#define COMMON_INTERNAL_CONV_IMPL_H

#include <common/types.h>

typedef enum CONV_RESULT {
    CONV_RESULT_SUCCESS              = 0, ///< Parsing completed successfully
    CONV_RESULT_INVALID_CHARACTER    = -1 ///< Parsing stopped due to a non-digit character
} CONV_RESULT;

// Parameterized integer parsing loop. Allows strconv (pointer increment) 
// and bufconv (index increment) to share the same core logic.
#define CONV_INT_IMPL(chr, out, on_init, on_return, while_condition, return_condition, increment)   \
    do {                                                                                            \
        on_init;                                                                                    \
        *out = 0;                                                                                   \
        while (while_condition) {                                                                   \
            const u8 c = (const u8)(chr - '0');                                                     \
            if (c > 9) break;                                                                       \
            *out = (*out * 10) + c;                                                                 \
            increment;                                                                              \
        }                                                                                           \
        on_return;                                                                                  \
        return (return_condition) ? CONV_RESULT_INVALID_CHARACTER                                   \
            : CONV_RESULT_SUCCESS;                                                                  \
    } while (0)                                                                                     \

// Extends CONV_INT_IMPL with decimal point handling for floats.
#define CONV_FLOAT_IMPL(chr, out, on_init, on_return, while_condition, return_condition, increment, T, suffix)  \
    CONV_INT_IMPL(chr, out, on_init, {                                                                          \
        if (chr == '.') {                                                                                       \
            increment;                                                                                          \
            T divisor = 10.##suffix;                                                                            \
            while (while_condition) {                                                                           \
                const u8 c = (const u8)(chr - '0');                                                             \
                if (c > 9) break;                                                                               \
                *out += c / divisor;                                                                            \
                divisor *= 10.##suffix;                                                                         \
                increment;                                                                                      \
            }                                                                                                   \
        }                                                                                                       \
        on_return;                                                                                              \
    }, while_condition, return_condition, increment)                                                            \

#endif // COMMON_INTERNAL_CONV_IMPL_H
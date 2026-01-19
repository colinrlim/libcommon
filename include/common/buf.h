#ifndef COMMON_BUF_H
#define COMMON_BUF_H

#include <common/compiler.h>
#include <stdlib.h>
#include <string.h>

/**
 * Generic dynamic buffer/vector macro.
 *
 * Creates a type-safe resizable array with amortized O(1) push.
 * Buffer doubles capacity when full (initial minimum: 8 elements).
 *
 * @param T         Element type to store
 * @param T_STRUCT  Name for the generated struct and functions
 *
 * @par Generated API:
 * @code
 * typedef struct T_STRUCT { T* data; size_t count, cap; } T_STRUCT;
 * int  T_STRUCT_init(T_STRUCT* buf, size_t cap);           // 0 on success, -1 on alloc failure
 * void T_STRUCT_free(T_STRUCT* buf);                       // Frees data, zeros struct
 * int  T_STRUCT_push(T_STRUCT* buf, T val);                // 0 on success, -1 on alloc failure
 * int  T_STRUCT_append(T_STRUCT* buf, T* src, size_t n);   // Batch append, 0 on success
 * @endcode
 */
#define DEFINE_BUFFER(T, T_STRUCT)                                                  \
    typedef struct T_STRUCT {                                                       \
        T*      data;                                                               \
        size_t  count;                                                              \
        size_t  cap;                                                                \
    } T_STRUCT;                                                                     \
                                                                                    \
    ALWAYS_INLINE int T_STRUCT##_init(T_STRUCT* buf, size_t cap) {                  \
        buf->data = (T*)malloc(cap * sizeof(T));                                    \
        if (!buf->data) return -1;                                                  \
        buf->count  = 0;                                                            \
        buf->cap    = cap;                                                          \
        return 0;                                                                   \
    }                                                                               \
                                                                                    \
    ALWAYS_INLINE void T_STRUCT##_free(T_STRUCT* buf) {                             \
        free(buf->data);                                                            \
        buf->data   = NULL;                                                         \
        buf->count  = buf->cap = 0;                                                 \
    }                                                                               \
                                                                                    \
    ALWAYS_INLINE int T_STRUCT##_grow(T_STRUCT* buf) {                              \
        size_t new_cap = buf->cap ? buf->cap * 2 : 8;                               \
        T* new_data = (T*)realloc(buf->data, new_cap * sizeof(T));                  \
        if (!new_data) return -1;                                                   \
        buf->data   = new_data;                                                     \
        buf->cap    = new_cap;                                                      \
        return 0;                                                                   \
    }                                                                               \
                                                                                    \
    ALWAYS_INLINE int T_STRUCT##_push(T_STRUCT* buf, T val) {                       \
        if (buf->count >= buf->cap && T_STRUCT##_grow(buf) < 0)                     \
            return -1;                                                              \
        buf->data[buf->count++] = val;                                              \
        return 0;                                                                   \
    }                                                                               \
                                                                                    \
    ALWAYS_INLINE int T_STRUCT##_append(T_STRUCT* buf, const T* src, size_t n) {    \
        if (n == 0) return 0;                                                       \
        const size_t required = buf->count + n;                                     \
        while (buf->cap < required) {                                               \
            if (T_STRUCT##_grow(buf) < 0) return -1;                                \
        }                                                                           \
        memcpy(buf->data + buf->count, src, n * sizeof(T));                         \
        buf->count += n;                                                            \
        return 0;                                                                   \
    }                                                                               \

#endif // COMMON_BUF_H

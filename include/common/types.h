#ifndef COMMON_TYPES_H
#define COMMON_TYPES_H

#include <stdint.h> // standard C fixed-width types
#include <stddef.h> // size_t

typedef uint8_t     u8;
typedef uint16_t    u16;
typedef uint32_t    u32;
typedef uint64_t    u64;

typedef int8_t      i8;
typedef int16_t     i16;
typedef int32_t     i32;
typedef int64_t     i64;

typedef float       f32;
typedef double      f64;

typedef struct data_buf {
    void*   data;
    size_t  size;
} data_buf;

typedef struct str_view {
    const char* ptr;
    size_t      len;
} str_view;

#endif // COMMON_TYPES_H
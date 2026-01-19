#ifndef COMMON_MAP_H
#define COMMON_MAP_H

#include <common/types.h>
#include <common/compiler.h>
#include <common/hash.h>
#include <stdlib.h>

/**
 * Generic hash map with str_view keys.
 *
 * Uses open addressing with linear probing. Grows at 70% load factor.
 *
 * @param T         Value type to store
 * @param T_STRUCT  Name for the generated struct and functions
 *
 * @par Generated API:
 * @code
 * typedef struct T_STRUCT { ... } T_STRUCT;
 * int  T_STRUCT_init(T_STRUCT* map, size_t capacity);      // 0 on success, -1 on alloc failure
 * void T_STRUCT_free(T_STRUCT* map);                       // Frees entries, zeros struct
 * T*   T_STRUCT_get(T_STRUCT* map, str_view key);          // Returns pointer to value or NULL
 * int  T_STRUCT_set(T_STRUCT* map, str_view key, T val);   // 0 on success, -1 on alloc failure
 * @endcode
 */
#define DEFINE_MAP(T, T_STRUCT)                                                 \
    typedef struct T_STRUCT##_entry {                                           \
        u32         hash;                                                       \
        str_view    key;                                                        \
        T           val;                                                        \
    } T_STRUCT##_entry;                                                         \
                                                                                \
    typedef struct T_STRUCT {                                                   \
        T_STRUCT##_entry*   entries;                                            \
        size_t              capacity;                                           \
        size_t              count;                                              \
    } T_STRUCT;                                                                 \
                                                                                \
    ALWAYS_INLINE int T_STRUCT##_init(T_STRUCT* map, size_t capacity) {         \
        map->entries = (T_STRUCT##_entry*)calloc(                               \
            capacity,                                                           \
            sizeof(T_STRUCT##_entry));                                          \
        if (!map->entries) return -1;                                           \
        map->capacity   = capacity;                                             \
        map->count      = 0;                                                    \
        return 0;                                                               \
    }                                                                           \
                                                                                \
    ALWAYS_INLINE void T_STRUCT##_free(T_STRUCT* map) {                         \
        free(map->entries);                                                     \
        map->entries    = NULL;                                                 \
        map->capacity   = map->count = 0;                                       \
    }                                                                           \
                                                                                \
    ALWAYS_INLINE int T_STRUCT##_grow(T_STRUCT* map) {                          \
        size_t new_cap = map->capacity * 2;                                     \
        T_STRUCT##_entry* new_entries = (T_STRUCT##_entry*)calloc(              \
                new_cap,                                                        \
                sizeof(T_STRUCT##_entry));                                      \
        if (!new_entries) return -1;                                            \
                                                                                \
        for (size_t i = 0; i < map->capacity; i++) {                            \
            if (map->entries[i].hash != 0) {                                    \
                size_t slot = map->entries[i].hash & (new_cap - 1);             \
                while (new_entries[slot].hash != 0)                             \
                    slot = (slot + 1) & (new_cap - 1);                          \
                new_entries[slot] = map->entries[i];                            \
            }                                                                   \
        }                                                                       \
                                                                                \
        free(map->entries);                                                     \
        map->entries    = new_entries;                                          \
        map->capacity   = new_cap;                                              \
        return 0;                                                               \
    }                                                                           \
                                                                                \
    ALWAYS_INLINE T* T_STRUCT##_get(T_STRUCT* map, str_view key) {              \
        if (map->count == 0) return NULL;                                       \
                                                                                \
        u32 h       = str_hash(key);                                            \
        size_t slot = h & (map->capacity - 1);                                  \
                                                                                \
        while (map->entries[slot].hash != 0) {                                  \
            if (map->entries[slot].hash == h &&                                 \
                str_eq(map->entries[slot].key, key)) {                          \
                return &map->entries[slot].val;                                 \
            }                                                                   \
            slot = (slot + 1) & (map->capacity - 1);                            \
        }                                                                       \
        return NULL;                                                            \
    }                                                                           \
                                                                                \
    ALWAYS_INLINE int T_STRUCT##_set(T_STRUCT* map, str_view key, T val) {      \
        if (map->count * 10 >= map->capacity * 7 && T_STRUCT##_grow(map) < 0)   \
            return -1;                                                          \
                                                                                \
        u32 h       = str_hash(key);                                            \
        size_t slot = h & (map->capacity - 1);                                  \
                                                                                \
        while (map->entries[slot].hash != 0) {                                  \
            if (map->entries[slot].hash == h &&                                 \
                str_eq(map->entries[slot].key, key)) {                          \
                map->entries[slot].val = val;                                   \
                return 0;                                                       \
            }                                                                   \
            slot = (slot + 1) & (map->capacity - 1);                            \
        }                                                                       \
                                                                                \
        map->entries[slot].hash = h;                                            \
        map->entries[slot].key  = key;                                          \
        map->entries[slot].val  = val;                                          \
        map->count++;                                                           \
        return 0;                                                               \
    }                                                                           \

#endif // COMMON_MAP_H
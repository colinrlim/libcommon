#ifndef COMMON_SYMBOL_TABLE_H
#define COMMON_SYMBOL_TABLE_H

#include <common/types.h>
#include <common/compiler.h>
#include <common/hash.h>
#include <stdlib.h>

/**
 * Generic scoped symbol table macro.
 *
 * Creates a type-safe hash table with scoped shadowing support.
 * Uses open-addressing with linear probing. Shadow stack enables
 * proper scoping by saving/restoring previous definitions when
 * entering/exiting scopes.
 *
 * @param T         Value type to store
 * @param T_STRUCT  Name for the generated struct and functions
 *
 * @par Generated Types:
 * @code
 * typedef struct T_STRUCT { ... } T_STRUCT;
 * @endcode
 *
 * @par Generated API:
 * @code
 * int  T_STRUCT_init(T_STRUCT* t);                         // 0 on success, -1 on alloc failure
 * void T_STRUCT_free(T_STRUCT* t);                         // Frees all memory, zeros struct
 * T*   T_STRUCT_lookup(T_STRUCT* t, str_view key);         // Returns pointer to value or NULL
 * int  T_STRUCT_insert(T_STRUCT* t, str_view key, T val);  // 0 on success, -1 on alloc failure
 * int  T_STRUCT_push_scope(T_STRUCT* t);                   // Enter new scope, 0 on success
 * void T_STRUCT_pop_scope(T_STRUCT* t);                    // Exit scope, restore shadowed entries
 * @endcode
 *
 * @par Example:
 * @code
 * typedef struct my_def {
 *     int id;
 *     int value;
 * } my_def;
 *
 * DEFINE_SYMBOL_TABLE(my_def, my_table)
 *
 * my_table t;
 * my_table_init(&t);
 * my_table_insert(&t, (str_view){"foo", 3}, (my_def){1, 42});
 * my_def* found = my_table_lookup(&t, (str_view){"foo", 3});
 * my_table_free(&t);
 * @endcode
 */
#define DEFINE_SYMBOL_TABLE(T, T_STRUCT)                                            \
                                                                                    \
typedef struct T_STRUCT##_entry {                                                   \
    u32         hash;                                                               \
    str_view    key;                                                                \
    T           value;                                                              \
} T_STRUCT##_entry;                                                                 \
                                                                                    \
typedef struct T_STRUCT##_shadow {                                                  \
    u32                 hash;                                                       \
    size_t              slot;                                                       \
    T_STRUCT##_entry    prev;                                                       \
    size_t              scope_depth;                                                \
} T_STRUCT##_shadow;                                                                \
                                                                                    \
typedef struct T_STRUCT {                                                           \
    T_STRUCT##_entry*   entries;                                                    \
    size_t              capacity;                                                   \
    size_t              count;                                                      \
    T_STRUCT##_shadow*  shadows;                                                    \
    size_t              shadow_count;                                               \
    size_t              shadow_cap;                                                 \
    size_t              scope_depth;                                                \
} T_STRUCT;                                                                         \
                                                                                    \
ALWAYS_INLINE int T_STRUCT##_grow(T_STRUCT* t) {                                    \
    const size_t new_cap = t->capacity * 2;                                         \
    T_STRUCT##_entry* const new_entries = (T_STRUCT##_entry*)calloc(                \
            new_cap,                                                                \
            sizeof(T_STRUCT##_entry));                                              \
    if (!new_entries) return -1;                                                    \
                                                                                    \
    for (size_t i = 0; i < t->capacity; i++) {                                      \
        if (t->entries[i].hash != 0) {                                              \
            size_t slot = t->entries[i].hash & (new_cap - 1);                       \
            while (new_entries[slot].hash != 0)                                     \
                slot = (slot + 1) & (new_cap - 1);                                  \
            new_entries[slot] = t->entries[i];                                      \
        }                                                                           \
    }                                                                               \
                                                                                    \
    free(t->entries);                                                               \
    t->entries  = new_entries;                                                      \
    t->capacity = new_cap;                                                          \
    return 0;                                                                       \
}                                                                                   \
                                                                                    \
ALWAYS_INLINE int T_STRUCT##_grow_shadows(T_STRUCT* t) {                            \
    const size_t new_cap = t->shadow_cap ? t->shadow_cap * 2 : 16;                  \
    T_STRUCT##_shadow* const new_shadows = (T_STRUCT##_shadow*)realloc(             \
            t->shadows,                                                             \
            new_cap * sizeof(T_STRUCT##_shadow));                                   \
    if (!new_shadows) return -1;                                                    \
    t->shadows      = new_shadows;                                                  \
    t->shadow_cap   = new_cap;                                                      \
    return 0;                                                                       \
}                                                                                   \
                                                                                    \
ALWAYS_INLINE int T_STRUCT##_init(T_STRUCT* t) {                                    \
    t->entries = (T_STRUCT##_entry*)calloc(64, sizeof(T_STRUCT##_entry));           \
    if (!t->entries) return -1;                                                     \
    t->capacity     = 64;                                                           \
    t->count        = 0;                                                            \
    t->shadows      = NULL;                                                         \
    t->shadow_count = 0;                                                            \
    t->shadow_cap   = 0;                                                            \
    t->scope_depth  = 0;                                                            \
    return 0;                                                                       \
}                                                                                   \
                                                                                    \
ALWAYS_INLINE void T_STRUCT##_free(T_STRUCT* t) {                                   \
    free(t->entries);                                                               \
    free(t->shadows);                                                               \
    t->entries      = NULL;                                                         \
    t->shadows      = NULL;                                                         \
    t->capacity     = 0;                                                            \
    t->count        = 0;                                                            \
    t->shadow_count = 0;                                                            \
    t->shadow_cap   = 0;                                                            \
    t->scope_depth  = 0;                                                            \
}                                                                                   \
                                                                                    \
ALWAYS_INLINE T* T_STRUCT##_lookup(T_STRUCT* t, str_view key) {                     \
    if (t->count == 0) return NULL;                                                 \
                                                                                    \
    const u32 h = str_hash(key);                                                    \
    size_t slot = h & (t->capacity - 1);                                            \
                                                                                    \
    while (t->entries[slot].hash != 0) {                                            \
        if (t->entries[slot].hash == h && str_eq(t->entries[slot].key, key))        \
            return &t->entries[slot].value;                                         \
        slot = (slot + 1) & (t->capacity - 1);                                      \
    }                                                                               \
    return NULL;                                                                    \
}                                                                                   \
                                                                                    \
ALWAYS_INLINE int T_STRUCT##_insert(T_STRUCT* t, str_view key, T value) {           \
    if (t->count * 10 >= t->capacity * 7 && T_STRUCT##_grow(t) < 0)                 \
        return -1;                                                                  \
                                                                                    \
    const u32 h = str_hash(key);                                                    \
    size_t slot = h & (t->capacity - 1);                                            \
                                                                                    \
    while (t->entries[slot].hash != 0) {                                            \
        if (t->entries[slot].hash == h &&                                           \
            str_eq(t->entries[slot].key, key)) {                                    \
            /* Key exists - shadow the previous entry */                            \
            if (t->shadow_count >= t->shadow_cap && T_STRUCT##_grow_shadows(t) < 0) \
                return -1;                                                          \
            const T_STRUCT##_shadow shadow = {                                      \
                .hash           = h,                                                \
                .slot           = slot,                                             \
                .prev           = t->entries[slot],                                 \
                .scope_depth    = t->scope_depth                                    \
            };                                                                      \
            t->shadows[t->shadow_count++] = shadow;                                 \
            t->entries[slot].value  = value;                                        \
            t->entries[slot].key    = key;                                          \
            return 0;                                                               \
        }                                                                           \
        slot = (slot + 1) & (t->capacity - 1);                                      \
    }                                                                               \
                                                                                    \
    t->entries[slot].hash   = h;                                                    \
    t->entries[slot].key    = key;                                                  \
    t->entries[slot].value  = value;                                                \
    t->count++;                                                                     \
    return 0;                                                                       \
}                                                                                   \
                                                                                    \
ALWAYS_INLINE int T_STRUCT##_push_scope(T_STRUCT* t) {                              \
    t->scope_depth++;                                                               \
    return 0;                                                                       \
}                                                                                   \
                                                                                    \
ALWAYS_INLINE void T_STRUCT##_pop_scope(T_STRUCT* t) {                              \
    while (t->shadow_count > 0) {                                                   \
        const T_STRUCT##_shadow* const top = &t->shadows[t->shadow_count - 1];      \
        if (top->scope_depth != t->scope_depth) break;                              \
                                                                                    \
        if (top->prev.hash == 0) {                                                  \
            t->entries[top->slot].hash = 0;                                         \
            t->count--;                                                             \
        } else                                                                      \
            t->entries[top->slot] = top->prev;                                      \
        t->shadow_count--;                                                          \
    }                                                                               \
    t->scope_depth--;                                                               \
}

#endif // COMMON_SYMBOL_TABLE_H
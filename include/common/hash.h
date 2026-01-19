#ifndef COMMON_HASH_H
#define COMMON_HASH_H

#include <common/types.h>
#include <common/compiler.h>
#include <string.h>

/**
 * Computes FNV-1a hash for a string view.
 *
 * Uses the 32-bit FNV-1a algorithm with standard prime and offset basis.
 * Returns minimum value of 1 since 0 is reserved for empty slots in
 * hash tables using open addressing.
 *
 * @param s  String view to hash
 * @return Non-zero 32-bit hash value (minimum 1)
 */
ALWAYS_INLINE u32 str_hash(str_view s) {
    u32 h = 2166136261u;
    for (size_t i = 0; i < s.len; i++) {
        h ^= (u8)s.ptr[i];
        h *= 16777619u;
    }
    return h ? h : 1;
}

/**
 * Compares two string views for equality.
 * @param a  First string view
 * @param b  Second string view
 * @return 1 if equal, 0 otherwise
 */
ALWAYS_INLINE int str_eq(str_view a, str_view b) {
    return a.len == b.len && memcmp(a.ptr, b.ptr, a.len) == 0;
}

#endif // COMMON_HASH_H

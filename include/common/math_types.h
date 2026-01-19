#ifndef COMMON_MATH_TYPES_H
#define COMMON_MATH_TYPES_H

#include <common/compiler.h>
#include <math.h> // sqrtf, acosf, etc.

/* --- Constants --- */
#define MATH_PI         3.14159265358979323846
#define MATH_HALF_PI    1.57079632679489661923
#define MATH_QUARTER_PI 0.78539816339744830962
#define MATH_TWO_PI     6.28318530717958647692
#define MATH_TAU        MATH_TWO_PI
#define MATH_EPSILON    1.19209290e-7

/* --- Components --- */
#define MATH_VEC_COMPONENTS_1(T)    \
    struct { T x; };                \
    struct { T r; };                \
    struct { T s; };                \

#define MATH_VEC_COMPONENTS_2(T)    \
    struct { T x, y; };             \
    struct { T r, g; };             \
    struct { T s, t; };             \

#define MATH_VEC_COMPONENTS_3(T)    \
    struct { T x, y, z; };          \
    struct { T r, g, b; };          \
    struct { T s, t, p; };          \

#define MATH_VEC_COMPONENTS_4(T)    \
    struct { T x, y, z, w; };       \
    struct { T r, g, b, a; };       \
    struct { T s, t, p, q; };       \

/* --- Type Definition Macro --- */

/**
 * Generates a vector struct and its associated functions.
 * @param T         The primitive type (float, double)
 * @param T_STRUCT  The name of the generated type (e.g., vec3f)
 * @param N The component count (1-4)
 *
 * @par Example Expansion for DEFINE_MATH_VEC(float, vec3f, 3):
 * @code
 * typedef union vec3f {
 *     float data[3];               // Array access:    v.data[0]
 *     struct { float x, y, z; };   // Position access: v.x
 *     struct { float r, g, b; };   // Color access:    v.r
 *     struct { float s, t, p; };   // Texture access:  v.s
 * } vec3f;
 *
 * // Generated Functions (Passed by VALUE for registers):
 * vec3f vec3f_add(vec3f a, vec3f b);       // Component-wise Add
 * vec3f vec3f_sub(vec3f a, vec3f b);       // Component-wise Sub
 * vec3f vec3f_mul(vec3f a, vec3f b);       // Component-wise Mul
 * vec3f vec3f_div(vec3f a, vec3f b);       // Component-wise Div
 * vec3f vec3f_adds(vec3f a, float s);      // Scalar Add
 * vec3f vec3f_muls(vec3f a, float s);      // Scalar Mul
 * float vec3f_dot(vec3f a, vec3f b);       // Dot Product
 * float vec3f_sum(vec3f a);                // Sum of components
 * @endcode
 */
#define DEFINE_MATH_VEC(T, T_STRUCT, N) \
typedef union ArgVector_##T##_##N {     \
    T data[N];                          \
    MATH_VEC_COMPONENTS_##N(T)          \
} ArgVector_##T##_##N;                  \
typedef ArgVector_##T##_##N T_STRUCT;   \
                                        \
_MATH_VEC_GEN_VEC_OP(T_STRUCT, add, +, N)       \
_MATH_VEC_GEN_VEC_OP(T_STRUCT, sub, -, N)       \
_MATH_VEC_GEN_VEC_OP(T_STRUCT, mul, *, N)       \
_MATH_VEC_GEN_VEC_OP(T_STRUCT, div, /, N)       \
                                                \
_MATH_VEC_GEN_SCA_OP(T, T_STRUCT, add, +, N)    \
_MATH_VEC_GEN_SCA_OP(T, T_STRUCT, sub, -, N)    \
_MATH_VEC_GEN_SCA_OP(T, T_STRUCT, mul, *, N)    \
_MATH_VEC_GEN_SCA_OP(T, T_STRUCT, div, /, N)    \
                                                \
ALWAYS_INLINE T_STRUCT##_sum(T_STRUCT vec) {            \
    T result;                                           \
    _MATH_VEC_SUM_##N(vec, result);                     \
    return result;                                      \
}                                                       \
ALWAYS_INLINE T_STRUCT##_dot(T_STRUCT a, T_STRUCT b) {  \
    T_STRUCT temp = T_STRUCT##_mul(a, b);               \
    return T_STRUCT##_sum(temp);                        \
}                                                       \

/* --- Internal Generator Macros --- */

#define _MATH_VEC_GEN_VEC_OP(T_STRUCT, name, op, N)                 \
MATH_INLINE(T_STRUCT) T_STRUCT##_##name(T_STRUCT a, T_STRUCT b) {   \
    T_STRUCT result;                                                \
    _MATH_VEC_OP_VEC_##N(a, op, b, result);                         \
    return result;                                                  \
}                                                                   \

#define _MATH_VEC_GEN_SCA_OP(T, T_STRUCT, name, op, N)              \
MATH_INLINE(T_STRUCT) T_STRUCT##_##name##s(T_STRUCT a, T b) {       \
    T_STRUCT result;                                                \
    _MATH_VEC_OP_SCALAR_##N(a, op, b, result);                      \
    return result;                                                  \
}                                                                   \

/* --- Internal Operations --- */

#define _MATH_VEC_OP_VEC_1(a, op, b, out) \
    (out).x = (a).x op (b).x;             \

#define _MATH_VEC_OP_VEC_2(a, op, b, out) \
    _MATH_VEC_OP_VEC_1(a, op, b, out)     \
    (out).y = (a).y op (b).y;             \

#define _MATH_VEC_OP_VEC_3(a, op, b, out) \
    _MATH_VEC_OP_VEC_2(a, op, b, out)     \
    (out).z = (a).z op (b).z;             \

#define _MATH_VEC_OP_VEC_4(a, op, b, out) \
    _MATH_VEC_OP_VEC_3(a, op, b, out)     \
    (out).w = (a).w op (b).w;             \

#define _MATH_VEC_OP_SCALAR_1(a, op, b, out) \
    (out).x = (a).x op (b);                  \

#define _MATH_VEC_OP_SCALAR_2(a, op, b, out) \
    _MATH_VEC_OP_SCALAR_1(a, op, b, out)     \
    (out).y = (a).y op (b);                  \

#define _MATH_VEC_OP_SCALAR_3(a, op, b, out) \
    _MATH_VEC_OP_SCALAR_2(a, op, b, out)     \
    (out).z = (a).z op (b);                  \

#define _MATH_VEC_OP_SCALAR_4(a, op, b, out) \
    _MATH_VEC_OP_SCALAR_3(a, op, b, out)     \
    (out).w = (a).w op (b);                  \

#define _MATH_VEC_SUM_1(vec, out) \
    (out) = (vec).x;              \

#define _MATH_VEC_SUM_2(vec, out) \
    _MATH_VEC_SUM_1(vec, out)     \
    (out) += (vec).y;             \

#define _MATH_VEC_SUM_3(vec, out) \
    _MATH_VEC_SUM_2(vec, out)     \
    (out) += (vec).z;             \

#define _MATH_VEC_SUM_4(vec, out) \
    _MATH_VEC_SUM_3(vec, out)     \
    (out) += (vec).w;             \

/* --- Instantiations --- */
DEFINE_MATH_VEC(float, vec1f, 1)
DEFINE_MATH_VEC(float, vec2f, 2)
DEFINE_MATH_VEC(float, vec3f, 3)
DEFINE_MATH_VEC(float, vec4f, 4)

DEFINE_MATH_VEC(double, vec1d, 1)
DEFINE_MATH_VEC(double, vec2d, 2)
DEFINE_MATH_VEC(double, vec3d, 3)
DEFINE_MATH_VEC(double, vec4d, 4)

#endif // COMMON_MATH_TYPES_H
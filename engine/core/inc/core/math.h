#pragma once

#include <math.h>
#include <stdint.h>

#ifdef WIN32
#define MATH_INLINE __forceinline
#else
#define MATH_INLINE static inline __attribute__((always_inline))
#endif

typedef uint32_t prng_t[1];
#define COG_PRNG(s) {s}

MATH_INLINE
uint32_t cogRandomu(prng_t prng)
{
    uint32_t next = ((prng[0]) * 0x7FFFFFEDu + 0x7FFFFFC3u) % 0xFFFFFFFFu;
    prng[0] = next;
    return next;
}

MATH_INLINE
float cogRandomf(prng_t prng)
{
    return cogRandomu(prng) / ((float)(0xFFFFFFFFu));
}

typedef float vec3f_t[3];
typedef float vec4f_t[4];
typedef float mat4f_t[4][4];

typedef vec4f_t quat_t;

#define cogDeg2Rad(d) (d*0.0174532925f)

MATH_INLINE
void cogVec3fSub(vec3f_t dest, const vec3f_t a, const vec3f_t b)
{
    dest[0] = a[0] - b[0];
    dest[1] = a[1] - b[1];
    dest[2] = a[2] - b[2];
}

MATH_INLINE
void cogVec3fCross(vec3f_t dest, const vec3f_t a, const vec3f_t b)
{
    dest[0] = a[1] * b[2] - a[2] * b[1];
    dest[1] = a[2] * b[0] - a[0] * b[2];
    dest[2] = a[0] * b[1] - a[1] * b[0];
}

MATH_INLINE
float cogVec3fDot(const vec3f_t a, const vec3f_t b)
{
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

MATH_INLINE
void cogVec3fNormalize(vec3f_t dest)
{
    float divisor = 1.f / sqrtf(cogVec3fDot(dest, dest));
    dest[0] *= divisor;
    dest[1] *= divisor;
    dest[2] *= divisor;
}

#define COG_MAT4_IDENTITY \
{\
    {1.f, 0.f, 0.f, 0.f},\
    {0.f, 1.f, 0.f, 0.f},\
    {0.f, 0.f, 1.f, 0.f},\
    {0.f, 0.f, 0.f, 1.f}\
}

MATH_INLINE
void cogMat4fIdentity(mat4f_t dest)
{
    dest[0][0] = 1.f, dest[0][1] = 0.f, dest[0][2] = 0.f, dest[0][3] = 0.f;
    dest[1][0] = 0.f, dest[1][1] = 1.f, dest[1][2] = 0.f, dest[1][3] = 0.f;
    dest[2][0] = 0.f, dest[2][1] = 0.f, dest[2][2] = 1.f, dest[2][3] = 0.f;
    dest[3][0] = 0.f, dest[3][1] = 0.f, dest[3][2] = 0.f, dest[3][3] = 1.f;
}

MATH_INLINE
void cogMat4fPerspective(mat4f_t dest, float fov, float ar, float near, float far)
{
    float tmp = 1.f / (far - near);
    float ctanFOV = 1.f / tanf(fov * 0.5f);
    dest[0][0] = ctanFOV;
    dest[1][1] = ctanFOV / ar;
    dest[2][2] = -(far + near) * tmp;
    dest[2][3] = -1.f;
    dest[3][2] = -2.f * far * near * tmp;
}

MATH_INLINE
void cogQuatInit(quat_t dest, float x, float y, float z, float angle)
{
    float a = 0.5f * angle;
    float sa = sinf(a);
    dest[0] = sa * x;
    dest[1] = sa * y;
    dest[2] = sa * z;
    dest[3] = cosf(a);
}

MATH_INLINE
void cogQuatMul(quat_t dest, quat_t a, quat_t b)
{
    dest[0] = b[0] * a[0] - b[1] * a[1] - b[2] * a[2] - b[3] * a[3];
    dest[1] = b[0] * a[1] + b[1] * a[0] - b[2] * a[3] + b[3] * a[2];
    dest[2] = b[0] * a[2] + b[1] * a[3] + b[2] * a[0] - b[3] * a[1];
    dest[3] = b[0] * a[3] - b[1] * a[2] + b[2] * a[1] + b[3] * a[0];
}

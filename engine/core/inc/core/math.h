#pragma once

#include <math.h>
#include <stdint.h>

#include "coredefs.h"

typedef uint32_t prng_t[1];
#define MATH_PRNG(s) {s}

FORCE_INLINE
uint32_t mathRandomu(prng_t prng)
{
    uint32_t next = ((prng[0]) * 0x7FFFFFEDu + 0x7FFFFFC3u) % 0xFFFFFFFFu;
    prng[0] = next;
    return next;
}

FORCE_INLINE
float mathRandomf(prng_t prng)
{
    return mathRandomu(prng) / ((float)(0xFFFFFFFFu));
}

typedef int32_t vec2i_t[2];
typedef int32_t vec3i_t[3];
typedef int32_t vec4i_t[4];

typedef uint32_t vec2u_t[2];
typedef uint32_t vec3u_t[3];
typedef uint32_t vec4u_t[4];

typedef float vec2f_t[2];
typedef float vec3f_t[3];
typedef float vec4f_t[4];

#define VEC2_MOV(dv, sv) do{dv[0]=sv[0];dv[1]=sv[1];}while(0)

typedef float mat4f_t[4][4];

typedef vec4f_t quat_t;

#define mathDeg2Rad(d) (d*0.0174532925f)

FORCE_INLINE
void mathVec2fSub(vec2f_t dest, const vec2f_t a, const vec2f_t b)
{
	dest[0] = a[0] - b[0];
	dest[1] = a[1] - b[1];
}

FORCE_INLINE
void mathVec3fSub(vec3f_t dest, const vec3f_t a, const vec3f_t b)
{
    dest[0] = a[0] - b[0];
    dest[1] = a[1] - b[1];
    dest[2] = a[2] - b[2];
}

FORCE_INLINE
void mathVec3fCross(vec3f_t dest, const vec3f_t a, const vec3f_t b)
{
    dest[0] = a[1] * b[2] - a[2] * b[1];
    dest[1] = a[2] * b[0] - a[0] * b[2];
    dest[2] = a[0] * b[1] - a[1] * b[0];
}

FORCE_INLINE
float mathVec2fDot(const vec2f_t a, const vec2f_t b)
{
	return a[0] * b[0] + a[1] * b[1];
}

FORCE_INLINE
float mathVec3fDot(const vec3f_t a, const vec3f_t b)
{
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

FORCE_INLINE
float mathVec2fLength(const vec2f_t a)
{
	return sqrtf(mathVec2fDot(a, a));
}

FORCE_INLINE
void mathVec3fNormalize(vec3f_t dest)
{
    float divisor = 1.f / sqrtf(mathVec3fDot(dest, dest));
    dest[0] *= divisor;
    dest[1] *= divisor;
    dest[2] *= divisor;
}

#define math_MAT4_IDENTITY \
{\
    {1.f, 0.f, 0.f, 0.f},\
    {0.f, 1.f, 0.f, 0.f},\
    {0.f, 0.f, 1.f, 0.f},\
    {0.f, 0.f, 0.f, 1.f}\
}

FORCE_INLINE
void mathMat4fIdentity(mat4f_t dest)
{
    dest[0][0] = 1.f, dest[0][1] = 0.f, dest[0][2] = 0.f, dest[0][3] = 0.f;
    dest[1][0] = 0.f, dest[1][1] = 1.f, dest[1][2] = 0.f, dest[1][3] = 0.f;
    dest[2][0] = 0.f, dest[2][1] = 0.f, dest[2][2] = 1.f, dest[2][3] = 0.f;
    dest[3][0] = 0.f, dest[3][1] = 0.f, dest[3][2] = 0.f, dest[3][3] = 1.f;
}

FORCE_INLINE
void mathMat4fPerspective(mat4f_t dest, float fov, float ar, float zNear, float zFar)
{
    float tmp = 1.f / (zFar - zNear);
    float ctanFOV = 1.f / tanf(fov * 0.5f);
    dest[0][0] = ctanFOV;
    dest[1][1] = ctanFOV / ar;
    dest[2][2] = -(zFar + zNear) * tmp;
    dest[2][3] = -1.f;
    dest[3][2] = -2.f * zFar * zNear * tmp;
}

FORCE_INLINE
void mathQuatInit(quat_t dest, float x, float y, float z, float angle)
{
    float a = 0.5f * angle;
    float sa = sinf(a);
    dest[0] = sa * x;
    dest[1] = sa * y;
    dest[2] = sa * z;
    dest[3] = cosf(a);
}

FORCE_INLINE
void mathQuatMul(quat_t dest, quat_t a, quat_t b)
{
    dest[0] = b[0] * a[0] - b[1] * a[1] - b[2] * a[2] - b[3] * a[3];
    dest[1] = b[0] * a[1] + b[1] * a[0] - b[2] * a[3] + b[3] * a[2];
    dest[2] = b[0] * a[2] + b[1] * a[3] + b[2] * a[0] - b[3] * a[1];
    dest[3] = b[0] * a[3] - b[1] * a[2] + b[2] * a[1] + b[3] * a[0];
}

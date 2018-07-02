#pragma once

#ifdef _cplusplus
extern "C"
{
#endif

#include <math.h>
#include <stdint.h>

#include "coredefs.h"
    
#define PROC(c) { c ;}
#define FUNC(c) {return (c);}
    
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

typedef float mat4f_t[4][4];
    
///////////////////////////////////////////////////////////////////////////////////
#define vec2_dot_impl(a,b) (a[0]*b[0]+a[1]*b[1])
#define vec3_dot_impl(a,b) (vec2_dot_impl(a,b)+a[2]*b[2])
#define vec4_dot_impl(a,b) (vec3_dot_impl(a,b)+a[3]*b[3])
FORCE_INLINE float    vec2f_dot(const vec2f_t a, const vec2f_t b) FUNC(vec2_dot_impl(a,b))
FORCE_INLINE int32_t  vec2i_dot(const vec2i_t a, const vec2i_t b) FUNC(vec2_dot_impl(a,b))
FORCE_INLINE uint32_t vec2u_dot(const vec2u_t a, const vec2u_t b) FUNC(vec2_dot_impl(a,b))
FORCE_INLINE float    vec3f_dot(const vec3f_t a, const vec3f_t b) FUNC(vec3_dot_impl(a,b))
FORCE_INLINE int32_t  vec3i_dot(const vec3i_t a, const vec3i_t b) FUNC(vec3_dot_impl(a,b))
FORCE_INLINE uint32_t vec3u_dot(const vec3u_t a, const vec3u_t b) FUNC(vec3_dot_impl(a,b))
FORCE_INLINE float    vec4f_dot(const vec4f_t a, const vec4f_t b) FUNC(vec4_dot_impl(a,b))
FORCE_INLINE int32_t  vec4i_dot(const vec4i_t a, const vec4i_t b) FUNC(vec4_dot_impl(a,b))
FORCE_INLINE uint32_t vec4u_dot(const vec4u_t a, const vec4u_t b) FUNC(vec4_dot_impl(a,b))
    
#define vec2_add_impl(d,a,b) do { d[0]=a[0]+b[0]; d[1]=a[1]+b[1]; } while(0)
#define vec3_add_impl(d,a,b) do { vec2_add_impl(d,a,b);d[2]=a[2]+b[2]; } while(0)
#define vec4_add_impl(d,a,b) do { vec3_add_impl(d,a,b);d[3]=a[3]+b[3]; } while(0)
FORCE_INLINE void vec2f_add(vec2f_t dest, const vec2f_t a, const vec2f_t b) PROC(vec2_add_impl(dest, a, b))
FORCE_INLINE void vec2i_add(vec2i_t dest, const vec2i_t a, const vec2i_t b) PROC(vec2_add_impl(dest, a, b))
FORCE_INLINE void vec2u_add(vec2u_t dest, const vec2u_t a, const vec2u_t b) PROC(vec2_add_impl(dest, a, b))
FORCE_INLINE void vec3f_add(vec3f_t dest, const vec3f_t a, const vec3f_t b) PROC(vec3_add_impl(dest, a, b))
FORCE_INLINE void vec3i_add(vec3i_t dest, const vec3i_t a, const vec3i_t b) PROC(vec3_add_impl(dest, a, b))
FORCE_INLINE void vec3u_add(vec3u_t dest, const vec3u_t a, const vec3u_t b) PROC(vec3_add_impl(dest, a, b))
FORCE_INLINE void vec4f_add(vec4f_t dest, const vec4f_t a, const vec4f_t b) PROC(vec4_add_impl(dest, a, b))
FORCE_INLINE void vec4i_add(vec4i_t dest, const vec4i_t a, const vec4i_t b) PROC(vec4_add_impl(dest, a, b))
FORCE_INLINE void vec4u_add(vec4u_t dest, const vec4u_t a, const vec4u_t b) PROC(vec4_add_impl(dest, a, b))
    
#define vec2_adds_impl(d,a,b) do { d[0]=a[0]+b; d[1]=a[1]+b; } while(0)
#define vec3_adds_impl(d,a,b) do { vec2_adds_impl(d,a,b);d[2]=a[2]+b; } while(0)
#define vec4_adds_impl(d,a,b) do { vec3_adds_impl(d,a,b);d[3]=a[3]+b; } while(0)
FORCE_INLINE void vec2f_adds(vec2f_t dest, const vec2f_t a, float b) PROC(vec2_adds_impl(dest, a, b))
FORCE_INLINE void vec2i_adds(vec2i_t dest, const vec2i_t a, int32_t b) PROC(vec2_adds_impl(dest, a, b))
FORCE_INLINE void vec2u_adds(vec2u_t dest, const vec2u_t a, uint32_t b) PROC(vec2_adds_impl(dest, a, b))
FORCE_INLINE void vec3f_adds(vec3f_t dest, const vec3f_t a, float b) PROC(vec3_adds_impl(dest, a, b))
FORCE_INLINE void vec3i_adds(vec3i_t dest, const vec3i_t a, int32_t b) PROC(vec3_adds_impl(dest, a, b))
FORCE_INLINE void vec3u_adds(vec3u_t dest, const vec3u_t a, uint32_t b) PROC(vec3_adds_impl(dest, a, b))
FORCE_INLINE void vec4f_adds(vec4f_t dest, const vec4f_t a, float b) PROC(vec4_adds_impl(dest, a, b))
FORCE_INLINE void vec4i_adds(vec4i_t dest, const vec4i_t a, int32_t b) PROC(vec4_adds_impl(dest, a, b))
FORCE_INLINE void vec4u_adds(vec4u_t dest, const vec4u_t a, uint32_t b) PROC(vec4_adds_impl(dest, a, b))
    
#define vec2_sub_impl(d,a,b) do { d[0]=a[0]-b[0]; d[1]=a[1]-b[1]; } while(0)
#define vec3_sub_impl(d,a,b) do { vec2_sub_impl(d,a,b);d[2]=a[2]-b[2]; } while(0)
#define vec4_sub_impl(d,a,b) do { vec3_sub_impl(d,a,b);d[3]=a[3]-b[3]; } while(0)
FORCE_INLINE void vec2f_sub(vec2f_t dest, const vec2f_t a, const vec2f_t b) PROC(vec2_sub_impl(dest, a, b))
FORCE_INLINE void vec2i_sub(vec2i_t dest, const vec2i_t a, const vec2i_t b) PROC(vec2_sub_impl(dest, a, b))
FORCE_INLINE void vec2u_sub(vec2u_t dest, const vec2u_t a, const vec2u_t b) PROC(vec2_sub_impl(dest, a, b))
FORCE_INLINE void vec3f_sub(vec3f_t dest, const vec3f_t a, const vec3f_t b) PROC(vec3_sub_impl(dest, a, b))
FORCE_INLINE void vec3i_sub(vec3i_t dest, const vec3i_t a, const vec3i_t b) PROC(vec3_sub_impl(dest, a, b))
FORCE_INLINE void vec3u_sub(vec3u_t dest, const vec3u_t a, const vec3u_t b) PROC(vec3_sub_impl(dest, a, b))
FORCE_INLINE void vec4f_sub(vec4f_t dest, const vec4f_t a, const vec4f_t b) PROC(vec4_sub_impl(dest, a, b))
FORCE_INLINE void vec4i_sub(vec4i_t dest, const vec4i_t a, const vec4i_t b) PROC(vec4_sub_impl(dest, a, b))
FORCE_INLINE void vec4u_sub(vec4u_t dest, const vec4u_t a, const vec4u_t b) PROC(vec4_sub_impl(dest, a, b))

#define vec2_subs_impl(d,a,b) do { d[0]=a[0]-b; d[1]=a[1]-b; } while(0)
#define vec3_subs_impl(d,a,b) do { vec2_subs_impl(d,a,b);d[2]=a[2]-b; } while(0)
#define vec4_subs_impl(d,a,b) do { vec3_subs_impl(d,a,b);d[3]=a[3]-b; } while(0)
FORCE_INLINE void vec2f_subs(vec2f_t dest, const vec2f_t a, float b) PROC(vec2_subs_impl(dest, a, b))
FORCE_INLINE void vec2i_subs(vec2i_t dest, const vec2i_t a, int32_t b) PROC(vec2_subs_impl(dest, a, b))
FORCE_INLINE void vec2u_subs(vec2u_t dest, const vec2u_t a, uint32_t b) PROC(vec2_subs_impl(dest, a, b))
FORCE_INLINE void vec3f_subs(vec3f_t dest, const vec3f_t a, float b) PROC(vec3_subs_impl(dest, a, b))
FORCE_INLINE void vec3i_subs(vec3i_t dest, const vec3i_t a, int32_t b) PROC(vec3_subs_impl(dest, a, b))
FORCE_INLINE void vec3u_subs(vec3u_t dest, const vec3u_t a, uint32_t b) PROC(vec3_subs_impl(dest, a, b))
FORCE_INLINE void vec4f_subs(vec4f_t dest, const vec4f_t a, float b) PROC(vec4_subs_impl(dest, a, b))
FORCE_INLINE void vec4i_subs(vec4i_t dest, const vec4i_t a, int32_t b) PROC(vec4_subs_impl(dest, a, b))
FORCE_INLINE void vec4u_subs(vec4u_t dest, const vec4u_t a, uint32_t b) PROC(vec4_subs_impl(dest, a, b))
    
#define vec2_mul_impl(d,a,b) do { d[0]=a[0]*b[0]; d[1]=a[1]*b[1]; } while(0)
#define vec3_mul_impl(d,a,b) do { vec2_mul_impl(d,a,b);d[2]=a[2]*b[2]; } while(0)
#define vec4_mul_impl(d,a,b) do { vec3_mul_impl(d,a,b);d[3]=a[3]*b[3]; } while(0)
FORCE_INLINE void vec2f_mul(vec2f_t dest, const vec2f_t a, const vec2f_t b) PROC(vec2_mul_impl(dest, a, b))
FORCE_INLINE void vec2i_mul(vec2i_t dest, const vec2i_t a, const vec2i_t b) PROC(vec2_mul_impl(dest, a, b))
FORCE_INLINE void vec2u_mul(vec2u_t dest, const vec2u_t a, const vec2u_t b) PROC(vec2_mul_impl(dest, a, b))
FORCE_INLINE void vec3f_mul(vec3f_t dest, const vec3f_t a, const vec3f_t b) PROC(vec3_mul_impl(dest, a, b))
FORCE_INLINE void vec3i_mul(vec3i_t dest, const vec3i_t a, const vec3i_t b) PROC(vec3_mul_impl(dest, a, b))
FORCE_INLINE void vec3u_mul(vec3u_t dest, const vec3u_t a, const vec3u_t b) PROC(vec3_mul_impl(dest, a, b))
FORCE_INLINE void vec4f_mul(vec4f_t dest, const vec4f_t a, const vec4f_t b) PROC(vec4_mul_impl(dest, a, b))
FORCE_INLINE void vec4i_mul(vec4i_t dest, const vec4i_t a, const vec4i_t b) PROC(vec4_mul_impl(dest, a, b))
FORCE_INLINE void vec4u_mul(vec4u_t dest, const vec4u_t a, const vec4u_t b) PROC(vec4_mul_impl(dest, a, b))
    
#define vec2_muls_impl(d,a,b) do { d[0]=a[0]*b; d[1]=a[1]*b; } while(0)
#define vec3_muls_impl(d,a,b) do { vec2_muls_impl(d,a,b);d[2]=a[2]*b; } while(0)
#define vec4_muls_impl(d,a,b) do { vec3_muls_impl(d,a,b);d[3]=a[3]*b; } while(0)
FORCE_INLINE void vec2f_muls(vec2f_t dest, const vec2f_t a, float b) PROC(vec2_muls_impl(dest, a, b))
FORCE_INLINE void vec2i_muls(vec2i_t dest, const vec2i_t a, int32_t b) PROC(vec2_muls_impl(dest, a, b))
FORCE_INLINE void vec2u_muls(vec2u_t dest, const vec2u_t a, uint32_t b) PROC(vec2_muls_impl(dest, a, b))
FORCE_INLINE void vec3f_muls(vec3f_t dest, const vec3f_t a, float b) PROC(vec3_muls_impl(dest, a, b))
FORCE_INLINE void vec3i_muls(vec3i_t dest, const vec3i_t a, int32_t b) PROC(vec3_muls_impl(dest, a, b))
FORCE_INLINE void vec3u_muls(vec3u_t dest, const vec3u_t a, uint32_t b) PROC(vec3_muls_impl(dest, a, b))
FORCE_INLINE void vec4f_muls(vec4f_t dest, const vec4f_t a, float b) PROC(vec4_muls_impl(dest, a, b))
FORCE_INLINE void vec4i_muls(vec4i_t dest, const vec4i_t a, int32_t b) PROC(vec4_muls_impl(dest, a, b))
FORCE_INLINE void vec4u_muls(vec4u_t dest, const vec4u_t a, uint32_t b) PROC(vec4_muls_impl(dest, a, b))
 
#define vec2_len_impl(a) sqrtf((float)(vec2_dot_impl(a,a)))
#define vec3_len_impl(a) sqrtf((float)(vec3_dot_impl(a,a)))
#define vec4_len_impl(a) sqrtf((float)(vec4_dot_impl(a,a)))
FORCE_INLINE float vec2f_len(const vec2f_t a) FUNC(vec2_len_impl(a))
FORCE_INLINE float vec2i_len(const vec2i_t a) FUNC(vec2_len_impl(a))
FORCE_INLINE float vec2u_len(const vec2u_t a) FUNC(vec2_len_impl(a))
FORCE_INLINE float vec3f_len(const vec3f_t a) FUNC(vec3_len_impl(a))
FORCE_INLINE float vec3i_len(const vec3i_t a) FUNC(vec3_len_impl(a))
FORCE_INLINE float vec3u_len(const vec3u_t a) FUNC(vec3_len_impl(a))
FORCE_INLINE float vec4f_len(const vec4f_t a) FUNC(vec4_len_impl(a))
FORCE_INLINE float vec4i_len(const vec4i_t a) FUNC(vec4_len_impl(a))
FORCE_INLINE float vec4u_len(const vec4u_t a) FUNC(vec4_len_impl(a))
    
FORCE_INLINE void vec3f_cross(vec3f_t dest, const vec3f_t a, const vec3f_t b)
{
    dest[0] = a[1] * b[2] - a[2] * b[1];
    dest[1] = a[2] * b[0] - a[0] * b[2];
    dest[2] = a[0] * b[1] - a[1] * b[0];
}
    
///////////////////////////////////////////////////////////////////////////////////

#define MATH_VEC3F(x, y, z) (vec3f_t){x, y, z}
    
#define MATH_VEC2_MOV(dv, sv) do{dv[0]=sv[0];dv[1]=sv[1];}while(0)
#define MATH_VEC3_MOV(dv, sv) do{dv[0]=sv[0];dv[1]=sv[1];dv[2]=sv[2];}while(0)

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
void mathVec3fAdd(vec3f_t dest, const vec3f_t a, const vec3f_t b)
{
    dest[0] = a[0] + b[0];
    dest[1] = a[1] + b[1];
    dest[2] = a[2] + b[2];
}

FORCE_INLINE
void mathVec3fVec3f(vec3f_t dest, const vec3f_t a, const vec3f_t b)
{
    dest[0] = a[0] * b[0];
    dest[1] = a[1] * b[1];
    dest[2] = a[2] * b[2];
}
    
FORCE_INLINE
void mathVec3fFloat(vec3f_t dest, const vec3f_t a, const float b)
{
    dest[0] = a[0] * b;
    dest[1] = a[1] * b;
    dest[2] = a[2] * b;
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

#define MATH_MAT4_IDENTITY \
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
void mathMat4fTranslate(mat4f_t dest, const vec3f_t offsets)
{
	dest[3][0] += offsets[0];
	dest[3][1] += offsets[1];
	dest[3][2] += offsets[2];
}

FORCE_INLINE
void mathMat4fPerspective(mat4f_t dest, float fov, float ar, float zNear, float zFar)
{
    float tmp = 1.f / (zFar - zNear);
    float ctanFOV = 1.f / tanf(fov * 0.5f);
    dest[0][0] = ctanFOV;
    dest[1][1] = ctanFOV * ar;
#ifndef MATH_D3D
    dest[2][2] = -(zFar + zNear) * tmp;
    dest[2][3] = -1.f;
    dest[3][2] = -2.f * zFar * zNear * tmp;
#else
	dest[2][2] = zFar * tmp;
	dest[2][3] = 1.f;
	dest[3][2] = -zFar * zNear * tmp;
#endif
}

FORCE_INLINE
void mathMat4LookAt(mat4f_t dest, const vec3f_t eye, const vec3f_t to, const vec3f_t up)
{
	vec3f_t forward, right;
	mathVec3fSub(forward, to, eye);
	mathVec3fNormalize(forward);
	mathVec3fCross(right, up, forward);
	mathVec3fNormalize(right);
	dest[0][0] = right[0];
	dest[0][1] = up[0];
	dest[0][2] = forward[0];
	dest[1][0] = right[1];
	dest[1][1] = up[1];
	dest[1][2] = forward[1];
	dest[2][0] = right[2];
	dest[2][1] = up[2];
	dest[2][2] = forward[2];
	dest[3][0] = -eye[0];
	dest[3][1] = -eye[1];
	dest[3][2] = -eye[2];
}

FORCE_INLINE
void mathQuatInit(quat_t dest, vec3f_t axis, float angle)
{
    float a = 0.5f * angle;
    float sa = sinf(a);
    dest[0] = sa * axis[0];
    dest[1] = sa * axis[1];
    dest[2] = sa * axis[2];
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

FORCE_INLINE
void mathQuatVec3(vec3f_t dest, quat_t q, vec3f_t v)
{
    vec3f_t tmp0, tmp1;
    mathVec3fCross(tmp0, q, v);
    mathVec3fFloat(tmp0, tmp0, 2.f);
    mathVec3fCross(tmp1, q, tmp0);
    mathVec3fFloat(tmp0, tmp0, q[3]);
    mathVec3fAdd(tmp0, tmp0, tmp1);
    mathVec3fAdd(dest, v, tmp0);
}
    
#ifdef _cplusplus
}
#endif

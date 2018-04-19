#include "core/math.h"

void cogMat4fLookAt(mat4f_t dest, const vec3f_t at, const vec3f_t to, const vec3f_t up)
{
    vec3f_t newZ, newX, newY;
    cogVec3fSub(newZ, at, to);
    cogVec3fNormalize(newZ);
    cogVec3fCross(newX, up, newZ);
    cogVec3fCross(newY, newZ, newX);
    dest[0][0] = newX[0];
    dest[0][1] = newY[0];
    dest[0][2] = newZ[0];
    dest[0][3] = 0.f;
    dest[1][0] = newX[1];
    dest[1][1] = newY[1];
    dest[1][2] = newZ[1];
    dest[1][3] = 0.f;
    dest[2][0] = newX[2];
    dest[2][1] = newY[2];
    dest[2][2] = newZ[2];
    dest[2][3] = 0.f;
    dest[3][0] = -cogVec3fDot(at, newX);
    dest[3][1] = -cogVec3fDot(at, newY);
    dest[3][2] = -cogVec3fDot(at, newZ);
    dest[3][3] = 1.f;
}

#include "core/math.h"

void mathMat4fLookAt(mat4f_t dest, const vec3f_t at, const vec3f_t to, const vec3f_t up)
{
    vec3f_t newZ, newX, newY;
    mathVec3fSub(newZ, at, to);
    mathVec3fNormalize(newZ);
    mathVec3fCross(newX, up, newZ);
    mathVec3fCross(newY, newZ, newX);
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
    dest[3][0] = -mathVec3fDot(at, newX);
    dest[3][1] = -mathVec3fDot(at, newY);
    dest[3][2] = -mathVec3fDot(at, newZ);
    dest[3][3] = 1.f;
}

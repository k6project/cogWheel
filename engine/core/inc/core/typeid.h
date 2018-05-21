#pragma once

#include <stdint.h>

enum
{
    TYPEID_VOID,
    TYPEID_FLOAT,
    TYPEID_VEC2F,
    TYPEID_VEC3F,
    TYPEID_VEC4F,
    TYPEID_MAT2F,
    TYPEID_MAT3F,
    TYPEID_MAT4F,
    TYPEID_MAX
};

struct typeid_t
{
    uint16_t valueType;
    uint16_t arraySize;
};

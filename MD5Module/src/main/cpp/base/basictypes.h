//
// Created by admin on 2021/6/10.
//

#ifndef DEMO_BASICTYPES_H
#define DEMO_BASICTYPES_H

typedef int8_t int8;
typedef uint8_t uint8;
typedef int16_t int16;
typedef uint16_t uint16;
typedef int32_t int32;
typedef uint32_t uint32;
typedef int64_t int64;
typedef uint64_t uint64;

// DEPRECATED: Please use std::numeric_limits (from <limits>) instead.
const uint8  kuint8max  =  0xFF;
const uint16 kuint16max =  0xFFFF;
const uint32 kuint32max =  0xFFFFFFFF;
const uint64 kuint64max =  0xFFFFFFFFFFFFFFFFULL;
const  int8  kint8min   = -0x7F - 1;
const  int8  kint8max   =  0x7F;
const  int16 kint16min  = -0x7FFF - 1;
const  int16 kint16max  =  0x7FFF;
const  int32 kint32min  = -0x7FFFFFFF - 1;
const  int32 kint32max  =  0x7FFFFFFF;
const  int64 kint64min  = -0x7FFFFFFFFFFFFFFFLL - 1;
const  int64 kint64max  =  0x7FFFFFFFFFFFFFFFLL;
#endif //DEMO_BASICTYPES_H

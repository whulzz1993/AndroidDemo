//
// Created by admin on 2021/7/7.
//

#ifndef DEMO_INTERNAL_H
#define DEMO_INTERNAL_H
#if defined(__arm__)
#define STRICT_ALIGNMENT 1
#endif

#define GETU32(pt)                                         \
  (((uint32_t)(pt)[0] << 24) ^ ((uint32_t)(pt)[1] << 16) ^ \
   ((uint32_t)(pt)[2] << 8) ^ ((uint32_t)(pt)[3]))

#define PUTU32(ct, st)          \
  {                             \
    (ct)[0] = (uint8_t)((st) >> 24); \
    (ct)[1] = (uint8_t)((st) >> 16); \
    (ct)[2] = (uint8_t)((st) >> 8);  \
    (ct)[3] = (uint8_t)(st);         \
  }
#endif //DEMO_INTERNAL_H

#ifndef _XEXPR_TYPES_H_
#define _XEXPR_TYPES_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "xexpr.h"

int xex_list_append_int8(xex_list_t *list, int8_t i);

int xex_list_append_int16(xex_list_t *list, int16_t i);

int xex_list_append_int32(xex_list_t *list, int32_t i);

int xex_list_append_uint32(xex_list_t *list, uint32_t i);

int xex_list_append_int64(xex_list_t *list, int64_t i);

int xex_list_append_float(xex_list_t *list, float f);

int xex_list_append_double(xex_list_t *list, double f);

int xex_list_get_int8(xex_list_t *list, int idx, int8_t *ret);

int xex_list_get_int16(xex_list_t *list, int idx, int16_t *ret);

int xex_list_get_int32(xex_list_t *list, int idx, int32_t *ret);

int xex_list_get_uint32(xex_list_t *list, int idx, uint32_t *ret);

int xex_list_get_int64(xex_list_t *list, int idx, int64_t *ret);

int xex_list_get_float(xex_list_t *list, int idx, float *ret);

int xex_list_get_double(xex_list_t *list, int idx, double *ret);

#ifdef __cplusplus
}
#endif

#endif

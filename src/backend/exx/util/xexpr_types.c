#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include "xexpr_types.h"

#define MAX_NUMERIC_LEN 128

int xex_list_append_int8(xex_list_t *list, int8_t i) {
	char s[MAX_NUMERIC_LEN];
	sprintf(s, "%d", i);
	return xex_list_append_string(list, s);
}

int xex_list_append_int16(xex_list_t *list, int16_t i) {
	char s[MAX_NUMERIC_LEN];
	sprintf(s, "%d", i);
	return xex_list_append_string(list, s);
}

int xex_list_append_int32(xex_list_t *list, int32_t i) {
	char s[MAX_NUMERIC_LEN];
	sprintf(s, "%d", i);
	return xex_list_append_string(list, s);
}

int xex_list_append_uint32(xex_list_t *list, uint32_t i) {
	char s[MAX_NUMERIC_LEN];
	sprintf(s, "%u", i);
	return xex_list_append_string(list, s);
}

int xex_list_append_int64(xex_list_t *list, int64_t i) {
	char s[MAX_NUMERIC_LEN];
	sprintf(s, "%" PRId64, i);
	return xex_list_append_string(list, s);
}

int xex_list_append_float(xex_list_t *list, float f) {
	char s[MAX_NUMERIC_LEN];
	sprintf(s, "%f", f);
	return xex_list_append_string(list, s);
}

int xex_list_append_double(xex_list_t *list, double f) {
	char s[MAX_NUMERIC_LEN];
	sprintf(s, "%f", f);
	return xex_list_append_string(list, s);
}

int xex_list_get_int8(xex_list_t *list, int idx, int8_t *ret) {
	const char *s = xex_list_get_string(list, idx);
	if (!s) return 1;
	*ret = strtol(s, 0, 0);
	return 0;
}

int xex_list_get_int16(xex_list_t *list, int idx, int16_t *ret) {
	const char *s = xex_list_get_string(list, idx);
	if (!s) return 1;
	*ret = strtol(s, 0, 0);
	return 0;
}

int xex_list_get_int32(xex_list_t *list, int idx, int32_t *ret) {
	const char *s = xex_list_get_string(list, idx);
	if (!s) return 1;
	*ret = strtol(s, 0, 0);
	return 0;
}

int xex_list_get_uint32(xex_list_t *list, int idx, uint32_t *ret) {
	const char *s = xex_list_get_string(list, idx);
	if (!s) return 1;
	*ret = strtoul(s, 0, 0);
	return 0;
}

int xex_list_get_int64(xex_list_t *list, int idx, int64_t *ret) {
	const char *s = xex_list_get_string(list, idx);
	if (!s) return 1;
	*ret = strtoll(s, 0, 0);
	return 0;
}

int xex_list_get_float(xex_list_t *list, int idx, float *ret) {
	const char *s = xex_list_get_string(list, idx);
	if (!s) return 1;
	*ret = strtof(s, 0);
	return 0;
}

int xex_list_get_double(xex_list_t *list, int idx, double *ret) {
	const char *s = xex_list_get_string(list, idx);
	if (!s) return 1;
	*ret = strtod(s, 0);
	return 0;
}

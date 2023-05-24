// Copyright (c) 2018-2021, Vitesse Data Inc. All rights reserved.
#ifndef EXX_REQUIRED_H
#define EXX_REQUIRED_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "access/sysattr.h"

typedef struct exx_required_t {
	uint64_t *m_map;
	int m_top;
	bool m_sys_col[-FirstLowInvalidHeapAttributeNumber];
} exx_required_t;

void exx_required_destroy(exx_required_t *req);

bool exx_required_set(exx_required_t *req, int att);
bool exx_required_isset(exx_required_t *req, int att);
int exx_required_maxset(exx_required_t *req);
int exx_required_numset(exx_required_t *req);
bool exx_required_add(exx_required_t *req, exx_required_t *newreq);
bool exx_required_sub(exx_required_t *req, exx_required_t *newreq);
bool exx_required_has_sys_col(exx_required_t *req);

int exx_required_to_json(exx_required_t *req, char *buf, int bufsz);

bool exx_required_extend(exx_required_t *req, int newtop);

exx_required_t *exx_required_create(void);

#ifdef __cplusplus
}
#endif

#endif /* EXX_REQUIRED_H */

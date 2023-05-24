// Copyright (c) 2018-2021, Vitesse Data Inc. All rights reserved.
#include "../exx_int.h"
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "exx_required.h"

void exx_required_destroy(exx_required_t *req) {
	if (req->m_map) {
		pfree(req->m_map);
		req->m_map = 0;
	}
	pfree(req);
}

exx_required_t *exx_required_create() {
	exx_required_t *req = palloc(sizeof(exx_required_t));
	req->m_map = 0;
	req->m_top = 0;
	memset(req->m_sys_col, 0, -FirstLowInvalidHeapAttributeNumber);
	return req;
}

bool exx_required_has_sys_col(exx_required_t *req) {
	for (int i = 0; i < -FirstLowInvalidHeapAttributeNumber; i++) {
		if (req->m_sys_col[i]) {
			return true;
		}
	}
	return false;
};

bool exx_required_set(exx_required_t *req, int att) {
	if (att < 0) {
		req->m_sys_col[-att] = true;
		return true;
	}

	if (att == 0) {
		return false;
	}

	const int idx = att / 64;
	const int off = att % 64;

	if (idx >= req->m_top) {
		if (!exx_required_extend(req, idx + 1))
			return false;
	}

	uint64_t mask = 1;
	mask <<= off;
	req->m_map[idx] |= mask;
	return true;
}

bool exx_required_isset(exx_required_t *req, int att) {
	if (att < 0) {
		return req->m_sys_col[-att];
	}

	if (att == 0) {
		return false;
	}

	const int idx = att / 64;
	const int off = att % 64;

	if (idx >= req->m_top)
		return false;

	uint64_t mask = 1;
	mask <<= off;
	return (req->m_map[idx] & mask) != 0;
}

int exx_required_maxset(exx_required_t *req) {
	const int idx = req->m_top - 1;
	if (idx >= 0) {
		int att = idx * 64;
		uint64_t word = req->m_map[idx];
		uint64_t mask = 1;
		for (mask <<= 63, att += 63; mask; mask >>= 1, att--) {
			if (mask & word)
				return att;
		}
	}
	return 0;
}

int exx_required_numset(exx_required_t *req) {
	int cnt = 0;
	for (int idx = 0; idx < req->m_top; idx++) {
		uint64_t word = req->m_map[idx];
		if (word) {
			int att = idx * 64;
			uint64_t mask = 1;
			for (mask <<= 63, att += 63; mask; mask >>= 1, att--) {
				if (mask & word)
					cnt++;
			}
		}
	}
	return cnt;
}

bool exx_required_add(exx_required_t *req, exx_required_t *newreq) {
	if (newreq->m_top > req->m_top) {
		if (!exx_required_extend(req, newreq->m_top))
			return false;
	}
	for (int i = 0; i < newreq->m_top; i++) {
		req->m_map[i] |= newreq->m_map[i];
	}

	for (int i = 0; i < -FirstLowInvalidHeapAttributeNumber; i++) {
		req->m_sys_col[i] = req->m_sys_col[i] || newreq->m_sys_col[i];
	}

	return true;
}

bool exx_required_sub(exx_required_t *req, exx_required_t *newreq) {
	int t = (req->m_top > newreq->m_top) ? newreq->m_top : req->m_top;
	for (int i = 0; i < t; i++) {
		req->m_map[i] &= ~(newreq->m_map[i]);
	}

	for (int i = 0; i < -FirstLowInvalidHeapAttributeNumber; i++) {
		req->m_sys_col[i] = req->m_sys_col[i] && newreq->m_sys_col[i];
	}
	return true;
}

int exx_required_to_json(exx_required_t *req, char *buf, int bufsz) {
	char *p = buf;
	char *q = buf + bufsz;

	snprintf(p, q - p, "[");
	p += strlen(p);
	{
		int top = exx_required_maxset(req);
		const char *comma = "";
		for (int i = 1; i <= top; i++) {
			if (exx_required_isset(req, i)) {
				snprintf(p, q - p, "%s%d", comma, i);
				p += strlen(p);
				comma = ",";
			}
		}
	}
	snprintf(p, q - p, "]");
	p += strlen(p);
	return p - buf;
}

bool exx_required_extend(exx_required_t *req, int newtop) {
	uint64_t *x = 0;

	if (req->m_map) {
		x = (uint64_t *)repalloc(req->m_map, sizeof(*x) * newtop);
	} else {
		x = (uint64_t *)palloc(sizeof(*x) * newtop);
	}

	if (!x)
		return false;

	const int addition = newtop - req->m_top;
	memset(&x[req->m_top], 0, addition * sizeof(*x));
	req->m_top = newtop;
	req->m_map = x;
	return true;
}

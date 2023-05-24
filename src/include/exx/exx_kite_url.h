// Copyright (c) 2018-2021, Vitesse Data Inc. All rights reserved.
#ifndef EXX_KITE_URL_H
#define EXX_KITE_URL_H

#include "utils/uri.h"
#include "access/url.h"

/*
 * Private state for an EXECUTE external table.
 */
typedef struct URL_KITE_FILE 
{
	URL_FILE	common;

	int64_t seq_number;

	kite_handle_t *hdl;
} URL_KITE_FILE;

#endif

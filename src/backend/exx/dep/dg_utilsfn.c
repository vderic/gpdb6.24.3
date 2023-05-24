// Copyright (c) 2018-2021, Vitesse Data Inc. All rights reserved.
#include "postgres.h"
#include "fmgr.h"
#include "funcapi.h"
#include "exx/exx.h" 

/*
 * Trivial xdrive_query function, so that we can pass extra args to xdrive.
 */
extern Datum xdrive_query(PG_FUNCTION_ARGS);
Datum xdrive_query(PG_FUNCTION_ARGS)
{
    PG_RETURN_BOOL(true);
}

static void *xdrive_query_fn_marker;
void exx_set_xdrive_query_marker(void *fn) {
    xdrive_query_fn_marker = fn;
}
bool exx_fn_is_xdrive_query(void *fn) {
    return xdrive_query_fn_marker == fn;
}

extern Datum query_hint(PG_FUNCTION_ARGS);
Datum query_hint(PG_FUNCTION_ARGS)
{
    PG_RETURN_BOOL(true);
}

static void *query_hint_fn_marker;
void exx_set_query_hint_marker(void *fn) {
	query_hint_fn_marker = fn;
}

bool exx_fn_is_query_hint(void *fn) {
    return query_hint_fn_marker == fn; 
}

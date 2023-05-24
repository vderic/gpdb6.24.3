// Copyright (c) 2018-2021, Vitesse Data Inc. All rights reserved.
#ifndef _AGGREF_H_
#define _AGGREF_H_

#include "../exx_int.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "kitesdk.h"
#include "exx/exx_kite_url.h"

/* 
 * Interface for planwalker. See ../dep/planwalker.i.c 
 * This function runs in master segment.  Check log in dg-1/pg_log.
 * If Agg is not null, it will copy the targetlist into JSON format.  
 * The order of JSON targetlist match with the order of GPDB targetlist.
 * There is no logic for KITE here.  It only does targetlist copy to JSON, saves to exx_bc and passes to segment host.
 * Segment shall check exx_bc to determine to use JSON or tupdesc to construct the columns.
 */
typedef bool (*GP_FUNC_PTR_MESS)();
void xscan_build_kite_query(ExternalScan *xscan, Agg *agg, PlannedStmt *stmt);

bool node_to_xexpr(Node *node, void *ptr);
bool not_required_fill_xexpr(void *ptr);

#ifdef __cplusplus
}
#endif

#endif

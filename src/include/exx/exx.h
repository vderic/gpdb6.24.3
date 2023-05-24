// Copyright (c) 2018-2021, Vitesse Data Inc. All rights reserved.
#ifndef EXX_H
#define EXX_H

#include "executor/executor.h"
#include "executor/execdesc.h"

#include "exx_vendor.h"
#include "exx_lli.h"

extern void exx_raise_error(int err, const char *msg); 

extern void exx_external_xrg_begin(ExternalScanState *node);
extern void exx_external_xrg_end(ExternalScanState *node);
extern TupleTableSlot* exx_external_xrg_next(ExternalScanState *node);
extern bool exx_external_kite_analyzable(Relation rel, List *locs, char fmt);

extern MemoryContext exx_estate_memctxt(); 

extern void exx_set_xdrive_query_marker(void *fn);
extern bool exx_fn_is_xdrive_query(void *fn);
extern void exx_set_query_hint_marker(void *fn);
extern bool exx_fn_is_query_hint(void *fn);

typedef enum QueryHint {
	QH_INVALID,
	QH_OPTIMIZER,
} QueryHint;

extern int32_t exx_get_next_ep_id();

extern void exx_pre_planner_pass(Query *q);
extern void exx_post_planner_pass(PlannedStmt *p);
extern int32_t exx_get_query_hint_i32(QueryHint qh); 

#endif /* EXX_H */

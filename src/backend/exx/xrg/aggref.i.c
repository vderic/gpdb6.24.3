// Copyright (c) 2018-2021, Vitesse Data Inc. All rights reserved.
#include "../exx_int.h"
#include "lz4.h"

#include "utils/datum.h"
#include "utils/datetime.h"
#include "utils/builtins.h"
#include "utils/inet.h"
#include "nodes/makefuncs.h"
#include "nodes/print.h"
#include "access/fileam.h"
#include "access/heapam.h"
#include "catalog/pg_exttable.h"
#include "executor/nodeExternalscan.h"
#include "nodes/makefuncs.h"
#include "utils/lsyscache.h"
#include "parser/parsetree.h"
#include "optimizer/var.h"
#include "optimizer/clauses.h"

#include "../dep/intel_decimal_000.h"

#include "aggop.h"
#include "aggref.h"
#include "../util/exx_sarg.h"
#include "../util/xexpr_types.h"

Oid lookup_agg_transtype(Aggref *aggref);

/**
 * Check operation valid for kite
 */
static bool expr_kite_valid(Node *node, void *ptr) {

	if (!node) {
		return false;
	}

	if (IsA(node, TargetEntry)) {
		TargetEntry *te = (TargetEntry *)node;
		expr_kite_valid((Node *)te->expr, ptr);
		return false;
	}

	if (IsA(node, Aggref)) {
		Aggref *aggref = (Aggref *)node;

		if (list_length(aggref->args) > 0) {
			ListCell *lc;
			foreach (lc, aggref->args) {
				expr_kite_valid((Node *)lfirst(lc), ptr);
			}
		}
		return false;
	}

	if (IsA(node, OpExpr)) {
		OpExpr *opexpr = (OpExpr *)node;
		int32_t op = pg_proc_to_op(opexpr->opfuncid);
		if (op == -1) {
			*((bool *)ptr) = false;
			elog(LOG, "Invalid OpFuncId(%d) for Kite.", opexpr->opfuncid);
			return false;
		}
		ListCell *lc;
		foreach (lc, opexpr->args) {
			expr_kite_valid((Node *)lfirst(lc), ptr);
		}
		return false;
	}
	if (IsA(node, BoolExpr)) {
		BoolExpr *boolexpr = (BoolExpr *)node;
		ListCell *lc;
		foreach (lc, boolexpr->args) {
			expr_kite_valid((Node *)lfirst(lc), ptr);
		}
		return false;
	}
	if (IsA(node, FuncExpr)) {
		FuncExpr *funcexpr = (FuncExpr *)node;
		int32_t op = pg_func_to_op(funcexpr->funcid);
		if (op == -1) {
			*((bool *)ptr) = false;
			elog(LOG, "Invalid FuncId(%d) for Kite.", funcexpr->funcid);
		}
		return false;
	}

	if (IsA(node, Const)) {
		return false;
	}
	if (IsA(node, Var)) {
		return false;
	}

	if (IsA(node, NullIfExpr)) {
		NullIfExpr *nullifexpr = (NullIfExpr *)node;

		// transverse the arguments
		ListCell *lc;
		foreach (lc, nullifexpr->args) {
			expr_kite_valid((Node *)lfirst(lc), ptr);
		}
		return false;
	}

	if (IsA(node, CoalesceExpr)) {
		CoalesceExpr *coalesceexpr = (CoalesceExpr *)node;

		// transverse the arguments
		ListCell *lc;
		foreach (lc, coalesceexpr->args) {
			expr_kite_valid((Node *)lfirst(lc), ptr);
		}
		return false;
	}

	if (IsA(node, NullTest)) {
		NullTest *ntest = (NullTest *)node;
		expr_kite_valid((Node *)ntest->arg, ptr);
		return false;
	}

	if (IsA(node, RelabelType)) {
		RelabelType *r = (RelabelType *)node;
		expr_kite_valid((Node *)r->arg, ptr);
		return false;
	}

	if (IsA(node, ScalarArrayOpExpr)) {
		ScalarArrayOpExpr *e = (ScalarArrayOpExpr *)node;
		ListCell *lc;
		foreach (lc, e->args) {
			expr_kite_valid((Node *)lfirst(lc), ptr);
		}
		return false;
	}

	elog_node_display(LOG, "KITE_INVALID", node, true);

	*((bool *)ptr) = false;
	return expression_tree_walker(node, (GP_FUNC_PTR_MESS)expr_kite_valid, ptr);
}

/*
 * This function runs in master segment.  Check log in dg-1/pg_log.
 * If Agg is not null, it will copy the targetlist that match with the order of GPDB targetlist into JSON.
 * There is no logic for KITE here.  It only does targetlist to JSON, save to exx_bc and pass to segment host.
 * Segment shall check exx_bc to determine to use JSON or tupdesc to construct the columns.
 */
void xscan_build_kite_query(ExternalScan *xscan, Agg *agg, PlannedStmt *stmt) {
	//elog_node_display(LOG, "xscan", xscan, true);

	if (agg) {
		elog_node_display(LOG, "agg", agg, true);
	} else {
		elog(LOG, "NULL agg");
	}

	//xscan->exx_bclv = BCLV_PROJ;

	if (!(fmttype_is_xrg_par_orc(xscan->fmtType) || fmttype_is_csv(xscan->fmtType))) {
		return;
	}

	/* if agg is null,  no aggregate action required */
	if (!agg) {
		return;
	}

	Plan *xplan = (Plan *)xscan;
	ListCell *lc;

	/* modify the xplan->targetlist  and convert the group by columns and aggref to json */
	List *aggrefs = NULL;
	foreach (lc, agg->plan.targetlist) {
		TargetEntry *tle = (TargetEntry *)lfirst(lc);
		if (IsA(tle->expr, Aggref)) {
			aggrefs = lappend(aggrefs, tle->expr);
		}
	}

	/* Check Valid Qual */
	bool kite_valid = true;
	foreach (lc, xplan->qual) {
		expr_kite_valid((Node *)lfirst(lc), &kite_valid);
		if (!kite_valid) {
			exx_raise_error(ERRCODE_INTERNAL_ERROR, "Qual is not supported by kite");
			return;
		}
	}

	/* Check Valid Aggref */
	kite_valid = true;
	foreach (lc, aggrefs) {
		expr_kite_valid((Node *)lfirst(lc), &kite_valid);
		if (!kite_valid) {
			elog(LOG, "Aggref is not valid for Kite and fall back to table scan");
			return;
		}
	}

	/* get the group by columns */
	int32_t gbycols[agg->numCols];
	for (int i = 0; i < agg->numCols; i++) {
		gbycols[i] = agg->grpColIdx[i] - 1;
	}

	int resno = list_length(xplan->targetlist);

	// xexpr
	xex_list_t *xexpr = xex_list_create();
	xex_list_t *tgtlist = xex_list_create();
	xex_list_t *gbyexpr = xex_list_create();

	/* add group by columns to xexpr */
	int j = 0;
	foreach (lc, xplan->targetlist) {
		TargetEntry *tle = (TargetEntry *)lfirst(lc);
		for (int i = 0; i < agg->numCols; i++) {
			if (gbycols[i] == j) {
				tle->ressortgroupref = 0x23456789;
				break;
			}
		}
		j++;
	}

	/* JSON maintains the same number of target entry and order by appending the not_required TargetEntry */
	foreach (lc, xplan->targetlist) {
		TargetEntry *tle = (TargetEntry *)lfirst(lc);
		if (tle->ressortgroupref == 0x23456789) {
			// add tle->expr to xexpr
			node_to_xexpr((Node *)tle->expr, tgtlist);

			// add group by columns to xexpr
			node_to_xexpr((Node *)tle->expr, gbyexpr);

		} else {
			// add not_required type to xexpr
			not_required_fill_xexpr(tgtlist);
		}
	}

	/* add aggref from agg to json */
	int i = 0;
	foreach (lc, aggrefs) {
		Node *aggnode = (Node *)lfirst(lc);

		Oid oid = lookup_agg_transtype((Aggref *)aggnode);
		int16_t typlen;
		bool typbyval;
		get_typlenbyval(oid, &typlen, &typbyval);
		Const *c = makeConst(oid, 0, InvalidOid, typlen, 0, true, typbyval);
		TargetEntry *tle = makeTargetEntry((Expr *)c, ++resno, 0, false);

		tle->ressortgroupref = i + 0x12345678;
		xplan->targetlist = lappend(xplan->targetlist, tle);

		// add aggrefs[i] to json
		node_to_xexpr((Node *)aggnode, tgtlist);

		i++;
	}

	/* save xexpr to exx_bc */
	xex_list_append_list(xexpr, gbyexpr);
	xex_list_append_list(xexpr, tgtlist);
	char *x = xex_to_text((xex_object_t *)xexpr);
	//elog(LOG, x);

	xscan->exx_bclv = BCLV_AGG;
	xscan->exx_bcsz = strlen(x) + 1;
	xscan->exx_bc = (char *)palloc(xscan->exx_bcsz);
	strcpy(xscan->exx_bc, x);
}

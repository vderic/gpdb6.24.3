// Copyright (c) 2018-2021, Vitesse Data Inc. All rights reserved.
#include "postgres.h"

#include <limits.h>

#include "utils/timestamp.h"
#include "catalog/pg_proc.h" 
#include "catalog/pg_exttable.h"
#include "executor/executor.h"
#include "miscadmin.h"
#include "access/tuptoaster.h"
#include "nodes/makefuncs.h"
#include "optimizer/clauses.h"
#include "optimizer/cost.h"
#include "optimizer/orca.h"
#include "optimizer/pathnode.h"
#include "optimizer/paths.h"
#include "optimizer/planmain.h"
#include "optimizer/planner.h"
#include "optimizer/prep.h"
#include "optimizer/subselect.h"
#include "optimizer/transform.h"
#include "optimizer/tlist.h"
#include "optimizer/var.h"
#ifdef OPTIMIZER_DEBUG
#include "nodes/print.h"
#endif
#include "parser/parse_expr.h"
#include "parser/parse_oper.h"
#include "parser/parsetree.h"
#include "utils/lsyscache.h"
#include "utils/selfuncs.h"
#include "utils/syscache.h"

#include "cdb/cdbllize.h"
#include "cdb/cdbmutate.h"		/* apply_shareinput */
#include "cdb/cdbpartition.h"
#include "cdb/cdbpath.h"		/* cdbpath_segments */
#include "cdb/cdbpathtoplan.h"	/* cdbpathtoplan_create_flow() */
#include "cdb/cdbgroup.h"		/* grouping_planner extensions */
#include "cdb/cdbsetop.h"		/* motion utilities */
#include "cdb/cdbsubselect.h"	/* cdbsubselect_flatten_sublinks() */
#include "cdb/cdbvars.h"

#include "exx/exx.h"


typedef struct exx_walker_ctxt_t {
	bool has_nlj;
	bool has_shareinput;
	bool has_window;
	bool has_agg;
	PlannedStmt *stmt;
} exx_walker_ctxt_t;

static void exx_find_nodetype(PlannedStmt *result, exx_walker_ctxt_t*); 
static void exx_hj_fix(exx_walker_ctxt_t *ctxt); 
static void hj_fix_walker(Node *node, void *ctxt);
static Node* query_hint_mutator(Node *node, void *ctxt); 

extern void xscan_build_kite_query(ExternalScan *xscan, Agg *agg, PlannedStmt *stmt); 

typedef struct exx_query_hint_t {
	bool disable_orca;
	int32_t use_orca; 
} exx_query_hint_t;
exx_query_hint_t g_queryhint;

int32_t exx_get_query_hint_i32(QueryHint qh)
{
	switch (qh) {
		case QH_OPTIMIZER:
			if (g_queryhint.disable_orca) {
				return -1;
			}
			return g_queryhint.use_orca;
		default:
			return 0;
	}
	return 0;
}

QueryHint qhname_to_qh(const char* qhname, int len)
{
	if (pg_strncasecmp("optimizer", qhname, len) == 0) {
		return QH_OPTIMIZER;
	}
	return QH_INVALID;
}

void exx_set_query_hint(const char* p0, int len0, const char *p1, int len1) 
{
	QueryHint qh = qhname_to_qh(p0, len0);
	switch (qh) {
		case QH_OPTIMIZER:
			if (pg_strncasecmp("on", p1, len1) == 0) {
				g_queryhint.use_orca = 1;
			}

			if (pg_strncasecmp("off", p1, len1) == 0) {
				g_queryhint.use_orca = -1;
			}
			break;
		case QH_INVALID:
			break;
	}
}

void exx_pre_planner_pass(Query *q)
{
	// clean up
	memset(&g_queryhint, 0, sizeof(g_queryhint));

	query_or_expression_tree_mutator((Node *) q, query_hint_mutator, 0, 0);
}

void exx_post_planner_pass(PlannedStmt *p) 
{
	exx_walker_ctxt_t ctxt;
	memset(&ctxt, 0, sizeof(ctxt));
	ctxt.stmt = p;
	
	exx_find_nodetype(p, &ctxt); 

	if (! ctxt.has_agg) {
		return;
	}

	exx_hj_fix(&ctxt); 
}

Node* query_hint_mutator(Node *node, void *ctxt) 
{
	if (node == NULL) {
		return NULL;
	}

	if (IsA(node, FuncExpr)) {
		FuncExpr *expr = (FuncExpr *) node;
		void *fnptr = fmgr_info_fn_ptr(expr->funcid);

		if (fnptr && exx_fn_is_query_hint(fnptr)) { 
			Node *arg0 = (Node *) linitial(expr->args);
			Node *arg1 = (Node *) lsecond(expr->args); 
			if (arg0 && arg1 && IsA(arg0, Const) && IsA(arg1, Const)) {
				Const *c0 = (Const *) arg0;
				Const *c1 = (Const *) arg1;

				if (!c0->constisnull && !c1->constisnull) {
					char *p0, *p1;
					int len0, len1;
					void *tofree0, *tofree1;

					varattrib_untoast_ptr_len(c0->constvalue, &p0, &len0, &tofree0);
					varattrib_untoast_ptr_len(c1->constvalue, &p1, &len1, &tofree1);

					exx_set_query_hint(p0, len0, p1, len1);
					if (tofree0) {
						pfree(tofree0);
					}
					if (tofree1) {
						pfree(tofree1);
					}
				}
			}
		}
	} else if (IsA(node, Query)) {
		return (Node *) query_tree_mutator((Query *) node, query_hint_mutator, ctxt, 0);
	}

	return expression_tree_mutator(node, query_hint_mutator, ctxt); 
}

void find_nodetype_walker(Node *node, void *ctxt)
{

	Plan	   *plan;
	if(node == NULL)
		return;

	exx_walker_ctxt_t *ntctxt = (exx_walker_ctxt_t *) ctxt;

	if(IsA(node, List))
	{
		List *l = (List *) node;
		ListCell *lc;
		foreach(lc, l)
		{
			Node* n = lfirst(lc);
			find_nodetype_walker(n, ctxt);
		}
		return;
	}

	if(!is_plan_node(node))
		return;
	plan = (Plan *) node;

	if(IsA(node, Append))
	{
		ListCell *cell;
		Append *app = (Append *) node;
		foreach(cell, app->appendplans)
			find_nodetype_walker((Node *)lfirst(cell), ctxt);
	}
	else if(IsA(node, BitmapAnd))
	{
		ListCell *cell;
		BitmapAnd *ba = (BitmapAnd *) node;
		foreach(cell, ba->bitmapplans)
			find_nodetype_walker((Node *)lfirst(cell), ctxt);
	}
	else if(IsA(node, BitmapOr))
	{
		ListCell *cell;
		BitmapOr *bo = (BitmapOr *) node;
		foreach(cell, bo->bitmapplans)
			find_nodetype_walker((Node *)lfirst(cell), ctxt);
	}
	else if(IsA(node, SubqueryScan))
	{
		SubqueryScan *subqscan = (SubqueryScan *) node;
		find_nodetype_walker((Node *) subqscan->subplan, ctxt);
	}
	else if(IsA(node, Sequence))
	{
		Sequence *sequence = (Sequence *) node; 
		find_nodetype_walker((Node *) sequence->subplans, ctxt);
	}
	else
	{
		if(IsA(node, ShareInputScan)) { 
			ntctxt->has_shareinput = true;
		} else if (IsA(node, WindowAgg)) {
			ntctxt->has_window = true;
		} else if (IsA(node, NestLoop)) {
			ntctxt->has_nlj = true;
		} else if (IsA(node, Agg)) {
			ntctxt->has_agg = true;
		}

		find_nodetype_walker((Node *)plan->lefttree, ctxt);
		find_nodetype_walker((Node *)plan->righttree, ctxt);
		find_nodetype_walker((Node *)plan->initPlan, ctxt);
	}
}

void exx_find_nodetype(PlannedStmt *stmt, exx_walker_ctxt_t *ctxt)
{
	ListCell *lc;
	find_nodetype_walker((Node *) stmt->planTree, ctxt); 
	foreach (lc, stmt->subplans)
	{
		Plan *subplan = lfirst(lc);
		Assert(subplan);
		find_nodetype_walker((Node *) subplan, ctxt); 
	}
}


/*
 * The logic is taken from cdbmutate.c, assign_plannode_id and shareinput_walker.
 *
 * HJ Fix do two things,
 * 1. Try to figure out if we should swap hj sides,
 * 2. Try to see if we can push down a bloom filter.
 */
void hj_fix_walker(Node *node, void *ctxt)
{

	Plan	   *plan;

	if(node == NULL)
		return;

	if(IsA(node, List))
	{
		List *l = (List *) node;
		ListCell *lc;
		foreach(lc, l)
		{
			Node* n = lfirst(lc);
			hj_fix_walker(n, ctxt);
		}
		return;
	}

	if(!is_plan_node(node))
		return;
	plan = (Plan *) node;

	if(IsA(node, Append))
	{
		ListCell *cell;
		Append *app = (Append *) node;
		foreach(cell, app->appendplans)
			hj_fix_walker((Node *)lfirst(cell), ctxt);
	}
	else if(IsA(node, BitmapAnd))
	{
		ListCell *cell;
		BitmapAnd *ba = (BitmapAnd *) node;
		foreach(cell, ba->bitmapplans)
			hj_fix_walker((Node *)lfirst(cell), ctxt);
	}
	else if(IsA(node, BitmapOr))
	{
		ListCell *cell;
		BitmapOr *bo = (BitmapOr *) node;
		foreach(cell, bo->bitmapplans)
			hj_fix_walker((Node *)lfirst(cell), ctxt);
	}
	else if(IsA(node, SubqueryScan))
	{
		SubqueryScan *subqscan = (SubqueryScan *) node;
		hj_fix_walker((Node *) subqscan->subplan, ctxt);
	}
	else if(IsA(node, Sequence))
	{
		Sequence *sequence = (Sequence *) node; 
		hj_fix_walker((Node *) sequence->subplans, ctxt);
	}
	else if(IsA(node, Agg)) 
	{
		Agg* agg = (Agg *) node;
		if (IsA(agg->plan.lefttree, ExternalScan)) {
			ExternalScan *xscan = (ExternalScan *) agg->plan.lefttree;
			exx_walker_ctxt_t *pctxt = (exx_walker_ctxt_t *) ctxt;
			xscan_build_kite_query(xscan, agg, pctxt->stmt);
			hj_fix_walker((Node *)plan->initPlan, ctxt);
		} else {
			hj_fix_walker((Node *)plan->lefttree, ctxt);
			hj_fix_walker((Node *)plan->righttree, ctxt);
			hj_fix_walker((Node *)plan->initPlan, ctxt);
		}
	}
	else if(IsA(node, ExternalScan)) 
	{
		ExternalScan *xscan = (ExternalScan *) node;
		exx_walker_ctxt_t *pctxt = (exx_walker_ctxt_t *) ctxt;
		xscan_build_kite_query(xscan, 0, pctxt->stmt);
		hj_fix_walker((Node *)plan->initPlan, ctxt);
	}
	else
	{
		hj_fix_walker((Node *)plan->lefttree, ctxt);
		hj_fix_walker((Node *)plan->righttree, ctxt);
		hj_fix_walker((Node *)plan->initPlan, ctxt);
	}
}


void exx_hj_fix(exx_walker_ctxt_t *ctxt)
{
	PlannedStmt *stmt = ctxt->stmt;
	/*
	 * hj_fix try to fix two things, hj swap and bloom filter 
	 */

	ListCell *lc;
	hj_fix_walker((Node *) stmt->planTree, ctxt); 
	foreach (lc, stmt->subplans)
	{
		Plan *subplan = lfirst(lc);
		Assert(subplan);
		hj_fix_walker((Node *) subplan, ctxt); 
	}
}

Node* replace_inout_expr_mutator(Node *node, void *ctxt)
{
	if (node == NULL) {
		return NULL;
	}

	if (IsA(node, Var)) {
		Var* var = (Var *) node;
		if (var->varno == INNER_VAR) {
			Var *ret = copyObject(var);
			ret->varno = OUTER_VAR;
			return (Node *) ret;
		}
		return NULL;
	}

	return expression_tree_mutator(node, replace_inout_expr_mutator, ctxt); 
}



Node* replace_aggexpr_mutator(Node *node, void *ctxt)
{
	if (node == NULL) {
		return NULL;
	}

	if (IsA(node, Var)) {
		Var* var = (Var *) node;
		Agg* agg = (Agg *) ctxt; 
		List *tlist = agg->plan.targetlist;
		AttrNumber att = var->varattno; 
		return copyObject(list_nth(tlist, att - 1));
	}

	return expression_tree_mutator(node, replace_aggexpr_mutator, ctxt);
}

bool aggexpr_walker(Node *node, void *ctxt)
{
	if (node == NULL) {
		return false; 
	}

	/* We cannot push filter below an Agg col. */
	if (IsA(node, Aggref)) {
		return true;
	}

	if (IsA(node, Var)) {
		Var* var = (Var *) node;
		if (var->varno != OUTER_VAR) {
			return true;
		} else {
			return false;
		}
	}
	return expression_tree_walker(node, aggexpr_walker, ctxt); 
}


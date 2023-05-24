#include "../exx_int.h"

#include "access/fileam.h"
#include "access/heapam.h"
#include "catalog/pg_exttable.h"
#include "cdb/cdbvars.h"
#include "cdb/cdbtm.h"
#include "executor/execdebug.h"
#include "executor/nodeExternalscan.h"
#include "utils/builtins.h"
#include "utils/datetime.h"
#include "utils/lsyscache.h"
#include "utils/uri.h"
#include "utils/memutils.h"
#include "utils/array.h"
#include "utils/relcache.h"
#include "utils/rel.h"
#include "parser/parsetree.h"
#include "optimizer/var.h"
#include "optimizer/clauses.h"

#include "aggref.h"
#include "kite_extscan.h"
#include "../util/exx_sarg.h"
#include "aggop.h"

static void kite_extscan_setup_targetlist(kite_extscan_t *scan);
static void kite_extscan_exec(kite_extscan_t *);
static void kite_extscan_mark_req(kite_extscan_t *ex, int attno);
static void kite_extscan_mark_all_req(kite_extscan_t *ex);
static void kite_extscan_mark_at_least_one_req(kite_extscan_t *ex);

static bool find_vars_walker(Node *node, void *ctxt) {
	if (!node) {
		return false;
	}

	if (IsA(node, Var)) {
		kite_extscan_t *ex = (kite_extscan_t *)ctxt;
		Var *var = (Var *)node;
		// Simple and stupid, just make attno.
		kite_extscan_mark_req(ex, var->varattno);
		return false;
	}

	return expression_tree_walker(node, (GP_FUNC_PTR_MESS)find_vars_walker, ctxt);
}

/* kite_extscan_t */
/** 
 * Create Kite externl table context
 */
kite_extscan_t *kite_extscan_create(ExternalScanState *node) {

	kite_extscan_t *scan = palloc(sizeof(kite_extscan_t));
	Insist(scan);

	memset(scan, 0, sizeof(kite_extscan_t));
	scan->m_node = node;
	scan->m_req = exx_required_create();

	// setup targetlist
	kite_extscan_setup_targetlist(scan);

	// create handle
	kite_extscan_exec(scan);

	return scan;
}

/**
 * Destory kite external table context
 */
void kite_extscan_destroy(kite_extscan_t *scan) {
	// m_req
	exx_required_destroy(scan->m_req);

	// m_targetlist
	if (scan->m_targetlist) {
		ListCell *l;
		foreach (l, scan->m_targetlist) {
			kite_target_t *tgt = (kite_target_t *)lfirst(l);
			kite_target_destroy(tgt);
		}
		list_free(scan->m_targetlist);
	}

	// m_xexpr
	if (scan->m_xexpr) {
		xex_release((xex_object_t *)scan->m_xexpr);
	}

	pfree(scan);
}

#if 0
static void target_print(kite_target_t *tgt) {
	ListCell *tuplecell = 0;
	if (tgt->tuplist) {
		foreach (tuplecell, tgt->tuplist) {
			char *s = (char *)lfirst(tuplecell);
			elog(LOG, "col = %s", s);
		}
	} else {
		elog(LOG, "tuplist is NULL");
	}

	if (tgt->attrs) {
		foreach (tuplecell, tgt->attrs) {
			int idx = lfirst_int(tuplecell);
			elog(LOG, "COL = %d", idx);
		}
	} else {
		elog(LOG, "attrs is nuLL");
	}
}
#endif

/**
 * Create target list from xexpr
 */
static void setup_targetlist_from_xexpr(kite_extscan_t *scan, xex_list_t *xexpr) {
	TupleDesc exec_tdesc = exec_tupdesc(scan->m_node);
	TupleDesc scan_tdesc = scan_tupdesc(scan->m_node);

	int len = xex_list_length(xexpr);

	Insist(len == 2);

	xex_object_t *objs = xex_list_get(xexpr, 1);
	Insist(objs);
	xex_list_t *aggrefs = xex_to_list(objs);
	Insist(aggrefs);

	scan->m_targetlist = NULL;
	int start_idx = 0;
	for (int i = 0; i < xex_list_length(aggrefs); i++) {
		xex_object_t *obj = xex_list_get(aggrefs, i);
		Insist(obj);
		xex_list_t *agg = xex_to_list(obj);
		Insist(agg);
		kite_target_t *tgt = kite_target_from_xexpr(agg, exec_tdesc->attrs[i], scan_tdesc, &start_idx);
		Insist(tgt);
		scan->m_targetlist = lappend(scan->m_targetlist, tgt);
	}

#if 0
	ListCell *lc;
	foreach (lc, scan->m_targetlist) {
		kite_target_t *tgt = (kite_target_t *) lfirst(lc);
		target_print(tgt);
	}
#endif
}

/**
 * Create target list from column projection (non-aggregate columns)
 */
static void setup_targetlist_from_project(kite_extscan_t *scan) {
	ProjectionInfo *proj = scan->m_node->ss.ps.ps_ProjInfo;
	FileScanDesc scandesc = scan->m_node->ess_ScanDesc;

	// check required columns
	if (!proj) {
		kite_extscan_mark_all_req(scan);
	} else {
		for (int i = 0; i < proj->pi_numSimpleVars; i++) {
			int varNo = proj->pi_varNumbers[i];
			kite_extscan_mark_req(scan, varNo);
		}

		if (proj->pi_targetlist) {
			ListCell *tl;
			foreach (tl, proj->pi_targetlist) {
				GenericExprState *gstate = (GenericExprState *)lfirst(tl);
				find_vars_walker((Node *)gstate->arg->expr, (void *)scan);
			}
		}
	}

	List *qual = scan->m_node->ss.ps.qual;
	if (qual) {
		ListCell *l;
		foreach (l, qual) {
			ExprState *clause = (ExprState *)lfirst(l);
			find_vars_walker((Node *)clause->expr, (void *)scan);
		}
	}

	if (scandesc->fs_constraintExprs) {
		Relation rel = scandesc->fs_rd;
		TupleConstr *constr = rel->rd_att->constr;
		uint16 ncheck = constr->num_check;

		for (uint16 i = 0; i < ncheck; i++) {
			List *qual = (List *)scandesc->fs_constraintExprs[i];
			ListCell *l;
			foreach (l, qual) {
				ExprState *clause = (ExprState *)lfirst(l);
				find_vars_walker((Node *)clause->expr, (void *)scan);
			}
		}
	}

	// fix SELECT count(*) from t
	// fix SELECT 1 from t;
	kite_extscan_mark_at_least_one_req(scan);

	// use scan_tupdesc to get the whole table columns
	TupleDesc scan_tdesc = scan_tupdesc(scan->m_node);

	int ncol = scan_tdesc->natts;
	Form_pg_attribute *attr = scan_tdesc->attrs;

	int start_idx = 0;
	for (int i = 0; i < ncol; i++) {
		bool notreq = false;
		if (attr[i]->attisdropped || (scan->m_req && !exx_required_isset(scan->m_req, i + 1))) {
			notreq = true;
		}

		if (notreq) {
			// simply create empty kite_target_t and don't advance start_idx for invalid column
			kite_target_t *target = kite_target_create();
			scan->m_targetlist = lappend(scan->m_targetlist, target);
		} else {
			kite_target_t *target = kite_target_from_tupdesc(attr[i], scan_tdesc, i, &start_idx);
			scan->m_targetlist = lappend(scan->m_targetlist, target);
		}
	}
}

/**
 * Create target list from info given by kite external table context
 */
static void kite_extscan_setup_targetlist(kite_extscan_t *scan) {
	ExternalScan *es = scan_plan(scan->m_node);

	if (es->exx_bcsz > 0 && es->exx_bclv == BCLV_AGG) {
		//elog(LOG, es->exx_bc);

		const char *endp;
		xex_parse_error_t err;

		xex_object_t *obj = xex_parse(es->exx_bc, strlen(es->exx_bc), &endp, &err);
		if (!obj) {
			elog(ERROR, "xexpr error: %s. [linenum=%d, offset=%d]", err.errmsg, err.linenum, err.lineoff);
		}

		scan->m_xexpr = xex_to_list(obj);
		if (!scan->m_xexpr) {
			elog(ERROR, "xexpr error: exx_bc is not a list");
		}

		setup_targetlist_from_xexpr(scan, scan->m_xexpr);

	} else {
		setup_targetlist_from_project(scan);
	}
}

/**
 * Execute kite external table scan by submit a kite remote query
 */
static void kite_extscan_exec(kite_extscan_t *ex) {
	char *addr, *schema, *sql;
	int fragid, fragcnt;
	int e;
	int errlen = 1024;
	char errmsg[errlen];
	kite_filespec_t fs;

	// open connection here
        FileScanDesc scandesc = ex->m_node->ess_ScanDesc;
        Insist(scandesc->fs_file == NULL);
        Insist(scandesc->fs_uri != NULL && IS_KITE_URI(scandesc->fs_uri));
        // open connection.  EDIT access/external/url_kite.c
        open_external_readable_source(scandesc, 0);
        URL_KITE_FILE *urlf = (URL_KITE_FILE *)scandesc->fs_file;

	e = setup_query(ex, &addr, &schema, &sql, &fragid, &fragcnt, &fs);
	if (e) {
		elog(ERROR, "setup_query failed");
		return;
	}

	urlf->hdl = kite_submit(addr, schema, sql, fragid, fragcnt, &fs, errmsg, errlen);
	if (!urlf->hdl) {
		elog(ERROR, "kite_submit failed");
		return;
	}

}

/**
 * Get next tuple from kite remote server
 */
bool kite_extscan_get_next(kite_extscan_t *ex, int ncol, Datum *datums, bool *isnulls) {
	URL_KITE_FILE *urlf = (URL_KITE_FILE *)scan_scandesc(ex->m_node)->fs_file;
	xrg_iter_t *iter = 0;
	char errmsg[1024];
	int errlen = sizeof(errmsg);

	int e = kite_next_row(urlf->hdl, &iter, errmsg, errlen);
	if (e == 0) {
		if (iter == 0) {
			// no more pending data
			return false;
		}
	} else if (e < 0) {
		// error
		elog(ERROR, "%s", errmsg);
		return false;
	}

	// decode with targetlist
	int i = 0;
	ListCell *l;
	foreach (l, ex->m_targetlist) {
		kite_target_t *target = (kite_target_t *)lfirst(l);
		if (target->decode) {
			target->decode(target, iter, &datums[i], &isnulls[i]);
		} else {
			datums[i] = 0;
			isnulls[i] = true;
		}
		i++;
	}

	return true;
}

/**
 * Mark column as required
 */
static void kite_extscan_mark_req(kite_extscan_t *ex, int attno) {
	exx_required_set(ex->m_req, attno);
}

/**
 * Mark all columns as required
 */
static void kite_extscan_mark_all_req(kite_extscan_t *ex) {
	for (int i = 1; i <= scan_ncol(ex->m_node); i++) {
		kite_extscan_mark_req(ex, i);
	}
}

/**
 * No columns specified.  Mark the first column as required
 */
static void kite_extscan_mark_at_least_one_req(kite_extscan_t *ex) {
	bool found = false;
	for (int i = 1; i <= scan_ncol(ex->m_node); i++) {
		if (exx_required_isset(ex->m_req, i)) {
			found = true;
			break;
		}
	}

	if (!found) {
		exx_required_set(ex->m_req, 1);
	}
}


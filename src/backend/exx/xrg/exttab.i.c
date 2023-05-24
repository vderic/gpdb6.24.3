#include "../exx_int.h"
#include "catalog/pg_exttable.h"
#include "kite_extscan.h"

bool exx_external_kite_analyzable(Relation rel, List *locs, char fmt) {
	if (list_length(locs) == 0) {
		return false;
	}

	if (fmttype_is_xrg_par_orc(fmt) || fmttype_is_csv(fmt)) {
		ListCell *lc;
		foreach (lc, locs) {
			Value *v = (Value *)lfirst(lc);
			Assert(v->type == T_String);

			char *url = v->val.str;

			if (!IS_KITE_URI(url)) {
				return false;
			}
		}
		return true;
	} else {
		return false;
	}
}

/* top-level functions declare here */
/**
 * Create kite external table context and return to PG
 */
void *exx_xrg_exttab_create_ctxt(ExternalScanState *node) {

	return kite_extscan_create(node);
}

/**
 * Destroy kite external table context
 */
void exx_xrg_exttab_release_ctxt(void *p) {

	kite_extscan_destroy(p);
}

/**
 * Get the next tuple from kite external table
 */
bool exx_xrg_exttab_get_next(void *p, int ncol, Datum *values, bool *isnulls) {
	kite_extscan_t *ex = (kite_extscan_t *)p;
	Insist(ncol == exec_ncol(ex->m_node));
	return kite_extscan_get_next(ex, ncol, values, isnulls);
}

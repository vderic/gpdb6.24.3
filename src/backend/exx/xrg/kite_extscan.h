#ifndef _KITE_EXTSCAN_H_
#define _KITE_EXTSCAN_H_

#include "xrg.h"
#include "kitesdk.h"
#include "exx/exx_kite_url.h"
#include "../util/exx_required.h"
#include "../util/xexpr_types.h"
#include "../util/stringbuffer.h"
#include "kitesdk.h"

/*
 *  create a target_list from xexpr
 *  if (invalid target) don't advance start_idx
 *  else if (aggregate == AVG operation) advance start_idx by 2.  SUM = start_idx and COUNT = start_idx + 1
 *  else advance start_idx by 1.
 *
 *
 *  start_idx = 0;
 *  for (int i = 0 ; i < list(aggrefs) ; i++) {
 *      kite_target_t *tgt = kite_target_from_xexpr(aggrefs[i], &start_idx);
 *      m_targetlist = list_append(m_targetlist, tgt);
 *  }
 *    
 *  Get Next Row
 *
 *  xrg_iter_t *iter = 0;
 *  if ((iter = kite_result_get_next(res)) == 0) {
 *
 *    kite_result_t *res = kite_result_fetch(sockstream);
 *    if ((iter = kite_result_get_next(res)) == 0) {
 *      return false;
 *    }
 *  }
 *
 *
 *  for (int i = 0 ; i < targetlist.size(); i++) {
 *         tgt = targetlist[i];
 *         tgt.decode(tgt, iter, &datum[i], &isnull[i]);
 *  }
 *
 *  return true;
 *
 *
 */
typedef struct StringBuffer stringbuffer_t;

/* exttab.i.c -- ENTRY interface for ../dep/external.i.c */
void *exx_xrg_exttab_create_ctxt(ExternalScanState *node);

void exx_xrg_exttab_release_ctxt(void *p);

bool exx_xrg_exttab_get_next(void *p, int ncol, Datum *values, bool *isnull);

/* kite_target.c */
// target entry from the gpdb target list
typedef struct kite_target_t {
	Form_pg_attribute pg_attr;
	xex_list_t *m_xexpr;
	List *attrs;   // index corresponding to kite result
	List *tuplist; // list of target expr in string
	void *data;	// storage of the current decoded data
	int (*decode)(struct kite_target_t *, xrg_iter_t *iter, Datum *pg_datum, bool *pg_isnull);
} kite_target_t;

// start_idx is the column idx from kite and will be changed to next start idx for the next call
kite_target_t *kite_target_create(void);
kite_target_t *kite_target_from_xexpr(xex_list_t *, const Form_pg_attribute attr, TupleDesc tupdesc, int *start_idx);
kite_target_t *kite_target_from_tupdesc(const Form_pg_attribute attr, TupleDesc tupdesc, int idx, int *start_idx);
void kite_target_destroy(kite_target_t *);

void traverse(xex_list_t *list, TupleDesc tupdesc, stringbuffer_t *strbuf);

/* kite_extscan.c */
typedef struct kite_extscan_t {
	ExternalScanState *m_node;
	xex_list_t *m_xexpr;
	List *m_targetlist; // mirror of the gpdb target list. List of kite_target_t
	exx_required_t *m_req;
} kite_extscan_t;

// create extscan, create targetlist, create JSON request and connect socket
kite_extscan_t *kite_extscan_create(ExternalScanState *node);

// release the resource
void kite_extscan_destroy(kite_extscan_t *);

// get next row
bool kite_extscan_get_next(kite_extscan_t *, int ncol, Datum *datums, bool *isnulls);

/* kite_json.c */
int setup_query(kite_extscan_t *ex, char **addr, char **schema, char **sql, int *fragid, int *fragcnt, kite_filespec_t *fs);

/* extscanstate.c -- ExternalScanState utility */
ExternalScan *scan_plan(ExternalScanState *node);

TupleTableSlot *scan_slot(ExternalScanState *node);

TupleDesc scan_tupdesc(ExternalScanState *node);

int scan_ncol(ExternalScanState *node);

TupleTableSlot *exec_slot(ExternalScanState *node);

TupleDesc exec_tupdesc(ExternalScanState *node);

int exec_ncol(ExternalScanState *node);

FileScanDesc scan_scandesc(ExternalScanState *node);

CopyState scan_copystate(ExternalScanState *node);

URL_FILE *scan_urlfile(ExternalScanState *node);

FmgrInfo *in_funcs(ExternalScanState *node);

Oid *in_ioparams(ExternalScanState *node);

char *colname(ExternalScanState *node, int attno);

#endif

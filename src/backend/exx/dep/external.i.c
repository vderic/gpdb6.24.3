// Copyright (c) 2018-2021, Vitesse Data Inc. All rights reserved.
#include "postgres.h"

#include "access/fileam.h"
#include "access/heapam.h"
#include "cdb/cdbvars.h"
#include "executor/execdebug.h"
#include "executor/nodeExternalscan.h"
#include "utils/lsyscache.h"
#include "utils/memutils.h"
#include "parser/parsetree.h"
#include "optimizer/var.h"
#include "optimizer/clauses.h"

#include "exx/exx.h"


extern void* exx_xrg_exttab_create_ctxt(ExternalScanState *node);
extern void exx_xrg_exttab_release_ctxt(void *p);
extern bool exx_xrg_exttab_get_next(void *p, int ncol, Datum *values, bool *isnull);

static void exx_external_begin(ExternalScanState *node)
{
    FileScanDesc desc = node->ess_ScanDesc;

    Assert( desc);
    Relation rel = desc->fs_rd;
    if (desc->fs_hasConstraints) {
        TupleConstr *constr = rel->rd_att->constr;
        ConstrCheck *check = constr->check;
        uint16_t ncheck = constr->num_check;
        EState *estate = node->ss.ps.state;

        /*
         * Build exprssion node trees for rel's constraint expression.  The expression 
         * should be build in the query context. 
         *
         * See nodeExternalscan.c, ExternalConstraintCheck.
         */
        if (ncheck != 0) {
            if (desc->fs_constraintExprs == NULL) {
                MemoryContext oldctxt = MemoryContextSwitchTo(estate->es_query_cxt);
                desc->fs_constraintExprs = (List **) palloc(ncheck * sizeof(List *));
                for (uint16_t i = 0; i < ncheck; i++) {
                    /* ExecQual wants implicate AND form */
                    List *qual = make_ands_implicit(stringToNode(check[i].ccbin));
                    desc->fs_constraintExprs[i] = (List *) ExecPrepareExpr((Expr *) qual, estate);
                }
                MemoryContextSwitchTo(oldctxt);
            }
        }
    }
}

/**
 * Begin of the kite external table interface
 */
void exx_external_xrg_begin(ExternalScanState *node)
{
    FileScanDesc desc = node->ess_ScanDesc;

    if (!desc->fs_formatter) {
        MemoryContext oldctxt = MemoryContextSwitchTo(CurTransactionContext);
        desc->fs_formatter = palloc0(sizeof(FormatterData));
        MemoryContextSwitchTo(oldctxt);
    }

    Assert( !desc->fs_formatter->fmt_user_ctx);
    exx_external_begin(node);

    desc->fs_formatter->fmt_user_ctx = exx_xrg_exttab_create_ctxt(node);
    if (!desc->fs_formatter->fmt_user_ctx) {
        elog(ERROR, EXX_VENDOR_PRODUCT ": cannot create xrg external table context.");
    } 
}


/**
 * End of the Kite external table interface
 */
void exx_external_xrg_end(ExternalScanState *node)
{
    FileScanDesc desc = node->ess_ScanDesc;
    Assert( desc && desc->fs_formatter);

    void *p = desc->fs_formatter->fmt_user_ctx;
    desc->fs_formatter->fmt_user_ctx = 0;

    if (p) { 
        exx_xrg_exttab_release_ctxt(p);
    }
}

/**
 * Get next tuple for kite external table
 */
TupleTableSlot* exx_external_xrg_next(ExternalScanState *node)
{
    FileScanDesc desc = node->ess_ScanDesc;
    void *ctxt = desc->fs_formatter->fmt_user_ctx;
	TupleTableSlot *slot;

	if (node->exx_bc_projslot) {
		slot = node->exx_bc_projslot;
	} else {
		slot = node->ss.ss_ScanTupleSlot;
	}

    Datum *values = slot_get_values(slot);
    bool *isnulls = slot_get_isnull(slot);
    int ncol = slot->tts_tupleDescriptor->natts;

    bool ok = exx_xrg_exttab_get_next(ctxt, ncol, values, isnulls);
    if (!ok) {
        return NULL;
    } 

    if (node->cdb_want_ctid && !TupIsNull(slot)) {
        slot_set_ctid_from_fake(slot, &node->cdb_fake_ctid);
    }

    TupSetVirtualTupleNValid(slot, ncol); 
    return slot;
}

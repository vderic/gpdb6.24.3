/*-------------------------------------------------------------------------
 *
 * nodeExternalscan.c
 *	  Support routines for scans of external relations (on flat files for example)
 *
 * Portions Copyright (c) 2007-2008, Greenplum inc
 * Portions Copyright (c) 2012-Present Pivotal Software, Inc.
 *
 *
 * IDENTIFICATION
 *	    src/backend/executor/nodeExternalscan.c
 *
 *-------------------------------------------------------------------------
 */

/*
 * INTERFACE ROUTINES
 *		ExecExternalScan				sequentially scans a relation.
 *		ExecExternalNext				retrieve next tuple in sequential order.
 *		ExecInitExternalScan			creates and initializes a externalscan node.
 *		ExecEndExternalScan				releases any storage allocated.
 *		ExecExternalReScan				rescans the relation
 */
#include "postgres.h"

#include "access/fileam.h"
#include "access/heapam.h"
#include "catalog/pg_exttable.h"
#include "cdb/cdbvars.h"
#include "executor/execdebug.h"
#include "executor/nodeExternalscan.h"
#include "utils/lsyscache.h"
#include "utils/memutils.h"
#include "utils/uri.h"
#include "utils/guc.h"
#include "parser/parsetree.h"
#include "optimizer/var.h"
#include "optimizer/clauses.h"

/* EXX_IN_PG */
#include "exx/exx.h"

static TupleTableSlot *ExternalNext(ExternalScanState *node);
static void ExecEagerFreeExternalScan(ExternalScanState *node);

static bool
ExternalConstraintCheck(TupleTableSlot *slot, ExternalScanState *node)
{
	FileScanDesc	scandesc = node->ess_ScanDesc;
	Relation		rel = scandesc->fs_rd;
	TupleConstr		*constr = rel->rd_att->constr;
	ConstrCheck		*check = constr->check;
	uint16			ncheck = constr->num_check;
	EState			*estate = node->ss.ps.state;
	ExprContext		*econtext = NULL;
	MemoryContext	oldContext = NULL;
	List	*qual = NULL;
	int		i = 0;

	/* No constraints */
	if (ncheck == 0)
	{
		return true;
	}

	/*
	 * Build expression nodetrees for rel's constraint expressions.
	 * Keep them in the per-query memory context so they'll survive throughout the query.
	 */
	if (scandesc->fs_constraintExprs == NULL)
	{
		oldContext = MemoryContextSwitchTo(estate->es_query_cxt);
		scandesc->fs_constraintExprs =
			(List **) palloc(ncheck * sizeof(List *));
		for (i = 0; i < ncheck; i++)
		{
			/* ExecQual wants implicit-AND form */
			qual = make_ands_implicit(stringToNode(check[i].ccbin));
			scandesc->fs_constraintExprs[i] = (List *)
				ExecPrepareExpr((Expr *) qual, estate);
		}
		MemoryContextSwitchTo(oldContext);
	}

	/*
	 * We will use the EState's per-tuple context for evaluating constraint
	 * expressions (creating it if it's not already there).
	 */
	econtext = GetPerTupleExprContext(estate);

	/* Arrange for econtext's scan tuple to be the tuple under test */
	econtext->ecxt_scantuple = slot;

	/* And evaluate the constraints */
	for (i = 0; i < ncheck; i++)
	{
		qual = scandesc->fs_constraintExprs[i];

		if (!ExecQual(qual, econtext, true))
			return false;
	}

	return true;
}
/* ----------------------------------------------------------------
*						Scan Support
* ----------------------------------------------------------------
*/
/* ----------------------------------------------------------------
*		ExternalNext
*
*		This is a workhorse for ExecExtScan
* ----------------------------------------------------------------
*/
static TupleTableSlot *
ExternalNext(ExternalScanState *node)
{
	HeapTuple	tuple;
	FileScanDesc scandesc;
	EState	   *estate;
	ScanDirection direction;
	TupleTableSlot *slot;
	bool		scanNext = true;
	ExternalSelectDesc externalSelectDesc;

	/*
	 * get information from the estate and scan state
	 */
	estate = node->ss.ps.state;
	scandesc = node->ess_ScanDesc;
	direction = estate->es_direction;
	slot = node->ss.ss_ScanTupleSlot;

	/* EXX_IN_PG */
	if (node->exx_bc_projslot != 0) {
		slot = node->exx_bc_projslot;
	} else {
		slot = node->ss.ss_ScanTupleSlot;
	}

	if ((scandesc->fs_uri != NULL && IS_KITE_URI(scandesc->fs_uri))
			|| (scandesc->fs_formatter && exx_is_xrg_format(scandesc->fs_formatter->fmt_user_tag))) {
		/*
		* Intercept spq formatter.
		* XDrive implies spq, in old loftd code, execute curl will run spq formatter.
		* XXX: I believe the formater stuff should go away.
		*/
		if (scandesc->fs_noop) {
			return NULL;
		}
		while (true) {
			if (scandesc->fs_formatter == 0 || scandesc->fs_formatter->fmt_user_ctx == 0) {
				exx_external_xrg_begin(node);
			}
			slot = exx_external_xrg_next(node);
			if (TupIsNull(slot)) {
				if (!node->delayEagerFree) {
					ExecEagerFreeExternalScan(node);
				}
				break;
			}

			if (node->ess_ScanDesc->fs_hasConstraints && !ExternalConstraintCheck(slot, node)) {
				ExecClearTuple(slot);
				continue;
			}
			if (node->cdb_want_ctid && !TupIsNull(slot)) {
				slot_set_ctid_from_fake(slot, &node->cdb_fake_ctid);
			}
			break;
		}
		return slot;
	}

	externalSelectDesc = external_getnext_init(&(node->ss.ps));

	if (gp_external_enable_filter_pushdown)
		externalSelectDesc->filter_quals = node->ss.ps.plan->qual;
	/*
	 * get the next tuple from the file access methods
	 */
	while(scanNext)
	{
		tuple = external_getnext(scandesc, direction, externalSelectDesc);

		/*
		 * save the tuple and the buffer returned to us by the access methods in
		 * our scan tuple slot and return the slot.  Note: we pass 'false' because
		 * tuples returned by heap_getnext() are pointers onto disk pages and were
		 * not created with palloc() and so should not be pfree()'d.  Note also
		 * that ExecStoreTuple will increment the refcount of the buffer; the
		 * refcount will not be dropped until the tuple table slot is cleared.
		 */
		if (tuple)
		{
			ExecStoreHeapTuple(tuple, slot, InvalidBuffer, true);
			if (node->ess_ScanDesc->fs_hasConstraints && !ExternalConstraintCheck(slot, node))
			{
				ExecClearTuple(slot);
				continue;
			}
		    /*
		     * CDB: Label each row with a synthetic ctid if needed for subquery dedup.
		     */
		    if (node->cdb_want_ctid &&
		        !TupIsNull(slot))
		    {
		    	slot_set_ctid_from_fake(slot, &node->cdb_fake_ctid);
		    }
		}
		else
		{
			ExecClearTuple(slot);

			if (!node->delayEagerFree)
			{
				ExecEagerFreeExternalScan(node);
			}
		}
		scanNext = false;
	}
	pfree(externalSelectDesc);

	return slot;
}

/*
 * ExternalRecheck -- access method routine to recheck a tuple in EvalPlanQual
 */
static bool
ExternalRecheck(ExternalScanState *node, TupleTableSlot *slot)
{
	/* There are no access-method-specific conditions to recheck. */
	return true;
}

/* ----------------------------------------------------------------
*		ExecExternalScan(node)
*
*		Scans the external relation sequentially and returns the next qualifying
*		tuple.
*		It calls the ExecScan() routine and passes it the access method
*		which retrieve tuples sequentially.
*
*/

TupleTableSlot *
ExecExternalScan(ExternalScanState *node)
{
	/*
	 * use SeqNext as access method
	 */
	return ExecScan(&node->ss,
					(ExecScanAccessMtd) ExternalNext,
					(ExecScanRecheckMtd) ExternalRecheck);
}


/* ----------------------------------------------------------------
*		ExecInitExternalScan
* ----------------------------------------------------------------
*/
ExternalScanState *
ExecInitExternalScan(ExternalScan *node, EState *estate, int eflags)
{
	ExternalScanState *externalstate;
	Relation	currentRelation;
	FileScanDesc currentScanDesc;

	Assert(outerPlan(node) == NULL);
	Assert(innerPlan(node) == NULL);

	/*
	 * create state structure
	 */
	externalstate = makeNode(ExternalScanState);
	externalstate->ss.ps.plan = (Plan *) node;
	externalstate->ss.ps.state = estate;

	/*
	 * Miscellaneous initialization
	 *
	 * create expression context for node
	 */
	ExecAssignExprContext(estate, &externalstate->ss.ps);

	/*
	 * initialize child expressions
	 */
	externalstate->ss.ps.targetlist = (List *)
		ExecInitExpr((Expr *) node->scan.plan.targetlist,
					 (PlanState *) externalstate);
	externalstate->ss.ps.qual = (List *)
		ExecInitExpr((Expr *) node->scan.plan.qual,
					 (PlanState *) externalstate);

	/* Check if targetlist or qual contains a var node referencing the ctid column */
	externalstate->cdb_want_ctid = contain_ctid_var_reference(&node->scan);
	ItemPointerSetInvalid(&externalstate->cdb_fake_ctid);

	/*
	 * tuple table initialization
	 */
	ExecInitResultTupleSlot(estate, &externalstate->ss.ps);
	ExecInitScanTupleSlot(estate, &externalstate->ss);

	/*
	 * get the relation object id from the relid'th entry in the range table
	 * and open that relation.
	 */
	currentRelation = ExecOpenScanExternalRelation(estate, node->scan.scanrelid);


	currentScanDesc = external_beginscan(currentRelation,
									 node->scancounter,
									 node->uriList,
									 node->fmtOptString,
									 node->fmtType,
									 node->isMasterOnly,
									 node->rejLimit,
									 node->rejLimitInRows,
									 node->logErrors,
									 node->encoding);

	externalstate->ss.ss_currentRelation = currentRelation;
	externalstate->ess_ScanDesc = currentScanDesc;

	ExecAssignScanType(&externalstate->ss, RelationGetDescr(currentRelation));

	/*
	 * Initialize result tuple type and projection info.
	 */
	ExecAssignResultTypeFromTL(&externalstate->ss.ps);
	ExecAssignScanProjectionInfo(&externalstate->ss);

	/*
	 * EXX_IN_PG: qual and proj are processed by xdrive fve query engine.
	 */
	if (currentScanDesc->fs_uri != NULL && (fmttype_is_xrg_par_orc(node->fmtType) || fmttype_is_csv(node->fmtType))) {
		// BCLV: PROJ/BLOOM, will use the proj tupdesc of the scan.
		// BCLV: AGG, we have extended targetlist with aggstates.
		if (node->exx_bclv >= BCLV_PROJ) {
			ProjectionInfo* proj = externalstate->ss.ps.ps_ProjInfo;
			if (proj) {
				externalstate->exx_bc_projslot = proj->pi_slot;
				TupleDesc tupdesc = externalstate->exx_bc_projslot->tts_tupleDescriptor;
				int nproj = tupdesc->natts;
				externalstate->exx_bc_in_functions = (FmgrInfo *) palloc(nproj * sizeof(FmgrInfo));
				externalstate->exx_bc_typioparams = (Oid *) palloc(nproj *sizeof(Oid));
				for (int i = 0; i < nproj; i++) {
					Oid infuncoid;
					getTypeInputInfo(tupdesc->attrs[i]->atttypid,
									&infuncoid, &externalstate->exx_bc_typioparams[i]);
					fmgr_info(infuncoid, &externalstate->exx_bc_in_functions[i]);
				}
			}
		}
	}

	/*
	 * If eflag contains EXEC_FLAG_REWIND or EXEC_FLAG_BACKWARD or EXEC_FLAG_MARK,
	 * then this node is not eager free safe.
	 */
	externalstate->delayEagerFree =
		((eflags & (EXEC_FLAG_REWIND | EXEC_FLAG_BACKWARD | EXEC_FLAG_MARK)) != 0);

	return externalstate;
}

/* ----------------------------------------------------------------
*		ExecEndExternalScan
*
*		frees any storage allocated through C routines.
* ----------------------------------------------------------------
*/
void
ExecEndExternalScan(ExternalScanState *node)
{
	/*
	 * Free the exprcontext
	 */
	ExecFreeExprContext(&node->ss.ps);

	/*
	 * clean out the tuple table
	 */
	ExecClearTuple(node->ss.ps.ps_ResultTupleSlot);
	ExecClearTuple(node->ss.ss_ScanTupleSlot);

	ExecEagerFreeExternalScan(node);
	pfree(node->ess_ScanDesc);

	/*
	 * close the external relation.
	 *
	 * MPP-8040: make sure we don't close it if it hasn't completed setup, or
	 * if we've already closed it.
	 */
	if (node->ss.ss_currentRelation)
	{
		Relation	relation = node->ss.ss_currentRelation;

		node->ss.ss_currentRelation = NULL;
		ExecCloseScanRelation(relation);
	}
	EndPlanStateGpmonPkt(&node->ss.ps);
}


/* ----------------------------------------------------------------
*						Join Support
* ----------------------------------------------------------------
*/

/* ----------------------------------------------------------------
*		ExecReScanExternal
*
*		Rescans the relation.
* ----------------------------------------------------------------
*/
void
ExecReScanExternal(ExternalScanState *node)
{
	FileScanDesc fileScan = node->ess_ScanDesc;

	ItemPointerSet(&node->cdb_fake_ctid, 0, 0);

	external_rescan(fileScan);

	/* EXX_IS_PG */
	if (fileScan->fs_formatter && exx_is_xrg_format(fileScan->fs_formatter->fmt_user_tag)) {
		elog(LOG, "XRG: External table rescan???");
		exx_external_xrg_end(node);
	} else {
		external_rescan(fileScan);
	}
}

static void
ExecEagerFreeExternalScan(ExternalScanState *node)
{
	Assert(node->ess_ScanDesc != NULL);
	external_endscan(node->ess_ScanDesc);
}

/* ----------------------------------------------------------------
*		ExecSquelchExternalScan
*
*		Performs identically to ExecEndExternalScan except that
*		closure errors are ignored.  This function is called for
*		normal termination when the external data source is NOT
*		exhausted (such as for a LIMIT clause).
* ----------------------------------------------------------------
*/
void
ExecSquelchExternalScan(ExternalScanState *node)
{
	FileScanDesc fileScanDesc;

	/*
	 * get information from node
	 */
	fileScanDesc = node->ess_ScanDesc;

	/* EXX_IN_PG */
	if (fileScanDesc->fs_formatter && exx_is_xrg_format(fileScanDesc->fs_formatter->fmt_user_tag)) {
		elog(LOG, "XRG: spq external node stop scan.");
		exx_external_xrg_end(node);
	}

	/*
	 * stop the file scan
	 */
	external_stopscan(fileScanDesc);

	if (!node->delayEagerFree)
		ExecEagerFreeExternalScan(node);
}

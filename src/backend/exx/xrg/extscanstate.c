#include "../exx_int.h"
#include "kite_extscan.h"

/* ExternalScanState utility */
ExternalScan *scan_plan(ExternalScanState *node) {
        return (ExternalScan *)node->ss.ps.plan;
}

TupleTableSlot *scan_slot(ExternalScanState *node) {
        return node->ss.ss_ScanTupleSlot;
}

TupleDesc scan_tupdesc(ExternalScanState *node) {
        return scan_slot(node)->tts_tupleDescriptor;
}

int scan_ncol(ExternalScanState *node) {
        return scan_tupdesc(node)->natts;
}

TupleTableSlot *exec_slot(ExternalScanState *node) {
        if (node->exx_bc_projslot) {
                return node->exx_bc_projslot;
        }
        return scan_slot(node);
}

TupleDesc exec_tupdesc(ExternalScanState *node) {
        return exec_slot(node)->tts_tupleDescriptor;
}

int exec_ncol(ExternalScanState *node) {
        return exec_tupdesc(node)->natts;
}

FileScanDesc scan_scandesc(ExternalScanState *node) {
        return node->ess_ScanDesc;
}

CopyState scan_copystate(ExternalScanState *node) {
        return scan_scandesc(node)->fs_pstate;
}

URL_FILE *scan_urlfile(ExternalScanState *node) {
        return (URL_FILE *)scan_scandesc(node)->fs_file;
}

FmgrInfo *in_funcs(ExternalScanState *node) {
        if (node->exx_bc_in_functions) {
                return node->exx_bc_in_functions;
        }
        return scan_scandesc(node)->in_functions;
}

Oid *in_ioparams(ExternalScanState *node) {
        if (node->exx_bc_typioparams) {
                return node->exx_bc_typioparams;
        }
        return scan_scandesc(node)->typioparams;
}

char *colname(ExternalScanState *node, int attno) {
        return NameStr(scan_tupdesc(node)->attrs[attno - 1]->attname);
}


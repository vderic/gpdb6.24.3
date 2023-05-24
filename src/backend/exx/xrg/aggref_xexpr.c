#include "../exx_int.h"

#ifdef __cplusplus
extern "C" {
#endif

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


#ifdef __cplusplus
}
#endif

#include "aggop.h"
#include "aggref.h"
#include "../util/exx_sarg.h"
#include "../util/xexpr_types.h"

/**
 * Serialize PG Node to XExpr format
 */

static void aggref_fill_xexpr(Node *node, xex_list_t *list) {
	Aggref *aggref = (Aggref *)node;

	xex_list_append_string(list, AGGREF_TYPE);

	// aggfnoid (Oid)
	xex_list_append_uint32(list, aggref->aggfnoid);

	// aggtype (Oid)
	xex_list_append_uint32(list, aggref->aggtype);

	// aggcollid (Oid)
	xex_list_append_uint32(list, aggref->aggcollid);

	// aggstar
	xex_list_append_int8(list, aggref->aggstar);

	// aggvariadic
	xex_list_append_int8(list, aggref->aggvariadic);
}

static void funcexpr_fill_xexpr(Node *node, xex_list_t *list) {
	FuncExpr *op = (FuncExpr *)node;

	xex_list_append_string(list, FUNCEXPR_TYPE);

	// funcid
	xex_list_append_uint32(list, op->funcid);

	// funcresulttype
	xex_list_append_uint32(list, op->funcresulttype);

	// funcretset
	xex_list_append_int8(list, op->funcretset);

	// funcvariadic
	xex_list_append_int8(list, op->funcvariadic);

	// opcollid
	xex_list_append_uint32(list, op->funccollid);

	// inputcollid
	xex_list_append_uint32(list, op->inputcollid);

	// is_tablefunc
	xex_list_append_int8(list, op->is_tablefunc);
}

static void opexpr_fill_xexpr(Node *node, xex_list_t *list) {

	OpExpr *op = (OpExpr *)node;

	xex_list_append_string(list, OPEXPR_TYPE);

	// opno
	xex_list_append_uint32(list, op->opno);

	// opfuncid
	xex_list_append_uint32(list, op->opfuncid);

	// opresulttype
	xex_list_append_uint32(list, op->opresulttype);

	// opretset
	xex_list_append_int8(list, op->opretset);

	// opcollid
	xex_list_append_uint32(list, op->opcollid);

	// inputcollid
	xex_list_append_uint32(list, op->inputcollid);
}

static void nullifexpr_fill_xexpr(Node *node, xex_list_t *list) {

	OpExpr *op = (OpExpr *)node;

	xex_list_append_string(list, NULLIFEXPR_TYPE);

	// opno
	xex_list_append_uint32(list, op->opno);

	// opfuncid
	xex_list_append_uint32(list, op->opfuncid);

	// opresulttype
	xex_list_append_uint32(list, op->opresulttype);

	// opretset
	xex_list_append_int8(list, op->opretset);

	// inputcollid
	xex_list_append_uint32(list, op->inputcollid);
}

static void coalesceexpr_fill_xexpr(Node *node, xex_list_t *list) {

	CoalesceExpr *op = (CoalesceExpr *)node;

	xex_list_append_string(list, COALESCEEXPR_TYPE);

	// coalescetype
	xex_list_append_uint32(list, op->coalescetype);

	// coalescecollid
	xex_list_append_uint32(list, op->coalescecollid);
}

static void boolexpr_fill_xexpr(Node *node, xex_list_t *list) {
	BoolExpr *op = (BoolExpr *)node;

	xex_list_append_string(list, BOOLEXPR_TYPE);

	const char *opstr = NULL;
	switch (op->boolop) {
	case AND_EXPR:
		opstr = "and";
		break;
	case OR_EXPR:
		opstr = "or";
		break;
	case NOT_EXPR:
		opstr = "not";
		break;
	}

	xex_list_append_string(list, opstr);
}

static void const_fill_xexpr(Node *node, xex_list_t *list) {
	Const *c = (Const *)node;
	int16_t ptyp, ltyp, precision, scale;
	bool is_array = false;
	const char *constvalue = 0;
	const char *ts = 0;

	xex_list_append_string(list, CONST_TYPE);

	// consttype (Oid)
	xex_list_append_uint32(list, c->consttype);

	// consttypmod
	xex_list_append_int32(list, c->consttypmod);

	// constcollid (Oid)
	xex_list_append_uint32(list, c->constcollid);

	// constlen
	xex_list_append_int32(list, c->constlen);

	// constbyval
	xex_list_append_int8(list, c->constbyval);

	// constisnull
	xex_list_append_int8(list, c->constisnull);

	// constvalue
	// constvalue
	ptyp = ltyp = precision = scale = 0;
	pg_typ_to_xrg_typ(c->consttype, c->consttypmod, &ptyp, &ltyp, &precision, &scale, &is_array);
	ts = xrg_typ_str(ptyp, ltyp);
	Insist(ts && *ts != 0);
	if (strcmp(ts, "string") == 0) {
		if (c->constlen == -1) {
			ts = "text";
		}
	}

	switch (c->consttype) {
	case INTERVALOID: // interval
	case DATEOID:
	case TIMEOID:
	case TIMESTAMPOID:
	case TIMESTAMPTZOID:
	case BPCHAROID:
	case TEXTOID:	// text
	case VARCHAROID: // cstring
		constvalue = op_sarg_const_str(ts, c->constvalue, 0);
		break;
	case 1000: // array of BOOL
	case INT2ARRAYOID:
	case INT4ARRAYOID:
	case INT8ARRAYOID:
	case FLOAT4ARRAYOID:
	case FLOAT8ARRAYOID:
	case TEXTARRAYOID:
	case 1014: // array of bpchar character(num)
	case 1182: // array of DATE
	case 1183: // array of TIME
	case 1115: // array of TIMESTAMP
	case 1185: // array of TIMESTAMPTZ
	case 1231: // array to numeric
		constvalue = op_arraytype_to_string(c);
		break;
	default:
		constvalue = op_sarg_const_str(ts, c->constvalue, 0);
		break;
	}

	Insist(constvalue);

	xex_list_append_string(list, constvalue);

	if (constvalue) {
		pfree((void *)constvalue);
	}
}

static void var_fill_xexpr(Node *node, xex_list_t *list) {
	Var *v = (Var *)node;

	xex_list_append_string(list, VAR_TYPE);

	// varno
	xex_list_append_uint32(list, v->varno);

	// varattno
	xex_list_append_int32(list, v->varattno);

	// vartype
	xex_list_append_uint32(list, v->vartype);

	// vartypmod
	xex_list_append_int32(list, v->vartypmod);

	// varlevelsup
	xex_list_append_uint32(list, v->varlevelsup);

	// varnoold
	xex_list_append_uint32(list, v->varnoold);

	// varoattno
	xex_list_append_int32(list, v->varoattno);
}

static void caseexpr_fill_xexpr(Node *node, xex_list_t *list) {
	CaseExpr *op = (CaseExpr *)node;

	xex_list_append_string(list, CASEEXPR_TYPE);

	// castype
	xex_list_append_uint32(list, op->casetype);

	// casecollid
	xex_list_append_uint32(list, op->casecollid);
}

static void scalararrayopexpr_fill_xexpr(Node *node, xex_list_t *list) {
	ScalarArrayOpExpr *op = (ScalarArrayOpExpr *)node;

	xex_list_append_string(list, SCALARARRAYOPEXPR_TYPE);

	// opno
	xex_list_append_uint32(list, op->opno);

	// opfuncid
	xex_list_append_uint32(list, op->opfuncid);

	// useOr
	xex_list_append_int8(list, op->useOr);

	// inputcollid
	xex_list_append_uint32(list, op->inputcollid);
}

static void casewhen_fill_xexpr(Node *node, xex_list_t *list) {
	(void)node;
	xex_list_append_string(list, CASEWHEN_TYPE);
}

static void casetestexpr_fill_xexpr(Node *node, xex_list_t *list) {
	CaseTestExpr *e = (CaseTestExpr *)node;

	xex_list_append_string(list, CASETESTEXPR_TYPE);

	// typeId
	xex_list_append_uint32(list, e->typeId);

	// typeMod
	xex_list_append_int32(list, e->typeMod);

	// collation
	xex_list_append_uint32(list, e->collation);
}

bool node_to_xexpr(Node *node, void *ptr) {
	xex_list_t *parent = (xex_list_t *)ptr;
	ListCell *lc;

	if (!node) {
		return false;
	}

	if (IsA(node, Aggref)) {
		Aggref *aggref = (Aggref *)node;

		xex_list_t *list = xex_list_create();

		aggref_fill_xexpr(node, list);

		xex_list_t *args = xex_list_create();
		if (list_length(aggref->args) > 0) {

			foreach (lc, aggref->args) {
				node_to_xexpr((Node *)lfirst(lc), args);
			}
		}

		xex_list_append_list(list, args);

		xex_list_append_list(parent, list);

		return false;
	}

	if (IsA(node, OpExpr)) {
		OpExpr *opexpr = (OpExpr *)node;

		xex_list_t *list = xex_list_create();

		opexpr_fill_xexpr(node, list);

		xex_list_t *args = xex_list_create();
		foreach (lc, opexpr->args) {
			node_to_xexpr((Node *)lfirst(lc), args);
		}
		xex_list_append_list(list, args);

		xex_list_append_list(parent, list);

		return false;
	}

	if (IsA(node, BoolExpr)) {
		BoolExpr *boolexpr = (BoolExpr *)node;
		xex_list_t *list = xex_list_create();

		boolexpr_fill_xexpr(node, list);

		xex_list_t *args = xex_list_create();
		foreach (lc, boolexpr->args) {
			node_to_xexpr((Node *)lfirst(lc), args);
		}
		xex_list_append_list(list, args);

		xex_list_append_list(parent, list);

		return false;
	}

	if (IsA(node, FuncExpr)) {
		FuncExpr *funcexpr = (FuncExpr *)node;
		xex_list_t *list = xex_list_create();

		funcexpr_fill_xexpr(node, list);

		xex_list_t *args = xex_list_create();
		foreach (lc, funcexpr->args) {
			node_to_xexpr((Node *)lfirst(lc), args);
		}
		xex_list_append_list(list, args);
		xex_list_append_list(parent, list);

		return false;
	}

	if (IsA(node, Const)) {
		xex_list_t *list = xex_list_create();
		const_fill_xexpr(node, list);
		xex_list_append_list(parent, list);
		return false;
	}

	if (IsA(node, Var)) {
		xex_list_t *list = xex_list_create();
		var_fill_xexpr(node, list);
		xex_list_append_list(parent, list);
		return false;
	}

	if (IsA(node, NullIfExpr)) {
		NullIfExpr *nullifexpr = (NullIfExpr *)node;
		xex_list_t *list = xex_list_create();
		nullifexpr_fill_xexpr(node, list);

		xex_list_t *args = xex_list_create();
		foreach (lc, nullifexpr->args) {
			node_to_xexpr((Node *)lfirst(lc), args);
		}
		xex_list_append_list(list, args);
		xex_list_append_list(parent, list);
		return false;
	}

	if (IsA(node, CoalesceExpr)) {
		CoalesceExpr *coalesceexpr = (CoalesceExpr *)node;
		xex_list_t *list = xex_list_create();
		coalesceexpr_fill_xexpr(node, list);

		xex_list_t *args = xex_list_create();
		foreach (lc, coalesceexpr->args) {
			node_to_xexpr((Node *)lfirst(lc), args);
		}
		xex_list_append_list(list, args);
		xex_list_append_list(parent, list);
		return false;
	}

	if (IsA(node, CaseExpr)) {
		CaseExpr *caseexpr = (CaseExpr *)node;
		xex_list_t *list = xex_list_create();
		caseexpr_fill_xexpr(node, list);

		// arg
		if (caseexpr->arg) {
			// second case - simple case CASE testexpr WHEN compexpr THEN expr [ WHEN compexpr THEN expr ... ]
			node_to_xexpr((Node *)caseexpr->arg, list);
		} else {
			// add NULL string to replace arg
			xex_list_append_string(list, "NULL");
		}

		// first case - general case
		xex_list_t *args = xex_list_create();
		foreach (lc, caseexpr->args) {
			node_to_xexpr((Node *)lfirst(lc), args);
		}
		xex_list_append_list(list, args);

		// default result (ELSE clause)
		// caseexpr->defresult
		xex_list_t *defresult = xex_list_create();
		node_to_xexpr((Node *)caseexpr->defresult, defresult);
		xex_list_append_list(list, defresult);

		xex_list_append_list(parent, list);
		return false;
	}

	if (IsA(node, CaseWhen)) {
		CaseWhen *casewhen = (CaseWhen *)node;
		xex_list_t *list = xex_list_create();
		casewhen_fill_xexpr(node, list);

		// casewhen->expr
		node_to_xexpr((Node *)casewhen->expr, list);

		// casewhen->result
		node_to_xexpr((Node *)casewhen->result, list);

		xex_list_append_list(parent, list);
		return false;
	}

	if (IsA(node, CaseTestExpr)) {
		xex_list_t *list = xex_list_create();
		casetestexpr_fill_xexpr(node, list);
		xex_list_append_list(parent, list);
		return false;
	}

	if (IsA(node, ScalarArrayOpExpr)) {
		ScalarArrayOpExpr *op = (ScalarArrayOpExpr *)node;
		xex_list_t *list = xex_list_create();
		scalararrayopexpr_fill_xexpr(node, list);

		xex_list_t *args = xex_list_create();
		foreach (lc, op->args) {
			node_to_xexpr((Node *)lfirst(lc), args);
		}

		xex_list_append_list(list, args);
		xex_list_append_list(parent, list);

		return false;
	}

	return expression_tree_walker(node, (GP_FUNC_PTR_MESS)node_to_xexpr, parent);
}

bool not_required_fill_xexpr(void *ptr) {
	xex_list_t *parent = (xex_list_t *)ptr;
	xex_list_t *list = xex_list_create();
	xex_list_append_string(list, INVALID_TYPE);
	xex_list_append_list(parent, list);

	return true;
}

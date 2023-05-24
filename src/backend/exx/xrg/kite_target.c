#include "../exx_int.h"

#include "kite_extscan.h"
#include "../util/xexpr_types.h"
#include "aggop.h"
#include "../util/stringbuffer.h"
#include "../util/decimal.h"
#include "decode.h"

/**
 * Convert serialized PG Node (XEXPR format) into kite_target_t data structure
 */

static const char *funcid_to_str(Oid funcid, int *nargs) {

	int32_t op = pg_proc_to_op(funcid);
	Insist(op != -1);

	*nargs = 2;
	return xrg_opexpr_str(op);
}

static const char *aggfnoid_to_opstr(Oid aggfnoid) {

	int32_t op = pg_agg_to_op(aggfnoid);
	Insist(op != -1);

	return xrg_opexpr_str(op);
}

/* xexpr parsing functions */
static void traverse_funcexpr(xex_list_t *list, TupleDesc tupdesc, stringbuffer_t *strbuf) {
	const char *type;
	Oid funcid, funcresulttype, funccollid, inputcollid;
	int8_t funcretset, funcvariadic, is_tablefunc;

	int n = xex_list_length(list);
	Insist(n == 9);

	type = xex_list_get_string(list, 0); // type
	Insist(type && strcmp(type, FUNCEXPR_TYPE) == 0);

	Insist(xex_list_get_uint32(list, 1, &funcid) == 0);			// funcid
	Insist(xex_list_get_uint32(list, 2, &funcresulttype) == 0); // funcresulttype
	Insist(xex_list_get_int8(list, 3, &funcretset) == 0);		// funcretset
	Insist(xex_list_get_int8(list, 4, &funcvariadic) == 0);		// funcvariadic
	Insist(xex_list_get_uint32(list, 5, &funccollid) == 0);		// funccollid
	Insist(xex_list_get_uint32(list, 6, &inputcollid) == 0);	// inputcollid
	Insist(xex_list_get_int8(list, 7, &is_tablefunc) == 0);		// is_tablefunc

	// args
	xex_object_t *obj = xex_list_get(list, 8); // args
	Insist(obj);
	xex_list_t *args = xex_to_list(obj);
	Insist(args);

	int32_t op = pg_func_to_op(funcid);

	if (op == XRG_OP_CAST) {
		for (int i = 0; i < xex_list_length(args); i++) {
			obj = xex_list_get(args, i);
			Insist(obj);
			xex_list_t *l = xex_to_list(obj);
			Insist(l);
			traverse(l, tupdesc, strbuf);
		}
	} else if (op != -1) {
		stringbuffer_append_string(strbuf, (char *)xrg_opexpr_str(op));
		stringbuffer_append(strbuf, '(');
		for (int i = 0; i < xex_list_length(args); i++) {
			if (i > 0) {
				stringbuffer_append(strbuf, ',');
			}
			obj = xex_list_get(args, i);
			Insist(obj);
			xex_list_t *l = xex_to_list(obj);
			Insist(l);
			traverse(l, tupdesc, strbuf);
		}
		stringbuffer_append(strbuf, ')');
	} else {
		elog(ERROR, "funcexpr not supported. funcid = %d", funcid);
	}
}

static void traverse_opexpr(xex_list_t *list, TupleDesc tupdesc, stringbuffer_t *strbuf) {
	const char *type;
	Oid opno, opfuncid, opresulttype, opcollid, inputcollid;
	int8_t opretset;

	int n = xex_list_length(list);
	Insist(n == 8);

	type = xex_list_get_string(list, 0); // type
	Insist(type && strcmp(type, OPEXPR_TYPE) == 0);

	Insist(xex_list_get_uint32(list, 1, &opno) == 0);		  // opno
	Insist(xex_list_get_uint32(list, 2, &opfuncid) == 0);	 // opfuncid
	Insist(xex_list_get_uint32(list, 3, &opresulttype) == 0); // opresulttype
	Insist(xex_list_get_int8(list, 4, &opretset) == 0);		  // opretset
	Insist(xex_list_get_uint32(list, 5, &opcollid) == 0);	 // opcollid
	Insist(xex_list_get_uint32(list, 6, &inputcollid) == 0);  // inputcollid

	int expect_nargs = 0;
	const char *opstr = funcid_to_str(opfuncid, &expect_nargs);
	Insist(opstr);

	// args
	xex_object_t *obj = xex_list_get(list, 7); // args
	Insist(obj);
	xex_list_t *args = xex_to_list(obj);
	Insist(args && xex_list_length(args) == expect_nargs);

	xex_object_t *leftobj = xex_list_get(args, 0);
	xex_list_t *left = xex_to_list(leftobj);
	xex_object_t *rightobj = xex_list_get(args, 1);
	xex_list_t *right = xex_to_list(rightobj);
	Insist(left);
	Insist(right);

	const char *ltype = xex_list_get_string(left, 0);
	Insist(ltype);
	const char *rtype = xex_list_get_string(right, 0);
	Insist(rtype);

	if (strcmp(ltype, CASETESTEXPR_TYPE) == 0) {
		traverse(right, tupdesc, strbuf);
	} else {
		if (strcmp(ltype, OPEXPR_TYPE) == 0) {
			stringbuffer_append(strbuf, '(');
			traverse(left, tupdesc, strbuf);
			stringbuffer_append(strbuf, ')');
		} else {
			traverse(left, tupdesc, strbuf);
		}
		stringbuffer_append(strbuf, ' ');
		stringbuffer_append_string(strbuf, (char *)opstr);
		stringbuffer_append(strbuf, ' ');
		if (strcmp(rtype, OPEXPR_TYPE) == 0) {
			stringbuffer_append(strbuf, '(');
			traverse(right, tupdesc, strbuf);
			stringbuffer_append(strbuf, ')');
		} else {
			traverse(right, tupdesc, strbuf);
		}
	}
}

static void traverse_const(xex_list_t *list, TupleDesc tupdesc, stringbuffer_t *strbuf) {
	const char *type, *constvalue;
	Oid consttype, constcollid;
	int32_t consttypmod, constlen;
	int8_t constbyval, constisnull;

	int n = xex_list_length(list);
	Insist(n == 8);

	type = xex_list_get_string(list, 0); // type
	Insist(type && strcmp(type, CONST_TYPE) == 0);

	Insist(xex_list_get_uint32(list, 1, &consttype) == 0); // consttype

	Insist(xex_list_get_int32(list, 2, &consttypmod) == 0); // consttypmod

	Insist(xex_list_get_uint32(list, 3, &constcollid) == 0); // constcollid

	Insist(xex_list_get_int32(list, 4, &constlen) == 0); // constlen

	Insist(xex_list_get_int8(list, 5, &constbyval) == 0); // constbyval

	Insist(xex_list_get_int8(list, 6, &constisnull) == 0); // constisnull

	constvalue = xex_list_get_string(list, 7); // constvalue
	Insist(constvalue);

	switch (consttype) {
	case INTERVALOID: // interval
		stringbuffer_append_string(strbuf, "INTERVAL ");
		stringbuffer_append(strbuf, '\'');
		stringbuffer_append_string(strbuf, (char *)constvalue);
		stringbuffer_append(strbuf, '\'');
		break;
	case DATEOID:
		stringbuffer_append_string(strbuf, "DATE ");
		stringbuffer_append(strbuf, '\'');
		stringbuffer_append_string(strbuf, (char *)constvalue);
		stringbuffer_append(strbuf, '\'');
		break;
	case TIMEOID:
		stringbuffer_append_string(strbuf, "TIME ");
		stringbuffer_append(strbuf, '\'');
		stringbuffer_append_string(strbuf, (char *)constvalue);
		stringbuffer_append(strbuf, '\'');
		break;
	case TIMESTAMPOID:
	case TIMESTAMPTZOID:
		stringbuffer_append_string(strbuf, "TIMESTAMP ");
		stringbuffer_append(strbuf, '\'');
		stringbuffer_append_string(strbuf, (char *)constvalue);
		stringbuffer_append(strbuf, '\'');
		break;
	case BPCHAROID:  // character(n)
	case TEXTOID:	// text
	case VARCHAROID: // cstring
		// single quote escaped inside op_sarg_const_str
		stringbuffer_append(strbuf, '\'');
		stringbuffer_append_string(strbuf, (char *)constvalue);
		stringbuffer_append(strbuf, '\'');
		break;
	default:
		stringbuffer_append_string(strbuf, (char *)constvalue);
		break;
	}
}

static void traverse_var(xex_list_t *list, TupleDesc tupdesc, stringbuffer_t *strbuf) {
	const char *type;
	Oid varno, vartype, varlevelsup, varnoold;
	int32_t varattno, vartypmod, varoattno;

	int n = xex_list_length(list);
	Insist(n == 8);

	type = xex_list_get_string(list, 0); // type
	Insist(type && strcmp(type, VAR_TYPE) == 0);

	Insist(xex_list_get_uint32(list, 1, &varno) == 0); // varno

	Insist(xex_list_get_int32(list, 2, &varattno) == 0); // varattno

	Insist(xex_list_get_uint32(list, 3, &vartype) == 0); // vartype

	Insist(xex_list_get_int32(list, 4, &vartypmod) == 0); // vartypmod

	Insist(xex_list_get_uint32(list, 5, &varlevelsup) == 0); // varlevelsup

	Insist(xex_list_get_uint32(list, 6, &varnoold) == 0); // varnoold

	Insist(xex_list_get_int32(list, 7, &varoattno) == 0); // varoattno

	char *cn = NameStr(tupdesc->attrs[varoattno - 1]->attname);
	stringbuffer_append_string(strbuf, cn);
}

static void traverse_nullifexpr(xex_list_t *list, TupleDesc tupdesc, stringbuffer_t *strbuf) {
	const char *type;
	Oid opno, opfuncid, opresulttype, inputcollid;
	int8_t opretset;

	int n = xex_list_length(list);
	Insist(n == 7);

	type = xex_list_get_string(list, 0); // type
	Insist(type && strcmp(type, NULLIFEXPR_TYPE) == 0);

	Insist(xex_list_get_uint32(list, 1, &opno) == 0);		  // opno
	Insist(xex_list_get_uint32(list, 2, &opfuncid) == 0);	 // opfuncid
	Insist(xex_list_get_uint32(list, 3, &opresulttype) == 0); // opresulttype
	Insist(xex_list_get_int8(list, 4, &opretset) == 0);		  // opretset
	Insist(xex_list_get_uint32(list, 5, &inputcollid) == 0);  // inputcollid

	// args
	xex_object_t *obj = xex_list_get(list, 6); // args
	Insist(obj);
	xex_list_t *args = xex_to_list(obj);
	Insist(args && xex_list_length(args) == 2);

	xex_object_t *leftobj = xex_list_get(args, 0);
	xex_list_t *left = xex_to_list(leftobj);
	xex_object_t *rightobj = xex_list_get(args, 1);
	xex_list_t *right = xex_to_list(rightobj);
	Insist(left);
	Insist(right);

	const char *ltype = xex_list_get_string(left, 0);
	Insist(ltype);
	const char *rtype = xex_list_get_string(right, 0);
	Insist(rtype);

	stringbuffer_append_string(strbuf, "NULLIF(");
	if (strcmp(ltype, OPEXPR_TYPE) == 0) {
		stringbuffer_append(strbuf, '(');
		traverse(left, tupdesc, strbuf);
		stringbuffer_append(strbuf, ')');
	} else {
		traverse(left, tupdesc, strbuf);
	}
	stringbuffer_append(strbuf, ',');
	if (strcmp(rtype, OPEXPR_TYPE) == 0) {
		stringbuffer_append(strbuf, '(');
		traverse(right, tupdesc, strbuf);
		stringbuffer_append(strbuf, ')');
	} else {
		traverse(right, tupdesc, strbuf);
	}
	stringbuffer_append(strbuf, ')');
}

static void traverse_coalesceexpr(xex_list_t *list, TupleDesc tupdesc, stringbuffer_t *strbuf) {
	const char *type;
	uint32_t coalescetype, coalescecollid;

	int n = xex_list_length(list);
	Insist(n == 4);

	type = xex_list_get_string(list, 0); // type
	Insist(type && strcmp(type, COALESCEEXPR_TYPE) == 0);

	Insist(xex_list_get_uint32(list, 1, &coalescetype) == 0); // coalescetype

	Insist(xex_list_get_uint32(list, 2, &coalescecollid) == 0); // coalescecollid

	// args
	xex_object_t *obj = xex_list_get(list, 3); // args
	Insist(obj);
	xex_list_t *args = xex_to_list(obj);
	Insist(args);

	stringbuffer_append_string(strbuf, "COALESCE(");
	for (int i = 0; i < xex_list_length(args); i++) {
		if (i > 0) {
			stringbuffer_append(strbuf, ',');
		}
		obj = xex_list_get(args, i);
		xex_list_t *l = xex_to_list(obj);
		Insist(l);
		traverse(l, tupdesc, strbuf);
	}
	stringbuffer_append(strbuf, ')');
}

static void traverse_boolexpr(xex_list_t *list, TupleDesc tupdesc, stringbuffer_t *strbuf) {
	const char *type, *opstr;

	int n = xex_list_length(list);
	Insist(n >= 2);

	type = xex_list_get_string(list, 0); // type
	Insist(type && strcmp(type, BOOLEXPR_TYPE) == 0);

	opstr = xex_list_get_string(list, 1); // opstr

	// args
}

static void traverse_caseexpr(xex_list_t *list, TupleDesc tupdesc, stringbuffer_t *strbuf) {
	const char *type;
	Oid casetype, casecollid;

	int n = xex_list_length(list);
	Insist(n == 6);

	type = xex_list_get_string(list, 0); // type
	Insist(type && strcmp(type, CASEEXPR_TYPE) == 0);

	Insist(xex_list_get_uint32(list, 1, &casetype) == 0); // casetype

	Insist(xex_list_get_uint32(list, 2, &casecollid) == 0); // &casecollid

	stringbuffer_append_string(strbuf, "CASE ");

	// arg
	xex_object_t *obj = xex_list_get(list, 3); // args
	Insist(obj);
	xex_list_t *arg = xex_to_list(obj);
	if (arg) {
		traverse(arg, tupdesc, strbuf);
		stringbuffer_append(strbuf, ' ');
	}

	// args
	obj = xex_list_get(list, 4); // args
	Insist(obj);
	xex_list_t *args = xex_to_list(obj);
	Insist(args);

	for (int i = 0; i < xex_list_length(args); i++) {
		obj = xex_list_get(args, i);
		Insist(obj);
		xex_list_t *l = xex_to_list(obj);
		Insist(l);
		traverse(l, tupdesc, strbuf);
		stringbuffer_append(strbuf, ' ');
	}

	stringbuffer_append_string(strbuf, "ELSE ");

	// defresult
	obj = xex_list_get(list, 5); // args
	Insist(obj);
	xex_list_t *defresult = xex_to_list(obj);
	Insist(defresult);
	traverse(defresult, tupdesc, strbuf);
	stringbuffer_append_string(strbuf, " END");
}

static void traverse_scalararrayopexpr(xex_list_t *list, TupleDesc tupdesc, stringbuffer_t *strbuf) {
	const char *type;
	Oid opno, opfuncid, inputcollid;
	int8_t useOr;

	int n = xex_list_length(list);
	Insist(n == 6);

	type = xex_list_get_string(list, 0); // type
	Insist(type && strcmp(type, SCALARARRAYOPEXPR_TYPE) == 0);

	Insist(xex_list_get_uint32(list, 1, &opno) == 0); // opno

	Insist(xex_list_get_uint32(list, 2, &opfuncid) == 0); // opfuncid

	Insist(xex_list_get_int8(list, 3, &useOr) == 0); // useOr

	Insist(xex_list_get_uint32(list, 4, &inputcollid) == 0); // inputcollid

	// args
	xex_object_t *obj = xex_list_get(list, 5); // args
	Insist(obj);
	xex_list_t *args = xex_to_list(obj);
	Insist(args && xex_list_length(args) == 2);
}

static void traverse_casewhen(xex_list_t *list, TupleDesc tupdesc, stringbuffer_t *strbuf) {
	const char *type;
	int n = xex_list_length(list);
	Insist(n == 3);

	type = xex_list_get_string(list, 0); // type
	Insist(type && strcmp(type, CASEWHEN_TYPE) == 0);

	stringbuffer_append_string(strbuf, "WHEN ");
	// expr
	xex_object_t *obj = xex_list_get(list, 1); // expr
	Insist(obj);
	xex_list_t *expr = xex_to_list(obj);

	traverse(expr, tupdesc, strbuf);

	stringbuffer_append_string(strbuf, " THEN ");

	// result
	obj = xex_list_get(list, 2); // result
	Insist(obj);
	xex_list_t *result = xex_to_list(obj);
	traverse(result, tupdesc, strbuf);
}

static void traverse_casetestexpr(xex_list_t *list, TupleDesc tupdesc, stringbuffer_t *strbuf) {
	const char *type;
	Oid typeId, collation;
	int32_t typeMod;

	int n = xex_list_length(list);
	Insist(n >= 4);

	type = xex_list_get_string(list, 0); // type
	Insist(type && strcmp(type, CASETESTEXPR_TYPE) == 0);

	Insist(xex_list_get_uint32(list, 1, &typeId) == 0);	// typeId
	Insist(xex_list_get_int32(list, 2, &typeMod) == 0);	// typeMod
	Insist(xex_list_get_uint32(list, 3, &collation) == 0); // collation

	// args
}

void traverse(xex_list_t *list, TupleDesc tupdesc, stringbuffer_t *strbuf) {
	const char *type = xex_list_get_string(list, 0);
	Insist(type);

	if (strcmp(type, VAR_TYPE) == 0) {
		traverse_var(list, tupdesc, strbuf);
	} else if (strcmp(type, CONST_TYPE) == 0) {
		traverse_const(list, tupdesc, strbuf);
	} else if (strcmp(type, FUNCEXPR_TYPE) == 0) {
		traverse_funcexpr(list, tupdesc, strbuf);
	} else if (strcmp(type, OPEXPR_TYPE) == 0) {
		traverse_opexpr(list, tupdesc, strbuf);
	} else if (strcmp(type, NULLIFEXPR_TYPE) == 0) {
		traverse_nullifexpr(list, tupdesc, strbuf);
	} else if (strcmp(type, COALESCEEXPR_TYPE) == 0) {
		traverse_coalesceexpr(list, tupdesc, strbuf);
	} else if (strcmp(type, BOOLEXPR_TYPE) == 0) {
		traverse_boolexpr(list, tupdesc, strbuf);
	} else if (strcmp(type, CASEEXPR_TYPE) == 0) {
		traverse_caseexpr(list, tupdesc, strbuf);
	} else if (strcmp(type, SCALARARRAYOPEXPR_TYPE) == 0) {
		traverse_scalararrayopexpr(list, tupdesc, strbuf);
	} else if (strcmp(type, CASEWHEN_TYPE) == 0) {
		traverse_casewhen(list, tupdesc, strbuf);
	} else if (strcmp(type, CASETESTEXPR_TYPE) == 0) {
		traverse_casetestexpr(list, tupdesc, strbuf);
	}
}

static void var_to_target(kite_target_t *target, xex_list_t *list, TupleDesc tupdesc, int *start_idx) {
	const char *type;
	Oid varno, vartype, varlevelsup, varnoold;
	int32_t varattno, vartypmod, varoattno;

	target->decode = decode_var;
	target->attrs = lappend_int(target->attrs, *start_idx);
	(*start_idx)++;

	int n = xex_list_length(list);
	Insist(n == 8);

	type = xex_list_get_string(list, 0); // type
	Insist(type && strcmp(type, VAR_TYPE) == 0);

	Insist(xex_list_get_uint32(list, 1, &varno) == 0); // varno

	Insist(xex_list_get_int32(list, 2, &varattno) == 0); // varattno

	Insist(xex_list_get_uint32(list, 3, &vartype) == 0); // vartype

	Insist(xex_list_get_int32(list, 4, &vartypmod) == 0); // vartypmod

	Insist(xex_list_get_uint32(list, 5, &varlevelsup) == 0); // varlevelsup

	Insist(xex_list_get_uint32(list, 6, &varnoold) == 0); // varnoold

	Insist(xex_list_get_int32(list, 7, &varoattno) == 0); // varoattno

	char *cn = NameStr(tupdesc->attrs[varoattno - 1]->attname);
	target->tuplist = lappend(target->tuplist, pstrdup(cn));
}

static void const_to_target(kite_target_t *target, xex_list_t *list, TupleDesc tupdesc, int *start_idx) {
	const char *type, *constvalue;
	Oid consttype, constcollid;
	int32_t consttypmod, constlen;
	int8_t constbyval, constisnull;

	target->decode = decode_var;
	target->attrs = lappend_int(target->attrs, *start_idx);
	(*start_idx)++;

	int n = xex_list_length(list);
	Insist(n == 8);

	type = xex_list_get_string(list, 0); // type
	Insist(type && strcmp(type, CONST_TYPE) == 0);

	Insist(xex_list_get_uint32(list, 1, &consttype) == 0); // consttype

	Insist(xex_list_get_int32(list, 2, &consttypmod) == 0); // consttypmod

	Insist(xex_list_get_uint32(list, 3, &constcollid) == 0); // constcollid

	Insist(xex_list_get_int32(list, 4, &constlen) == 0); // constlen

	Insist(xex_list_get_int8(list, 5, &constbyval) == 0); // constbyval

	Insist(xex_list_get_int8(list, 6, &constisnull) == 0); // constisnull

	constvalue = xex_list_get_string(list, 7); // constvalue
	Insist(constvalue);

	target->tuplist = lappend(target->tuplist, pstrdup(constvalue));
}

static void opexpr_to_target(kite_target_t *target, xex_list_t *list, TupleDesc tupdesc, int *start_idx) {
	target->decode = decode_var;
	target->attrs = lappend_int(target->attrs, *start_idx);
	(*start_idx)++;

	stringbuffer_t *strbuf = stringbuffer_new();
	Insist(strbuf);

	traverse(list, tupdesc, strbuf);

	target->tuplist = lappend(target->tuplist, stringbuffer_to_string(strbuf));

	stringbuffer_release(strbuf);
}

static void funcexpr_to_target(kite_target_t *target, xex_list_t *list, TupleDesc tupdesc, int *start_idx) {
	target->decode = decode_var;
	target->attrs = lappend_int(target->attrs, *start_idx);
	(*start_idx)++;

	stringbuffer_t *strbuf = stringbuffer_new();
	Insist(strbuf);

	traverse(list, tupdesc, strbuf);

	target->tuplist = lappend(target->tuplist, stringbuffer_to_string(strbuf));

	stringbuffer_release(strbuf);
}

static void aggref_to_target(kite_target_t *target, xex_list_t *list, TupleDesc tupdesc, int *start_idx) {

	const char *type;
	Oid aggfnoid, aggtype, aggcollid;
	int8_t aggstar, aggvaradic;
	xex_object_t *obj;
	xex_list_t *args;

	int n = xex_list_length(list);
	Insist(n == 7);
	type = xex_list_get_string(list, 0); // type
	Insist(type);

	Insist(xex_list_get_uint32(list, 1, &aggfnoid) == 0); // aggfnoid

	Insist(xex_list_get_uint32(list, 2, &aggtype) == 0); // aggtype

	Insist(xex_list_get_uint32(list, 3, &aggcollid) == 0); // aggcollid

	Insist(xex_list_get_int8(list, 4, &aggstar) == 0); // aggstar

	Insist(xex_list_get_int8(list, 5, &aggvaradic) == 0); // aggvariadic

	obj = xex_list_get(list, 6); // args
	Insist(obj);
	args = xex_to_list(obj);
	Insist(args);

	int32_t opcode = pg_agg_to_op(aggfnoid);
	Insist(opcode != -1);
	const char *opstr = aggfnoid_to_opstr(aggfnoid);

	// decode function pointer
	switch (opcode) {
	case XRG_OP_COUNT:
	case XRG_OP_COUNT_STAR:
	case XRG_OP_SUM_INT64:
	case XRG_OP_SUM_INT128:
	case XRG_OP_SUM_DOUBLE:
	case XRG_OP_SUM_NUMERIC:
	case XRG_OP_MIN:
	case XRG_OP_MAX:
		target->decode = decode_var;
		break;
	case XRG_OP_AVG_INT64:
		target->decode = decode_avg_int64;
		break;
	case XRG_OP_AVG_INT128:
		target->decode = decode_avg_int128;
		break;
	case XRG_OP_AVG_DOUBLE:
		target->decode = decode_avg_double;
		break;
	case XRG_OP_AVG_NUMERIC:
		target->decode = decode_avg_numeric;
		break;
	default:
		elog(ERROR, "AGG operation not supported. op = %d, aggfnoid = %d", opcode, aggfnoid);
	}

	stringbuffer_t *strbuf = stringbuffer_new();
	Insist(strbuf);

	// setup tuple
	bool is_avg = false;
	if (strcmp(opstr, XRG_OP_AVG_STR) == 0) {
		is_avg = true;
		opstr = XRG_OP_SUM_STR;
	}

	target->attrs = lappend_int(target->attrs, *start_idx);
	(*start_idx)++;

	stringbuffer_append_string(strbuf, (char *)opstr);

	int nargs = xex_list_length(args);
	if (nargs > 0) {
		stringbuffer_append(strbuf, '(');

		for (int i = 0; i < xex_list_length(args); i++) {
			obj = xex_list_get(args, i);
			Insist(obj);
			xex_list_t *arg = xex_to_list(obj);
			Insist(arg);
			traverse(arg, tupdesc, strbuf);
		}
		stringbuffer_append(strbuf, ')');
	}

	target->tuplist = lappend(target->tuplist, stringbuffer_to_string(strbuf));

	if (is_avg) {
		target->attrs = lappend_int(target->attrs, *start_idx);
		(*start_idx)++;
		stringbuffer_clear(strbuf);
		stringbuffer_append_string(strbuf, XRG_OP_COUNT_STR);
		stringbuffer_append(strbuf, '(');
		for (int i = 0; i < xex_list_length(args); i++) {
			obj = xex_list_get(args, i);
			Insist(obj);
			xex_list_t *arg = xex_to_list(obj);
			Insist(arg);
			traverse(arg, tupdesc, strbuf);
		}
		stringbuffer_append(strbuf, ')');
		target->tuplist = lappend(target->tuplist, stringbuffer_to_string(strbuf));
	}

	stringbuffer_release(strbuf);
}

kite_target_t *kite_target_create() {
	kite_target_t *target = palloc(sizeof(kite_target_t));
	Insist(target);
	memset(target, 0, sizeof(kite_target_t));
	return target;
}

kite_target_t *kite_target_from_xexpr(xex_list_t *list, const Form_pg_attribute attr, TupleDesc tupdesc, int *start_idx) {

	kite_target_t *target = kite_target_create();

	target->pg_attr = attr;
	const char *type = xex_list_get_string(list, 0);
	Insist(type);

	if (strcmp(type, INVALID_TYPE) == 0) {
		target->decode = 0;
		target->attrs = 0;
		target->tuplist = 0;
	} else if (strcmp(type, AGGREF_TYPE) == 0) {
		aggref_to_target(target, list, tupdesc, start_idx);
	} else if (strcmp(type, VAR_TYPE) == 0) {
		var_to_target(target, list, tupdesc, start_idx);
	} else if (strcmp(type, CONST_TYPE) == 0) {
		const_to_target(target, list, tupdesc, start_idx);
	} else if (strcmp(type, FUNCEXPR_TYPE) == 0) {
		funcexpr_to_target(target, list, tupdesc, start_idx);
	} else if (strcmp(type, OPEXPR_TYPE) == 0) {
		opexpr_to_target(target, list, tupdesc, start_idx);
	} else {
		elog(ERROR, "xexpr_to_target: type %s not supported", type);
	}
	return target;
}

void kite_target_destroy(kite_target_t *tgt) {
	if (tgt) {
		if (tgt->attrs) {
			list_free(tgt->attrs);
		}

		if (tgt->tuplist) {
			list_free_deep(tgt->tuplist);
		}

		if (tgt->data) {
			pfree(tgt->data);
		}
		pfree(tgt);
	}
}

kite_target_t *kite_target_from_tupdesc(const Form_pg_attribute attr, TupleDesc tupdesc, int idx, int *start_idx) {
	kite_target_t *target = kite_target_create();
	target->attrs = lappend_int(target->attrs, *start_idx);
	target->pg_attr = attr;
	(*start_idx)++;

	char *cn = NameStr(tupdesc->attrs[idx]->attname);
	target->tuplist = lappend(target->tuplist, pstrdup(cn));
	target->decode = decode_var;
	//elog(LOG, "col = %s", cn);
	return target;
}

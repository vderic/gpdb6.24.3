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
#include "parser/parsetree.h"
#include "optimizer/var.h"
#include "optimizer/clauses.h"

#include "kite_extscan.h"
#include "../util/xexpr_types.h"
#include "../util/exx_sarg.h"
#include "aggop.h"
#include "../util/stringbuffer.h"

/**
 * Generate Kite specified JSON format
 */

static char *setup_qual(kite_extscan_t *ex);

static void traverse_qual(kite_extscan_t *ex, ExprState *exprstate, stringbuffer_t *strbuf);

/**
 * Generate schema JSON
 */
static void setup_schema(kite_extscan_t *ex, stringbuffer_t *strbuf) {
	TupleDesc scan_tdesc = scan_tupdesc(ex->m_node);
	int ncol = scan_tdesc->natts;
	Form_pg_attribute *attr = scan_tdesc->attrs;

	for (int i = 0; i < ncol; i++) {
		int16_t ptyp, ltyp, precision, scale;
		bool is_array = false;
		ptyp = ltyp = precision = scale = 0;

		pg_typ_to_xrg_typ(attr[i]->atttypid, attr[i]->atttypmod, &ptyp, &ltyp, &precision, &scale, &is_array);

		/*
		elog(LOG, "schema: typ=%d, typmod=%d, ptyp =%d, ltyp=%d, precision =%d, scale=%d, is_array = %d",
				attr[i]->atttypid, attr[i]->atttypmod, ptyp, ltyp, precision, scale, is_array);
		*/

		char *name = colname(ex->m_node, i + 1);
		const char *t = xrg_typ_str(ptyp, ltyp);

		stringbuffer_append_string(strbuf, name);
		stringbuffer_append(strbuf, ':');
		stringbuffer_append_string(strbuf, t);
		if (strcmp(t, "decimal") == 0) {
			stringbuffer_append(strbuf, ':');
			stringbuffer_append_int(strbuf, precision);
			stringbuffer_append(strbuf, ':');
			stringbuffer_append_int(strbuf, scale);
		}

		stringbuffer_append(strbuf, '\n');
	}

}

static char *get_uri(kite_extscan_t *ex) {
	FileScanDesc scandesc = ex->m_node->ess_ScanDesc;
	URL_KITE_FILE *urlf = (URL_KITE_FILE *)scandesc->fs_file;

	char *path = urlf->common.url + strlen(PROTOCOL_KITE);
	path = strchr(path, '/') + 1;

	return pstrdup(path);
}

/**
 * Generate SQL
 */
static char *generate_sql(kite_extscan_t *ex, xex_list_t *xexpr, List *targetlist) {
	TupleDesc scan_tdesc = scan_tupdesc(ex->m_node);
	ListCell *l;
	stringbuffer_t *sbuf = stringbuffer_new();
	if (!sbuf) {
		elog(ERROR, "stringbuffer_new: out of memory error");
		return 0;
	}

	stringbuffer_append_string(sbuf, "SELECT ");
	bool firstcol = true;
	foreach (l, targetlist) {
		kite_target_t *tgt = (kite_target_t *)lfirst(l);
		if (!tgt->tuplist) {
			continue;
		}

		ListCell *ll;
		foreach (ll, tgt->tuplist) {
			const char *tup = (const char *)lfirst(ll);
			if (firstcol) {
				firstcol = false;
			} else {
				stringbuffer_append(sbuf, ',');
			}
			stringbuffer_append_string(sbuf, tup);
		}
	}

	char *uri = get_uri(ex);
	stringbuffer_append_string(sbuf, " FROM ");
	stringbuffer_append(sbuf, '"');
	stringbuffer_append_string(sbuf, uri);
	stringbuffer_append(sbuf, '"');

	if (uri) {
		pfree(uri);
		uri = 0;
	}

	char *qual = setup_qual(ex);
	if (qual && *qual) {
		stringbuffer_append_string(sbuf, " WHERE ");
		stringbuffer_append_string(sbuf, qual);
	}

	if (qual) {
		pfree(qual);
		qual = 0;
	}

	// group by
	if (xexpr) {
		xex_object_t *obj = xex_list_get(xexpr, 0);
		Insist(obj);
		xex_list_t *gby = xex_to_list(obj);
		Insist(gby);

		if (xex_list_length(gby) > 0) {
			stringbuffer_append_string(sbuf, " GROUP BY ");

			for (int i = 0; i < xex_list_length(gby); i++) {
				if (i > 0) {
					stringbuffer_append(sbuf, ',');
				}

				obj = xex_list_get(gby, i);
				Insist(obj);
				xex_list_t *gbyitem = xex_to_list(obj);
				Insist(gbyitem);
				traverse(gbyitem, scan_tdesc, sbuf);
			}
		}
	}

	char *ret = stringbuffer_to_string(sbuf);
	stringbuffer_release(sbuf);
	return ret;
}

/**
 * Generate Qual
 */
static char *setup_qual(kite_extscan_t *ex) {
	List *qlist = 0;
	stringbuffer_t *strbuf = stringbuffer_new();
	if (!strbuf) {
		elog(ERROR, "stringbuffer_new failed: Out of memory");
		return 0;
	}

	List *qual = ex->m_node->ss.ps.qual;
	if (qual) {
		ListCell *l;
		foreach (l, qual) {
			stringbuffer_clear(strbuf);
			ExprState *clause = (ExprState *)lfirst(l);
			//elog_node_display(LOG, "QUAL EXPR", clause->expr, true);
			traverse_qual(ex, clause, strbuf);
			qlist = lappend(qlist, stringbuffer_to_string(strbuf));
		}
	}

	FileScanDesc scandesc = ex->m_node->ess_ScanDesc;

	if (scandesc->fs_constraintExprs) {
		Relation rel = scandesc->fs_rd;
		TupleConstr *constr = rel->rd_att->constr;
		uint16 ncheck = constr->num_check;

		for (uint16 i = 0; i < ncheck; i++) {
			List *qual = (List *)scandesc->fs_constraintExprs[i];
			ListCell *l;
			foreach (l, qual) {
				stringbuffer_clear(strbuf);
				ExprState *clause = (ExprState *)lfirst(l);
				//elog_node_display(LOG, "CONSTR", clause->expr, true);
				traverse_qual(ex, clause, strbuf);
				qlist = lappend(qlist, stringbuffer_to_string(strbuf));
			}
		}
	}

	stringbuffer_clear(strbuf);
	ListCell *l;
	int i = 0;
	foreach (l, qlist) {
		if (i > 0) {
			stringbuffer_append_string(strbuf, " AND ");
		}

		const char *s = (const char *)lfirst(l);

		stringbuffer_append_string(strbuf, s);
		i++;
	}

	char *ret = stringbuffer_to_string(strbuf);
	stringbuffer_release(strbuf);

	if (qlist) {
		list_free_deep(qlist);
	}
	return ret;
}

/* traverse the Qual Expression */
static void traverse_qual_expr(kite_extscan_t *ex, Expr *expr, stringbuffer_t *strbuf) {

	if (IsA(expr, Const)) {
		Const *c = (Const *)expr;
		int16_t ptyp, ltyp, precision, scale;
		bool is_array = false;
		ptyp = ltyp = precision = scale = 0;
		pg_typ_to_xrg_typ(c->consttype, c->consttypmod, &ptyp, &ltyp, &precision, &scale, &is_array);

		const char *ts = xrg_typ_str(ptyp, ltyp);
		Insist(ts && *ts != 0);
		if (strcmp(ts, "string") == 0) {
			if (c->constlen == -1) {
				ts = "text";
			}
		}

		const char *constvalue = 0;
		switch (c->consttype) {
		case INTERVALOID: // interval
		{
			constvalue = op_sarg_const_str(ts, c->constvalue, 0);
			stringbuffer_append_string(strbuf, "INTERVAL '");
			stringbuffer_append_string(strbuf, constvalue);
			stringbuffer_append(strbuf, '\'');
		} break;
		case DATEOID: {
			constvalue = op_sarg_const_str(ts, c->constvalue, 0);
			stringbuffer_append_string(strbuf, "DATE '");
			stringbuffer_append_string(strbuf, constvalue);
			stringbuffer_append(strbuf, '\'');
		} break;
		case TIMEOID: {
			constvalue = op_sarg_const_str(ts, c->constvalue, 0);
			stringbuffer_append_string(strbuf, "TIME '");
			stringbuffer_append_string(strbuf, constvalue);
			stringbuffer_append(strbuf, '\'');
		} break;
		case TIMESTAMPOID:
		case TIMESTAMPTZOID: {
			constvalue = op_sarg_const_str(ts, c->constvalue, 0);
			stringbuffer_append_string(strbuf, "TIMESTAMP '");
			stringbuffer_append_string(strbuf, constvalue);
			stringbuffer_append(strbuf, '\'');
		} break;
		case BPCHAROID:
		case TEXTOID:	// text
		case VARCHAROID: // cstring
		{
			constvalue = op_sarg_const_str(ts, c->constvalue, 0);
			stringbuffer_append(strbuf, '\'');
			// escape single quote inside op_sarg_const_str
			stringbuffer_append_string(strbuf, constvalue);
			stringbuffer_append(strbuf, '\'');
		} break;
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
		{
			constvalue = op_arraytype_to_string(c);
			stringbuffer_append_string(strbuf, constvalue);
		} break;
		default: {
			constvalue = op_sarg_const_str(ts, c->constvalue, 0);
			stringbuffer_append_string(strbuf, constvalue);
		} break;
		}
		//elog(LOG, "col[%d] = %s", i, xrg_typ_str(ptyp, ltyp));
		if (constvalue) { pfree((void *)constvalue); }
		return;
	} else if (IsA(expr, Var)) {
		Var *var = (Var *)expr;
		stringbuffer_append_string(strbuf, colname(ex->m_node, var->varattno));
		return;
	} else if (IsA(expr, RelabelType)) {
		RelabelType *r = (RelabelType *)expr;
		traverse_qual_expr(ex, r->arg, strbuf);
		return;
	} else if (IsA(expr, FuncExpr)) {
		FuncExpr *fp = (FuncExpr *)expr;
		int opcode = pg_func_to_op(fp->funcid);
		Insist(opcode != -1);
		if (opcode == XRG_OP_CAST) {
			ListCell *l;
			List *oplist = fp->args;
			foreach (l, oplist) {
				traverse_qual_expr(ex, (Expr *)lfirst(l), strbuf);
			}
		} else {
			const char *opstr = xrg_opexpr_str(opcode);
			stringbuffer_append_string(strbuf, opstr);
			stringbuffer_append(strbuf, '(');

			ListCell *l;
			List *oplist = fp->args;
			int i = 0;
			foreach (l, oplist) {
				if (i > 0) {
					stringbuffer_append(strbuf, ',');
				}
				traverse_qual_expr(ex, (Expr *)lfirst(l), strbuf);
				i++;
			}
			stringbuffer_append(strbuf, ')');
		}
		return;
	} else if (IsA(expr, OpExpr)) {
		OpExpr *op = (OpExpr *)expr;

		/* check sargable? */
		//op_is_sargable(op->opno, true);
		//std::string opstr = op_sarg_str(op->opno, false);
		int opcode = pg_proc_to_op(op->opfuncid);
		Insist(opcode != -1);
		const char *opstr = xrg_opexpr_str(opcode);

		Expr *left = (Expr *)linitial(op->args);
		Expr *right = (Expr *)lsecond(op->args);

		if (IsA(left, CaseTestExpr)) {
			traverse_qual_expr(ex, right, strbuf);
		} else {

			traverse_qual_expr(ex, left, strbuf);
			stringbuffer_append(strbuf, ' ');
			stringbuffer_append_string(strbuf, opstr);
			stringbuffer_append(strbuf, ' ');
			traverse_qual_expr(ex, right, strbuf);
		}

		return;
	} else if (IsA(expr, BoolExpr)) {
		BoolExpr *bp = (BoolExpr *)expr;
		const char *opstr = 0;
		switch (bp->boolop) {
		case AND_EXPR:
			opstr = "AND";
			break;
		case OR_EXPR:
			opstr = "OR";
			break;
		case NOT_EXPR:
			opstr = "NOT";
			break;
		default: {
			elog(ERROR, "kite_json: BoolExpr operation not supported");
			return;
		}
		}

		ListCell *l;
		List *oplist = ((BoolExpr *)expr)->args;

		// NOT or ! operation only require ONE args
		if (list_length(oplist) == 1) {
			Insist(strcmp(opstr, "NOT") == 0);
			stringbuffer_append_string(strbuf, opstr);
			stringbuffer_append(strbuf, ' ');
			traverse_qual_expr(ex, (Expr *)linitial(oplist), strbuf);
		} else {

			stringbuffer_append_string(strbuf, " (");
			int i = 0;
			foreach (l, oplist) {
				if (i > 0) {
					stringbuffer_append(strbuf, ' ');
					stringbuffer_append_string(strbuf, opstr);
					stringbuffer_append(strbuf, ' ');
				}
				traverse_qual_expr(ex, (Expr *)lfirst(l), strbuf);
				i++;
			}
			stringbuffer_append_string(strbuf, ") ");
		}
		return;
	} else if (IsA(expr, ScalarArrayOpExpr)) {
		// setup qual for ScalarArray IN (...). can also be field NOT IN (1, 2)
		// sa->opfuncid 468, opno = 411 => (NOT IN),  opno=401, opfuncid=467 => (IN)
		ScalarArrayOpExpr *sp = (ScalarArrayOpExpr *)expr;
		Expr *left = (Expr *)linitial(sp->args);
		Expr *right = (Expr *)lsecond(sp->args);

		traverse_qual_expr(ex, left, strbuf);

		int32_t op = pg_proc_to_op(sp->opfuncid);
		switch (op) {
		case XRG_OP_EQ:
			stringbuffer_append_string(strbuf, " IN ");
			break;
		case XRG_OP_NE:
			stringbuffer_append_string(strbuf, " NOT IN ");
			break;
		default:
			elog(ERROR, "ScalarArrayOpExpr: Invalid operation. (op = %d, funcid = %d)", sp->opno, sp->opfuncid);
			break;
		}

		traverse_qual_expr(ex, right, strbuf);
		return;
	} else if (IsA(expr, NullTest)) {
		NullTest *ntest = (NullTest *)expr;
		traverse_qual_expr(ex, ntest->arg, strbuf);
		if (ntest->nulltesttype == IS_NULL) {
			stringbuffer_append_string(strbuf, " IS NULL ");
		} else {
			stringbuffer_append_string(strbuf, " IS NOT NULL ");
		}
		return;
	} else if (IsA(expr, CaseExpr)) {
		CaseExpr *caseexpr = (CaseExpr *)expr;
		stringbuffer_append_string(strbuf, "CASE ");
		if (caseexpr->arg) {
			traverse_qual_expr(ex, caseexpr->arg, strbuf);
			stringbuffer_append(strbuf, ' ');
		}
		// list of WHEN clauses
		ListCell *l;
		foreach (l, caseexpr->args) {
			traverse_qual_expr(ex, (Expr *)lfirst(l), strbuf);
			stringbuffer_append(strbuf, ' ');
		}
		stringbuffer_append_string(strbuf, "ELSE ");
		traverse_qual_expr(ex, caseexpr->defresult, strbuf);
		stringbuffer_append_string(strbuf, " END");

		return;
	} else if (IsA(expr, CaseWhen)) {
		CaseWhen *cw = (CaseWhen *)expr;
		stringbuffer_append_string(strbuf, "WHEN ");
		traverse_qual_expr(ex, cw->expr, strbuf);
		stringbuffer_append_string(strbuf, " THEN ");
		traverse_qual_expr(ex, cw->result, strbuf);

		return;
	}
}

/* Entry - traverse Qual Expression State */
static void traverse_qual(kite_extscan_t *ex, ExprState *exprstate, stringbuffer_t *strbuf) {

	if (IsA(exprstate->expr, OpExpr)) {
		traverse_qual_expr(ex, exprstate->expr, strbuf);
	} else if (IsA(exprstate->expr, BoolExpr)) {
		traverse_qual_expr(ex, exprstate->expr, strbuf);
	} else if (IsA(exprstate->expr, ScalarArrayOpExpr)) {
		traverse_qual_expr(ex, exprstate->expr, strbuf);
	} else if (IsA(exprstate->expr, NullTest)) {
		traverse_qual_expr(ex, exprstate->expr, strbuf);
	} else if (IsA(exprstate->expr, CaseExpr)) {
		traverse_qual_expr(ex, exprstate->expr, strbuf);
	}
}

/**
 * Generate the Kite request in JSON format
 */
int setup_query(kite_extscan_t *ex, char **addr, char **schema, char **sql, int *fragid, int *fragcnt, kite_filespec_t *fs) {

	FileScanDesc scandesc = ex->m_node->ess_ScanDesc;
	// list of addresses
	{
        	URL_KITE_FILE *urlf = (URL_KITE_FILE *)scandesc->fs_file;
		char *p;
		char *uri = urlf->common.url;
		char *host = uri + strlen(PROTOCOL_KITE);
		char *path = strchr(host, '/');
		if (! path) {
			elog(ERROR, "setup_query: invalid uri %s", uri);
		}

		int len = path - host;
		char *ret = (char *) palloc(len + 1);

		if (!ret) {
			elog(ERROR, "setup_query: palloc out of memory");
			return 1;
		}
		strncpy(ret, host, len);
		ret[len] = 0;
		while ((p = strchr(ret, ','))) {
			*p = '\n';
		}
		*addr = ret;
	}


	// SQL
	{
		xex_list_t *xexpr = ex->m_xexpr;
       		List *targetlist = ex->m_targetlist;
        	*sql = generate_sql(ex, xexpr, targetlist);
		//elog(LOG, "sql = %s", *sql);
	}

	// schema
	{
		stringbuffer_t *sbuf = stringbuffer_new();
		setup_schema(ex, sbuf);
		*schema = stringbuffer_to_string(sbuf);
		stringbuffer_release(sbuf);
		//elog(LOG, "scahem  = %s", *schema);
	}

	// fragment
        *fragid = GpIdentity.segindex;
        *fragcnt = getgpsegmentCount();
	//elog(LOG, "fragid = %d, fragcnt = %d", *fragid, *fragcnt);

	// format
	{
		CopyStateData *pstate = scandesc->fs_pstate;

		if (fmttype_is_parquet(pstate->exx_fmtcode)) {
			strcpy(fs->fmt, "parquet");

		} else if (fmttype_is_csv(pstate->exx_fmtcode)) {
			strcpy(fs->fmt, "csv");

			fs->u.csv.delim = ',';
			fs->u.csv.quote = '"';
			fs->u.csv.escape = '"';
			*fs->u.csv.nullstr = 0;
			fs->u.csv.header_line = false;

			if (pstate->delim) fs->u.csv.delim = *pstate->delim;
			if (pstate->quote) fs->u.csv.quote = *pstate->quote;
			if (pstate->escape) fs->u.csv.escape = *pstate->escape;
			if (pstate->null_print) strcpy(fs->u.csv.nullstr, pstate->null_print);

			/*
			elog(LOG, "TYPE %s, delin = %c, quote %c, escape %c",
					fs->fmt, fs->u.csv.delim,
					fs->u.csv.quote,
					fs->u.csv.escape);
					*/
		} else {
			elog(ERROR, "invalid file format %d", pstate->exx_fmtcode);
		}
	}
	return 0;
}



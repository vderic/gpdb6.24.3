#ifndef _AGGOP_H_
#define _AGGOP_H_

/**
 * PG_OPER code to XRG_OPER conversion
 */

#define AGGREF_TYPE "AGGREF"
#define OPEXPR_TYPE "OPEXPR"
#define BOOLEXPR_TYPE "BOOLEXPR"
#define FUNCEXPR_TYPE "FUNCEXPR"
#define NULLIFEXPR_TYPE "NULLIFEXPR"
#define COALESCEEXPR_TYPE "COALESCE"
#define VAR_TYPE "VAR"
#define CONST_TYPE "CONST"
#define INVALID_TYPE "INVALID"
#define CASEEXPR_TYPE "CASE"
#define CASEWHEN_TYPE "WHEN"
#define CASETESTEXPR_TYPE "CASETESTEXPR"
#define SCALARARRAYOPEXPR_TYPE "SCALARARRAYOPEXPR"

#define XRG_OP_COUNT_STR "COUNT"
#define XRG_OP_COUNT_STAR_STR "COUNT(*)"
#define XRG_OP_AVG_STR "AVG"
#define XRG_OP_SUM_STR "SUM"
#define XRG_OP_MIN_STR "MIN"
#define XRG_OP_MAX_STR "MAX"
#define XRG_OP_AND_STR "AND"
#define XRG_OP_OR_STR "OR"
#define XRG_OP_LIKE_STR "LIKE"
#define XRG_OP_NOT_LIKE_STR "NOT LIKE"
#define XRG_OP_CAST_STR "CAST"
#define XRG_OP_SUBSTR_STR "SUBSTR"
#define XRG_OP_ABS_STR "ABS"
#define XRG_OP_ROUND_STR "ROUND"
#define XRG_OP_FLOOR_STR "FLOOR"
#define XRG_OP_CEIL_STR "CEIL"
#define XRG_OP_MOD_STR "MOD"
#define XRG_OP_RANDOM_STR "RANDOM"

enum xrg_opexpr_t {
	XRG_OP_UNKNOWN = 0,
	XRG_OP_COUNT,
	XRG_OP_COUNT_STAR,
	XRG_OP_SUM_INT64,
	XRG_OP_SUM_INT128,
	XRG_OP_SUM_DOUBLE,
	XRG_OP_SUM_NUMERIC,
	XRG_OP_AVG_INT64,
	XRG_OP_AVG_INT128,
	XRG_OP_AVG_DOUBLE,
	XRG_OP_AVG_NUMERIC,
	XRG_OP_MIN,
	XRG_OP_MAX,
	XRG_OP_AND,
	XRG_OP_OR,
	XRG_OP_NOT,
	XRG_OP_EQ,
	XRG_OP_NE,
	XRG_OP_LT,
	XRG_OP_LE,
	XRG_OP_GT,
	XRG_OP_GE,
	XRG_OP_ADD,
	XRG_OP_SUB,
	XRG_OP_MUL,
	XRG_OP_DIV,
	XRG_OP_REM,
	XRG_OP_TEXTLIKE,
	XRG_OP_TEXTNLIKE,
	XRG_OP_CAST,
	XRG_OP_SUBSTR,
	XRG_OP_ABS,
	XRG_OP_ROUND,
	XRG_OP_MOD,
	XRG_OP_FLOOR,
	XRG_OP_CEIL,
	XRG_OP_RANDOM,
};

static inline const char *xrg_opexpr_str(int32_t op) {
	switch (op) {
	case XRG_OP_COUNT:
		return XRG_OP_COUNT_STR;
	case XRG_OP_COUNT_STAR:
		return XRG_OP_COUNT_STAR_STR;
	case XRG_OP_SUM_INT64:
	case XRG_OP_SUM_INT128:
	case XRG_OP_SUM_DOUBLE:
	case XRG_OP_SUM_NUMERIC:
		return XRG_OP_SUM_STR;
	case XRG_OP_AVG_INT64:
	case XRG_OP_AVG_INT128:
	case XRG_OP_AVG_DOUBLE:
	case XRG_OP_AVG_NUMERIC:
		return XRG_OP_AVG_STR;
	case XRG_OP_MIN:
		return XRG_OP_MIN_STR;
	case XRG_OP_MAX:
		return XRG_OP_MAX_STR;
	case XRG_OP_AND:
		return XRG_OP_AND_STR;
	case XRG_OP_OR:
		return XRG_OP_OR_STR;
	case XRG_OP_NOT:
		return "!";
	case XRG_OP_EQ:
		return "=";
	case XRG_OP_NE:
		return "!=";
	case XRG_OP_LT:
		return "<";
	case XRG_OP_LE:
		return "<=";
	case XRG_OP_GT:
		return ">";
	case XRG_OP_GE:
		return ">=";
	case XRG_OP_ADD:
		return "+";
	case XRG_OP_SUB:
		return "-";
	case XRG_OP_MUL:
		return "*";
	case XRG_OP_DIV:
		return "/";
	case XRG_OP_REM:
		return "%";
	case XRG_OP_TEXTLIKE:
		return XRG_OP_LIKE_STR;
	case XRG_OP_TEXTNLIKE:
		return XRG_OP_NOT_LIKE_STR;
	case XRG_OP_CAST:
		return XRG_OP_CAST_STR;
	case XRG_OP_SUBSTR:
		return XRG_OP_SUBSTR_STR;
	case XRG_OP_ABS:
		return XRG_OP_ABS_STR;
	case XRG_OP_ROUND:
		return XRG_OP_ROUND_STR;
	case XRG_OP_MOD:
		return XRG_OP_MOD_STR;
	case XRG_OP_FLOOR:
		return XRG_OP_FLOOR_STR;
	case XRG_OP_CEIL:
		return XRG_OP_CEIL_STR;
	case XRG_OP_RANDOM:
		return XRG_OP_RANDOM_STR;
	default:
		return NULL;
	}
	return NULL;
};

static inline int32_t pg_proc_to_op(int32_t funcid) {
	switch (funcid) {
	case 60:   // PG_PROC_booleq:
	case 61:   // PG_PROC_chareq: // This is int8_t eq
	case 63:   // PG_PROC_int2eq:
	case 158:  // PG_PROC_int24eq:
	case 1850: // PG_PROC_int28eq:
	case 65:   // PG_PROC_int4eq:
	case 852:  // PG_PROC_int48eq:
	case 467:  // PG_PROC_int8eq:
	case 1856: // PG_PROC_int82eq:
	case 474:  // PG_PROC_int84eq:
	case 287:  // PG_PROC_float4eq:
	case 299:  // PG_PROC_float48eq:
	case 293:  // PG_PROC_float8eq:
	case 305:  // PG_PROC_float84eq:
	case 67:   // PG_PROC_texteq:
	case 1048: // PG_PROC_bpchareq: // char(N) eq, space fill -- do we care?
	case 1086: // PG_PROC_date_eq:
	case 1145: // PG_PROC_time_eq:
	case 2052: // PG_PROC_timestamp_eq:
	case 1152: // PG_PROC_timestamptz_eq_1152:
	case 2340: // PG_PROC_date_eq_timestamp:
	case 2353: // PG_PROC_date_eq_timestamptz:
	case 1718: // PG_PROC_numeric_eq:
		return XRG_OP_EQ;

	case 84:   // PG_PROC_boolne:
	case 70:   // PG_PROC_charne: // This is int8_t eq
	case 145:  // PG_PROC_int2ne:
	case 164:  // PG_PROC_int24ne:
	case 1851: // PG_PROC_int28ne:
	case 144:  // PG_PROC_int4ne:
	case 853:  // PG_PROC_int48ne:
	case 468:  // PG_PROC_int8ne:
	case 1857: // PG_PROC_int82ne:
	case 475:  // PG_PROC_int84ne:
	case 288:  // PG_PROC_float4ne:
	case 300:  // PG_PROC_float48ne:
	case 294:  // PG_PROC_float8ne:
	case 306:  // PG_PROC_float84ne:
	case 157:  // PG_PROC_textne:
	case 1053: // PG_PROC_bpcharne: // char(N) eq, space fill -- do we care?
	case 1091: // PG_PROC_date_ne:
	case 1106: // PG_PROC_time_ne:
	case 2053: // PG_PROC_timestamp_ne:
	case 1153: // PG_PROC_timestamptz_ne_1153:
	case 2343: // PG_PROC_date_ne_timestamp:
	case 2356: // PG_PROC_date_ne_timestamptz:
	case 1719: // PG_PROC_numeric_ne:
		return XRG_OP_NE;

	case 1246: // PG_PROC_charlt:
	case 64:   // PG_PROC_int2lt:
	case 160:  // PG_PROC_int24lt:
	case 1852: // PG_PROC_int28lt:
	case 66:   // PG_PROC_int4lt:
	case 854:  // PG_PROC_int48lt:
	case 469:  // PG_PROC_int8lt:
	case 1858: // PG_PROC_int82lt:
	case 476:  // PG_PROC_int84lt:
	case 289:  // PG_PROC_float4lt:
	case 301:  // PG_PROC_float48lt:
	case 295:  // PG_PROC_float8lt:
	case 307:  // PG_PROC_float84lt:
	case 740:  // PG_PROC_text_lt:
	case 1049: // PG_PROC_bpcharlt:
	case 1087: // PG_PROC_date_lt:
	case 1102: // PG_PROC_time_lt:
	case 2054: // PG_PROC_timestamp_lt:
	case 1154: // PG_PROC_timestamptz_lt_1154:
	case 2338: // PG_PROC_date_lt_timestamp:
	case 2351: // PG_PROC_date_lt_timestamptz:
	case 1722: // PG_PROC_numeric_lt:
		return XRG_OP_LT;

	case 72:   // PG_PROC_charle:
	case 148:  // PG_PROC_int2le:
	case 166:  // PG_PROC_int24le:
	case 1854: // PG_PROC_int28le:
	case 149:  // PG_PROC_int4le:
	case 856:  // PG_PROC_int48le:
	case 471:  // PG_PROC_int8le:
	case 1860: // PG_PROC_int82le:
	case 478:  // PG_PROC_int84le:
	case 290:  // PG_PROC_float4le:
	case 302:  // PG_PROC_float48le:
	case 296:  // PG_PROC_float8le:
	case 308:  // PG_PROC_float84le:
	case 741:  // PG_PROC_text_le:
	case 1050: // PG_PROC_bpcharle:
	case 1088: // PG_PROC_date_le:
	case 1103: // PG_PROC_time_le:
	case 2055: // PG_PROC_timestamp_le:
	case 1155: // PG_PROC_timestamptz_le_1155:
	case 2339: // PG_PROC_date_le_timestamp:
	case 2352: // PG_PROC_date_le_timestamptz:
	case 1723: // PG_PROC_numeric_le:
		return XRG_OP_LE;

	case 73:   // PG_PROC_chargt:
	case 146:  // PG_PROC_int2gt:
	case 162:  // PG_PROC_int24gt:
	case 1853: // PG_PROC_int28gt:
	case 147:  // PG_PROC_int4gt:
	case 855:  // PG_PROC_int48gt:
	case 470:  // PG_PROC_int8gt:
	case 1859: // PG_PROC_int82gt:
	case 477:  // PG_PROC_int84gt:
	case 291:  // PG_PROC_float4gt:
	case 303:  // PG_PROC_float48gt:
	case 297:  // PG_PROC_float8gt:
	case 309:  // PG_PROC_float84gt:
	case 742:  // PG_PROC_text_gt:
	case 1051: // PG_PROC_bpchargt:
	case 1089: // PG_PROC_date_gt:
	case 1104: // PG_PROC_time_gt:
	case 2057: // PG_PROC_timestamp_gt:
	case 1157: // PG_PROC_timestamptz_gt_1157:
	case 2341: // PG_PROC_date_gt_timestamp:
	case 2354: // PG_PROC_date_gt_timestamptz:
	case 1720: // PG_PROC_numeric_gt:
		return XRG_OP_GT;

	case 74:   // PG_PROC_charge:
	case 151:  // PG_PROC_int2ge:
	case 168:  // PG_PROC_int24ge:
	case 1855: // PG_PROC_int28ge:
	case 150:  // PG_PROC_int4ge:
	case 857:  // PG_PROC_int48ge:
	case 472:  // PG_PROC_int8ge:
	case 1861: // PG_PROC_int82ge:
	case 479:  // PG_PROC_int84ge:
	case 292:  // PG_PROC_float4ge:
	case 304:  // PG_PROC_float48ge:
	case 298:  // PG_PROC_float8ge:
	case 310:  // PG_PROC_float84ge:
	case 743:  // PG_PROC_text_ge:
	case 1052: // PG_PROC_bpcharge:
	case 1090: // PG_PROC_date_ge:
	case 1105: // PG_PROC_time_ge:
	case 2056: // PG_PROC_timestamp_ge:
	case 1156: // PG_PROC_timestamptz_ge_1156:
	case 2342: // PG_PROC_date_ge_timestamp:
	case 2355: // PG_PROC_date_ge_timestamptz:
	case 1721: // PG_PROC_numeric_ge:
		return XRG_OP_GE;

	case 176:  // PG_PROC_int2pl:
	case 178:  // PG_PROC_int24pl:
	case 841:  // PG_PROC_int28pl:
	case 177:  // PG_PROC_int4pl:
	case 1278: // PG_PROC_int48pl:
	case 463:  // PG_PROC_int8pl:
	case 837:  // PG_PROC_int82pl:
	case 1274: // PG_PROC_int84pl:
	case 204:  // PG_PROC_float4pl:
	case 281:  // PG_PROC_float48pl:
	case 218:  // PG_PROC_float8pl:
	case 285:  // PG_PROC_float84pl:
	case 2071: // PG_PROC_date_pl_interval:
	case 2032: // PG_PROC_timestamp_pl_interval:
	case 1724: // PG_PROC_numeric_add:
		return XRG_OP_ADD;

	case 180:  // PG_PROC_int2mi:
	case 182:  // PG_PROC_int24mi:
	case 942:  // PG_PROC_int28mi:
	case 181:  // PG_PROC_int4mi:
	case 1279: // PG_PROC_int48mi:
	case 464:  // PG_PROC_int8mi:
	case 838:  // PG_PROC_int82mi:
	case 1275: // PG_PROC_int84mi:
	case 205:  // PG_PROC_float4mi:
	case 282:  // PG_PROC_float48mi:
	case 219:  // PG_PROC_float8mi:
	case 286:  // PG_PROC_float84mi:
	case 1140: // PG_PROC_date_mi:
	case 2072: // PG_PROC_date_mi_interval:
	case 2031: // PG_PROC_timestamp_mi:
	case 2033: // PG_PROC_timestamp_mi_interval:
	case 1725: // PG_PROC_numeric_sub:
		return XRG_OP_SUB;

	case 152:  // PG_PROC_int2mul:
	case 170:  // PG_PROC_int24mul:
	case 943:  // PG_PROC_int28mul:
	case 141:  // PG_PROC_int4mul:
	case 1280: // PG_PROC_int48mul:
	case 465:  // PG_PROC_int8mul:
	case 839:  // PG_PROC_int82mul:
	case 1276: // PG_PROC_int84mul:
	case 202:  // PG_PROC_float4mul:
	case 279:  // PG_PROC_float48mul:
	case 216:  // PG_PROC_float8mul:
	case 283:  // PG_PROC_float84mul:
	case 1726: // PG_PROC_numeric_mul:
		return XRG_OP_MUL;

	case 153:  // PG_PROC_int2div:
	case 172:  // PG_PROC_int24div:
	case 948:  // PG_PROC_int28div:
	case 154:  // PG_PROC_int4div:
	case 1281: // PG_PROC_int48div:
	case 466:  // PG_PROC_int8div:
	case 840:  // PG_PROC_int82div:
	case 1277: // PG_PROC_int84div:
	case 203:  // PG_PROC_float4div:
	case 280:  // PG_PROC_float48div:
	case 217:  // PG_PROC_float8div:
	case 284:  // PG_PROC_float84div:
	case 1727: // PG_PROC_numeric_div:
		return XRG_OP_DIV;
	case 850: // PG_PROC_textlike:
		return XRG_OP_TEXTLIKE;
	case 851: // PG_PROC_textnlike:
		return XRG_OP_TEXTNLIKE;
	default:
		/* Unsupported. */
		return -1;
	}
}

static inline int32_t pg_func_to_op(int32_t funcid) {
	switch (funcid) {
	case 1598: // random
		return XRG_OP_RANDOM;
	case 1342: // round double
	case 1707: // numeric_round
		return XRG_OP_ROUND;
	case 940:  // int2mod
	case 941:  // int4mod
	case 947:  // int8mod
	case 1728: // numeric_mod
		return XRG_OP_MOD;
	case 2308: // ceil double
	case 2320: // ceiling double
	case 1711: // ceil numeric
	case 2167: // ceiling numeric
		return XRG_OP_CEIL;
	case 2309: // floor double
	case 1712: // floor_numeric
		return XRG_OP_FLOOR;
	case 207:  // float4abs
	case 1394: // abs -> float4abs
	case 221:  // float8abs
	case 1395: // abs -> float8abs
	case 1230: // int8abs
	case 1396: // abs -> int8abs
	case 1251: // int4abs
	case 1397: // abs -> int4abs
	case 1253: // int2abs
	case 1398: // abs -> int2abs
	case 1704: // numeric_abs
	case 1705: // abs -> numeric_abs
		return XRG_OP_ABS;
	case 877:
		return XRG_OP_SUBSTR;
	// See pg_cast.h for casting OID
	case 714:
	case 480:
	case 652:
	case 482:
	case 1781:
	case 754:
	case 313:
	case 236:
	case 235:
	case 1782:
	case 481:
	case 314:
	case 318:
	case 316:
	case 1740:
	case 653:
	case 238:
	case 319:
	case 311:
	case 483:
	case 237:
	case 317:
	case 312:
	case 1779:
	case 1783:
	case 1744:
	case 1745:
	case 1746:
	case 3824:
	case 3811:
	case 3812:
	case 2557:
	case 2558:
		return XRG_OP_CAST;
	default:
		/* unsupported */
		return -1;
	}
}

static inline int32_t pg_agg_to_op(int32_t funcid) {
	switch (funcid) {
	case 2147: // PG_PROC_count_2147:
		return XRG_OP_COUNT;
	case 2803: // PG_PROC_count_2803:
		return XRG_OP_COUNT_STAR;

	case 2100: // PG_PROC_avg_2100: /* avg int8 */
		return XRG_OP_AVG_INT128;
	case 2101: // PG_PROC_avg_2101: /* avg int4 */
	case 2102: // PG_PROC_avg_2102: /* avg int2 */
		return XRG_OP_AVG_INT64;
	case 2103: // PG_PROC_avg_2103: /* avg numeric */
		return XRG_OP_AVG_NUMERIC;
	case 2104: // PG_PROC_avg_2104: /* avg float4 */
	case 2105: // PG_PROC_avg_2105: /* avg float8 */
		/* 2106 is avg interval, not supported yet. */
		return XRG_OP_AVG_DOUBLE;

	case 2107: // PG_PROC_sum_2107: /* sum int8 */
		return XRG_OP_SUM_INT128;
	case 2108: // PG_PROC_sum_2108: /* sum int4 */
	case 2109: // PG_PROC_sum_2109: /* sum int2 */
		return XRG_OP_SUM_INT64;
	case 2110: // PG_PROC_sum_2110: /* sum float4 */
	case 2111: // PG_PROC_sum_2111: /* sum float8 */
		/* 2112 is sum cash, nyi */
		/* 2113 is sum interval, nyi */
		return XRG_OP_SUM_DOUBLE;
	case 2114: // PG_PROC_sum_2114: /* sum numeric */
		return XRG_OP_SUM_NUMERIC;

	case 2115: // PG_PROC_max_2115: /* int8 */
	case 2116: // PG_PROC_max_2116: /* int4 */
	case 2117: // PG_PROC_max_2117: /* int2 */
			   /* 2118 is oid, nyi */
	case 2119: // PG_PROC_max_2119: /* float4 */
	case 2120: // PG_PROC_max_2120: /* float8 */
			   /* 2121 is abstime, nyi */
	case 2122: // PG_PROC_max_2122: /* date, same as int4 */
	case 2123: // PG_PROC_max_2123: /* time, same as int8 */
			   /* 2124 is time tz, nyi */
	case 2125: // PG_PROC_max_2125: /* money/cash, same as int8 */
	case 2126: // PG_PROC_max_2126: /* timestamp, same as int8 */
	case 2127: // PG_PROC_max_2127: /* timestamptz, same as int8 */
			   /* 2128, interval nyi */
			   /* NOTE the following: what about collation? */
			   /* case PG_PROC_max_2129:       text */
	case 2130: // PG_PROC_max_2130: /* numeric */
		/* 2050, any arrray nyi */
		/* 2244, bpchar, nyi */
		/* 2797, tid, nyi */
		return XRG_OP_MAX;

	case 2131: // PG_PROC_min_2131: /* int8 */
	case 2132: // PG_PROC_min_2132: /* int4 */
	case 2133: // PG_PROC_min_2133: /* int2 */
			   /* 2134 is oid, nyi */
	case 2135: // PG_PROC_min_2135: /* float4 */
	case 2136: // PG_PROC_min_2136: /* float8 */
			   /* 2137 is abstime, nyi */
	case 2138: // PG_PROC_min_2138: /* date */
	case 2139: // PG_PROC_min_2139: /* time */
			   /* 2140 is timetz, nyi */
	case 2141: // PG_PROC_min_2141: /* money/cash */
	case 2142: // PG_PROC_min_2142: /* timestamp */
	case 2143: // PG_PROC_min_2143: /* timestamptz */
			   /* 2144, internval */
			   /* NOTE: text, collation? */
			   /* case PG_PROC_min_2145:       */
	case 2146: // PG_PROC_min_2146: /* numeric */
		/* 2051 is any array, nyi */
		/* 2245 bpchar nyi */
		/* 2798 tid nyi */
		return XRG_OP_MIN;

	default:
		return -1;
	}
}

#endif

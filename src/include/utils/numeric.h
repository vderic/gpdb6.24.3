/*-------------------------------------------------------------------------
 *
 * numeric.h
 *	  Definitions for the exact numeric data type of Postgres
 *
 * Original coding 1998, Jan Wieck.  Heavily revised 2003, Tom Lane.
 *
 * Copyright (c) 1998-2014, PostgreSQL Global Development Group
 *
 * src/include/utils/numeric.h
 *
 *-------------------------------------------------------------------------
 */
#ifndef _PG_NUMERIC_H_
#define _PG_NUMERIC_H_

#include "fmgr.h"

/*
 * Limit on the precision (and hence scale) specifiable in a NUMERIC typmod.
 * Note that the implementation limit on the length of a numeric value is
 * much larger --- beware of what you use this for!
 */
#define NUMERIC_MAX_PRECISION		1000

/*
 * Internal limits on the scales chosen for calculation results
 */
#define NUMERIC_MAX_DISPLAY_SCALE	NUMERIC_MAX_PRECISION
#define NUMERIC_MIN_DISPLAY_SCALE	0

#define NUMERIC_MAX_RESULT_SCALE	(NUMERIC_MAX_PRECISION * 2)

/*
 * For inherently inexact calculations such as division and square root,
 * we try to get at least this many significant digits; the idea is to
 * deliver a result no worse than float8 would.
 */
#define NUMERIC_MIN_SIG_DIGITS		16

/* EXX_IN_PG */
#define NUMERIC_POS                     0x0000
#define NUMERIC_NEG                     0x4000
#define NUMERIC_NAN                     0xC000
#define NUMERIC_HDRSZ   (VARHDRSZ + sizeof(uint16) + sizeof(int16))
#define NUMERIC_HDRSZ_SHORT (VARHDRSZ + sizeof(uint16))


/* The actual contents of Numeric are private to numeric.c */
struct NumericData;
typedef struct NumericData *Numeric;

/*
 * fmgr interface macros
 */

#define DatumGetNumeric(X)		  ((Numeric) PG_DETOAST_DATUM(X))
#define DatumGetNumericCopy(X)	  ((Numeric) PG_DETOAST_DATUM_COPY(X))
#define NumericGetDatum(X)		  PointerGetDatum(X)
#define PG_GETARG_NUMERIC(n)	  DatumGetNumeric(PG_GETARG_DATUM(n))
#define PG_GETARG_NUMERIC_COPY(n) DatumGetNumericCopy(PG_GETARG_DATUM(n))
#define PG_RETURN_NUMERIC(x)	  return NumericGetDatum(x)
extern double numeric_to_double_no_overflow(Numeric num);
extern int cmp_numerics(Numeric num1, Numeric num2);
extern float8 numeric_li_fraction(Numeric x, Numeric x0, Numeric x1, 
								  bool *eq_bounds, bool *eq_abscissas);
extern Numeric numeric_li_value(float8 f, Numeric y0, Numeric y1);

/*
 * Utility functions in numeric.c
 */
extern bool numeric_is_nan(Numeric num);
extern int16 *numeric_digits(Numeric num);
extern int numeric_len(Numeric num);
int32		numeric_maximum_size(int32 typmod);
extern char *numeric_out_sci(Numeric num, int scale);
extern char *numeric_normalize(Numeric num);

#endif   /* _PG_NUMERIC_H_ */

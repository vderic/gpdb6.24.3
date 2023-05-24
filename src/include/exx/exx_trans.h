#ifndef _EXX_TRANS_H_
#define _EXX_TRANS_H_

#include "utils/array.h"
#include "utils/numeric.h"

typedef struct ExxFloatAvgTransdata
{
        ArrayType arraytype;
        int32   nelem;
        float8  data[3];     // float8[3]
} ExxFloatAvgTransdata;

/* same data structure as Int128AggState.  See utils/adt/numeric.c */
typedef struct ExxInt128AggState
{
        bool            calcSumX2;              /* if true, calculate sumX2 */
        int64           N;                              /* count of processed numbers */
        int128          sumX;                   /* sum of processed numbers */
        int128          sumX2;                  /* sum of squares of processed numbers */
} ExxInt128AggState;


/* for numeric aggregate 
 * 
 * sum() calls numeric_avg_accum, it will return NumericAggState.
 *
 * For avg(),
 * 1. pass ExxNumericAggState to nodeAgg.i.c
 * 2. call exx_numeric_avg_combine(trans_value, sumX, N)
 *
 */

typedef struct ExxNumericAggState
{
	Numeric sumX;
	int64 	N;
} ExxNumericAggState;

#endif


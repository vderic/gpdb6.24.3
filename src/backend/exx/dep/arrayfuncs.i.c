// Copyright (c) 2018-2021, Vitesse Data Inc. All rights reserved.
#include "postgres.h"

#include <ctype.h>

#include "funcapi.h"
#include "libpq/pqformat.h"
#include "utils/array.h"
#include "utils/builtins.h"
#include "utils/datum.h"
#include "utils/lsyscache.h"
#include "utils/memutils.h"
#include "utils/typcache.h"
#include "exx/exx_lli.h"

Datum exx_construct_1d_array(Datum* datum_arr, 
							 bool* isnull_arr,
							 int32_t nelm,
							 Oid elmtyp,
							 int32_t elmlen,
							 bool elmbyval,
							 int8_t elmalign)
{
	int dims[1];
	int lbs[1];
	ArrayType* p;

	dims[0] = nelm;
	lbs[0] = 1;
	p = construct_md_array(datum_arr, isnull_arr, 1, dims, lbs,
						   elmtyp, elmlen, elmbyval, elmalign);
	return PointerGetDatum(p);
}


// Copyright (c) 2018-2021, Vitesse Data Inc. All rights reserved.
#include <time.h>
#include <unistd.h>
#include "exx_int.h"
#ifdef __cplusplus
extern "C" {
#endif
#include "access/printtup.h"
#include "libpq/libpq-be.h"
#include "libpq/md5.h"
#include "utils/guc.h"
#include "utils/inet.h"

/* c_main is the original postgres main function */
int c_main(int argc, char *argv[]);
MemoryContext exx_perquery_memctxt(); 
MemoryContext exx_estate_memctxt(); 
void exx_instrument_reset(); 
void exx_instrument_charge(Instrumentation *in);


#ifdef __cplusplus
}
#endif

void exx_raise_error(int err, const char *msg) 
{
    // No msg, find the default one based on err code.
    if (msg == NULL) {
        switch (err) {
            case ERRCODE_DIVISION_BY_ZERO:
                msg = "division by zero";
                break;
            case ERRCODE_DATETIME_VALUE_OUT_OF_RANGE:
                msg = "date out of range for timestamp"; 
                break;
            case ERRCODE_NUMERIC_VALUE_OUT_OF_RANGE:
                msg = "integer out of range";
                break;
            default:
                msg = "generic error";
                break;
        }
    }
    ereport(ERROR, (errcode(err), errmsg("%s", msg))); 
}


void exx_check_failed(const char* file, int line, const char* msg)
{
	return;
}


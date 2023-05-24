// Copyright (c) 2018-2021, Vitesse Data Inc. All rights reserved.
#ifndef EXX_INT_HPP
#define EXX_INT_HPP

/* these .h files from postgres when included produce 
   too many warnings. */
// #define S_LOCK_H  
// #define LWLOCK_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif
#include "postgres.h"
#include "access/relscan.h"
#include "catalog/pg_type.h"
#include "executor/execdesc.h"
#include "executor/instrument.h"
#include "utils/memutils.h"
#include "utils/timestamp.h"
#include "exx/exx.h"
#include "miscadmin.h"
#include "cdb/cdbvars.h"
#include "utils/numeric.h"
#include "utils/builtins.h"
#ifdef __cplusplus
}
#endif

#endif /* EXX_INT_HPP */

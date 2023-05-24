// Copyright (c) 2018-2021, Vitesse Data Inc. All rights reserved.
#include <unistd.h>
#include "exx_int.h"
#include "libpq/md5.h"
#include "utils/guc.h"
#include "utils/date.h"
#include "xexpr.h"

void exx_init()
{
	// our assumptions
	Assert(sizeof(intptr_t) == sizeof(Datum));
	Assert(sizeof(bool) == sizeof(uint8_t));
	Assert(sizeof(DateADT) == sizeof(int32));
#ifndef HAVE_INT64_TIMESTAMP
	Assert(0 && "we require int64 timestamp");
#endif
	xex_set_memutil(palloc, pfree);

}

#ifndef _DECODE_H_
#define _DECODE_H_

#include "../exx_int.h"

#include "xrg.h"
#include "kite_extscan.h"

/* decode functions */
int decode_var(struct kite_target_t *tgt, xrg_iter_t *iter, Datum *pg_datum, bool *pg_isnull);

int decode_avg_int64(struct kite_target_t *tgt, xrg_iter_t *iter, Datum *pg_datum, bool *pg_isnull);

int decode_avg_int128(struct kite_target_t *tgt, xrg_iter_t *iter, Datum *pg_datum, bool *pg_isnull);

int decode_avg_double(struct kite_target_t *tgt, xrg_iter_t *iter, Datum *pg_datum, bool *pg_isnull);

int decode_avg_numeric(struct kite_target_t *tgt, xrg_iter_t *iter, Datum *pg_datum, bool *pg_isnull);

#endif

// Copyright (c) 2018-2021, Vitesse Data Inc. All rights reserved.
#ifndef EXX_SARG_HPP
#define EXX_SARG_HPP

#ifdef __cplusplus
extern "C" {
#endif

char *op_sarg_const_str(const char *ts, Datum d, int *plen);

const char *op_arraytype_to_string(Const *c);

size_t xrg_typ_size(int16_t ptyp);

const char *xrg_typ_str(int16_t ptyp, int16_t ltyp);

void pg_typ_to_xrg_typ(Oid t, int32_t typmod, int16_t *ptyp, int16_t *ltyp, int16_t *precision, int16_t *scale, bool *is_array);

bool pg_typ_match_xrg_typ(Oid pgtyp, int32_t typmod, int16_t ptyp, int16_t ltyp);

#ifdef __cplusplus
}
#endif

#endif

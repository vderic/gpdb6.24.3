/*-------------------------------------------------------------------------
 *
 * ic_proxy_bgworker.h
 *	  TODO file description
 *
 *
 * Copyright (c) 2020-Present Pivotal Software, Inc.
 *
 *
 *-------------------------------------------------------------------------
 */

#ifndef IC_PROXY_BGWORKER_H
#define IC_PROXY_BGWORKER_H

#include "postgres.h"


extern bool ICProxyStartRule(Datum main_arg);
extern void ICProxyMain(Datum main_arg);

#endif   /* IC_PROXY_BGWORKER_H */

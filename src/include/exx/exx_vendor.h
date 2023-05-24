// Copyright (c) 2018-2021, Vitesse Data Inc. All rights reserved.
#ifndef _EXX_VENDOR_H_
#define _EXX_VENDOR_H_

#include "exx_engine.h"

#if defined(EXX_VENDOR_HIGHGO)
/* Build for HighGo. */
#define EXX_VENDOR_NAME "HighGo"
#define EXX_VENDOR_PRODUCT "HighGo DW"
#define EXX_VENDOR_GUC "highgo"

#elif defined(EXX_VENDOR_DIGITALCHINA)
/* Build for Digital China */
#define EXX_VENDOR_NAME "DigitalChina"
#define EXX_VENDOR_PRODUCT "Deepgreen DB"
#define EXX_VENDOR_GUC "deepgreen"

#elif defined(EXX_VENDOR_FLOURISHING)
/* Build for Flourishing */
#define EXX_VENDOR_NAME "Flourishing"
#define EXX_VENDOR_PRODUCT "Origin DB"
#define EXX_VENDOR_GUC "origindb"


#else
/* Build for ourselves */
#define EXX_VENDOR_VITESSE
#define EXX_VENDOR_NAME "VitesseData" 
#define EXX_VENDOR_PRODUCT "Deepgreen DB" 
#define EXX_VENDOR_GUC "vitesse" 
#endif

#ifdef USE_ASSERT_CHECKING
#define EXX_BUILD_TYPE " (dev)"
#else
#define EXX_BUILD_TYPE ""
#endif

#define EXX_VENDOR_VERSION (EXX_VENDOR_PRODUCT " " EXX_BUILD_VENDOR_VERSION " [rev " EXX_BUILD_HASH " on " EXX_BUILD_DATE "]" EXX_BUILD_TYPE)

#endif  /* _H_ */

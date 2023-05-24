/******************************************************************************
  Copyright (c) 2011, Intel Corp.
  All rights reserved.

  Redistribution and use in source and binary forms, with or without 
  modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice, 
      this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright 
      notice, this list of conditions and the following disclaimer in the 
      documentation and/or other materials provided with the distribution.
    * Neither the name of Intel Corporation nor the names of its contributors 
      may be used to endorse or promote products derived from this software 
      without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
  ARE DISCLAIMED. IN NO EVENT SHALL THE Portions Copyright (c) 2018-2021, Vitesse Data Inc. All rights reserved.
  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
  THE POSSIBILITY OF SUCH DAMAGE.
******************************************************************************/

// 000:
// 0 arguments passed by value (except fpsf)
// 0 rounding mode passed as argument
// 0 pointer to status flags passed as argument 

#ifdef WINDOWS
  #define LX "%I64x"
#else
  #ifdef HPUX_OS
    #define LX "%llx"
  #else
    #define LX "%Lx"
  #endif
#endif

/* basic decimal floating-point types */

#if defined _MSC_VER
#if defined _M_IX86 && !defined __INTEL_COMPILER // Win IA-32, MS compiler
#define ALIGN(n)
#else
#define ALIGN(n) __declspec(align(n))
#endif
#else
#define ALIGN(n) __attribute__ ((aligned(n)))
#endif

typedef unsigned int Decimal32;
typedef unsigned long long Decimal64;
typedef ALIGN(16) struct { unsigned long long w[2]; } Decimal128;

/* rounding modes */

typedef enum _IDEC_roundingmode {
    _IDEC_nearesteven = 0,
    _IDEC_downward    = 1,
    _IDEC_upward      = 2,
    _IDEC_towardzero  = 3,
    _IDEC_nearestaway = 4,
    _IDEC_dflround    = _IDEC_nearesteven
} _IDEC_roundingmode;
typedef unsigned int _IDEC_round;


/* exception flags */

typedef enum _IDEC_flagbits {
    _IDEC_invalid       = 0x01,
    _IDEC_zerodivide    = 0x04,
    _IDEC_overflow      = 0x08,
    _IDEC_underflow     = 0x10,
    _IDEC_inexact       = 0x20,
    _IDEC_allflagsclear = 0x00
} _IDEC_flagbits;
typedef unsigned int _IDEC_flags;  // could be a struct with diagnostic info

/*
 * The functions we use ...
 */
extern bool __bid64_isZero(Decimal64); 
extern bool __bid128_isZero(Decimal128); 

extern bool __bid64_isSigned(Decimal64); 
extern bool __bid128_isSigned(Decimal128); 

extern Decimal64 __bid64_from_int32(int32_t); 
extern Decimal128 __bid128_from_int32(int32_t); 

extern Decimal64 __bid64_from_int64(int64_t, _IDEC_round, _IDEC_flags*); 
extern Decimal128 __bid128_from_int64(int64_t); 

extern Decimal64 __bid64_from_string(char*, _IDEC_round, _IDEC_flags*); 
extern Decimal128 __bid128_from_string(char*, _IDEC_round, _IDEC_flags*); 

extern Decimal64 __binary32_to_bid64(float, _IDEC_round, _IDEC_flags*); 
extern Decimal128 __binary32_to_bid128(float, _IDEC_round, _IDEC_flags*); 

extern Decimal64 __binary64_to_bid64(float, _IDEC_round, _IDEC_flags*); 
extern Decimal128 __binary64_to_bid128(float, _IDEC_round, _IDEC_flags*); 

extern int16_t __bid64_to_int16_rnint(Decimal64, _IDEC_flags*);
extern int16_t __bid128_to_int16_rnint(Decimal128, _IDEC_flags*);

extern int32_t __bid64_to_int32_rnint(Decimal64, _IDEC_flags*);
extern int32_t __bid128_to_int32_rnint(Decimal128, _IDEC_flags*);

extern int64_t __bid64_to_int64_rnint(Decimal64, _IDEC_flags*);
extern int64_t __bid128_to_int64_rnint(Decimal128, _IDEC_flags*);

extern float __bid64_to_binary32(Decimal64, _IDEC_round, _IDEC_flags*);
extern float __bid128_to_binary32(Decimal128, _IDEC_round, _IDEC_flags*);

extern double __bid64_to_binary64(Decimal64, _IDEC_round, _IDEC_flags*);
extern double __bid128_to_binary64(Decimal128, _IDEC_round, _IDEC_flags*);

extern void __bid64_to_string(char*, Decimal64, _IDEC_flags*);
extern void __bid128_to_string(char*, Decimal128, _IDEC_flags*);

extern Decimal128 __bid64_to_bid128(Decimal64, _IDEC_flags*);
extern Decimal64 __bid128_to_bid64(Decimal128, _IDEC_round, _IDEC_flags*);

extern Decimal64 __bid64_abs(Decimal64);
extern Decimal128 __bid128_abs(Decimal128);

extern Decimal64 __bid64_negate(Decimal64);
extern Decimal128 __bid128_negate(Decimal128);

extern Decimal64 __bid64_scalbn(Decimal64, int, _IDEC_round, _IDEC_flags*); 
extern Decimal128 __bid128_scalbn(Decimal128, int, _IDEC_round, _IDEC_flags*); 

extern Decimal64 __bid64_quantize(Decimal64, Decimal64, _IDEC_round, _IDEC_flags*); 
extern Decimal128 __bid128_quantize(Decimal128, Decimal128, _IDEC_round, _IDEC_flags*); 

extern Decimal64 __bid64_round_integral_nearest_even(Decimal64, _IDEC_flags*); 
extern Decimal128 __bid128_round_integral_nearest_even(Decimal128, _IDEC_flags*); 

extern Decimal64 __bid64_round_integral_positive(Decimal64, _IDEC_flags*); 
extern Decimal128 __bid128_round_integral_positive(Decimal128, _IDEC_flags*); 

extern Decimal64 __bid64_round_integral_negative(Decimal64, _IDEC_flags*); 
extern Decimal128 __bid128_round_integral_negative(Decimal128, _IDEC_flags*); 

extern Decimal64 __bid64_round_integral_zero(Decimal64, _IDEC_flags*); 
extern Decimal128 __bid128_round_integral_zero(Decimal128, _IDEC_flags*); 

extern Decimal64 __bid64_minnum(Decimal64, Decimal64, _IDEC_flags*); 
extern Decimal128 __bid128_minnum(Decimal128, Decimal128, _IDEC_flags*); 

extern Decimal64 __bid64_maxnum(Decimal64, Decimal64, _IDEC_flags*); 
extern Decimal128 __bid128_maxnum(Decimal128, Decimal128, _IDEC_flags*); 

extern bool __bid64_quiet_equal(Decimal64, Decimal64, _IDEC_flags*); 
extern bool __bid128_quiet_equal(Decimal128, Decimal128, _IDEC_flags*); 

extern bool __bid64_quiet_not_equal(Decimal64, Decimal64, _IDEC_flags*); 
extern bool __bid128_quiet_not_equal(Decimal128, Decimal128, _IDEC_flags*); 

extern bool __bid64_quiet_less(Decimal64, Decimal64, _IDEC_flags*); 
extern bool __bid128_quiet_less(Decimal128, Decimal128, _IDEC_flags*); 

extern bool __bid64_quiet_less_equal(Decimal64, Decimal64, _IDEC_flags*); 
extern bool __bid128_quiet_less_equal(Decimal128, Decimal128, _IDEC_flags*); 

extern bool __bid64_quiet_greater(Decimal64, Decimal64, _IDEC_flags*); 
extern bool __bid128_quiet_greater(Decimal128, Decimal128, _IDEC_flags*); 

extern bool __bid64_quiet_greater_equal(Decimal64, Decimal64, _IDEC_flags*); 
extern bool __bid128_quiet_greater_equal(Decimal128, Decimal128, _IDEC_flags*); 

extern Decimal64 __bid64_add(Decimal64, Decimal64, _IDEC_round, _IDEC_flags*); 
extern Decimal128 __bid128_add(Decimal128, Decimal128, _IDEC_round, _IDEC_flags*); 

extern Decimal64 __bid64_sub(Decimal64, Decimal64, _IDEC_round, _IDEC_flags*); 
extern Decimal128 __bid128_sub(Decimal128, Decimal128, _IDEC_round, _IDEC_flags*); 

extern Decimal64 __bid64_mul(Decimal64, Decimal64, _IDEC_round, _IDEC_flags*); 
extern Decimal128 __bid128_mul(Decimal128, Decimal128, _IDEC_round, _IDEC_flags*); 

extern Decimal64 __bid64_div(Decimal64, Decimal64, _IDEC_round, _IDEC_flags*); 
extern Decimal128 __bid128_div(Decimal128, Decimal128, _IDEC_round, _IDEC_flags*); 

extern Decimal64 __bid_dpd_to_bid64(Decimal64);
extern Decimal128 __bid_dpd_to_bid128(Decimal128);

#if BID_BIG_ENDIAN
#define HIGH_128W 0
#define LOW_128W  1
#else
#define HIGH_128W 1
#define LOW_128W  0
#endif

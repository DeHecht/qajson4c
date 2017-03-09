/**
  @file

  Quite-Alright JSON for C - https://github.com/USESystemEngineeringBV/qajson4c

  Licensed under the MIT License <http://opensource.org/licenses/MIT>.

  Copyright (c) 2016 Pascal Proksch - USE System Engineering BV

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
  THE SOFTWARE.
*/

/*
 * Within this file all method to std libraries are placed in order to be able to
 * use qajson4c without std library support and to implement the missing functions.
 */

#ifndef QAJ4C_STDWRAP_H_
#define QAJ4C_STDWRAP_H_

#if (defined(__STDC_VERSION__) && (__STDC_VERSION__ < 199901L))
#error "You need to specify a own implementation of the standard library when not using at least C99"
#define NO_STDLIB
#endif

#ifndef NO_STDLIB

#include <stdbool.h>
#include <stdint.h>
#include <limits.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <signal.h>
#include <float.h>
#include <errno.h>
#include <inttypes.h>

#if ((ULONG_MAX) == (UINT64_MAX))
#define QAJ4C_strtoul strtoul
#elif ((ULLONG_MAX) == (UINT64_MAX))
#define QAJ4C_strtoul strtoull
#else
#error "ULONG_MAX >= UINT64_MAX"
#endif

#if ((LONG_MAX) == (INT64_MAX))
#define QAJ4C_strtol strtol
#elif ((LLONG_MAX) == (INT64_MAX))
#define QAJ4C_strtol strtoll
#else
#error "LONG_MAX >= INT64_MAX"
#endif

#define QAJ4C_strlen strlen
#define QAJ4C_strncmp strncmp
#define QAJ4C_memmove memmove
#define QAJ4C_memcpy memcpy

#ifndef _WIN32
#define QAJ4C_snprintf snprintf
#else
#define QAJ4C_snprintf __mingw_snprintf
#endif

#if defined(_WIN64) || (defined(__WORDSIZE) && __WORDSIZE == 64)
#define QAJ4C_ALIGN __attribute__((packed, aligned(8)))
#elif defined(_WIN32) || (defined(__WORDSIZE) && __WORDSIZE == 32)
#define QAJ4C_ALIGN __attribute__((packed, aligned(4)))
#else
#define "Invalid word size detected!"
#endif

#define QAJ4C_itostrn(buffer, n, value) QAJ4C_snprintf(buffer, n, "%" PRIi64, value)
#define QAJ4C_utostrn(buffer, n, value) QAJ4C_snprintf(buffer, n, "%" PRIu64, value)
#define QAJ4C_strtod strtod
#define QAJ4C_qsort qsort
#define QAJ4C_bsearch bsearch
#define QAJ4C_raise raise

#endif

#endif

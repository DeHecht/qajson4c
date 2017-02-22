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

#if (__STDC_VERSION__ >= 199901L)
#define QAJ4C_INLINE inline
#else
#define QAJ4C_INLINE
#endif

#ifndef NO_STDLIB

#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <signal.h>
#include <float.h>

#if (__STDC_VERSION__ >= 199901L)
#define QAJ4C_SNPRINTF snprintf
#define FMT_UINT64_T "ju"
#define FMT_INT64_T  "jd"
#else
#define QAJ4C_SNPRINTF(s, n, format, arg) sprintf(s, format, arg)
#ifdef __LP64__
#  define FMT_UINT64_T "lu"
#  define FMT_INT64_T  "ld"
#else
#  define FMT_UINT64_T "lu"
#  define FMT_INT64_T  "l"
#endif
#endif

#define QAJ4C_strlen strlen
#define QAJ4C_strncmp strncmp
#define QAJ4C_memmove memmove
#define QAJ4C_memcpy memcpy
#define QAJ4C_itostrn(buffer, n, value) QAJ4C_SNPRINTF(buffer, n, "%"FMT_INT64_T, value)
#define QAJ4C_utostrn(buffer, n, value) QAJ4C_SNPRINTF(buffer, n, "%"FMT_UINT64_T, value)
#define QAJ4C_strtol strtol
#define QAJ4C_strtoul strtoul
#define QAJ4C_strtod strtod
#define QAJ4C_qsort qsort
#define QAJ4C_bsearch bsearch
#define QAJ4C_raise raise

#endif

#endif

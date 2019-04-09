/*
  Quite-Alright JSON for C - https://github.com/DeHecht/qajson4c

  Licensed under the MIT License <http://opensource.org/licenses/MIT>.

  Copyright (c) 2019 Pascal Proksch

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

#ifndef TEST_UNIT_TESTS_TEST_H_
#define TEST_UNIT_TESTS_TEST_H_

#ifdef __STRICT_ANSI__
#undef __STRICT_ANSI__
#endif

#ifndef _WIN32
#include <wait.h>
#define FMT_SIZE "%zu"
#else
#define FMT_SIZE "%Iu"
#endif

/**
 * This has been copied from gtest
 */
#if defined(__GNUC__) && !defined(COMPILER_ICC)
# define UNITTESTS_ATTRIBUTE_UNUSED_ __attribute__ ((unused))
#else
# define UNITTESTS_ATTRIBUTE_UNUSED_
#endif

#define ARRAY_COUNT(x)  (sizeof(x) / sizeof(x[0]))

typedef struct QAJ4C_TEST_DEF QAJ4C_TEST_DEF;

QAJ4C_TEST_DEF* create_test( const char* subject, const char* test, void (*test_func)() );

int test_main( int argc, char **argv );

#define TEST(a, b) \
   void a## _## b## _test(); \
   UNITTESTS_ATTRIBUTE_UNUSED_ QAJ4C_TEST_DEF* a## _## b## _test_ptr = create_test(#a, #b, a## _## b## _test); \
   void a## _## b## _test()

#endif /* TEST_UNIT_TESTS_TEST_H_ */

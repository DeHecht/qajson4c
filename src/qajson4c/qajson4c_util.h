/**
  @file

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

#ifndef QAJ4C_UTIL_H_
#define QAJ4C_UTIL_H_

#include "qajson4c_types.h"

#ifdef __cplusplus
extern "C" {
#endif


/**
 * This method will compare the value's string value with the handed over string
 * with the given length.
 *
 * @note: The length of the string will be compared first ... so the results will
 * most likely differ to the c library strcmp.
 */
int QAJ4C_string_cmp_n( const QAJ4C_Value* value_ptr, const char* str, size_t len );

/**
 * This method will compare the value's string value with the handed over string
 * using strlen to determine the strings size.
 *
 * @note: The length of the string will be compared first ... so the results will
 * most likely differ to the c library strcmp.
 */
int QAJ4C_string_cmp( const QAJ4C_Value* value_ptr, const char* str );

/**
 * This method will return true, in case the handed over string with the given size
 * is equal to the value's string.
 */
bool QAJ4C_string_equals_n( const QAJ4C_Value* value_ptr, const char* str, size_t len );

/**
 * This method will return true, in case the handed over string is equal to the
 * value's string. The string's length is determined using strlen.
 */
bool QAJ4C_string_equals( const QAJ4C_Value* value_ptr, const char* str );

/**
 * This method creates a deep copy of the source value to the destination value using the
 * handed over builder.
 */
void QAJ4C_copy( const QAJ4C_Value* src, QAJ4C_Value* dest, QAJ4C_Builder* builder );

/**
 * This method checks if two elements are equal to each other.
 */
bool QAJ4C_equals( const QAJ4C_Value* lhs, const QAJ4C_Value* rhs );

/**
 * This method calculates the size of the QAJ4C_Value DOM.
 *
 * @note QAJ4C_Value cannot be allocated stand alone so in case you intend to extract
 * a value into a new document you need to use QAJ4C_value_sizeof_as_document instead.
 */
size_t QAJ4C_value_sizeof( const QAJ4C_Value* value_ptr );

#ifdef __cplusplus
}
#endif

#endif /* QAJ4C_UTIL_H_ */

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

#ifndef QAJ4C_PRINT_H_
#define QAJ4C_PRINT_H_

#include "qajson4c_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * This type defines a callback method for the print method. This callback will be called
 * for each individual char.
 * @return true, on success else false.
 */
typedef bool (*QAJ4C_print_callback_fn)( void *ptr, char c );

/**
 * Second print callback type that will be called with a buffer and size.
 * @return true, on success else false.
 */
typedef bool (*QAJ4C_print_buffer_callback_fn)( void *ptr, const char* buffer, size_t size );

/**
 * This method prints the DOM as JSON in the handed over buffer.
 *
 * @return the amount of data written to the buffer, including the '\0' character.
 */
size_t QAJ4C_sprint( const QAJ4C_Value* value_ptr, char* buffer, size_t buffer_size );

/**
 * This method prints the DOM as JSON using the provided callback. Also a data ptr can be
 * supplied to be handed over to the callback.
 */
bool QAJ4C_print_callback( const QAJ4C_Value* value_ptr, QAJ4C_print_callback_fn callback, void* ptr );

/**
 * This method prints the DOM as JSON using the provided callback. Also a data ptr can be
 * supplied to be handed over to the callback.
 */
bool QAJ4C_print_buffer_callback( const QAJ4C_Value* value_ptr, QAJ4C_print_buffer_callback_fn callback, void* ptr );

#ifdef __cplusplus
}
#endif

#endif /* QAJ4C_PRINT_H_ */

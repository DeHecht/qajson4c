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

#include "qajson4c_print.h"
#include "internal/print.h"

size_t QAJ4C_sprint( const QAJ4C_Value* value_ptr, char* buffer, size_t buffer_size ) {
    size_t index;
    if (QAJ4C_UNLIKELY(buffer_size == 0)) {
        return 0;
    }

    index = QAJ4C_sprint_impl( value_ptr, buffer, buffer_size, 0 );

    if (QAJ4C_UNLIKELY(index >= buffer_size)) {
        index = buffer_size - 1;
    }
    buffer[index] = '\0';
    return index + 1;
}

bool QAJ4C_print_callback( const QAJ4C_Value* value_ptr, QAJ4C_print_callback_fn callback, void* ptr ) {
    if (QAJ4C_UNLIKELY(callback == NULL)) {
        return false;
    }
    return QAJ4C_print_callback_impl(value_ptr, callback, ptr);
}

bool QAJ4C_print_buffer_callback( const QAJ4C_Value* value_ptr, QAJ4C_print_buffer_callback_fn callback, void* ptr )
{
    if (QAJ4C_UNLIKELY(callback == NULL)) {
        return false;
    }
    return QAJ4C_print_buffer_callback_impl(value_ptr, callback, ptr);
}


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

#include "print.h"




size_t QAJ4C_sprint_impl(const QAJ4C_Value* value_ptr, char* buffer, size_t buffer_size, size_t index) {
	(void)value_ptr;
	(void)buffer;
	(void)buffer_size;
	(void)index;
	return 0;
}
bool QAJ4C_print_callback_impl(const QAJ4C_Value* value_ptr, QAJ4C_print_callback_fn callback, void* ptr) {
	(void)value_ptr;
	(void)callback;
	(void)ptr;
	return false;
}
bool QAJ4C_print_buffer_callback_impl(const QAJ4C_Value* value_ptr, QAJ4C_print_buffer_callback_fn callback, void* ptr) {
	(void)value_ptr;
	(void)callback;
	(void)ptr;
	return false;
}


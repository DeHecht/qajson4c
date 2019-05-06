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

#ifndef QAJSON4C_INTERNAL_SECOND_PASS_H_
#define QAJSON4C_INTERNAL_SECOND_PASS_H_

#include "first_pass.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct QAJ4C_Second_pass_builder {
    void* buffer;
    size_t buffer_length;
    QAJ4C_Value* object_pos;
    size_type* stats_pos;
    char* str_pos;
} QAJ4C_Second_pass_builder;

typedef struct QAJ4C_Second_pass_parser {
    QAJ4C_Second_pass_builder builder;
    bool insitu_parsing;
    bool deny_trailing_commas;
    bool deny_duplicate_keys;
    bool deny_uncompliant_numbers;
    QAJ4C_ERROR_CODE err_code;
    fast_size_type curr_buffer_pos;
} QAJ4C_Second_pass_parser;

QAJ4C_Second_pass_parser QAJ4C_second_pass_parser_create( const QAJ4C_First_pass_parser* parser );

/**
 * @note the first pass parser validates that all objects, arrays and strings are closed properly.
 *
 * @param bytes_written the amount of bytes written
 * @returns the parsed value
 */
const QAJ4C_Value* QAJ4C_second_pass_process( QAJ4C_Second_pass_parser* me, QAJ4C_Json_message* msg, size_t* bytes_written );

#ifdef __cplusplus
}
#endif

#endif /* QAJSON4C_INTERNAL_SECOND_PASS_H_ */

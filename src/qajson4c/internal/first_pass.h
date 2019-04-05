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

#ifndef QAJSON4C_INTERNAL_FIRST_PASS_H_
#define QAJSON4C_INTERNAL_FIRST_PASS_H_

#include "types.h"
#include "../qajson4c.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct QAJ4C_First_pass_parser {
    QAJ4C_Builder* builder; /* Storage about object sizes */
    QAJ4C_realloc_fn realloc_callback;

    bool strict_parsing;
    bool insitu_parsing;
    bool optimize_object;

    size_type amount_nodes;
    size_type complete_string_length;
    size_type storage_counter;

    QAJ4C_ERROR_CODE err_code;

} QAJ4C_First_pass_parser;

/**
 * Creates a initialized QAJ4C_First_pass_parser struct.
 * @param builder the builder to use (or NULL when only statistics should be calculated)
 * @param opts the parse options
 * @param realloc_callback the realloc method to use in case the buffer size is insufficient.
 * @return the initialized instance of the QAJ4C_First_pass_parser.
 */
QAJ4C_First_pass_parser QAJ4C_first_pass_parser_create( QAJ4C_Builder* builder, int opts, QAJ4C_realloc_fn realloc_callback );

/**
 * Performs the actual parsing and stores the results into the QAJ4C_First_pass_parser.
 */
void QAJ4C_first_pass_parse( QAJ4C_First_pass_parser* me, QAJ4C_Json_message* msg );

/**
 * This method calculates the amount of bytes required to parse a json message.
 */
size_t QAJ4C_calculate_max_buffer_parser( QAJ4C_First_pass_parser* parser );

#endif /* QAJSON4C_INTERNAL_FIRST_PASS_H_ */

#ifdef __cplusplus
}
#endif


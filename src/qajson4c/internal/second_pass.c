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

#include "first_pass.h"
#include "second_pass.h"

typedef struct QAJ4C_Second_pass_stack_entry {
	QAJ4C_TYPE type;
	QAJ4C_Value* current_key; /* unused for arrays */
	QAJ4C_Value* current_value;
	size_type index;
} QAJ4C_Second_pass_stack_entry;

typedef struct QAJ4C_Second_pass_stack {
	QAJ4C_Second_pass_stack_entry info[32];
	size_t pos;
} QAJ4C_Second_pass_stack;


QAJ4C_Second_pass_parser QAJ4C_second_pass_parser_create( QAJ4C_First_pass_parser* parser ) {
	QAJ4C_Second_pass_parser me;

    QAJ4C_Builder* builder = parser->builder;
    size_type required_object_storage = parser->amount_nodes * sizeof(QAJ4C_Value);
    size_type required_tempoary_storage = parser->storage_counter *  sizeof(size_type);
    size_type copy_to_index = required_object_storage - required_tempoary_storage;

    memmove(builder->buffer + copy_to_index, builder->buffer, required_tempoary_storage);
    me.builder = parser->builder;
    me.realloc_callback = parser->realloc_callback;
    me.insitu_parsing = parser->insitu_parsing;
    me.optimize_object = parser->optimize_object;
    me.curr_buffer_pos = copy_to_index;

    /* reset the builder to its original state! */
    QAJ4C_builder_init(builder, builder->buffer, builder->buffer_size);
    builder->cur_str_pos = required_object_storage;

    return me;
}

static void QAJ4C_second_pass_number( QAJ4C_Second_pass_parser* me, QAJ4C_Json_message* msg, QAJ4C_Second_pass_stack* stack );
static void QAJ4C_second_pass_object_open( QAJ4C_Second_pass_parser* me, QAJ4C_Json_message* msg, QAJ4C_Second_pass_stack* stack );
static void QAJ4C_second_pass_object_colon( QAJ4C_Second_pass_parser* me, QAJ4C_Json_message* msg, QAJ4C_Second_pass_stack* stack );
static void QAJ4C_second_pass_object_close( QAJ4C_Second_pass_parser* me, QAJ4C_Json_message* msg, QAJ4C_Second_pass_stack* stack );
static void QAJ4C_second_pass_array_open( QAJ4C_Second_pass_parser* me, QAJ4C_Json_message* msg, QAJ4C_Second_pass_stack* stack );
static void QAJ4C_second_pass_array_close( QAJ4C_Second_pass_parser* me, QAJ4C_Json_message* msg, QAJ4C_Second_pass_stack* stack );
static void QAJ4C_second_pass_comma( QAJ4C_Second_pass_parser* me, QAJ4C_Json_message* msg, QAJ4C_Second_pass_stack* stack );
static void QAJ4C_second_pass_string_start( QAJ4C_Second_pass_parser* me, QAJ4C_Json_message* msg, QAJ4C_Second_pass_stack* stack );
static void QAJ4C_second_pass_literal_start( QAJ4C_Second_pass_parser* me, QAJ4C_Json_message* msg, QAJ4C_Second_pass_stack* stack );
static void QAJ4C_second_pass_comment_start( QAJ4C_Second_pass_parser* me, QAJ4C_Json_message* msg, QAJ4C_Second_pass_stack* stack );
static size_type QAJ4C_second_pass_fetch_stats_data( QAJ4C_Second_pass_parser* me );

QAJ4C_Value* QAJ4C_second_pass_process( QAJ4C_Second_pass_parser* me, QAJ4C_Json_message* msg ) {
	QAJ4C_Value* result = QAJ4C_builder_get_document(me->builder);

	QAJ4C_Second_pass_stack stack;
	stack.info[0].current_value = result;
	msg->pos = msg->begin;
	QAJ4C_json_message_skip_whitespaces(msg);

	while ( msg->pos < msg->end )
	{
		QAJ4C_Char_type type = QAJ4C_parse_char(*msg->pos);
		switch (type) {
		case QAJ4C_CHAR_NUMERIC_START:
			QAJ4C_second_pass_number(me, msg, &stack);
			break;
		case QAJ4C_CHAR_OBJECT_START:
			QAJ4C_second_pass_object_open(me, msg, &stack);
			break;
		case QAJ4C_CHAR_OBJECT_END:
			QAJ4C_second_pass_object_close(me, msg, &stack);
			break;
		case QAJ4C_CHAR_ARRAY_START:
			QAJ4C_second_pass_array_open(me, msg, &stack);
			break;
		case QAJ4C_CHAR_ARRAY_END:
			QAJ4C_second_pass_array_close(me, msg, &stack);
			break;
		case QAJ4C_CHAR_COLON:
			QAJ4C_second_pass_object_colon(me, msg, &stack);
			break;
		case QAJ4C_CHAR_COMMA:
			QAJ4C_second_pass_comma(me, msg, &stack);
			break;
		case QAJ4C_CHAR_COMMENT_START:
			QAJ4C_second_pass_comment_start(me, msg, &stack);
			break;
		case QAJ4C_CHAR_STRING_START:
			QAJ4C_second_pass_string_start(me, msg, &stack);
			break;
		case QAJ4C_CHAR_LITERAL_START:
			QAJ4C_second_pass_literal_start(me, msg, &stack);
			break;
		default:
			// Unreachable error!
			break;
		}
		msg->pos += 1;
		QAJ4C_json_message_skip_whitespaces(msg);
	}
	return result;
}

static void QAJ4C_second_pass_number(QAJ4C_Second_pass_parser* me, QAJ4C_Json_message* msg, QAJ4C_Second_pass_stack* stack) {
	(void) me;
	(void) msg;
	(void) stack;
}

static void QAJ4C_second_pass_object_open(QAJ4C_Second_pass_parser* me, QAJ4C_Json_message* msg, QAJ4C_Second_pass_stack* stack) {
	(void)msg;
	(void)stack;
	size_type count = QAJ4C_second_pass_fetch_stats_data(me);
	(void)count;
}

static void QAJ4C_second_pass_object_colon(QAJ4C_Second_pass_parser* me, QAJ4C_Json_message* msg, QAJ4C_Second_pass_stack* stack) {
	(void) me;
	(void) msg;
	(void) stack;
}

static void QAJ4C_second_pass_object_close(QAJ4C_Second_pass_parser* me, QAJ4C_Json_message* msg, QAJ4C_Second_pass_stack* stack) {
	(void) me;
	(void) msg;
	(void) stack;
}

static void QAJ4C_second_pass_array_open(QAJ4C_Second_pass_parser* me, QAJ4C_Json_message* msg, QAJ4C_Second_pass_stack* stack) {
	(void) me;
	(void) msg;
	(void) stack;
}

static void QAJ4C_second_pass_array_close(QAJ4C_Second_pass_parser* me, QAJ4C_Json_message* msg, QAJ4C_Second_pass_stack* stack) {
	(void) me;
	(void) msg;
	(void) stack;
}

static void QAJ4C_second_pass_comma(QAJ4C_Second_pass_parser* me, QAJ4C_Json_message* msg, QAJ4C_Second_pass_stack* stack) {
	(void) me;
	(void) msg;
	(void) stack;
}

static void QAJ4C_second_pass_string_start(QAJ4C_Second_pass_parser* me, QAJ4C_Json_message* msg, QAJ4C_Second_pass_stack* stack) {
	(void) me;
	(void) msg;
	(void) stack;
}

static void QAJ4C_second_pass_literal_start(QAJ4C_Second_pass_parser* me, QAJ4C_Json_message* msg, QAJ4C_Second_pass_stack* stack) {
	(void) me;
	(void) msg;
	(void) stack;
}

static void QAJ4C_second_pass_comment_start(QAJ4C_Second_pass_parser* me, QAJ4C_Json_message* msg, QAJ4C_Second_pass_stack* stack) {
	(void) me;
	(void) msg;
	(void) stack;
}

static size_type QAJ4C_second_pass_fetch_stats_data( QAJ4C_Second_pass_parser* me ) {
    if (QAJ4C_UNLIKELY(me->curr_buffer_pos <= me->builder->cur_obj_pos - sizeof(size_type))) {
        /*
         * Corner case that the last entry is empty. In some cases this causes that
         * the statistics are already overwritten (see Corner-case tests).
         * In these cases the last item was an empty array or object so returning 0 is the
         * correct value.
         */
        return 0;
    }
    size_type data = *((size_type*)(me->builder->buffer + me->curr_buffer_pos));
    me->curr_buffer_pos += sizeof(size_type);
    return data;
}

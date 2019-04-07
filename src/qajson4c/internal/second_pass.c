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
	QAJ4C_Value* base_ptr;
	QAJ4C_Value* value_ptr;
	bool value_flag;
} QAJ4C_Second_pass_stack_entry;

typedef struct QAJ4C_Second_pass_stack {
	QAJ4C_Second_pass_stack_entry info[32];
	QAJ4C_Second_pass_stack_entry* it;
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

static void QAJ4C_second_pass_stack_up( QAJ4C_Second_pass_parser* me, QAJ4C_Second_pass_stack* stack, QAJ4C_Json_message* msg );
static void QAJ4C_second_pass_stack_down( QAJ4C_Second_pass_parser* me, QAJ4C_Second_pass_stack* stack, QAJ4C_Json_message* msg );
static void QAJ4C_second_pass_number( QAJ4C_Second_pass_parser* me, QAJ4C_Second_pass_stack* stack, QAJ4C_Json_message* msg );
static void QAJ4C_second_pass_object_colon( QAJ4C_Second_pass_parser* me, QAJ4C_Second_pass_stack* stack, QAJ4C_Json_message* msg );
static void QAJ4C_second_pass_comma( QAJ4C_Second_pass_parser* me, QAJ4C_Second_pass_stack* stack, QAJ4C_Json_message* msg );
static void QAJ4C_second_pass_string_start( QAJ4C_Second_pass_parser* me, QAJ4C_Second_pass_stack* stack, QAJ4C_Json_message* msg );
static void QAJ4C_second_pass_literal_start( QAJ4C_Second_pass_parser* me, QAJ4C_Second_pass_stack* stack, QAJ4C_Json_message* msg );
static void QAJ4C_second_pass_comment_start( QAJ4C_Second_pass_parser* me, QAJ4C_Second_pass_stack* stack, QAJ4C_Json_message* msg );
static size_type QAJ4C_second_pass_fetch_stats_data( QAJ4C_Second_pass_parser* me );
static void QAJ4C_second_pass_parser_set_error( QAJ4C_Second_pass_parser* parser, QAJ4C_Json_message* msg, QAJ4C_ERROR_CODE error );

QAJ4C_Value* QAJ4C_second_pass_process( QAJ4C_Second_pass_parser* me, QAJ4C_Json_message* msg ) {
	QAJ4C_Value* result = QAJ4C_builder_get_document(me->builder);

	QAJ4C_Second_pass_stack stack;
	stack.it = stack.info;
	stack.it->type = QAJ4C_TYPE_ARRAY;
	stack.it->base_ptr = result;
	stack.it->value_ptr = result;
	stack.it->value_flag = false;
	msg->pos = msg->begin;
	QAJ4C_json_message_skip_whitespaces(msg);

	while ( msg->pos < msg->end )
	{
		QAJ4C_Char_type type = QAJ4C_parse_char(*msg->pos);
		switch (type) {
		case QAJ4C_CHAR_NUMERIC_START:
			QAJ4C_second_pass_number(me, &stack, msg);
			break;
		case QAJ4C_CHAR_OBJECT_START:
		case QAJ4C_CHAR_ARRAY_START:
			QAJ4C_second_pass_stack_up(me, &stack, msg);
			break;
		case QAJ4C_CHAR_ARRAY_END:
		case QAJ4C_CHAR_OBJECT_END:
			QAJ4C_second_pass_stack_down(me, &stack, msg);
			break;
		case QAJ4C_CHAR_COLON:
			QAJ4C_second_pass_object_colon(me, &stack, msg);
			break;
		case QAJ4C_CHAR_COMMA:
			QAJ4C_second_pass_comma(me, &stack, msg);
			break;
		case QAJ4C_CHAR_COMMENT_START:
			QAJ4C_second_pass_comment_start(me, &stack, msg);
			break;
		case QAJ4C_CHAR_STRING_START:
			QAJ4C_second_pass_string_start(me, &stack, msg);
			break;
		case QAJ4C_CHAR_LITERAL_START:
			QAJ4C_second_pass_literal_start(me, &stack, msg);
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

static void QAJ4C_second_pass_number(QAJ4C_Second_pass_parser* me, QAJ4C_Second_pass_stack* stack, QAJ4C_Json_message* msg) {
	(void)me;

	char* c = (char*)msg->pos;
	QAJ4C_Second_pass_stack_entry* stack_entry = stack->it;

	if ( stack_entry->value_flag ) {
		QAJ4C_second_pass_parser_set_error(me, msg, QAJ4C_ERROR_UNEXPECTED_CHAR);
	} else {
		QAJ4C_Char_type char_type = QAJ4C_CHAR_UNSUPPORTED;
		if ( *msg->pos == '-' ) {
			int64_t i = QAJ4C_STRTOL(msg->pos, &c, 10);
			QAJ4C_set_int64(stack_entry->value_ptr, i);
		} else {
			uint64_t i = QAJ4C_STRTOL(msg->pos, &c, 10);
			QAJ4C_set_uint64(stack_entry->value_ptr, i);
		}
		char_type = QAJ4C_parse_char(*c);
		if (char_type == QAJ4C_CHAR_UNSUPPORTED) {
			double d = QAJ4C_STRTOD(msg->pos, &c );
			QAJ4C_set_double(stack_entry->value_ptr, d);
		}
		stack_entry->value_flag = true;
		stack_entry->value_ptr += 1;
	}
}

static void QAJ4C_second_pass_stack_up(QAJ4C_Second_pass_parser* me, QAJ4C_Second_pass_stack* stack, QAJ4C_Json_message* msg) {
	(void)stack;
	(void)msg;
	size_type count = QAJ4C_second_pass_fetch_stats_data(me);
	(void)count;
}

static void QAJ4C_second_pass_stack_down(QAJ4C_Second_pass_parser* me, QAJ4C_Second_pass_stack* stack, QAJ4C_Json_message* msg) {
	(void)me;
	(void)stack;
	(void)msg;

	/* FIXME: Check for trailing comma */
}

static void QAJ4C_second_pass_object_colon(QAJ4C_Second_pass_parser* me, QAJ4C_Second_pass_stack* stack, QAJ4C_Json_message* msg) {
	size_type diff = stack->it->value_ptr - stack->it->base_ptr;
	bool is_index_odd = (diff & 1) == 1;

	if (stack->it->type == QAJ4C_TYPE_OBJECT && is_index_odd) {
		stack->it->value_flag = false;
		msg->pos += 1;
	} else {
		QAJ4C_second_pass_parser_set_error(me, msg, QAJ4C_ERROR_UNEXPECTED_CHAR);
	}
}

static void QAJ4C_second_pass_comma(QAJ4C_Second_pass_parser* me, QAJ4C_Second_pass_stack* stack, QAJ4C_Json_message* msg) {
	size_type diff = stack->it->value_ptr - stack->it->base_ptr;
	bool is_index_even = (diff & 1) == 0;

	if (stack->it->type == QAJ4C_TYPE_ARRAY || is_index_even) {
		stack->it->value_flag = false;
		msg->pos += 1;
	} else {
		QAJ4C_second_pass_parser_set_error(me, msg, QAJ4C_ERROR_UNEXPECTED_CHAR);
	}
}

static void QAJ4C_second_pass_string_start(QAJ4C_Second_pass_parser* me, QAJ4C_Second_pass_stack* stack, QAJ4C_Json_message* msg) {
	(void) me;
	(void) msg;
	(void) stack;
}

static void QAJ4C_second_pass_literal_start(QAJ4C_Second_pass_parser* me, QAJ4C_Second_pass_stack* stack, QAJ4C_Json_message* msg) {
	(void) me;
	(void) msg;
	(void) stack;
}

static void QAJ4C_second_pass_comment_start(QAJ4C_Second_pass_parser* me, QAJ4C_Second_pass_stack* stack, QAJ4C_Json_message* msg) {
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

static void QAJ4C_second_pass_parser_set_error( QAJ4C_Second_pass_parser* parser, QAJ4C_Json_message* msg, QAJ4C_ERROR_CODE error )
{
	(void)parser;
	(void)msg;
	(void)error;
}


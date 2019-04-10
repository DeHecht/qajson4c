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

typedef struct QAJ4C_First_pass_stack_entry {
    size_type size;
    size_type storage_pos;
    QAJ4C_TYPE type;
} QAJ4C_First_pass_stack_entry;

typedef struct QAJ4C_First_pass_stack {
    QAJ4C_First_pass_stack_entry info[32];
    QAJ4C_First_pass_stack_entry* it;
} QAJ4C_First_pass_stack;

static void QAJ4C_first_pass_stack_up( QAJ4C_First_pass_parser* me, QAJ4C_First_pass_stack* stack, QAJ4C_Json_message* msg );
static void QAJ4C_first_pass_stack_down( QAJ4C_First_pass_parser* me, QAJ4C_First_pass_stack* stack, QAJ4C_Json_message* msg );
static void QAJ4C_first_pass_string_start( QAJ4C_First_pass_parser* me, QAJ4C_First_pass_stack* stack, QAJ4C_Json_message* msg );
static void QAJ4C_first_pass_number_start( QAJ4C_First_pass_parser* me, QAJ4C_First_pass_stack* stack, QAJ4C_Json_message* msg );
static void QAJ4C_first_pass_literal_start( QAJ4C_First_pass_parser* me, QAJ4C_First_pass_stack* stack, QAJ4C_Json_message* msg );
static size_t QAJ4C_first_pass_string_escape( QAJ4C_First_pass_parser* me, QAJ4C_Json_message* msg );
static void QAJ4C_first_pass_comment( QAJ4C_First_pass_parser* me, QAJ4C_Json_message* msg );
static void QAJ4C_first_pass_end_of_message( QAJ4C_First_pass_parser* me, QAJ4C_First_pass_stack* stack, QAJ4C_Json_message* msg );
static void QAJ4C_first_pass_set_error( QAJ4C_First_pass_parser* me, QAJ4C_Json_message* msg, QAJ4C_ERROR_CODE error );
static size_type* QAJ4C_first_pass_fetch_stats_buffer( QAJ4C_First_pass_parser* me, QAJ4C_Json_message* msg, size_type storage_pos );
static uint32_t QAJ4C_first_pass_4digits( QAJ4C_First_pass_parser* me, QAJ4C_Json_message* msg );

static int QAJ4C_xdigit( char c ) {
    return (c > '9') ? (c & ~0x20) - 'A' + 10 : (c - '0');
}

QAJ4C_First_pass_parser QAJ4C_first_pass_parser_create( QAJ4C_Builder* builder, int opts, QAJ4C_realloc_fn realloc_callback ) {
    QAJ4C_First_pass_parser parser;
    parser.builder = builder;
    parser.realloc_callback = realloc_callback;

    parser.strict_parsing = (opts & QAJ4C_PARSE_OPTS_STRICT) != 0;
    parser.optimize_object = (opts & QAJ4C_PARSE_OPTS_DONT_SORT_OBJECT_MEMBERS) == 0;
    parser.insitu_parsing = (opts & 1) != 0;

    parser.amount_nodes = 0;
    parser.complete_string_length = 0;
    parser.storage_counter = 0;
    parser.err_code = QAJ4C_ERROR_NO_ERROR;

    return parser;
}

void QAJ4C_first_pass_parse( QAJ4C_First_pass_parser* me, QAJ4C_Json_message* msg ) {
    QAJ4C_First_pass_stack stack;
    stack.it = stack.info;
    me->amount_nodes = 0;

    QAJ4C_json_message_skip_whitespaces(msg);
    if (msg->pos < msg->end) {
        do {
            switch (QAJ4C_parse_char(*msg->pos)) {
            case QAJ4C_CHAR_NULL:
                QAJ4C_first_pass_end_of_message(me, &stack, msg);
                break;
            case QAJ4C_CHAR_NUMERIC_START:
                QAJ4C_first_pass_number_start(me, &stack, msg);
                break;
            case QAJ4C_CHAR_LITERAL_START:
                QAJ4C_first_pass_literal_start(me, &stack, msg);
                break;
            case QAJ4C_CHAR_ARRAY_START:
            case QAJ4C_CHAR_OBJECT_START:
                QAJ4C_first_pass_stack_up(me, &stack, msg);
                break;
            case QAJ4C_CHAR_OBJECT_END:
            case QAJ4C_CHAR_ARRAY_END:
                QAJ4C_first_pass_stack_down(me, &stack, msg);
                break;
            case QAJ4C_CHAR_COMMENT_START:
                QAJ4C_first_pass_comment(me, msg);
                break;
            case QAJ4C_CHAR_STRING_START:
                QAJ4C_first_pass_string_start(me, &stack, msg);
                break;
            case QAJ4C_CHAR_COLON:
            case QAJ4C_CHAR_COMMA:
                /* in first pass this does not have to be checked */
                msg->pos += 1;
                break;
            default:
                QAJ4C_first_pass_set_error(me, msg, QAJ4C_ERROR_UNEXPECTED_CHAR);
                break;
            }
            QAJ4C_json_message_skip_whitespaces(msg);
        } while (msg->pos < msg->end && stack.it > stack.info);
    }

    if (stack.it != stack.info) {
        QAJ4C_first_pass_set_error(me, msg, QAJ4C_ERROR_JSON_MESSAGE_TRUNCATED);
    } else if (me->amount_nodes == 0) {
        QAJ4C_first_pass_set_error(me, msg, QAJ4C_ERROR_FATAL_PARSER_ERROR);
    }
}

size_t QAJ4C_calculate_max_buffer_parser( QAJ4C_First_pass_parser* me ) {
    if (QAJ4C_UNLIKELY(me->err_code != QAJ4C_ERROR_NO_ERROR)) {
        return sizeof(QAJ4C_Value) + sizeof(QAJ4C_Error_information);
    }
    return me->amount_nodes * sizeof(QAJ4C_Value) + me->complete_string_length;
}

void QAJ4C_first_pass_parser_set_error( QAJ4C_First_pass_parser* me, QAJ4C_Json_message* msg, QAJ4C_ERROR_CODE code ) {
    QAJ4C_first_pass_set_error(me, msg, code);
}

static void QAJ4C_first_pass_stack_up( QAJ4C_First_pass_parser* me, QAJ4C_First_pass_stack* stack, QAJ4C_Json_message* msg ) {
    stack->it->size += 1;

    if (stack->it + 1 >= stack->info + QAJ4C_ARRAY_COUNT(stack->info)) {
        QAJ4C_first_pass_set_error(me, msg, QAJ4C_ERROR_DEPTH_OVERFLOW);
    } else {
        stack->it += 1;
        QAJ4C_First_pass_stack_entry* entry_ptr = stack->it;
        entry_ptr->storage_pos = me->storage_counter;
        entry_ptr->size = 0;
        entry_ptr->type = *msg->pos == '{' ? QAJ4C_TYPE_OBJECT : QAJ4C_TYPE_ARRAY;
        msg->pos += 1;
        me->amount_nodes += 1;
        me->storage_counter += 1;
    }
}

static void QAJ4C_first_pass_string_start( QAJ4C_First_pass_parser* me, QAJ4C_First_pass_stack* stack, QAJ4C_Json_message* msg ) {
    stack->it->size += 1;
    me->amount_nodes += 1;

    msg->pos += 1;
    size_t size = 0;
    while (msg->pos < msg->end && *msg->pos != '\0' && *msg->pos != '\"') {
        if (*msg->pos == '\\') {
            size += QAJ4C_first_pass_string_escape(me, msg);
        } else {
            size += 1;
        }
        msg->pos += 1;
    }
    if (!me->insitu_parsing && size >= QAJ4C_INLINE_STRING_SIZE) {
        me->complete_string_length += size + 1;
    }
    if (*msg->pos == '\"') {
        msg->pos += 1;
    }
}

static void QAJ4C_first_pass_number_start( QAJ4C_First_pass_parser* me, QAJ4C_First_pass_stack* stack, QAJ4C_Json_message* msg ) {
    stack->it->size += 1;
    me->amount_nodes += 1;

    /* Fast forward to the end of the number */
    msg->pos += 1;
    while (msg->pos < msg->end) {
        QAJ4C_Char_type c = QAJ4C_parse_char(*msg->pos);
        if (c != QAJ4C_CHAR_NUMERIC_START && c != QAJ4C_CHAR_UNSUPPORTED) {
            /* fast forward check, let the second pass look at the number more throughly */
            break;
        }
        msg->pos += 1;
    }
}

static void QAJ4C_first_pass_literal_start( QAJ4C_First_pass_parser* me, QAJ4C_First_pass_stack* stack, QAJ4C_Json_message* msg ) {
    stack->it->size += 1;
    me->amount_nodes += 1;

    msg->pos += 1;
    while (msg->pos < msg->end && QAJ4C_parse_char(*msg->pos) == QAJ4C_CHAR_UNSUPPORTED) {
        msg->pos += 1;
    }
}

static size_t QAJ4C_first_pass_string_escape( QAJ4C_First_pass_parser* me, QAJ4C_Json_message* msg ) {
    int string_length = 1;
    msg->pos += 1;
    if (*msg->pos == 'u') {
        msg->pos += 1;

        uint32_t value = QAJ4C_first_pass_4digits(me, msg);
        if ( me->err_code == QAJ4C_ERROR_NO_ERROR )
        {
            if (value < 0x80) { /* [0, 0x80) */
                string_length = 1;
            } else if (value < 0x800) { /* [0x80, 0x800) */
                string_length = 2;
            } else if (value < 0xd800 || value > 0xdfff) { /* [0x800, 0xd800) or (0xdfff, 0xffff] */
                string_length = 3;
            } else { /* [0xd800,0xdbff] or invalid => can be checked in 2nd pass*/
                msg->pos += 1;
                if (*msg->pos != '\\' || *(msg->pos + 1) != 'u') {
                    QAJ4C_first_pass_set_error(me, msg, QAJ4C_ERROR_INVALID_UNICODE_SEQUENCE);
                }
                msg->pos += 2;
                QAJ4C_first_pass_4digits(me, msg); // expect 4 digits!
                string_length = 4;
            }
        }
    }

    return string_length;
}

static void QAJ4C_first_pass_comment( QAJ4C_First_pass_parser* me, QAJ4C_Json_message* msg ) {
    msg->pos += 1;
    // FIXME: Check strict parsing!
    if (*msg->pos == '/') {
        /* line comment */
        while (msg->pos < msg->end) {
            char c = *msg->pos;
            if (c == '\0' || c == '\n') {
                break;
            }
            msg->pos += 1;
        }
    } else if (*msg->pos == '*' && msg->pos + 2 < msg->end && *(msg->pos + 1) != '\0' && *(msg->pos + 2) != '\0') {
        /* block comment */
        msg->pos += 2;

        while (msg->pos < msg->end) {
            char c = *msg->pos;
            if (c == '\0' || (c == '/' && *(msg->pos - 1) == '*')) {
                break;
            }
            msg->pos += 1;
        }
    } else {
        msg->pos -= 1;
        QAJ4C_first_pass_set_error(me, msg, QAJ4C_ERROR_UNEXPECTED_CHAR);
    }
}

static void QAJ4C_first_pass_stack_down( QAJ4C_First_pass_parser* me, QAJ4C_First_pass_stack* stack, QAJ4C_Json_message* msg ) {
    QAJ4C_First_pass_stack_entry* entry = stack->it;

    /* Check if the character matches the stack type and in case of an object if the amount of values is even! */
    if (entry > stack->info && ((*msg->pos == '}' && entry->type == QAJ4C_TYPE_OBJECT && (entry->size & 1) == 0) || (*msg->pos == ']' && entry->type == QAJ4C_TYPE_ARRAY))) {
        if (me->builder != NULL) {
            size_type* stats_ptr = QAJ4C_first_pass_fetch_stats_buffer(me, msg, entry->storage_pos);
            if (stats_ptr != NULL) {
                *stats_ptr = entry->size;
            }
        }
        stack->it -= 1;
        msg->pos += 1;
    } else {
        QAJ4C_first_pass_set_error(me, msg, QAJ4C_ERROR_UNEXPECTED_CHAR);
    }
}

static void QAJ4C_first_pass_end_of_message( QAJ4C_First_pass_parser* me, QAJ4C_First_pass_stack* stack, QAJ4C_Json_message* msg ) {
    if (stack->it == stack->info) {
        msg->end = msg->pos;
    } else {
        QAJ4C_first_pass_set_error(me, msg, QAJ4C_ERROR_JSON_MESSAGE_TRUNCATED);
    }
}

static void QAJ4C_first_pass_set_error( QAJ4C_First_pass_parser* me, QAJ4C_Json_message* msg, QAJ4C_ERROR_CODE error ) {
    if (me->err_code == QAJ4C_ERROR_NO_ERROR) {
        me->err_code = error;
        msg->end = msg->pos;
    }
}

static size_type* QAJ4C_first_pass_fetch_stats_buffer( QAJ4C_First_pass_parser* me, QAJ4C_Json_message* msg, size_type storage_pos ) {
    QAJ4C_Builder* builder = me->builder;
    size_t in_buffer_pos = storage_pos * sizeof(size_type);
    if (in_buffer_pos + sizeof(size_type) > builder->buffer_size) {
        void *tmp;
        size_t required_size = QAJ4C_calculate_max_buffer_parser(me);
        if (me->realloc_callback == NULL) {
            QAJ4C_first_pass_set_error(me, msg, QAJ4C_ERROR_STORAGE_BUFFER_TO_SMALL);
            return NULL;
        }

        tmp = me->realloc_callback(builder->buffer, required_size);
        if (tmp == NULL) {
            QAJ4C_first_pass_set_error(me, msg, QAJ4C_ERROR_ALLOCATION_ERROR);
            return NULL;
        }
        builder->buffer = tmp;
        builder->buffer_size = required_size;
    }
    return (size_type*)&builder->buffer[in_buffer_pos];
}

static uint32_t QAJ4C_first_pass_4digits( QAJ4C_First_pass_parser* me, QAJ4C_Json_message* msg ) {
    uint32_t value = 0;

    if (msg->pos + 3 < msg->end) {
        size_type i = 0;
        for (i = 0; i < 4; ++i) {
            uint8_t xdigit = QAJ4C_xdigit(msg->pos[i]);
            if (xdigit > 0xF) {
                QAJ4C_first_pass_set_error(me, msg, QAJ4C_ERROR_INVALID_UNICODE_SEQUENCE);
            }
            value = value << 4 | xdigit;
        }
        msg->pos += 3; // point to the last digit character
    } else {
        QAJ4C_first_pass_set_error(me, msg, QAJ4C_ERROR_JSON_MESSAGE_TRUNCATED);
    }

    return value;
}

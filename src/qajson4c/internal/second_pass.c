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

#include "second_pass.h"
#include "parse.h"
#include "../qajson4c_builder.h"

typedef struct QAJ4C_Second_pass_stack_entry {
    QAJ4C_TYPE type;
    QAJ4C_Value* base_ptr;
    QAJ4C_Value* value_ptr;
    bool value_flag;
} QAJ4C_Second_pass_stack_entry;

typedef struct QAJ4C_Second_pass_stack {
    QAJ4C_Second_pass_stack_entry info[QAJ4C_STACK_SIZE];
    QAJ4C_Second_pass_stack_entry* it;
} QAJ4C_Second_pass_stack;

static void QAJ4C_second_pass_stack_up( QAJ4C_Second_pass_parser* me, QAJ4C_Second_pass_stack* stack, QAJ4C_Json_message* msg );
static void QAJ4C_second_pass_stack_down( QAJ4C_Second_pass_parser* me, QAJ4C_Second_pass_stack* stack, QAJ4C_Json_message* msg );
static void QAJ4C_second_pass_object_colon( QAJ4C_Second_pass_parser* me, QAJ4C_Second_pass_stack* stack, QAJ4C_Json_message* msg );
static void QAJ4C_second_pass_comma( QAJ4C_Second_pass_parser* me, QAJ4C_Second_pass_stack* stack, QAJ4C_Json_message* msg );
static void QAJ4C_second_pass_string_start( QAJ4C_Second_pass_parser* me, QAJ4C_Second_pass_stack* stack, QAJ4C_Json_message* msg );
static void QAJ4C_second_pass_number( QAJ4C_Second_pass_parser* me, QAJ4C_Second_pass_stack* stack, QAJ4C_Json_message* msg );
static void QAJ4C_second_pass_literal_start( QAJ4C_Second_pass_parser* me, QAJ4C_Second_pass_stack* stack, QAJ4C_Json_message* msg );
static void QAJ4C_second_pass_comment_start( QAJ4C_Json_message* msg );
static bool QAJ4C_second_pass_short_string( QAJ4C_Second_pass_parser* me, QAJ4C_Json_message* msg, QAJ4C_Short_string* short_string );
static size_type QAJ4C_second_pass_fetch_stats_data( QAJ4C_Second_pass_parser* me );
static void QAJ4C_second_pass_set_missing_seperator_error( QAJ4C_Second_pass_parser* me, QAJ4C_Second_pass_stack* stack, QAJ4C_Json_message* msg );
static void QAJ4C_second_pass_set_error( QAJ4C_Second_pass_parser* me, QAJ4C_Json_message* msg, QAJ4C_ERROR_CODE error );
static const QAJ4C_Value* QAJ4C_create_error_description( QAJ4C_Second_pass_parser* parser, QAJ4C_Json_message* msg, size_t* bytes_written );

static int QAJ4C_xdigit( char c ) {
    return (c > '9') ? (c & ~0x20) - 'A' + 10 : (c - '0');
}

static bool QAJ4C_is_digit( char c ) {
    return (uint8_t)(c - '0') < 10;
}

QAJ4C_Second_pass_parser QAJ4C_second_pass_parser_create( const QAJ4C_First_pass_parser* parser ) {
    QAJ4C_Second_pass_parser me;
    QAJ4C_First_pass_builder* builder = parser->builder;
    size_type required_object_storage = parser->amount_nodes * sizeof(QAJ4C_Value);
    size_type required_tempoary_storage = parser->storage_counter * sizeof(size_type);
    size_type copy_to_index = required_object_storage - required_tempoary_storage;
    uint8_t* buffer_ptr = (uint8_t*)builder->elements;

    memmove(buffer_ptr + copy_to_index, buffer_ptr, required_tempoary_storage);
    me.builder.buffer = builder->elements;
    me.builder.buffer_length = builder->alloc_length;
    me.builder.object_pos = (QAJ4C_Value*)buffer_ptr;
    me.builder.str_pos = (char*)(buffer_ptr + required_object_storage);
    me.builder.stats_pos = (size_type*)(buffer_ptr + copy_to_index);
    /** Copy the options that are processed within the second pass */
    me.insitu_parsing = parser->insitu_parsing;
    me.deny_trailing_commas = parser->deny_trailing_commas;
    me.deny_duplicate_keys = parser->deny_duplicate_keys;
    me.deny_uncompliant_numbers = parser->deny_uncompliant_numbers;

    me.curr_buffer_pos = copy_to_index;
    me.err_code = QAJ4C_ERROR_NO_ERROR;
    return me;
}

const QAJ4C_Value* QAJ4C_second_pass_process( QAJ4C_Second_pass_parser* me, QAJ4C_Json_message* msg, size_t* bytes_written ) {
    QAJ4C_Value* result = me->builder.object_pos;
    me->builder.object_pos += 1;

    QAJ4C_Second_pass_stack stack;
    stack.it = stack.info;
    stack.it->type = QAJ4C_TYPE_ARRAY;
    stack.it->base_ptr = result;
    stack.it->value_ptr = result;
    stack.it->value_flag = false;
    msg->pos = msg->begin;
    QAJ4C_json_message_skip_whitespaces(msg);

    while (msg->pos < msg->end) {
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
            QAJ4C_second_pass_comment_start(msg);
            break;
        case QAJ4C_CHAR_STRING_START:
            QAJ4C_second_pass_string_start(me, &stack, msg);
            break;
        case QAJ4C_CHAR_LITERAL_START:
            QAJ4C_second_pass_literal_start(me, &stack, msg);
            break;
        default:
            // Unreachable error!
            QAJ4C_second_pass_set_error(me, msg, QAJ4C_ERROR_FATAL_PARSER_ERROR);
            break;
        }
        QAJ4C_json_message_skip_whitespaces(msg);
    }

    if (me->err_code != QAJ4C_ERROR_NO_ERROR) {
        return QAJ4C_create_error_description(me, msg, bytes_written);
    }

    if (bytes_written != NULL) {
        // str_pos points to the char after the last written string. Thus is perfect to
        // be used to determine the overall buffer usage size.
        *bytes_written = me->builder.str_pos - (char*)me->builder.buffer;
    }

    return result;
}

static void QAJ4C_second_pass_stack_up( QAJ4C_Second_pass_parser* me, QAJ4C_Second_pass_stack* stack, QAJ4C_Json_message* msg ) {
    QAJ4C_Second_pass_stack_entry* stack_entry = stack->it;
    size_type count = QAJ4C_second_pass_fetch_stats_data(me);
    QAJ4C_Second_pass_stack_entry* new_stack_entry = ++stack->it;

    if (stack_entry->value_flag) {
        QAJ4C_second_pass_set_missing_seperator_error(me, stack, msg);
        return;
    }

    new_stack_entry->type = *msg->pos == '{' ? QAJ4C_TYPE_OBJECT : QAJ4C_TYPE_ARRAY;
    new_stack_entry->value_ptr = me->builder.object_pos;
    new_stack_entry->base_ptr = new_stack_entry->value_ptr;
    new_stack_entry->value_flag = false;
    me->builder.object_pos += count;

    if (new_stack_entry->type == QAJ4C_TYPE_OBJECT) {
        QAJ4C_Object* obj_ptr = (QAJ4C_Object*)stack_entry->value_ptr;
        stack_entry->value_ptr->type = QAJ4C_OBJECT_TYPE_CONSTANT;
        obj_ptr->count = count >> 1;
        obj_ptr->top = (QAJ4C_Member*)new_stack_entry->value_ptr;
    } else {
        QAJ4C_Array* a_ptr = (QAJ4C_Array*)stack_entry->value_ptr;
        stack_entry->value_ptr->type = QAJ4C_ARRAY_TYPE_CONSTANT;
        a_ptr->count = count;
        a_ptr->top = new_stack_entry->value_ptr;
    }
    msg->pos += 1;
    stack_entry->value_flag = true;
    stack_entry->value_ptr += 1;
}

static void QAJ4C_second_pass_stack_down( QAJ4C_Second_pass_parser* me, QAJ4C_Second_pass_stack* stack, QAJ4C_Json_message* msg ) {
    QAJ4C_Second_pass_stack_entry* stack_entry = stack->it;
    stack->it -= 1;
    msg->pos += 1;

    if (stack_entry->type == QAJ4C_TYPE_OBJECT) {
        QAJ4C_Object* object_ptr = (QAJ4C_Object*)(stack->it->value_ptr - 1);
        QAJ4C_object_optimize( object_ptr );
        if (me->deny_duplicate_keys && QAJ4C_object_has_duplicate(object_ptr)) {
            QAJ4C_second_pass_set_error(me, msg, QAJ4C_ERROR_DUPLICATE_KEY);
        }
    }

    if (me->deny_trailing_commas && !stack_entry->value_flag && stack_entry->value_ptr - stack_entry->base_ptr > 0) {
        QAJ4C_second_pass_set_error(me, msg, QAJ4C_ERROR_TRAILING_COMMA);
    }
}

static void QAJ4C_second_pass_object_colon( QAJ4C_Second_pass_parser* me, QAJ4C_Second_pass_stack* stack, QAJ4C_Json_message* msg ) {
    size_type diff = stack->it->value_ptr - stack->it->base_ptr;
    bool is_index_odd = (diff & 1) == 1;

    if (stack->it->type == QAJ4C_TYPE_OBJECT && is_index_odd && QAJ4C_is_string(stack->it->value_ptr - 1)) {
        stack->it->value_flag = false;
        msg->pos += 1;
    } else {
        QAJ4C_second_pass_set_error(me, msg, QAJ4C_ERROR_UNEXPECTED_CHAR);
    }
}

static void QAJ4C_second_pass_comma( QAJ4C_Second_pass_parser* me, QAJ4C_Second_pass_stack* stack, QAJ4C_Json_message* msg ) {
    size_type diff = stack->it->value_ptr - stack->it->base_ptr;
    bool is_index_even = diff > 0 && (diff & 1) == 0;

    if (stack->it->type == QAJ4C_TYPE_ARRAY || is_index_even) {
        stack->it->value_flag = false;
        msg->pos += 1;
    } else {
        QAJ4C_second_pass_set_error(me, msg, QAJ4C_ERROR_UNEXPECTED_CHAR);
    }
}

static char QAJ4C_second_pass_escape_char( QAJ4C_Second_pass_parser* me, QAJ4C_Json_message* msg, char c ) {
    switch (c) {
    case '"':
        return '"';
    case '\\':
        return '\\';
    case '/':
        return '/';
    case 'b':
        return '\b';
    case 'f':
        return '\f';
    case 'n':
        return '\n';
    case 'r':
        return '\r';
    case 't':
        return '\t';
    default: /* all others are failures */
        QAJ4C_second_pass_set_error(me, msg, QAJ4C_ERROR_INVALID_ESCAPE_SEQUENCE);
        return '\0';
    }
}

static uint32_t QAJ4C_second_pass_utf16( QAJ4C_Second_pass_parser* me, QAJ4C_Json_message* msg ) {
    uint32_t value = QAJ4C_xdigit(msg->pos[0]) << 12 | QAJ4C_xdigit(msg->pos[1]) << 8 | QAJ4C_xdigit(msg->pos[2]) << 4 | QAJ4C_xdigit(msg->pos[3]);
    msg->pos += 4;
    if (value >= 0xd800 && value <= 0xdbff) {
        uint32_t low_surrogate;
        msg->pos += 2;
        low_surrogate = QAJ4C_xdigit(msg->pos[0]) << 12 | QAJ4C_xdigit(msg->pos[1]) << 8 | QAJ4C_xdigit(msg->pos[2]) << 4 | QAJ4C_xdigit(msg->pos[3]);
        if (low_surrogate < 0xdc00 || low_surrogate > 0xdfff) {
            QAJ4C_second_pass_set_error(me, msg, QAJ4C_ERROR_INVALID_UNICODE_SEQUENCE);
        } else {
            value = ((((value - 0xD800) & 0x3FF) << 10) | ((low_surrogate - 0xDC00) & 0x3FF)) + 0x010000;
            msg->pos += 4;
        }
    } else if (value >= 0xdc00 && value <= 0xdfff) {
        QAJ4C_second_pass_set_error(me, msg, QAJ4C_ERROR_INVALID_UNICODE_SEQUENCE);
    }
    if ( me->err_code == QAJ4C_ERROR_NO_ERROR)
    {
        /* set pointer to last char */
        msg->pos -= 1;
    }
    return value;
}

static bool QAJ4C_second_pass_unicode_sequence( QAJ4C_Second_pass_parser* me, QAJ4C_Short_string* string, QAJ4C_Json_message* msg ) {
    static const uint8_t FIRST_BYTE_TWO_BYTE_MARK_MASK = 0xC0;
    static const uint8_t FIRST_BYTE_TWO_BYTE_PAYLOAD_MASK = 0x1F;
    static const uint8_t FIRST_BYTE_THREE_BYTE_MARK_MASK = 0xE0;
    static const uint8_t FIRST_BYTE_THREE_BYTE_PAYLOAD_MASK = 0x0F;
    static const uint8_t FIRST_BYTE_FOUR_BYTE_MARK_MASK = 0xF0;
    static const uint8_t FIRST_BYTE_FOUR_BYTE_PAYLOAD_MASK = 0x07;
    static const uint8_t FOLLOW_BYTE_MARK_MASK = 0x80;
    static const uint8_t FOLLOW_BYTE_PAYLOAD_MASK = 0x3F;

    uint32_t utf16 = QAJ4C_second_pass_utf16(me, msg); // FIXME: check low and high suggorate!!
    /* utf-8 conversion inspired by parson */
    if (utf16 < 0x80) {
        string->s[0] = (char)utf16;
        string->count = 1;
    } else if (utf16 < 0x800) {
        string->s[0] = ((utf16 >> 6) & FIRST_BYTE_TWO_BYTE_PAYLOAD_MASK) | FIRST_BYTE_TWO_BYTE_MARK_MASK;
        string->s[1] = (utf16 & 0x3f) | FOLLOW_BYTE_MARK_MASK;
        string->count = 2;
    } else if (utf16 < 0x010000) {
        string->s[0] = ((utf16 >> 12) & FIRST_BYTE_THREE_BYTE_PAYLOAD_MASK) | FIRST_BYTE_THREE_BYTE_MARK_MASK;
        string->s[1] = ((utf16 >> 6) & FOLLOW_BYTE_PAYLOAD_MASK) | FOLLOW_BYTE_MARK_MASK;
        string->s[2] = ((utf16) & FOLLOW_BYTE_PAYLOAD_MASK) | FOLLOW_BYTE_MARK_MASK;
        string->count = 3;
    } else {
        string->s[0] = (((utf16 >> 18) & FIRST_BYTE_FOUR_BYTE_PAYLOAD_MASK) | FIRST_BYTE_FOUR_BYTE_MARK_MASK);
        string->s[1] = (((utf16 >> 12) & FOLLOW_BYTE_PAYLOAD_MASK) | FOLLOW_BYTE_MARK_MASK);
        string->s[2] = (((utf16 >> 6) & FOLLOW_BYTE_PAYLOAD_MASK) | FOLLOW_BYTE_MARK_MASK);
        string->s[3] = (((utf16) & FOLLOW_BYTE_PAYLOAD_MASK) | FOLLOW_BYTE_MARK_MASK);
        string->count = 4;
    }
    return true;
}

static bool QAJ4C_second_pass_short_string( QAJ4C_Second_pass_parser* me, QAJ4C_Json_message* msg, QAJ4C_Short_string* short_string ) {
    ((QAJ4C_Value*)short_string)->type = QAJ4C_STRING_INLINE_TYPE_CONSTANT;
    short_string->count = 0;
    while (*msg->pos != '"' && me->err_code == QAJ4C_ERROR_NO_ERROR) {
        if (short_string->count >= QAJ4C_INLINE_STRING_SIZE) {
            return false;
        }
        if ( QAJ4C_UNLIKELY(*msg->pos == '\\' ) ) {
            msg->pos += 1;
            if ( QAJ4C_UNLIKELY(*msg->pos == 'u' ) ) {
                /* Handle utf-16 escape sequence */
                QAJ4C_Short_string tmp;
                tmp.count = 0;
                msg->pos += 1;
                if (!QAJ4C_second_pass_unicode_sequence(me, &tmp, msg)) {
                    QAJ4C_second_pass_set_error(me, msg, QAJ4C_ERROR_INVALID_UNICODE_SEQUENCE);
                } else if ( short_string->count + tmp.count > QAJ4C_INLINE_STRING_SIZE) {
                    return false;
                }
                QAJ4C_MEMCPY(short_string->s + short_string->count, tmp.s, tmp.count);
                short_string->count += tmp.count - 1;
            } else {
                /* Handle simple escape char */
                short_string->s[short_string->count] = QAJ4C_second_pass_escape_char( me, msg, *msg->pos );
            }
        } else if (QAJ4C_UNLIKELY((uint8_t )(*msg->pos) < 0x20)) {
            QAJ4C_second_pass_set_error(me, msg, QAJ4C_ERROR_UNEXPECTED_CHAR);
        } else {
            short_string->s[short_string->count] = *msg->pos;
        }
        short_string->count += 1;
        msg->pos += 1;
    }
    short_string->s[short_string->count] = '\0';
    return true;
}

static size_type QAJ4C_second_pass_string_append( QAJ4C_Second_pass_parser* me, QAJ4C_Json_message* msg, char* append_ptr ) {
    char* base_string_ptr = append_ptr;
    while (*msg->pos != '"'  && me->err_code == QAJ4C_ERROR_NO_ERROR) {
        if ( QAJ4C_UNLIKELY(*msg->pos == '\\' ) ) {
            msg->pos += 1;
            if ( QAJ4C_UNLIKELY(*msg->pos == 'u' ) ) {
                /* Handle utf-16 escape sequence */
                QAJ4C_Short_string tmp;
                tmp.count = 0;
                msg->pos += 1;
                if (!QAJ4C_second_pass_unicode_sequence(me, &tmp, msg)) {
                    QAJ4C_second_pass_set_error(me, msg, QAJ4C_ERROR_INVALID_UNICODE_SEQUENCE);
                }
                QAJ4C_MEMCPY(append_ptr, tmp.s, tmp.count);
                append_ptr += tmp.count - 1;
            } else {
                /* Handle simple escape char */
                *append_ptr = QAJ4C_second_pass_escape_char( me, msg, *msg->pos );
            }
        } else if (QAJ4C_UNLIKELY((uint8_t )(*msg->pos) < 0x20)) {
            QAJ4C_second_pass_set_error(me, msg, QAJ4C_ERROR_UNEXPECTED_CHAR);
        } else {
            *append_ptr = *msg->pos;
        }
        append_ptr += 1;
        msg->pos += 1;
    }
    *append_ptr = '\0'; /* Terminate the string */
    return append_ptr - base_string_ptr;
}

static void QAJ4C_second_pass_string_start( QAJ4C_Second_pass_parser* me, QAJ4C_Second_pass_stack* stack, QAJ4C_Json_message* msg ) {
    QAJ4C_Second_pass_stack_entry* stack_entry = stack->it;
    QAJ4C_Short_string* short_string_ptr = (QAJ4C_Short_string*)stack_entry->value_ptr;
    QAJ4C_String* string_ptr = (QAJ4C_String*)stack_entry->value_ptr;
    QAJ4C_Second_pass_builder* builder = &me->builder;

    if (stack_entry->value_flag) {
        QAJ4C_second_pass_set_missing_seperator_error(me, stack, msg);
        return;
    }

    msg->pos += 1;
    if ( me->insitu_parsing ) {
        stack_entry->value_ptr->type = QAJ4C_STRING_REF_TYPE_CONSTANT;
        string_ptr->s = msg->pos;
        string_ptr->count = QAJ4C_second_pass_string_append(me, msg, (char*)msg->pos);
    } else if (!QAJ4C_second_pass_short_string(me, msg, short_string_ptr)) {
        char* base_put_str = builder->str_pos;
        size_type chars = short_string_ptr->count; /* do not include the \0 char */

        QAJ4C_MEMCPY(base_put_str, short_string_ptr->s, chars * sizeof(char));
        stack_entry->value_ptr->type = QAJ4C_STRING_TYPE_CONSTANT;
        string_ptr->s = base_put_str;
        string_ptr->count = chars + QAJ4C_second_pass_string_append(me, msg, base_put_str + chars);
        builder->str_pos += string_ptr->count + 1;
    }

    stack_entry->value_flag = true;
    stack_entry->value_ptr += 1;
    msg->pos += 1;
}

static void QAJ4C_second_pass_number( QAJ4C_Second_pass_parser* me, QAJ4C_Second_pass_stack* stack, QAJ4C_Json_message* msg ) {
    char* c = (char*)msg->pos;
    QAJ4C_Second_pass_stack_entry* stack_entry = stack->it;

    if (stack_entry->value_flag) {
        QAJ4C_second_pass_set_missing_seperator_error(me, stack, msg);
        return;
    }
    if (me->deny_uncompliant_numbers && (*msg->pos == '+' || (*msg->pos == '0' && QAJ4C_is_digit(*(msg->pos + 1))))) {
        QAJ4C_second_pass_set_error(me, msg, QAJ4C_ERROR_INVALID_NUMBER_FORMAT);
        return;
    }

    QAJ4C_set_null(stack_entry->value_ptr);
    if (*msg->pos == '-') {
        int64_t i = QAJ4C_STRTOL(msg->pos, &c, 10);
        if (!((i == INT64_MAX || i == INT64_MIN) && errno == ERANGE)) {
            QAJ4C_set_int64(stack_entry->value_ptr, i);
        }
    } else {
        uint64_t i = QAJ4C_STRTOUL(msg->pos, &c, 10);
        if (!(i == UINT64_MAX && errno == ERANGE)) {
            QAJ4C_set_uint64(stack_entry->value_ptr, i);
        }
    }

    if (QAJ4C_is_null(stack_entry->value_ptr) || QAJ4C_parse_char(*c) == QAJ4C_CHAR_UNSUPPORTED) {
        double d = QAJ4C_STRTOD(msg->pos, &c);
        QAJ4C_set_double(stack_entry->value_ptr, d);

        if ( QAJ4C_parse_char(*c) == QAJ4C_CHAR_UNSUPPORTED || *(c-1) == '.') {
            QAJ4C_second_pass_set_error(me, msg, QAJ4C_ERROR_INVALID_NUMBER_FORMAT);
        }
    }
    stack_entry->value_flag = true;
    stack_entry->value_ptr += 1;
    msg->pos = c;
}

static void QAJ4C_second_pass_literal_start( QAJ4C_Second_pass_parser* me, QAJ4C_Second_pass_stack* stack, QAJ4C_Json_message* msg ) {
    static const char TRUE_CONST[] = "true";
    static const char FALSE_CONST[] = "false";
    static const char NULL_CONST[] = "null";
    QAJ4C_Second_pass_stack_entry* stack_entry = stack->it;

    if (stack_entry->value_flag) {
        QAJ4C_second_pass_set_missing_seperator_error(me, stack, msg);
        return;
    }
    const char* str = NULL;
    int len = 0;
    if (*msg->pos == 't') {
        str = TRUE_CONST;
        len = QAJ4C_ARRAY_COUNT(TRUE_CONST) - 1;
    } else if (*msg->pos == 'f') {
        str = FALSE_CONST;
        len = QAJ4C_ARRAY_COUNT(FALSE_CONST) - 1;
    } else {
        str = NULL_CONST;
        len = QAJ4C_ARRAY_COUNT(NULL_CONST) - 1;
    }

    if (msg->end - msg->pos >= len && QAJ4C_MEMCMP(str, msg->pos, len) == 0) {
        if (str == NULL_CONST) {
            QAJ4C_set_null(stack_entry->value_ptr);
        } else {
            QAJ4C_set_bool(stack_entry->value_ptr, str == TRUE_CONST);
        }
        stack_entry->value_flag = true;
        stack_entry->value_ptr += 1;
        msg->pos += len;
    } else {
        QAJ4C_second_pass_set_error(me, msg, QAJ4C_ERROR_UNEXPECTED_CHAR);
    }
}

static void QAJ4C_second_pass_comment_start( QAJ4C_Json_message* msg ) {
    msg->pos += 1;
    // First pass has already validated that comments end correctly!
    if (*msg->pos == '/') {
        /* ptrline comment */
        while (*msg->pos != '\n') {
            msg->pos += 1;
        }
        msg->pos += 1;
    } else {
        /* block comment */
        msg->pos += 1;
        while (*msg->pos != '*' || *(msg->pos + 1) != '/' ) {
            msg->pos += 1;
        }
        msg->pos += 2;
    }
}

static size_type QAJ4C_second_pass_fetch_stats_data( QAJ4C_Second_pass_parser* me ) {
    if (QAJ4C_UNLIKELY((void*)me->builder.stats_pos <= (void*)me->builder.object_pos)) {
        /*
         * Corner case that the last entry is empty. In some cases this causes that
         * the statistics are already overwritten (see Corner-case tests).
         * In these cases the last item was an empty array or object so returning 0 is the
         * correct value.
         */
        return 0;
    }
    size_type data = *me->builder.stats_pos;
    me->builder.stats_pos += 1;
    return data;
}

static void QAJ4C_second_pass_set_missing_seperator_error( QAJ4C_Second_pass_parser* me, QAJ4C_Second_pass_stack* stack, QAJ4C_Json_message* msg ) {
    QAJ4C_Second_pass_stack_entry* stack_entry = stack->it;
    if ( stack_entry->type == QAJ4C_TYPE_OBJECT && (((QAJ4C_Object*)stack_entry->value_ptr)->count & 0x1) == 0) {
        QAJ4C_second_pass_set_error(me, msg, QAJ4C_ERROR_MISSING_COLON);
    } else {
        QAJ4C_second_pass_set_error(me, msg, QAJ4C_ERROR_MISSING_COMMA);
    }
}

static void QAJ4C_second_pass_set_error( QAJ4C_Second_pass_parser* me, QAJ4C_Json_message* msg, QAJ4C_ERROR_CODE error ) {
    if (me->err_code == QAJ4C_ERROR_NO_ERROR) {
        me->err_code = error;
        msg->end = msg->pos;
    }
}

static const QAJ4C_Value* QAJ4C_create_error_description( QAJ4C_Second_pass_parser* parser, QAJ4C_Json_message* msg, size_t* bytes_written ) {
    static const size_t REQUIRED_STORAGE = sizeof(QAJ4C_Value) + sizeof(QAJ4C_Error_information);
    QAJ4C_Value* document;
    QAJ4C_Error_information* err_info;
    /* not enough space to store error information in buffer... */
    if (parser->builder.buffer_length < REQUIRED_STORAGE) {
        return NULL;
    }

    //QAJ4C_builder_init(parser->builder, parser->builder->buffer, parser->builder->buffer_size);
    document = (QAJ4C_Value*)parser->builder.buffer;
    document->type = QAJ4C_ERROR_DESCRIPTION_TYPE_CONSTANT;

    err_info = (QAJ4C_Error_information*)(document + 1);
    err_info->err_no = parser->err_code;
    err_info->json = msg->begin;
    err_info->json_pos = msg->pos - msg->begin;
    ((QAJ4C_Error*)document)->info = err_info;

    if (bytes_written != NULL) {
       *bytes_written = REQUIRED_STORAGE;
    }
    return document;
}


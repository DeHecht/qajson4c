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

#include "types.h"

#include "qajson_stdwrap.h"
#include "first_pass.h"
#include "second_pass.h"

typedef struct QAJ4C_Buffer_printer {
    char* buffer;
    size_type index;
    size_type len;
} QAJ4C_Buffer_printer;

typedef struct QAJ4C_callback_printer {
    void* external_ptr;
    QAJ4C_print_callback_fn callback;
} QAJ4C_callback_printer;

void QAJ4C_std_err_function( void ) {
    QAJ4C_RAISE(SIGABRT);
}

static bool QAJ4C_builder_validate_buffer( QAJ4C_Builder* builder ) {
    return builder->cur_obj_pos - 1 <= builder->cur_str_pos;
}

static QAJ4C_Value* QAJ4C_create_error_description( QAJ4C_First_pass_parser* parser, QAJ4C_Json_message* msg );


QAJ4C_fatal_error_fn g_qaj4c_err_function = &QAJ4C_std_err_function;

size_t QAJ4C_parse_generic( QAJ4C_Builder* builder, const char* json, const char* json_end, int opts, const QAJ4C_Value** result_ptr, QAJ4C_realloc_fn realloc_callback ) {
    QAJ4C_First_pass_parser parser = QAJ4C_first_pass_parser_create(builder, opts, realloc_callback);
    QAJ4C_Second_pass_parser second_parser;
    QAJ4C_Json_message msg;
    size_type required_size;
    msg.begin = json;
    msg.end = json_end;
    msg.pos = json;

    QAJ4C_first_pass_parse(&parser, &msg);

    if (parser.strict_parsing && *msg.pos != '\0') {
        /* skip whitespaces and comments after the json (we are graceful) */
        while (msg.pos < msg.end && QAJ4C_parse_char(*msg.pos) == QAJ4C_CHAR_WHITESPACE) {
            msg.pos += 1;
        }
        if ( msg.pos < msg.end && *msg.pos != '\0') {
            QAJ4C_first_pass_parser_set_error(&parser, &msg, QAJ4C_ERROR_UNEXPECTED_JSON_APPENDIX);
        }
    }
    msg.end = msg.pos;

    if (parser.err_code != QAJ4C_ERROR_NO_ERROR) {
        *result_ptr = QAJ4C_create_error_description(&parser, &msg);
        return builder->cur_obj_pos;
    }

    required_size = QAJ4C_calculate_max_buffer_parser(&parser);
    if (required_size > builder->buffer_size) {
        if (parser.realloc_callback != NULL) {
            void* tmp = parser.realloc_callback(builder->buffer, required_size);
            if (tmp == NULL) {
                QAJ4C_first_pass_parser_set_error(&parser, &msg, QAJ4C_ERROR_ALLOCATION_ERROR);
            } else {
                QAJ4C_builder_init(builder, tmp, required_size);
            }
        } else {
            QAJ4C_first_pass_parser_set_error(&parser, &msg, QAJ4C_ERROR_STORAGE_BUFFER_TO_SMALL);
        }
    }
    else
    {
        QAJ4C_builder_init(builder, builder->buffer, required_size);
    }

    if ( parser.err_code != QAJ4C_ERROR_NO_ERROR ) {
        *result_ptr = QAJ4C_create_error_description(&parser, &msg);
        return builder->cur_obj_pos;
    }

    second_parser = QAJ4C_second_pass_parser_create(&parser);
    *result_ptr = QAJ4C_second_pass_process(&second_parser, &msg);

return builder->buffer_size;
}

size_t QAJ4C_calculate_max_buffer_generic( const char* json, size_t json_len, int opts ) {
    QAJ4C_First_pass_parser parser = QAJ4C_first_pass_parser_create(NULL, opts, NULL);
    QAJ4C_Json_message msg;
    msg.begin = json;
    msg.end = json + json_len;
    msg.pos = json;

    QAJ4C_first_pass_parse(&parser, &msg);

    return QAJ4C_calculate_max_buffer_parser(&parser);
}


QAJ4C_Value* QAJ4C_builder_pop_values( QAJ4C_Builder* builder, size_type count ) {
    QAJ4C_Value* new_pointer;
    size_type i;
    if (count == 0) {
        return NULL;
    }
    new_pointer = (QAJ4C_Value*)(&builder->buffer[builder->cur_obj_pos]);
    builder->cur_obj_pos += count * sizeof(QAJ4C_Value);

    QAJ4C_ASSERT(QAJ4C_builder_validate_buffer(builder), {return NULL;});

    for (i = 0; i < count; i++) {
        (new_pointer + i)->type = QAJ4C_NULL_TYPE_CONSTANT;
    }
    return new_pointer;
}

char* QAJ4C_builder_pop_string( QAJ4C_Builder* builder, size_type length ) {
    QAJ4C_ASSERT(builder->cur_str_pos >= length * sizeof(char), {return NULL;});
    builder->cur_str_pos -= length * sizeof(char);
    QAJ4C_ASSERT(QAJ4C_builder_validate_buffer(builder), {return NULL;});
    return (char*)(&builder->buffer[builder->cur_str_pos]);
}

QAJ4C_Member* QAJ4C_builder_pop_members( QAJ4C_Builder* builder, size_type count ) {
    QAJ4C_Member* new_pointer;
    size_type i;
    if (count == 0) {
        return NULL;
    }
    new_pointer = (QAJ4C_Member*)(&builder->buffer[builder->cur_obj_pos]);
    builder->cur_obj_pos += count * sizeof(QAJ4C_Member);
    QAJ4C_ASSERT(QAJ4C_builder_validate_buffer(builder), {return NULL;});

    for (i = 0; i < count; i++) {
        new_pointer[i].key.type = QAJ4C_NULL_TYPE_CONSTANT;
        new_pointer[i].value.type = QAJ4C_NULL_TYPE_CONSTANT;
    }

    return new_pointer;
}

QAJ4C_INTERNAL_TYPE QAJ4C_get_internal_type( const QAJ4C_Value* value_ptr ) {
    if (value_ptr == NULL) {
        return QAJ4C_NULL;
    }
    return (value_ptr->type >> 8) & 0xFF;
}

uint8_t QAJ4C_get_compatibility_types( const QAJ4C_Value* value_ptr ) {
    return (value_ptr->type >> 16) & 0xFF;
}

uint8_t QAJ4C_get_storage_type( const QAJ4C_Value* value_ptr ) {
    return (value_ptr->type >> 24) & 0xFF;
}

const QAJ4C_Value* QAJ4C_object_get_unsorted( QAJ4C_Object* obj_ptr, QAJ4C_Value* str_ptr ) {
    QAJ4C_Member* entry;
    size_type i;
    for (i = 0; i < obj_ptr->count; ++i) {
        entry = obj_ptr->top + i;
        if (!QAJ4C_is_null(&entry->key) && QAJ4C_strcmp(str_ptr, &entry->key) == 0) {
            return &entry->value;
        }
    }
    return NULL;
}

const QAJ4C_Value* QAJ4C_object_get_sorted( QAJ4C_Object* obj_ptr, QAJ4C_Value* str_ptr ) {
    QAJ4C_Member* result;
    QAJ4C_Member member;
    member.key = *str_ptr;
    result = QAJ4C_BSEARCH(&member, obj_ptr->top, obj_ptr->count, sizeof(QAJ4C_Member), QAJ4C_compare_members);
    if (result != NULL) {
        return &result->value;
    }
    return NULL;
}


/*
 * The comparison will first check on the string size and then on the content as we
 * only require this for matching purposes. The string length is also stored within
 * the objects, so check is not expensive!
 */
int QAJ4C_strcmp( const QAJ4C_Value* lhs, const QAJ4C_Value* rhs ) {
    size_type lhs_size = QAJ4C_get_string_length(lhs);
    size_type rhs_size = QAJ4C_get_string_length(rhs);
    const char* lhs_string = QAJ4C_get_string(lhs);
    const char* rhs_string = QAJ4C_get_string(rhs);

    if (lhs_size != rhs_size) {
        return lhs_size - rhs_size;
    }
    return QAJ4C_MEMCMP(lhs_string, rhs_string, lhs_size);
}

/*
 * In some situations objects are not fully filled ... all unset members (key is null)
 * have to be located at the end of the array in order to improve the attach and compare
 * methods.
 */
int QAJ4C_compare_members( const void* lhs, const void* rhs ) {
    const QAJ4C_Member* left = lhs;
    const QAJ4C_Member* right = rhs;

    if (QAJ4C_is_string(&left->key) && QAJ4C_is_string(&right->key)) {
        return QAJ4C_strcmp(&left->key, &right->key);
    }
    if (QAJ4C_is_null(&left->key) && QAJ4C_is_null(&right->key)) {
        return 0;
    }

    /*
     * Either left or right is null (but not both). Count "null" bigger as any string so it
     * will always be moved to the end of the array!
     */
    return QAJ4C_is_null(&left->key) ? 1 : -1;
}

static QAJ4C_Value* QAJ4C_create_error_description( QAJ4C_First_pass_parser* parser, QAJ4C_Json_message* msg ) {
    QAJ4C_Value* document;
    QAJ4C_Error_information* err_info;
    /* not enough space to store error information in buffer... */
    if (parser->builder->buffer_size < sizeof(QAJ4C_Value) + sizeof(QAJ4C_Error_information)) {
        return NULL;
    }

    QAJ4C_builder_init(parser->builder, parser->builder->buffer, parser->builder->buffer_size);
    document = QAJ4C_builder_get_document(parser->builder);
    document->type = QAJ4C_ERROR_DESCRIPTION_TYPE_CONSTANT;

    err_info = (QAJ4C_Error_information*)(parser->builder->buffer + sizeof(QAJ4C_Value));
    err_info->err_no = parser->err_code;
    err_info->json = msg->begin;
    err_info->json_pos = msg->pos - msg->begin;

    parser->builder->cur_obj_pos = sizeof(QAJ4C_Value) + sizeof(QAJ4C_Error_information);
    ((QAJ4C_Error*)document)->info = err_info;

    return document;
}

QAJ4C_Char_type QAJ4C_parse_char( char c ) {
    switch (c) {
    case '\0':
        return QAJ4C_CHAR_NULL;
    case ' ':
    case '\t':
    case '\n':
    case '\r':
        return QAJ4C_CHAR_WHITESPACE;
    case 'f':
    case 't':
    case 'n':
        return QAJ4C_CHAR_LITERAL_START;
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
    case '+':
    case '-':
        return QAJ4C_CHAR_NUMERIC_START;
    case '{':
        return QAJ4C_CHAR_OBJECT_START;
    case '[':
        return QAJ4C_CHAR_ARRAY_START;
    case '"':
        return QAJ4C_CHAR_STRING_START;
    case '/':
        return QAJ4C_CHAR_COMMENT_START;
    case '}':
        return QAJ4C_CHAR_OBJECT_END;
    case ']':
        return QAJ4C_CHAR_ARRAY_END;
    case ':':
        return QAJ4C_CHAR_COLON;
    case ',':
        return QAJ4C_CHAR_COMMA;
    default:
        return QAJ4C_CHAR_UNSUPPORTED;
    }
}

void QAJ4C_json_message_skip_whitespaces( QAJ4C_Json_message* msg ) {
    while (msg->pos < msg->end) {
        if (QAJ4C_parse_char(*msg->pos) != QAJ4C_CHAR_WHITESPACE) {
            break;
        }
        msg->pos += 1;
    }
}


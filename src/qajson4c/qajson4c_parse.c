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

#include "qajson4c_parse.h"
#include "internal/first_pass.h"
#include "internal/second_pass.h"

static const QAJ4C_Value* QAJ4C_parse_generic( QAJ4C_First_pass_builder* builder, const char* json, size_t json_len, int opts, size_t* bytes_written );
static size_t QAJ4C_calculate_max_buffer_generic( const char* json, size_t json_len, int opts );
static const QAJ4C_Value* QAJ4C_create_error_description( QAJ4C_First_pass_parser* parser, QAJ4C_Json_message* msg, size_t* bytes_written );

size_t QAJ4C_calculate_max_buffer_size( const char* json, size_t json_len ) {
    return QAJ4C_calculate_max_buffer_generic(json, json_len, 0);
}

size_t QAJ4C_calculate_max_buffer_size_insitu( const char* json, size_t json_len ) {
    return QAJ4C_calculate_max_buffer_generic(json, json_len, QAJ4C_PARSE_OPTS_RESERVED_1);
}

const QAJ4C_Value* QAJ4C_parse( const char* json, size_t json_len, void* buffer, size_t buffer_size, int opts, size_t* bytes_written ) {
    QAJ4C_First_pass_builder builder = QAJ4C_First_pass_builder_create(buffer, buffer_size, NULL);
    return QAJ4C_parse_generic(&builder, json, json_len, opts & ~QAJ4C_PARSE_OPTS_RESERVED_1, bytes_written);
}

const QAJ4C_Value* QAJ4C_parse_dynamic( const char* json, size_t json_len, int opts, QAJ4C_realloc_fn realloc_callback ) {
    QAJ4C_First_pass_builder builder = QAJ4C_First_pass_builder_create(NULL, 0, realloc_callback);
    return QAJ4C_parse_generic(&builder, json, json_len, opts & ~QAJ4C_PARSE_OPTS_RESERVED_1, NULL);
}

const QAJ4C_Value* QAJ4C_parse_insitu( char* json, size_t json_len, void* buffer, size_t buffer_size, int opts, size_t* bytes_written ) {
    QAJ4C_First_pass_builder builder = QAJ4C_First_pass_builder_create(buffer, buffer_size, NULL);
    return QAJ4C_parse_generic(&builder, json, json_len, opts | QAJ4C_PARSE_OPTS_RESERVED_1, bytes_written);
}

const char* QAJ4C_error_get_json( const QAJ4C_Value* value_ptr ) {
    QAJ4C_ASSERT(QAJ4C_is_error(value_ptr), {return "";});
    return QAJ4C_ERROR_GET_PTR(value_ptr)->json;
}

QAJ4C_ERROR_CODE QAJ4C_error_get_errno( const QAJ4C_Value* value_ptr ) {
    QAJ4C_ASSERT(QAJ4C_is_error(value_ptr), {return 0;});
    return QAJ4C_ERROR_GET_PTR(value_ptr)->err_no;
}

size_t QAJ4C_error_get_json_pos( const QAJ4C_Value* value_ptr ) {
    QAJ4C_ASSERT(QAJ4C_is_error(value_ptr), {return 0;});
    return QAJ4C_ERROR_GET_PTR(value_ptr)->json_pos;
}

static const QAJ4C_Value* QAJ4C_parse_generic( QAJ4C_First_pass_builder* builder, const char* json, size_t json_len, int opts, size_t* bytes_written ) {
// static size_t QAJ4C_parse_generic( QAJ4C_First_pass_builder* builder, const char* json, size_t json_len, int opts, const QAJ4C_Value** result_ptr ) {
    QAJ4C_First_pass_parser parser = QAJ4C_first_pass_parser_create(builder, opts);
    QAJ4C_Second_pass_parser second_parser;
    const char* json_end = (json_len != SIZE_MAX) ? (json + json_len) : (const char*)INTPTR_MAX;
    QAJ4C_Json_message msg = {.begin = json, .end = json_end, .pos = json};

    QAJ4C_first_pass_parse(&parser, &msg);
    if (parser.err_code != QAJ4C_ERROR_NO_ERROR) {
        return QAJ4C_create_error_description(&parser, &msg, bytes_written);
    }
    second_parser = QAJ4C_second_pass_parser_create(&parser);
    return QAJ4C_second_pass_process(&second_parser, &msg, bytes_written);
}

static size_t QAJ4C_calculate_max_buffer_generic( const char* json, size_t json_len, int opts ) {
    QAJ4C_First_pass_parser parser = QAJ4C_first_pass_parser_create(NULL, opts);
    const char* json_end = (json_len != SIZE_MAX) ? (json + json_len) : (const char*)UINTPTR_MAX;
    QAJ4C_Json_message msg = {.begin = json, .end = json_end, .pos = json};

    QAJ4C_first_pass_parse(&parser, &msg);

    return QAJ4C_calculate_max_buffer_parser(&parser);
}

static const QAJ4C_Value* QAJ4C_create_error_description( QAJ4C_First_pass_parser* parser, QAJ4C_Json_message* msg, size_t* bytes_written ) {
    static const size_t REQUIRED_STORAGE = sizeof(QAJ4C_Value) + sizeof(QAJ4C_Error_information);
    QAJ4C_Value* document;
    QAJ4C_Error_information* err_info;
    /* not enough space to store error information in buffer... */
    if (parser->builder->alloc_length < REQUIRED_STORAGE) {
        return NULL;
    }

    //QAJ4C_builder_init(parser->builder, parser->builder->buffer, parser->builder->buffer_size);
    document = (QAJ4C_Value*)parser->builder->elements;
    document->type = QAJ4C_ERROR_DESCRIPTION_TYPE_CONSTANT;

    err_info = (QAJ4C_Error_information*)(document + 1);
    err_info->err_no = parser->err_code;
    err_info->json = msg->begin;
    err_info->json_pos = msg->pos - msg->begin;
    QAJ4C_ERROR_GET_PTR(document) = err_info;

    if (bytes_written != NULL) {
       *bytes_written = REQUIRED_STORAGE;
    }
    return document;
}


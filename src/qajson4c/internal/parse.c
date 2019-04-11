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

#include "../qajson4c_builder.h" // FIXME: Remove dependency ?!?
#include "parse.h"
#include "first_pass.h"
#include "second_pass.h"

static QAJ4C_Value* QAJ4C_create_error_description( QAJ4C_First_pass_parser* parser, QAJ4C_Json_message* msg );

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

    if ( second_parser.err_code != QAJ4C_ERROR_NO_ERROR ) {
        *result_ptr = QAJ4C_create_error_description(&parser, &msg);
        return builder->cur_obj_pos;
    }

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

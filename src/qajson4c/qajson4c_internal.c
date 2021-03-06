/**
  @file

  Quite-Alright JSON for C - https://github.com/USESystemEngineeringBV/qajson4c

  Licensed under the MIT License <http://opensource.org/licenses/MIT>.

  Copyright (c) 2016 Pascal Proksch - USE System Engineering BV

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

#include "qajson_stdwrap.h"
#include "qajson4c.h"
#include "qajson4c_internal.h"

typedef struct QAJ4C_Json_message {
    const char* json;
    size_type json_len;
    size_type json_pos;
} QAJ4C_Json_message;

typedef struct QAJ4C_First_pass_parser {
    QAJ4C_Json_message* msg;

    QAJ4C_Builder* builder; /* Storage about object sizes */
    QAJ4C_realloc_fn realloc_callback;

    bool strict_parsing;
    bool insitu_parsing;
    bool optimize_object;

    int max_depth;
    size_type amount_nodes;
    size_type complete_string_length;
    size_type storage_counter;

    QAJ4C_ERROR_CODE err_code;

} QAJ4C_First_pass_parser;

typedef struct QAJ4C_Second_pass_parser {
    const char* json_char;
    QAJ4C_Builder* builder;
    QAJ4C_realloc_fn realloc_callback;
    bool insitu_parsing;
    bool optimize_object;

    size_type curr_buffer_pos;
} QAJ4C_Second_pass_parser;

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

QAJ4C_fatal_error_fn g_qaj4c_err_function = &QAJ4C_std_err_function;

static void QAJ4C_first_pass_parser_init( QAJ4C_First_pass_parser* parser, QAJ4C_Builder* builder, QAJ4C_Json_message* msg, int opts, QAJ4C_realloc_fn realloc_callback );
static void QAJ4C_first_pass_parser_set_error( QAJ4C_First_pass_parser* parser, QAJ4C_ERROR_CODE error );
static void QAJ4C_first_pass_process( QAJ4C_First_pass_parser* parser, int depth );
static void QAJ4C_first_pass_object( QAJ4C_First_pass_parser* parser, int depth );
static void QAJ4C_first_pass_array( QAJ4C_First_pass_parser* parser, int depth );
static void QAJ4C_first_pass_string( QAJ4C_First_pass_parser* parser );
static void QAJ4C_first_pass_numeric_value( QAJ4C_First_pass_parser* parser );
static void QAJ4C_first_pass_constant( QAJ4C_First_pass_parser* parser, const char* str, size_t len );
static uint32_t QAJ4C_first_pass_4digits( QAJ4C_First_pass_parser* parser );
static int QAJ4C_first_pass_utf16( QAJ4C_First_pass_parser* parser );
static void QAJ4C_first_pass_skip_whitespaces_and_comments( QAJ4C_First_pass_parser* parser );
static void QAJ4C_first_pass_skip_comment( QAJ4C_First_pass_parser* parser );

static size_type* QAJ4C_first_pass_fetch_stats_buffer( QAJ4C_First_pass_parser* parser, size_type storage_pos );
static QAJ4C_Value* QAJ4C_create_error_description( QAJ4C_First_pass_parser* me );

size_t QAJ4C_calculate_max_buffer_parser( QAJ4C_First_pass_parser* parser );

static void QAJ4C_second_pass_parser_init( QAJ4C_Second_pass_parser* me, QAJ4C_First_pass_parser* parser );
static void QAJ4C_second_pass_process( QAJ4C_Second_pass_parser* me, QAJ4C_Value* result_ptr );
static void QAJ4C_second_pass_object( QAJ4C_Second_pass_parser* me, QAJ4C_Value* result_ptr );
static void QAJ4C_second_pass_array( QAJ4C_Second_pass_parser* me, QAJ4C_Value* result_ptr );
static void QAJ4C_second_pass_string( QAJ4C_Second_pass_parser* me, QAJ4C_Value* result_ptr );
static char* QAJ4C_second_pass_string_escape_sequence( QAJ4C_Second_pass_parser* me, char* put_str );
static char* QAJ4C_second_pass_unicode_sequence( QAJ4C_Second_pass_parser* me, char* put_str );
static void QAJ4C_second_pass_numeric_value( QAJ4C_Second_pass_parser* me, QAJ4C_Value* result_ptr );
static uint32_t QAJ4C_second_pass_utf16( QAJ4C_Second_pass_parser* me );

static size_type QAJ4C_second_pass_fetch_stats_data( QAJ4C_Second_pass_parser* me );

static const char* QAJ4C_skip_whitespaces_and_comments_second_pass( const char* json );

static char QAJ4C_json_message_peek( QAJ4C_Json_message* msg );
static char QAJ4C_json_message_read( QAJ4C_Json_message* msg );
static void QAJ4C_json_message_forward( QAJ4C_Json_message* msg );
static char QAJ4C_json_message_forward_and_peek( QAJ4C_Json_message* msg );

bool QAJ4C_std_sprint_function( void *ptr, const char* buffer, size_t size );
bool QAJ4C_std_print_callback_function( void *ptr, const char* buffer, size_t size );

static char* QAJ4C_do_print_uint64( uint64_t value, char* buffer, size_t size );

bool QAJ4C_print_callback_object( const QAJ4C_Object* value_ptr, QAJ4C_print_buffer_callback_fn callback, void *ptr );
bool QAJ4C_print_callback_array( const QAJ4C_Array* value_ptr, QAJ4C_print_buffer_callback_fn callback, void *ptr );
bool QAJ4C_print_callback_primitive( const QAJ4C_Value* value_ptr, QAJ4C_print_buffer_callback_fn callback, void *ptr );
bool QAJ4C_print_callback_double( double d, QAJ4C_print_buffer_callback_fn callback, void *ptr );
bool QAJ4C_print_callback_uint64( uint64_t value, QAJ4C_print_buffer_callback_fn callback, void *ptr );
bool QAJ4C_print_callback_int64( int64_t value, QAJ4C_print_buffer_callback_fn callback, void *ptr );
bool QAJ4C_print_callback_error( const QAJ4C_Error* value_ptr, QAJ4C_print_buffer_callback_fn callback, void *ptr );
bool QAJ4C_print_callback_constant( const char *string, size_t size, QAJ4C_print_buffer_callback_fn callback, void *ptr );
bool QAJ4C_print_callback_string( const char *string, QAJ4C_print_buffer_callback_fn callback, void *ptr );

static const char QAJ4C_NULL_STR[] = "null";
static const char QAJ4C_TRUE_STR[] = "true";
static const char QAJ4C_FALSE_STR[] = "false";

#define ARRAY_COUNT(a) (sizeof(a) / sizeof(a[0]) - 1)

static const unsigned QAJ4C_NULL_STR_LEN = ARRAY_COUNT(QAJ4C_NULL_STR);
static const unsigned QAJ4C_TRUE_STR_LEN = ARRAY_COUNT(QAJ4C_TRUE_STR);
static const unsigned QAJ4C_FALSE_STR_LEN = ARRAY_COUNT(QAJ4C_FALSE_STR);

static int QAJ4C_xdigit( char c ) {
    return (c > '9')? (c &~ 0x20) - 'A' + 10: (c - '0');
}

static bool QAJ4C_is_digit( char c ) {
    return (uint8_t)(c - '0') < 10;
}

static bool QAJ4C_is_double_separation_char( char c ) {
    return c == '.' || c == 'e' || c == 'E';
}

size_t QAJ4C_parse_generic( QAJ4C_Builder* builder, const char* json, size_t json_len, int opts, const QAJ4C_Value** result_ptr, QAJ4C_realloc_fn realloc_callback ) {
    QAJ4C_First_pass_parser parser;
    QAJ4C_Second_pass_parser second_parser;
    QAJ4C_Json_message msg;
    size_type required_size;
    msg.json = json;
    msg.json_len = json_len;
    msg.json_pos = 0;

    QAJ4C_first_pass_parser_init(&parser, builder, &msg, opts, realloc_callback);

    QAJ4C_first_pass_process(&parser, 0);

    if (parser.strict_parsing && parser.msg->json[parser.msg->json_pos] != '\0') {
        /* skip whitespaces and comments after the json, even though we are graceful */
        QAJ4C_first_pass_skip_whitespaces_and_comments(&parser);
        if (parser.msg->json[parser.msg->json_pos] != '\0') {
            QAJ4C_first_pass_parser_set_error(&parser, QAJ4C_ERROR_UNEXPECTED_JSON_APPENDIX);
        }
    }

    if ( parser.err_code != QAJ4C_ERROR_NO_ERROR ) {
        *result_ptr = QAJ4C_create_error_description(&parser);
        return builder->cur_obj_pos;
    }

    required_size = QAJ4C_calculate_max_buffer_parser(&parser);
    if (required_size > builder->buffer_size) {
        if (parser.realloc_callback != NULL) {
            void* tmp = parser.realloc_callback(builder->buffer, required_size);
            if (tmp == NULL) {
                QAJ4C_first_pass_parser_set_error(&parser, QAJ4C_ERROR_ALLOCATION_ERROR);
            } else {
                QAJ4C_builder_init(builder, tmp, required_size);
            }
        } else {
            QAJ4C_first_pass_parser_set_error(&parser, QAJ4C_ERROR_STORAGE_BUFFER_TO_SMALL);
        }
    }
    else
    {
        QAJ4C_builder_init(builder, builder->buffer, required_size);
    }

    if ( parser.err_code != QAJ4C_ERROR_NO_ERROR ) {
        *result_ptr = QAJ4C_create_error_description(&parser);
        return builder->cur_obj_pos;
    }

    QAJ4C_second_pass_parser_init(&second_parser, &parser);
    *result_ptr = QAJ4C_builder_get_document(builder);
    QAJ4C_second_pass_process(&second_parser, (QAJ4C_Value*)*result_ptr);

    return builder->buffer_size;
}

size_t QAJ4C_calculate_max_buffer_parser( QAJ4C_First_pass_parser* parser ) {
    if (QAJ4C_UNLIKELY(parser->err_code != QAJ4C_ERROR_NO_ERROR)) {
        return sizeof(QAJ4C_Value) + sizeof(QAJ4C_Error_information);
    }
    return parser->amount_nodes * sizeof(QAJ4C_Value) + parser->complete_string_length;
}

size_t QAJ4C_calculate_max_buffer_generic( const char* json, size_t json_len, int opts ) {
    QAJ4C_First_pass_parser parser;
    QAJ4C_Json_message msg;
    msg.json = json;
    msg.json_len = json_len;
    msg.json_pos = 0;

    QAJ4C_first_pass_parser_init(&parser, NULL, &msg, opts, NULL);
    QAJ4C_first_pass_process(&parser, 0);

    return QAJ4C_calculate_max_buffer_parser(&parser);
}

static void QAJ4C_first_pass_parser_init( QAJ4C_First_pass_parser* parser, QAJ4C_Builder* builder, QAJ4C_Json_message* msg, int opts, QAJ4C_realloc_fn realloc_callback ) {
    parser->msg = msg;

    parser->builder = builder;
    parser->realloc_callback = realloc_callback;

    parser->strict_parsing = (opts & QAJ4C_PARSE_OPTS_STRICT) != 0;
    parser->optimize_object = (opts & QAJ4C_PARSE_OPTS_DONT_SORT_OBJECT_MEMBERS) == 0;
    parser->insitu_parsing = (opts & 1) != 0;

    parser->max_depth = 32;
    parser->amount_nodes = 0;
    parser->complete_string_length = 0;
    parser->storage_counter = 0;
    parser->err_code = QAJ4C_ERROR_NO_ERROR;
}

static void QAJ4C_first_pass_parser_set_error( QAJ4C_First_pass_parser* parser, QAJ4C_ERROR_CODE error ) {
    if( parser->err_code == QAJ4C_ERROR_NO_ERROR) {
        parser->err_code = error;
        /* set the length of the json message to the current position to avoid the parser will continue parsing */
        if (parser->msg->json_len > parser->msg->json_pos) {
            parser->msg->json_len = parser->msg->json_pos;
        }
    }
}

static void QAJ4C_first_pass_process( QAJ4C_First_pass_parser* parser, int depth ) {
    QAJ4C_first_pass_skip_whitespaces_and_comments(parser);
    parser->amount_nodes++;
    switch (QAJ4C_json_message_peek(parser->msg)) {
    case '{':
        QAJ4C_json_message_forward(parser->msg);
        QAJ4C_first_pass_object(parser, depth);
        break;
    case '[':
        QAJ4C_json_message_forward(parser->msg);
        QAJ4C_first_pass_array(parser, depth);
        break;
    case '"':
        QAJ4C_json_message_forward(parser->msg);
        QAJ4C_first_pass_string(parser);
        break;
    case 't':
        QAJ4C_first_pass_constant(parser, QAJ4C_TRUE_STR, QAJ4C_TRUE_STR_LEN);
        break;
    case 'f':
        QAJ4C_first_pass_constant(parser, QAJ4C_FALSE_STR, QAJ4C_FALSE_STR_LEN);
        break;
    case 'n':
        QAJ4C_first_pass_constant(parser, QAJ4C_NULL_STR, QAJ4C_NULL_STR_LEN);
        break;
    case '\0':
        QAJ4C_first_pass_parser_set_error(parser, QAJ4C_ERROR_JSON_MESSAGE_TRUNCATED);
        break;
    case '-':
    case '+':
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
        QAJ4C_first_pass_numeric_value(parser);
        break;
    default:
        QAJ4C_first_pass_parser_set_error(parser, QAJ4C_ERROR_UNEXPECTED_CHAR);
        break;
    }
}

static void QAJ4C_first_pass_object( QAJ4C_First_pass_parser* parser, int depth ) {
    char json_char;
    size_type member_count = 0;
    size_type storage_pos = parser->storage_counter;
    parser->storage_counter++;

    if (parser->max_depth < depth) {
        QAJ4C_first_pass_parser_set_error(parser, QAJ4C_ERROR_DEPTH_OVERFLOW);
        return;
    }

    QAJ4C_first_pass_skip_whitespaces_and_comments(parser);
    json_char = QAJ4C_json_message_read(parser->msg);

    while (json_char != '\0' && json_char != '}') {
        if (member_count > 0) {
            if (json_char != ',') {
                QAJ4C_first_pass_parser_set_error(parser, QAJ4C_ERROR_MISSING_COMMA);
            }
            QAJ4C_first_pass_skip_whitespaces_and_comments(parser);
            json_char = QAJ4C_json_message_read(parser->msg);
        }
        if (json_char == '"') {
            parser->amount_nodes++; /* count the string as node */
            QAJ4C_first_pass_string(parser);
            QAJ4C_first_pass_skip_whitespaces_and_comments(parser);
            json_char = QAJ4C_json_message_read(parser->msg);
            if (json_char != ':') {
                QAJ4C_first_pass_parser_set_error(parser, QAJ4C_ERROR_MISSING_COLON);
            }
            QAJ4C_first_pass_skip_whitespaces_and_comments(parser);
            QAJ4C_first_pass_process(parser, depth + 1);
            ++member_count;
        } else if (json_char == '}') {
            if (parser->strict_parsing) {
                QAJ4C_first_pass_parser_set_error(parser, QAJ4C_ERROR_TRAILING_COMMA);
            }
            break;
        } else {
            QAJ4C_first_pass_parser_set_error(parser, QAJ4C_ERROR_UNEXPECTED_CHAR);
        }

        QAJ4C_first_pass_skip_whitespaces_and_comments(parser);
        json_char = QAJ4C_json_message_read(parser->msg);
    }

    if (json_char == '\0') {
        QAJ4C_first_pass_parser_set_error(parser, QAJ4C_ERROR_JSON_MESSAGE_TRUNCATED);
    }

    if (parser->builder != NULL && parser->err_code == QAJ4C_ERROR_NO_ERROR) {
        size_type* obj_data = QAJ4C_first_pass_fetch_stats_buffer(parser, storage_pos);
        if (obj_data != NULL) {
            *obj_data = member_count;
        }
    }
}

static void QAJ4C_first_pass_array( QAJ4C_First_pass_parser* parser, int depth ) {
    char json_char;
    size_type member_count = 0;
    size_type storage_pos = parser->storage_counter;
    parser->storage_counter++;

    if (parser->max_depth < depth) {
        QAJ4C_first_pass_parser_set_error(parser, QAJ4C_ERROR_DEPTH_OVERFLOW);
        return;
    }

    QAJ4C_first_pass_skip_whitespaces_and_comments(parser);
    json_char = QAJ4C_json_message_peek(parser->msg);
    if (json_char != ']') {
        QAJ4C_first_pass_process(parser, depth + 1);
        QAJ4C_first_pass_skip_whitespaces_and_comments(parser);
        json_char = QAJ4C_json_message_peek(parser->msg);
        member_count = 1;
        while (json_char == ',') {
            QAJ4C_json_message_forward(parser->msg);
            QAJ4C_first_pass_skip_whitespaces_and_comments(parser);
            json_char = QAJ4C_json_message_peek(parser->msg);
            if (json_char != ']') {
                member_count += 1;
                QAJ4C_first_pass_process(parser, depth + 1);
                QAJ4C_first_pass_skip_whitespaces_and_comments(parser);
                json_char = QAJ4C_json_message_peek(parser->msg);
            } else if (parser->strict_parsing) {
                QAJ4C_first_pass_parser_set_error(parser, QAJ4C_ERROR_TRAILING_COMMA);
            }
        }
        if (json_char != ']') {
            QAJ4C_first_pass_parser_set_error(parser, QAJ4C_ERROR_MISSING_COMMA);
        }
    }

    QAJ4C_json_message_forward(parser->msg);

    if (parser->builder != NULL && parser->err_code == QAJ4C_ERROR_NO_ERROR) {
        size_type* obj_data = QAJ4C_first_pass_fetch_stats_buffer( parser, storage_pos );
        if ( obj_data != NULL ) {
            *obj_data = member_count;
        }
    }
}

static void QAJ4C_first_pass_string( QAJ4C_First_pass_parser* parser ) {
    char json_char;
    size_type chars = 0;

    json_char = QAJ4C_json_message_read(parser->msg);
    while (json_char != '\0' && json_char != '"') {
        if (json_char == '\\') {
            json_char = QAJ4C_json_message_read(parser->msg);
            switch (json_char) {
            case 'u':
                chars += QAJ4C_first_pass_utf16(parser) - 1;
                break;
            case '\\':
            case '/':
            case 'b':
            case 'f':
            case 'n':
            case 'r':
            case 't':
            case '"':
                break;
            default:
                QAJ4C_first_pass_parser_set_error(parser, QAJ4C_ERROR_INVALID_ESCAPE_SEQUENCE);
                return;
            }
        } else if (((uint8_t)json_char) < 32) {
            QAJ4C_first_pass_parser_set_error(parser, QAJ4C_ERROR_UNEXPECTED_CHAR);
        }
        ++chars;
        json_char = QAJ4C_json_message_read(parser->msg);
    }

    if (json_char != '"') {
        QAJ4C_first_pass_parser_set_error(parser, QAJ4C_ERROR_JSON_MESSAGE_TRUNCATED);
        return;
    }

    if (!parser->insitu_parsing && chars > QAJ4C_INLINE_STRING_SIZE) {
        parser->complete_string_length += chars + 1; /* count the \0 to the complete string length! */
    }
}

static uint32_t QAJ4C_first_pass_4digits( QAJ4C_First_pass_parser* parser ) {
    uint32_t value = 0;
    int char_count;

    for (char_count = 0; char_count < 4; char_count++) {
        char json_char = QAJ4C_json_message_read(parser->msg);
        uint8_t xdigit = QAJ4C_xdigit(json_char);

        if (xdigit > 0xF) {
            QAJ4C_first_pass_parser_set_error(parser, QAJ4C_ERROR_INVALID_UNICODE_SEQUENCE);
        }
        value = value << 4 | QAJ4C_xdigit(json_char);
    }

    return value;
}

static int QAJ4C_first_pass_utf16( QAJ4C_First_pass_parser* parser ) {
    int amount_utf8_chars = 0;
    uint32_t value = QAJ4C_first_pass_4digits(parser);

    if ( value < 0x80 ) { /* [0, 0x80) */
        amount_utf8_chars = 1;
    } else if ( value < 0x800 ) { /* [0x80, 0x800) */
        amount_utf8_chars = 2;
    } else if (value < 0xd800 || value > 0xdfff) { /* [0x800, 0xd800) or (0xdfff, 0xffff] */
        amount_utf8_chars = 3;
    } else if (value <= 0xdbff) { /* [0xd800,0xdbff] */
        if (QAJ4C_json_message_read(parser->msg) != '\\' || QAJ4C_json_message_read(parser->msg) != 'u') {
            QAJ4C_first_pass_parser_set_error(parser, QAJ4C_ERROR_INVALID_UNICODE_SEQUENCE);
        }
        else {
            uint32_t low_surrogate = QAJ4C_first_pass_4digits(parser);
            /* check valid surrogate range */
            if ( low_surrogate < 0xdc00 || low_surrogate > 0xdfff) {
                QAJ4C_first_pass_parser_set_error(parser, QAJ4C_ERROR_INVALID_UNICODE_SEQUENCE);
            }
        }
        amount_utf8_chars = 4;
    } else { /* [0xdc00, 0xdfff] */
        /* invalid (low-surrogate before high-surrogate) */
        QAJ4C_first_pass_parser_set_error(parser, QAJ4C_ERROR_INVALID_UNICODE_SEQUENCE);
    }

    return amount_utf8_chars;
}

static void QAJ4C_first_pass_numeric_value( QAJ4C_First_pass_parser* parser ) {
    char json_char = QAJ4C_json_message_peek(parser->msg);

    if ( json_char == '-' ) {
        json_char = QAJ4C_json_message_forward_and_peek(parser->msg);
    } else if (json_char == '+') {
        if (parser->strict_parsing) {
            QAJ4C_first_pass_parser_set_error(parser, QAJ4C_ERROR_INVALID_NUMBER_FORMAT);
        } else {
            json_char = QAJ4C_json_message_forward_and_peek(parser->msg);
        }
    }

	if (!QAJ4C_is_digit(json_char)) {
		QAJ4C_first_pass_parser_set_error(parser, QAJ4C_ERROR_INVALID_NUMBER_FORMAT);
	} else if (json_char == '0' && parser->strict_parsing) {
        json_char = QAJ4C_json_message_forward_and_peek(parser->msg);
        /* next char is not allowed to be numeric! */
        if (QAJ4C_is_digit(json_char)) {
            QAJ4C_first_pass_parser_set_error(parser, QAJ4C_ERROR_INVALID_NUMBER_FORMAT);
        }
    }

    while (QAJ4C_is_digit(json_char)) {
        json_char = QAJ4C_json_message_forward_and_peek(parser->msg);
    }

    if (QAJ4C_is_double_separation_char(json_char)) {
        /* check the format! */
        if (json_char == '.') {
            json_char = QAJ4C_json_message_forward_and_peek(parser->msg);
            /* expect at least one digit! */
            if (!QAJ4C_is_digit(json_char)) {
                QAJ4C_first_pass_parser_set_error(parser, QAJ4C_ERROR_INVALID_NUMBER_FORMAT);
            }
            while (QAJ4C_is_digit(json_char)) {
                json_char = QAJ4C_json_message_forward_and_peek(parser->msg);
            }
        }
        if (json_char == 'E' || json_char == 'e') {
            /* next char can be a + or - */
            json_char = QAJ4C_json_message_forward_and_peek(parser->msg);
            if (json_char == '+' || json_char == '-') {
                json_char = QAJ4C_json_message_forward_and_peek(parser->msg);
            }
            /* expect at least one digit! */
            if (!QAJ4C_is_digit(json_char)) {
                QAJ4C_first_pass_parser_set_error(parser, QAJ4C_ERROR_INVALID_NUMBER_FORMAT);
            }
            /* forward until the end! */
            while (QAJ4C_is_digit(json_char)) {
                json_char = QAJ4C_json_message_forward_and_peek(parser->msg);
            }
        }
    }
}

static void QAJ4C_first_pass_constant( QAJ4C_First_pass_parser* parser, const char* str, size_t len ) {
    size_t i;
    for (i = 0; i < len; i++) {
		char c = QAJ4C_json_message_read(parser->msg);
        if (c != str[i]) {
            QAJ4C_first_pass_parser_set_error(parser, QAJ4C_ERROR_UNEXPECTED_CHAR);
        }
    }
}

static void QAJ4C_second_pass_parser_init( QAJ4C_Second_pass_parser* me, QAJ4C_First_pass_parser* parser ) {
    QAJ4C_Builder* builder = parser->builder;
    size_type required_object_storage = parser->amount_nodes * sizeof(QAJ4C_Value);
    size_type required_tempoary_storage = parser->storage_counter *  sizeof(size_type);
    size_type copy_to_index = required_object_storage - required_tempoary_storage;

    memmove(builder->buffer + copy_to_index, builder->buffer, required_tempoary_storage);
    me->json_char = parser->msg->json;
    me->builder = parser->builder;
    me->realloc_callback = parser->realloc_callback;
    me->insitu_parsing = parser->insitu_parsing;
    me->optimize_object = parser->optimize_object;
    me->curr_buffer_pos = copy_to_index;

    /* reset the builder to its original state! */
    QAJ4C_builder_init(builder, builder->buffer, builder->buffer_size);
    builder->cur_str_pos = required_object_storage;
}

static void QAJ4C_second_pass_process( QAJ4C_Second_pass_parser* me, QAJ4C_Value* result_ptr ) {
    /* skip those stupid whitespaces! */
    me->json_char = QAJ4C_skip_whitespaces_and_comments_second_pass(me->json_char);
    switch (*me->json_char) {
    case '{':
        ++me->json_char;
        QAJ4C_second_pass_object(me, result_ptr);
        break;
    case '[':
        ++me->json_char;
        QAJ4C_second_pass_array(me, result_ptr);
        break;
    case '"':
        ++me->json_char;
        QAJ4C_second_pass_string(me, result_ptr);
        break;
    case 't':
        QAJ4C_set_bool(result_ptr, true);
        me->json_char += QAJ4C_TRUE_STR_LEN; /* jump to the end of the constant */
        break;
    case 'f':
        QAJ4C_set_bool(result_ptr, false);
        me->json_char += QAJ4C_FALSE_STR_LEN; /* jump to the end of the constant */
        break;
    case 'n':
        QAJ4C_set_null(result_ptr);
        me->json_char += QAJ4C_NULL_STR_LEN; /* jump to the end of the constant */
        break;
    default: /* As first pass was successful, it can only be a numeric value */
        QAJ4C_second_pass_numeric_value(me, result_ptr);
        break;
    }
}

static void QAJ4C_second_pass_object( QAJ4C_Second_pass_parser* me, QAJ4C_Value* result_ptr  ) {
    size_type elements = QAJ4C_second_pass_fetch_stats_data(me);
    size_type index;
    QAJ4C_Member* top;

    /*
     * Do not use set_object as it would initialize memory and thus corrupt the buffer
     * that stores string sizes and integer types
     */
    result_ptr->type = QAJ4C_OBJECT_TYPE_CONSTANT;
    ((QAJ4C_Object*)result_ptr)->count = elements;
    ((QAJ4C_Object*)result_ptr)->top = (QAJ4C_Member*)(&me->builder->buffer[me->builder->cur_obj_pos]);
    me->builder->cur_obj_pos += sizeof(QAJ4C_Member) * elements;
    top = ((QAJ4C_Object*)result_ptr)->top;

    me->json_char = QAJ4C_skip_whitespaces_and_comments_second_pass(me->json_char);
    for (index = 0; index < elements; ++index) {
        if (*me->json_char == ',') {
            ++me->json_char;
            me->json_char = QAJ4C_skip_whitespaces_and_comments_second_pass(me->json_char);
        }
        ++me->json_char; /* skip the first " */
        QAJ4C_second_pass_string(me, &top[index].key);
        me->json_char = QAJ4C_skip_whitespaces_and_comments_second_pass(me->json_char);
        ++me->json_char; /* skip the : */
        me->json_char = QAJ4C_skip_whitespaces_and_comments_second_pass(me->json_char);

        QAJ4C_second_pass_process(me, &top[index].value);
        me->json_char = QAJ4C_skip_whitespaces_and_comments_second_pass(me->json_char);
    }

    while( *me->json_char != '}') {
        me->json_char += 1;
        me->json_char = QAJ4C_skip_whitespaces_and_comments_second_pass(me->json_char);
    }

    if (me->optimize_object && elements > 2) {
        QAJ4C_object_optimize(result_ptr);
    }
    ++me->json_char; /* walk over the } */
}

static void QAJ4C_second_pass_array( QAJ4C_Second_pass_parser* me, QAJ4C_Value* result_ptr ) {
    size_type elements = QAJ4C_second_pass_fetch_stats_data(me);
    size_type index;
    QAJ4C_Value* top;

    /*
     * Do not use set_array as it would initialize memory and thus corrupt the buffer
     * that stores string sizes and integer types
     */
    result_ptr->type = QAJ4C_ARRAY_TYPE_CONSTANT;
    ((QAJ4C_Array*)result_ptr)->count = elements;
    ((QAJ4C_Array*)result_ptr)->top = (QAJ4C_Value*)(&me->builder->buffer[me->builder->cur_obj_pos]);
    me->builder->cur_obj_pos += sizeof(QAJ4C_Value) * elements;
    top = ((QAJ4C_Array*)result_ptr)->top;

    me->json_char = QAJ4C_skip_whitespaces_and_comments_second_pass(me->json_char);
    for (index = 0; index < elements; index++) {
        if (*me->json_char == ',') {
            ++me->json_char;
            me->json_char = QAJ4C_skip_whitespaces_and_comments_second_pass(
                    me->json_char);
        }
        QAJ4C_second_pass_process(me, &top[index]);
        me->json_char = QAJ4C_skip_whitespaces_and_comments_second_pass(me->json_char);
    }

    while (*me->json_char != ']') {
        me->json_char += 1;
        me->json_char = QAJ4C_skip_whitespaces_and_comments_second_pass(me->json_char);
    }
    ++me->json_char; /* walk over the ] */
}

static void QAJ4C_second_pass_string( QAJ4C_Second_pass_parser* me, QAJ4C_Value* result_ptr  ) {
    char* base_put_str = NULL;
    char* put_str = NULL;
    uintptr_t chars;

    bool check_size = true;

    if (me->insitu_parsing) {
        result_ptr->type = QAJ4C_STRING_REF_TYPE_CONSTANT;
        ((QAJ4C_String*)result_ptr)->s = me->json_char;
        put_str = (char*)me->json_char;
        check_size = false;
    } else {
        result_ptr->type = QAJ4C_INLINE_STRING_TYPE_CONSTANT;
        put_str = ((QAJ4C_Short_string*)result_ptr)->s;
    }

    base_put_str = put_str;

    while (*me->json_char != '"') {
        if (*me->json_char == '\\') {
            put_str = QAJ4C_second_pass_string_escape_sequence(me, put_str);
        } else {
            *put_str = *me->json_char;
        }
        put_str += 1;
        me->json_char += 1;

        if (check_size) {
            chars = put_str - base_put_str;
            if (chars > QAJ4C_INLINE_STRING_SIZE) {
                put_str = (char*)&me->builder->buffer[me->builder->cur_str_pos];
                /* copy over to normal string */
                QAJ4C_MEMCPY(put_str, base_put_str, chars * sizeof(char));
                result_ptr->type = QAJ4C_STRING_TYPE_CONSTANT;
                ((QAJ4C_String*)result_ptr)->s = put_str;
                base_put_str = put_str;
                put_str += chars;
                check_size = false; /* now the copy has been performed, stop checking the length! */
            }
        }
    }
    *put_str = '\0';
    me->json_char += 1;
    chars = put_str - base_put_str;
    if (QAJ4C_get_internal_type(result_ptr) == QAJ4C_INLINE_STRING) {
        ((QAJ4C_Short_string*)result_ptr)->count = chars;
    } else {
        ((QAJ4C_String*)result_ptr)->count = chars;
        if (!me->insitu_parsing) {
            me->builder->cur_str_pos += chars + 1;
        }
    }
}

static char* QAJ4C_second_pass_string_escape_sequence( QAJ4C_Second_pass_parser* me, char* put_ptr ) {
    char* put_str = put_ptr;
    me->json_char += 1;
    switch (*me->json_char) {
    case '"':
        *put_str = '"';
        break;
    case '\\':
        *put_str = '\\';
        break;
    case '/':
        *put_str = '/';
        break;
    case 'b':
        *put_str = '\b';
        break;
    case 'f':
        *put_str = '\f';
        break;
    case 'n':
        *put_str = '\n';
        break;
    case 'r':
        *put_str = '\r';
        break;
    case 't':
        *put_str = '\t';
        break;
    default: /* is has to be u */
        put_str = QAJ4C_second_pass_unicode_sequence(me, put_str);
        break;
    }
    return put_str;
}

static char* QAJ4C_second_pass_unicode_sequence( QAJ4C_Second_pass_parser* me, char* put_ptr )
{
    static const uint8_t FIRST_BYTE_TWO_BYTE_MARK_MASK = 0xC0;
    static const uint8_t FIRST_BYTE_TWO_BYTE_PAYLOAD_MASK = 0x1F;
    static const uint8_t FIRST_BYTE_THREE_BYTE_MARK_MASK = 0xE0;
    static const uint8_t FIRST_BYTE_THREE_BYTE_PAYLOAD_MASK = 0x0F;
    static const uint8_t FIRST_BYTE_FOUR_BYTE_MARK_MASK = 0xF0;
    static const uint8_t FIRST_BYTE_FOUR_BYTE_PAYLOAD_MASK = 0x07;
    static const uint8_t FOLLOW_BYTE_MARK_MASK = 0x80;
    static const uint8_t FOLLOW_BYTE_PAYLOAD_MASK = 0x3F;

    char* put_str = put_ptr;
    uint32_t utf16;
    me->json_char += 1;
    /* utf-8 conversion inspired by parson */
    utf16 = QAJ4C_second_pass_utf16(me);
    if (utf16 < 0x80) {
        *put_str = (char)utf16;
    } else if (utf16 < 0x800) {
        *put_str = ((utf16 >> 6) & FIRST_BYTE_TWO_BYTE_PAYLOAD_MASK) | FIRST_BYTE_TWO_BYTE_MARK_MASK;
        put_str++;
        *put_str = (utf16 & 0x3f) | FOLLOW_BYTE_MARK_MASK;
    } else if (utf16 < 0x010000) {
        *put_str = ((utf16 >> 12) & FIRST_BYTE_THREE_BYTE_PAYLOAD_MASK) | FIRST_BYTE_THREE_BYTE_MARK_MASK;
        put_str++;
        *put_str = ((utf16 >> 6) & FOLLOW_BYTE_PAYLOAD_MASK) | FOLLOW_BYTE_MARK_MASK;
        put_str++;
        *put_str = ((utf16) & FOLLOW_BYTE_PAYLOAD_MASK) | FOLLOW_BYTE_MARK_MASK;
    } else {
        *put_str = (((utf16 >> 18) & FIRST_BYTE_FOUR_BYTE_PAYLOAD_MASK) | FIRST_BYTE_FOUR_BYTE_MARK_MASK);
        put_str++;
        *put_str = (((utf16 >> 12) & FOLLOW_BYTE_PAYLOAD_MASK) | FOLLOW_BYTE_MARK_MASK);
        put_str++;
        *put_str = (((utf16 >> 6) & FOLLOW_BYTE_PAYLOAD_MASK) | FOLLOW_BYTE_MARK_MASK);
        put_str++;
        *put_str = (((utf16) & FOLLOW_BYTE_PAYLOAD_MASK) | FOLLOW_BYTE_MARK_MASK);
    }
    return put_str;
}

static uint32_t QAJ4C_second_pass_utf16( QAJ4C_Second_pass_parser* me ) {
    uint32_t value = QAJ4C_xdigit(me->json_char[0]) << 12 | QAJ4C_xdigit(me->json_char[1]) << 8 | QAJ4C_xdigit(me->json_char[2]) << 4 | QAJ4C_xdigit(me->json_char[3]);
    me->json_char += 4;
    if (value >= 0xd800 && value <= 0xdbff) {
        uint32_t low_surrogate;
        me->json_char += 2;
        low_surrogate = QAJ4C_xdigit(me->json_char[0]) << 12 | QAJ4C_xdigit(me->json_char[1]) << 8 | QAJ4C_xdigit(me->json_char[2]) << 4 | QAJ4C_xdigit(me->json_char[3]);
        value = ((((value-0xD800)&0x3FF)<<10)|((low_surrogate-0xDC00)&0x3FF))+0x010000;
        me->json_char += 4;
    }
    /* set pointer to last char */
    me->json_char -= 1;
    return value;
}

static void QAJ4C_second_pass_numeric_value( QAJ4C_Second_pass_parser* me, QAJ4C_Value* result_ptr ) {
    char* c = (char*)me->json_char;
    bool double_value = false;
    if (*me->json_char == '-') {
        int64_t i = QAJ4C_STRTOL(me->json_char, &c, 10);
        if (QAJ4C_is_double_separation_char(*c) || ((i == INT64_MAX || i == INT64_MIN) && errno == ERANGE)) {
            double_value = true;
        } else {
            QAJ4C_set_int64(result_ptr, i);
        }
    } else {
        uint64_t i = QAJ4C_STRTOUL(me->json_char, &c, 10);
        if (QAJ4C_is_double_separation_char(*c) || (i == UINT64_MAX && errno == ERANGE)) {
            double_value = true;
        } else {
            QAJ4C_set_uint64(result_ptr, i);
        }

    }
    if (double_value) {
        QAJ4C_set_double(result_ptr, QAJ4C_STRTOD(me->json_char, &c));
    }
    me->json_char = c;
}

static size_type QAJ4C_second_pass_fetch_stats_data( QAJ4C_Second_pass_parser* me ) {
    if (QAJ4C_UNLIKELY(me->curr_buffer_pos <= me->builder->cur_obj_pos - sizeof(size_type))) {
        /*
         * Corner case that the last entry is empty. In some cases this causes that
         * the statistics are already overwritten (see Corner-case tests)
         */
        return 0;
    }
    size_type data = *((size_type*)(me->builder->buffer + me->curr_buffer_pos));
    me->curr_buffer_pos += sizeof(size_type);
    return data;
}

static char QAJ4C_json_message_peek( QAJ4C_Json_message* msg ) {
    /* Also very unlikely to happen (only in case json is invalid) */
    return QAJ4C_UNLIKELY(msg->json_pos >= msg->json_len) ? '\0' : msg->json[msg->json_pos];
}

static char QAJ4C_json_message_read( QAJ4C_Json_message* msg ) {
    char result = QAJ4C_json_message_peek(msg);
    if (QAJ4C_UNLIKELY(result == '\0') && msg->json_pos < msg->json_len ) {
        msg->json_len = msg->json_pos;
    }
    msg->json_pos += 1;
    return result;
}

static void QAJ4C_json_message_forward( QAJ4C_Json_message* msg ) {
    msg->json_pos += 1;
}

static char QAJ4C_json_message_forward_and_peek( QAJ4C_Json_message* msg ) {
    msg->json_pos += 1;
    return QAJ4C_json_message_peek(msg);
}

static void QAJ4C_first_pass_skip_whitespaces_and_comments( QAJ4C_First_pass_parser* parser ) {
    char current_char = QAJ4C_json_message_peek(parser->msg);

    while (current_char != '\0') {
        switch (current_char) {
        case '\t':
        case '\n':
        case '\b':
        case '\r':
        case ' ':
            break;
        case '/': /* also skip comments! */
            QAJ4C_first_pass_skip_comment(parser);
            break;
        default:
            return;
        }
        current_char = QAJ4C_json_message_forward_and_peek(parser->msg);
    }
}

static void QAJ4C_first_pass_skip_comment( QAJ4C_First_pass_parser* parser )
{
    char current_char = QAJ4C_json_message_forward_and_peek(parser->msg);
    if (current_char == '*') {
        QAJ4C_json_message_forward(parser->msg);
        do {
            current_char = QAJ4C_json_message_read(parser->msg);
        } while (current_char != '\0' && !(current_char == '*' && QAJ4C_json_message_peek(parser->msg) == '/'));
    } else if (current_char == '/') {
        do {
            current_char = QAJ4C_json_message_forward_and_peek(parser->msg);
        } while (current_char != '\0' && current_char != '\n');
    } else {
    	QAJ4C_first_pass_parser_set_error(parser, QAJ4C_ERROR_UNEXPECTED_CHAR);
    }
}

static const char* QAJ4C_skip_whitespaces_and_comments_second_pass( const char* json ) {
    const char* c_ptr = json;
    while ( true ) {
        if (*c_ptr == '/') {
            ++c_ptr;
            /* It can either be a c-comment or a line comment*/
            if (*c_ptr == '*') {
                ++c_ptr;
                while (!(*c_ptr == '*' && *(c_ptr + 1) == '/')) {
                    ++c_ptr;
                }
                ++c_ptr;
            } else {
                while (*c_ptr != '\n') {
                    ++c_ptr;
                }
            }
        } else if (*c_ptr > 0x20) {
            break;
        }
        ++c_ptr;
    }
    return c_ptr;
}

static QAJ4C_Value* QAJ4C_create_error_description( QAJ4C_First_pass_parser* parser ) {
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
    err_info->json = parser->msg->json;
    err_info->json_pos = parser->msg->json_len;

    parser->builder->cur_obj_pos = sizeof(QAJ4C_Value) + sizeof(QAJ4C_Error_information);
    ((QAJ4C_Error*)document)->info = err_info;

    return document;
}

static size_type* QAJ4C_first_pass_fetch_stats_buffer( QAJ4C_First_pass_parser* parser, size_type storage_pos ) {
    QAJ4C_Builder* builder = parser->builder;
    size_t in_buffer_pos = storage_pos * sizeof(size_type);
    if (in_buffer_pos + sizeof(size_type) > builder->buffer_size) {
        void *tmp;
        size_t required_size = QAJ4C_calculate_max_buffer_parser(parser);
        if (parser->realloc_callback == NULL) {
            QAJ4C_first_pass_parser_set_error(parser,
                                              QAJ4C_ERROR_STORAGE_BUFFER_TO_SMALL);
            return NULL;
        }

        tmp = parser->realloc_callback(builder->buffer, required_size);
        if (tmp == NULL) {
            QAJ4C_first_pass_parser_set_error(parser, QAJ4C_ERROR_ALLOCATION_ERROR);
            return NULL;
        }
        builder->buffer = tmp;
        builder->buffer_size = required_size;
    }
    return (size_type*)&builder->buffer[in_buffer_pos];
}

static bool QAJ4C_builder_validate_buffer( QAJ4C_Builder* builder ) {
    return builder->cur_obj_pos - 1 <= builder->cur_str_pos;
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

bool QAJ4C_print_buffer_callback_impl( const QAJ4C_Value* value_ptr, QAJ4C_print_buffer_callback_fn callback, void* ptr )
{
    bool result = false;
    switch (QAJ4C_get_internal_type(value_ptr)) {
    case QAJ4C_OBJECT_SORTED:
    case QAJ4C_OBJECT:
        result = QAJ4C_print_callback_object((const QAJ4C_Object*)value_ptr, callback, ptr);
        break;
    case QAJ4C_ARRAY:
        result = QAJ4C_print_callback_array((const QAJ4C_Array*)value_ptr, callback, ptr);
        break;
    case QAJ4C_PRIMITIVE:
        result = QAJ4C_print_callback_primitive(value_ptr, callback, ptr);
        break;
    case QAJ4C_NULL:
        result = QAJ4C_print_callback_constant(QAJ4C_NULL_STR, QAJ4C_NULL_STR_LEN, callback, ptr);
        break;
    case QAJ4C_STRING_REF:
    case QAJ4C_INLINE_STRING:
    case QAJ4C_STRING:
        result = QAJ4C_print_callback_string(QAJ4C_get_string(value_ptr), callback, ptr);
        break;
    case QAJ4C_ERROR_DESCRIPTION:
        result = QAJ4C_print_callback_error((const QAJ4C_Error*)value_ptr, callback, ptr);
        break;
    default:
        g_qaj4c_err_function();
    }
    return result;
}

bool QAJ4C_print_callback_object( const QAJ4C_Object* value_ptr, QAJ4C_print_buffer_callback_fn callback, void *ptr ) {
    QAJ4C_Member* top = value_ptr->top;
    size_type i;
    size_type n = ((QAJ4C_Object*)value_ptr)->count;
    bool result = true;

    result = callback(ptr, "{", 1);
    for (i = 0; i < n; ++i) {
        if (!QAJ4C_is_null(&top[i].key)) {
            if (i > 0) {
                result = result && callback(ptr, ",", 1);
            }

            result = result && QAJ4C_print_buffer_callback(&top[i].key, callback, ptr)
                    && callback(ptr, ":", 1)
                    && QAJ4C_print_buffer_callback(&top[i].value, callback, ptr);
        }
    }
    return result && callback(ptr, "}", 1);
}

bool QAJ4C_print_callback_array( const QAJ4C_Array* value_ptr, QAJ4C_print_buffer_callback_fn callback, void *ptr )
{
    QAJ4C_Value* top = value_ptr->top;
    bool result = true;
    size_type i;
    size_type n = ((QAJ4C_Array*)value_ptr)->count;

    result = callback(ptr, "[", 1);
    for (i = 0; i < n; ++i) {
        if (i > 0) {
            result = result && callback(ptr, ",", 1);
        }
        result = result && QAJ4C_print_buffer_callback(&top[i], callback, ptr);
    }
    return result && callback(ptr, "]", 1);
}

bool QAJ4C_print_callback_primitive( const QAJ4C_Value* value_ptr, QAJ4C_print_buffer_callback_fn callback, void *ptr )
{
    bool result = true;
    switch (QAJ4C_get_storage_type(value_ptr)) {
    case QAJ4C_PRIMITIVE_BOOL:
        if (((QAJ4C_Primitive*)value_ptr)->data.b) {
            result = QAJ4C_print_callback_constant( QAJ4C_TRUE_STR, QAJ4C_TRUE_STR_LEN, callback, ptr );
        } else {
            result = QAJ4C_print_callback_constant( QAJ4C_FALSE_STR, QAJ4C_FALSE_STR_LEN, callback, ptr );
        }
        break;
    case QAJ4C_PRIMITIVE_INT:
    case QAJ4C_PRIMITIVE_INT64:
        result = QAJ4C_print_callback_int64(((QAJ4C_Primitive*)value_ptr)->data.i, callback, ptr);
        break;
    case QAJ4C_PRIMITIVE_UINT:
    case QAJ4C_PRIMITIVE_UINT64:
        result = QAJ4C_print_callback_uint64(((QAJ4C_Primitive*)value_ptr)->data.u, callback, ptr);
        break;
    default: /* it has to be double */
        result = QAJ4C_print_callback_double(((QAJ4C_Primitive*)value_ptr)->data.d, callback, ptr);
        break;
    }
    return result;
}

bool QAJ4C_print_callback_double( double d, QAJ4C_print_buffer_callback_fn callback, void *ptr )
{
    static int BUFFER_SIZE = 32;
    char buffer[BUFFER_SIZE];
    bool result = true;

    int printf_result = QAJ4C_SNPRINTF(buffer, BUFFER_SIZE, "%1.10g", d);

    if ( printf_result > 0 && printf_result < BUFFER_SIZE && (QAJ4C_is_digit(buffer[1]) || QAJ4C_is_double_separation_char(buffer[1]) || printf_result == 1) )
    {
       result = callback(ptr, buffer, printf_result);
    }
    else
    {
       result = QAJ4C_print_callback_constant(QAJ4C_NULL_STR, QAJ4C_NULL_STR_LEN, callback, ptr);
    }
    return result;
}

static char* QAJ4C_do_print_uint64( uint64_t value, char* buffer, size_t size )
{
    uint64_t number = value;
    char* pos_ptr = buffer + size;
    if (number == 0) {
        pos_ptr -= 1;
        *pos_ptr = '0';
    } else {
        while (number != 0) {
            pos_ptr -= 1;
            *pos_ptr = '0' + number % 10;
            number /= 10;
        }
    }

    return pos_ptr;
}

bool QAJ4C_print_callback_uint64( uint64_t value, QAJ4C_print_buffer_callback_fn callback, void *ptr )
{
    static int BUFFER_SIZE = 32;
    char buffer[BUFFER_SIZE];
    char* pos_ptr = QAJ4C_do_print_uint64(value, buffer, BUFFER_SIZE);
    return callback(ptr, pos_ptr, (buffer + BUFFER_SIZE) - pos_ptr);
}

bool QAJ4C_print_callback_int64( int64_t value, QAJ4C_print_buffer_callback_fn callback, void *ptr )
{
    static int BUFFER_SIZE = 32;
    char buffer[BUFFER_SIZE];

	/* this callback is only called with a negative number as it otherwise has been classified uint64 */
    char* pos_ptr = QAJ4C_do_print_uint64(-value, buffer + 1, BUFFER_SIZE - 1);

	pos_ptr -= 1;
	*pos_ptr = '-';
    return callback(ptr, pos_ptr, (buffer + BUFFER_SIZE) - pos_ptr);
}

bool QAJ4C_print_callback_constant( const char *string, size_t size, QAJ4C_print_buffer_callback_fn callback, void *ptr )
{
    return callback(ptr, string, size);
}

bool QAJ4C_print_callback_string( const char *string, QAJ4C_print_buffer_callback_fn callback, void *ptr )
{
    static const char* replacement_buf[] = {
            "\\u0000", "\\u0001", "\\u0002", "\\u0003", "\\u0004", "\\u0005", "\\u0006", "\\u0007", "\\b",     "\\t",     "\\n",     "\\u000b", "\\f",     "\\r",     "\\u000e", "\\u000f",
            "\\u0010", "\\u0011", "\\u0012", "\\u0013", "\\u0014", "\\u0015", "\\u0016", "\\u0017", "\\u0018", "\\u0019", "\\u001a", "\\u001b", "\\u001c", "\\u001d", "\\u001e", "\\u001f",
            "", "", "\\\"", "", "", "", "", "", "", "", "", "", "\\\\", "", "", "\\/"
    };
    const char *p = string;
    size_t size = 0;
    bool result = callback(ptr, "\"", 1);

    while( result && p[size] != '\0' ) {
    	uint8_t c = (uint8_t)p[size];
        if (QAJ4C_UNLIKELY(c < 32 || c == '"' || c == '/' || c == '\\')) {
            if ( c >= 32 ) {
                /* set the char in the 2x range so we can use the replacement buffer to replace the string. */
                c = (c & 0xF) | 0x20;
            }
            const char* replacement_string = replacement_buf[c];
            result = result && callback(ptr, p, size); /* flush the string until now */
            result = result && callback(ptr, replacement_string, strlen(replacement_string));
            p = p + size + 1;
            size = 0;
        } else {
            size += 1;
        }
    }

    if (size > 0) {
        result = result && callback(ptr, p, size);
    }

    return result && callback(ptr, "\"", 1);
}

bool QAJ4C_print_callback_error( const QAJ4C_Error* value_ptr, QAJ4C_print_buffer_callback_fn callback, void *ptr )
{
    static const char ERR_MSG[] = "{\"error\":\"Unable to parse json message. Error (";
    static const size_t ERR_MSG_LEN = ARRAY_COUNT(ERR_MSG);
    static const char ERR_MSG_2[] = ") at position ";
    static const size_t ERR_MSG_2_LEN = ARRAY_COUNT(ERR_MSG_2);
    static const char ERR_MSG_3[] = "\"}";
    static const size_t ERR_MSG_3_LEN = ARRAY_COUNT(ERR_MSG_3);

    return QAJ4C_print_callback_constant(ERR_MSG, ERR_MSG_LEN, callback, ptr)
            && QAJ4C_print_callback_uint64(value_ptr->info->err_no, callback, ptr)
            && QAJ4C_print_callback_constant(ERR_MSG_2, ERR_MSG_2_LEN, callback, ptr)
            && QAJ4C_print_callback_uint64(value_ptr->info->json_pos, callback, ptr)
            && QAJ4C_print_callback_constant(ERR_MSG_3, ERR_MSG_3_LEN, callback, ptr);
}


size_t QAJ4C_sprint_impl( const QAJ4C_Value* value_ptr, char* buffer, size_t buffer_size, size_t index ) {
    QAJ4C_Buffer_printer printer = {buffer, index, buffer_size};
    QAJ4C_print_buffer_callback( value_ptr, QAJ4C_std_sprint_function, &printer );
    return printer.index;
}

bool QAJ4C_print_callback_impl( const QAJ4C_Value* value_ptr, QAJ4C_print_callback_fn callback, void* ptr ) {
    QAJ4C_callback_printer printer = {ptr, callback};
    return QAJ4C_print_buffer_callback(value_ptr, QAJ4C_std_print_callback_function, &printer);
}

bool QAJ4C_std_sprint_function( void *ptr, const char* buffer, size_t size ) {
    QAJ4C_Buffer_printer* printer = (QAJ4C_Buffer_printer*)ptr;
    size_t bytes_left = printer->len - printer->index;
    size_t copy_bytes = (bytes_left > size) ? size : bytes_left;
    QAJ4C_MEMCPY(printer->buffer + printer->index, buffer, copy_bytes);
    printer->index += copy_bytes;
    return copy_bytes == size;
}

bool QAJ4C_std_print_callback_function( void *ptr, const char* buffer, size_t size ) {
    bool result = true;
    size_t pos = 0;
    QAJ4C_callback_printer* callback_printer = (QAJ4C_callback_printer*)ptr;
    while (result && pos < size) {
        result = callback_printer->callback(callback_printer->external_ptr, buffer[pos]);
        pos += 1;
    }
    return result;
}

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

typedef struct QSJ4C_Json_message {
    const char* json;
    size_t json_len;
    size_t json_pos;
} QSJ4C_Json_message;

typedef struct QSJ4C_First_pass_parser {
    QSJ4C_Json_message* msg;

    QSJ4C_Builder* builder; /* Storage about object sizes */
    QSJ4C_realloc_fn realloc_callback;

    bool strict_parsing;

    int max_depth;
    size_t amount_nodes;
    size_t complete_string_length;
    size_t storage_counter;
    size_t string_storage_counter;

    size_t highest_storage_counter;
    size_t highest_string_storage_counter;

    QSJ4C_ERROR_CODE err_code;

} QSJ4C_First_pass_parser;

typedef struct QSJ4C_Second_pass_parser {
    const char* json_char;
    QSJ4C_Builder* builder;
    QSJ4C_realloc_fn realloc_callback;
    bool insitu_parsing;
    bool optimize_object;

    size_t curr_object_pos;
    size_t curr_string_pos;
} QSJ4C_Second_pass_parser;


void QSJ4C_std_err_function(void) {
    QSJ4C_RAISE(SIGABRT);
}

QSJ4C_fatal_error_fn g_QSJ4C_err_function = &QSJ4C_std_err_function;

static void QSJ4C_first_pass_parser_init( QSJ4C_First_pass_parser* parser, QSJ4C_Builder* builder, QSJ4C_Json_message* msg, int opts, QSJ4C_realloc_fn realloc_callback );
static void QSJ4C_first_pass_parser_set_error( QSJ4C_First_pass_parser* parser, QSJ4C_ERROR_CODE error );
static void QSJ4C_first_pass_process( QSJ4C_First_pass_parser* parser, int depth );
static void QSJ4C_first_pass_object( QSJ4C_First_pass_parser* parser, int depth );
static void QSJ4C_first_pass_array( QSJ4C_First_pass_parser* parser, int depth );
static void QSJ4C_first_pass_string( QSJ4C_First_pass_parser* parser );
static void QSJ4C_first_pass_numeric_value( QSJ4C_First_pass_parser* parser );
static void QSJ4C_first_pass_constant( QSJ4C_First_pass_parser* parser, const char* str, size_t len );
static uint32_t QSJ4C_first_pass_4digits( QSJ4C_First_pass_parser* parser );
static int QSJ4C_first_pass_utf16( QSJ4C_First_pass_parser* parser );
static void QSJ4C_first_pass_skip_whitespaces_and_comments( QSJ4C_First_pass_parser* parser );
static void QSJ4C_first_pass_skip_comment( QSJ4C_First_pass_parser* parser );

static void* QSJ4C_allocate( QSJ4C_Builder* builder, QSJ4C_First_pass_parser* parser );
static size_type* QSJ4C_first_pass_fetch_stats_buffer( QSJ4C_First_pass_parser* parser, size_t storage_pos );
static size_type* QSJ4C_first_pass_fetch_string_stats_buffer( QSJ4C_First_pass_parser* parser, size_t storage_pos );
static QSJ4C_Value* QSJ4C_create_error_description( QSJ4C_First_pass_parser* me );

size_t QSJ4C_calculate_max_buffer_parser( QSJ4C_First_pass_parser* parser );

static void QSJ4C_second_pass_parser_init( QSJ4C_Second_pass_parser* me, QSJ4C_First_pass_parser* parser);
static void QSJ4C_second_pass_process( QSJ4C_Second_pass_parser* me, QSJ4C_Value* result_ptr );
static void QSJ4C_second_pass_object( QSJ4C_Second_pass_parser* me, QSJ4C_Value* result_ptr );
static void QSJ4C_second_pass_array( QSJ4C_Second_pass_parser* me, QSJ4C_Value* result_ptr );
static void QSJ4C_second_pass_string( QSJ4C_Second_pass_parser* me, QSJ4C_Value* result_ptr );
static size_type QSJ4C_second_pass_read_char( QSJ4C_Second_pass_parser* me, char* out );
static size_type QSJ4C_second_pass_unicode_sequence( QSJ4C_Second_pass_parser* me, char* out );
static void QSJ4C_second_pass_numeric_value( QSJ4C_Second_pass_parser* me, QSJ4C_Value* result_ptr );
static uint32_t QSJ4C_second_pass_utf16( QSJ4C_Second_pass_parser* me );

static size_type QSJ4C_second_pass_fetch_stats_data( QSJ4C_Second_pass_parser* me );
static size_type QSJ4C_second_pass_fetch_string_stats_data( QSJ4C_Second_pass_parser* me );

static const char* QSJ4C_skip_whitespaces_and_comments_second_pass( const char* json );

static char QSJ4C_json_message_peek( QSJ4C_Json_message* msg );
static char QSJ4C_json_message_read( QSJ4C_Json_message* msg );
static void QSJ4C_json_message_forward( QSJ4C_Json_message* msg );
static char QSJ4C_json_message_forward_and_peek( QSJ4C_Json_message* msg );

size_t QSJ4C_sprint_object( QSJ4C_Value* value_ptr, char* buffer, size_t buffer_size, size_t index );
size_t QSJ4C_sprint_array( QSJ4C_Value* value_ptr, char* buffer, size_t buffer_size, size_t index );
size_t QSJ4C_sprint_bool( QSJ4C_Value* value_ptr, char* buffer, size_t buffer_size, size_t index );
size_t QSJ4C_sprint_primitive( QSJ4C_Value* value_ptr, char* buffer, size_t buffer_size, size_t index );
size_t QSJ4C_sprint_float( float f, char* buffer, size_t buffer_size, size_t index );

static const char QSJ4C_NULL_STR[] = "null";
static const char QSJ4C_TRUE_STR[] = "true";
static const char QSJ4C_FALSE_STR[] = "false";
static const unsigned QSJ4C_NULL_STR_LEN = sizeof(QSJ4C_NULL_STR) / sizeof(QSJ4C_NULL_STR[0]) - 1;
static const unsigned QSJ4C_TRUE_STR_LEN = sizeof(QSJ4C_TRUE_STR) / sizeof(QSJ4C_TRUE_STR[0]) - 1;
static const unsigned QSJ4C_FALSE_STR_LEN = sizeof(QSJ4C_FALSE_STR) / sizeof(QSJ4C_FALSE_STR[0]) - 1;

static int QSJ4C_xdigit( char c ) {
    return (c > '9')? (c &~ 0x20) - 'A' + 10: (c - '0');
}

static bool QSJ4C_is_digit( char c ) {
    return (uint8_t)(c - '0') < 10;
}

static bool QSJ4C_is_float_separation_char( char c ) {
    return c == '.' || c == 'e' || c == 'E';
}

uint32_t QSJ4C_read_uint24(const uint8_t* buff)
{
	return ((uint32_t) buff[0]) << 16 | ((uint16_t) buff[1]) << 8 | buff[2];
}

void QSJ4C_write_uint24(uint8_t* buff, uint32_t value)
{
	buff[0] = (value >> 16) & 0xFF;
	buff[1] = (value >> 8) & 0xFF;
	buff[2] = value & 0xFF;
}

size_t QSJ4C_parse_generic( QSJ4C_Builder* builder, const char* json, size_t json_len, int opts, QSJ4C_Value** result_ptr, QSJ4C_realloc_fn realloc_callback ) {
    QSJ4C_First_pass_parser parser;
    QSJ4C_Second_pass_parser second_parser;
    QSJ4C_Json_message msg;
    size_t required_size;
    msg.json = json;
    msg.json_len = json_len;
    msg.json_pos = 0;

    QSJ4C_first_pass_parser_init(&parser, builder, &msg, opts, realloc_callback);

    QSJ4C_first_pass_process(&parser, 0);

    if (parser.strict_parsing && QSJ4C_json_message_peek(parser.msg) != '\0') {
        /* skip whitespaces and comments after the json, even though we are graceful */
        QSJ4C_first_pass_skip_whitespaces_and_comments(&parser);
        if (QSJ4C_json_message_peek(parser.msg) != '\0') {
            QSJ4C_first_pass_parser_set_error(&parser, QSJ4C_ERROR_UNEXPECTED_JSON_APPENDIX);
        }
    }

    if ( parser.err_code != QSJ4C_ERROR_NO_ERROR ) {
        *result_ptr = QSJ4C_create_error_description(&parser);
        return builder->cur_obj_pos;
    }

    required_size = QSJ4C_calculate_max_buffer_parser(&parser);
	if (required_size > builder->buffer_size) {
		void* tmp = QSJ4C_allocate(builder, &parser);
		if (tmp != NULL) {
			QSJ4C_builder_init(builder, tmp, required_size);
		}
	}
    else
    {
        QSJ4C_builder_init(builder, builder->buffer, required_size);
    }

    if ( parser.err_code != QSJ4C_ERROR_NO_ERROR ) {
        *result_ptr = QSJ4C_create_error_description(&parser);
        return builder->cur_obj_pos;
    }

    QSJ4C_second_pass_parser_init(&second_parser, &parser);
    *result_ptr = QSJ4C_builder_get_document(builder);
    QSJ4C_second_pass_process(&second_parser, (QSJ4C_Value*)*result_ptr);

    return builder->buffer_size;
}

size_t QSJ4C_calculate_max_buffer_parser( QSJ4C_First_pass_parser* parser ) {
    if (QSJ4C_UNLIKELY(parser->err_code != QSJ4C_ERROR_NO_ERROR)) {
        return sizeof(QSJ4C_Value);
    }
    return parser->amount_nodes * sizeof(QSJ4C_Value) + parser->complete_string_length;
}

size_t QSJ4C_calculate_max_buffer_generic( const char* json, size_t json_len, int opts ) {
    QSJ4C_First_pass_parser parser;
    QSJ4C_Json_message msg;
    msg.json = json;
    msg.json_len = json_len;
    msg.json_pos = 0;

    QSJ4C_first_pass_parser_init(&parser, NULL, &msg, opts, NULL);
    QSJ4C_first_pass_process(&parser, 0);

    return QSJ4C_calculate_max_buffer_parser(&parser);
}

static void QSJ4C_first_pass_parser_init( QSJ4C_First_pass_parser* parser, QSJ4C_Builder* builder, QSJ4C_Json_message* msg, int opts, QSJ4C_realloc_fn realloc_callback ) {
    parser->msg = msg;

    parser->builder = builder;
    parser->realloc_callback = realloc_callback;

    parser->strict_parsing = (opts & QSJ4C_PARSE_OPTS_STRICT) != 0;

    parser->max_depth = 32;
    parser->amount_nodes = 0;
    parser->complete_string_length = 0;
    parser->storage_counter = 0;
    parser->highest_storage_counter = 0;
    parser->string_storage_counter = 0;
    parser->highest_string_storage_counter = 0;
    parser->err_code = QSJ4C_ERROR_NO_ERROR;
}

static void QSJ4C_first_pass_parser_set_error( QSJ4C_First_pass_parser* parser, QSJ4C_ERROR_CODE error ) {
    if( parser->err_code == QSJ4C_ERROR_NO_ERROR) {
        parser->err_code = error;
        /* set the length of the json message to the current position to avoid the parser will continue parsing */
        if (parser->msg->json_len > parser->msg->json_pos) {
            parser->msg->json_len = parser->msg->json_pos;
        }
    }
}

static void QSJ4C_first_pass_process( QSJ4C_First_pass_parser* parser, int depth) {
    QSJ4C_first_pass_skip_whitespaces_and_comments(parser);
    parser->amount_nodes++;
    switch (QSJ4C_json_message_peek(parser->msg)) {
    case '{':
        QSJ4C_json_message_forward(parser->msg);
        QSJ4C_first_pass_object(parser, depth);
        break;
    case '[':
        QSJ4C_json_message_forward(parser->msg);
        QSJ4C_first_pass_array(parser, depth);
        break;
    case '"':
        QSJ4C_json_message_forward(parser->msg);
        QSJ4C_first_pass_string(parser);
        break;
    case 't':
        QSJ4C_first_pass_constant(parser, QSJ4C_TRUE_STR, QSJ4C_TRUE_STR_LEN);
        break;
    case 'f':
        QSJ4C_first_pass_constant(parser, QSJ4C_FALSE_STR, QSJ4C_FALSE_STR_LEN);
        break;
    case 'n':
        QSJ4C_first_pass_constant(parser, QSJ4C_NULL_STR, QSJ4C_NULL_STR_LEN);
        break;
    case '\0':
        QSJ4C_first_pass_parser_set_error(parser, QSJ4C_ERROR_JSON_MESSAGE_TRUNCATED);
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
        QSJ4C_first_pass_numeric_value(parser);
        break;
    default:
        QSJ4C_first_pass_parser_set_error(parser, QSJ4C_ERROR_UNEXPECTED_CHAR);
        break;
    }
}

static void QSJ4C_first_pass_object( QSJ4C_First_pass_parser* parser, int depth ) {
    char json_char;
    size_type member_count = 0;
    size_t storage_pos = parser->storage_counter;
    parser->storage_counter++;

    if (parser->max_depth < depth) {
        QSJ4C_first_pass_parser_set_error(parser, QSJ4C_ERROR_DEPTH_OVERFLOW);
        return;
    }

    QSJ4C_first_pass_skip_whitespaces_and_comments(parser);
    json_char = QSJ4C_json_message_read(parser->msg);

    while (json_char != '\0' && json_char != '}') {
        if (member_count > 0) {
            if (json_char != ',') {
                QSJ4C_first_pass_parser_set_error(parser, QSJ4C_ERROR_MISSING_COMMA);
            }
            QSJ4C_first_pass_skip_whitespaces_and_comments(parser);
            json_char = QSJ4C_json_message_read(parser->msg);
        }
        if (json_char == '"') {
            parser->amount_nodes++; /* count the string as node */
            QSJ4C_first_pass_string(parser);
            QSJ4C_first_pass_skip_whitespaces_and_comments(parser);
            json_char = QSJ4C_json_message_read(parser->msg);
            if (json_char != ':') {
                QSJ4C_first_pass_parser_set_error(parser, QSJ4C_ERROR_MISSING_COLON);
            }
            QSJ4C_first_pass_skip_whitespaces_and_comments(parser);
            QSJ4C_first_pass_process(parser, depth + 1);
            ++member_count;
        } else if (json_char == '}') {
            if (parser->strict_parsing) {
                QSJ4C_first_pass_parser_set_error(parser, QSJ4C_ERROR_TRAILING_COMMA);
            }
            break;
        } else {
            QSJ4C_first_pass_parser_set_error(parser, QSJ4C_ERROR_UNEXPECTED_CHAR);
        }

        QSJ4C_first_pass_skip_whitespaces_and_comments(parser);
        json_char = QSJ4C_json_message_read(parser->msg);
    }

    if (json_char == '\0') {
        QSJ4C_first_pass_parser_set_error(parser, QSJ4C_ERROR_JSON_MESSAGE_TRUNCATED);
    }

    if (parser->builder != NULL && parser->err_code == QSJ4C_ERROR_NO_ERROR) {
        size_type* obj_data = QSJ4C_first_pass_fetch_stats_buffer(parser, storage_pos);
        if (obj_data != NULL) {
            *obj_data = member_count;
        }
    }
}

static void QSJ4C_first_pass_array( QSJ4C_First_pass_parser* parser, int depth ) {
    char json_char;
    size_type member_count = 0;
    size_t storage_pos = parser->storage_counter;
    parser->storage_counter++;

    if (parser->max_depth < depth) {
        QSJ4C_first_pass_parser_set_error(parser, QSJ4C_ERROR_DEPTH_OVERFLOW);
        return;
    }

    QSJ4C_first_pass_skip_whitespaces_and_comments(parser);
    json_char = QSJ4C_json_message_peek(parser->msg);
    if (json_char != ']') {
        QSJ4C_first_pass_process(parser, depth + 1);
        QSJ4C_first_pass_skip_whitespaces_and_comments(parser);
        json_char = QSJ4C_json_message_peek(parser->msg);
        member_count = 1;
        while (json_char == ',') {
            QSJ4C_json_message_forward(parser->msg);
            QSJ4C_first_pass_skip_whitespaces_and_comments(parser);
            json_char = QSJ4C_json_message_peek(parser->msg);
            if (json_char != ']') {
                member_count += 1;
                QSJ4C_first_pass_process(parser, depth + 1);
                QSJ4C_first_pass_skip_whitespaces_and_comments(parser);
                json_char = QSJ4C_json_message_peek(parser->msg);
            } else if (parser->strict_parsing) {
                QSJ4C_first_pass_parser_set_error(parser, QSJ4C_ERROR_TRAILING_COMMA);
            }
        }
        if (json_char != ']') {
            QSJ4C_first_pass_parser_set_error(parser, QSJ4C_ERROR_MISSING_COMMA);
        }
    }

    QSJ4C_json_message_forward(parser->msg);

    if (parser->builder != NULL && parser->err_code == QSJ4C_ERROR_NO_ERROR) {
        size_type* obj_data = QSJ4C_first_pass_fetch_stats_buffer( parser, storage_pos );
        if ( obj_data != NULL ) {
            *obj_data = member_count;
        }
    }
}

static void QSJ4C_first_pass_string( QSJ4C_First_pass_parser* parser ) {
    char json_char;
    size_type chars = 0;

    json_char = QSJ4C_json_message_read(parser->msg);
    while (json_char != '\0' && json_char != '"') {
        if (json_char == '\\') {
            json_char = QSJ4C_json_message_read(parser->msg);
            switch (json_char) {
            case 'u':
                chars += QSJ4C_first_pass_utf16(parser) - 1;
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
                QSJ4C_first_pass_parser_set_error(parser, QSJ4C_ERROR_INVALID_ESCAPE_SEQUENCE);
                return;
            }
        } else if (((uint8_t)json_char) < 32) {
            QSJ4C_first_pass_parser_set_error(parser, QSJ4C_ERROR_UNEXPECTED_CHAR);
        }
        ++chars;
        json_char = QSJ4C_json_message_read(parser->msg);
    }

    if (json_char != '"') {
        QSJ4C_first_pass_parser_set_error(parser, QSJ4C_ERROR_JSON_MESSAGE_TRUNCATED);
        return;
    }

    parser->complete_string_length += chars + 1; /* count the \0 to the complete string length! */

    if (parser->builder != NULL && chars > 0) {
    	size_t storage_pos = parser->string_storage_counter;
    	parser->string_storage_counter++;
        size_type* string_data = QSJ4C_first_pass_fetch_string_stats_buffer(parser, storage_pos);
        if (string_data != NULL) {
            *string_data = chars;
        }
    }
}

static uint32_t QSJ4C_first_pass_4digits( QSJ4C_First_pass_parser* parser ) {
    uint32_t value = 0;
    int char_count;

    for (char_count = 0; char_count < 4; char_count++) {
        char json_char = QSJ4C_json_message_read(parser->msg);
        uint8_t xdigit = QSJ4C_xdigit(json_char);

        if (xdigit > 0xF) {
            QSJ4C_first_pass_parser_set_error(parser, QSJ4C_ERROR_INVALID_UNICODE_SEQUENCE);
        }
        value = value << 4 | QSJ4C_xdigit(json_char);
    }

    return value;
}

static int QSJ4C_first_pass_utf16( QSJ4C_First_pass_parser* parser) {
    int amount_utf8_chars = 0;
    uint32_t value = QSJ4C_first_pass_4digits(parser);

    if ( value < 0x80 ) { /* [0, 0x80) */
        amount_utf8_chars = 1;
    } else if ( value < 0x800 ) { /* [0x80, 0x800) */
        amount_utf8_chars = 2;
    } else if (value < 0xd800 || value > 0xdfff) { /* [0x800, 0xd800) or (0xdfff, 0xffff] */
        amount_utf8_chars = 3;
    } else if (value <= 0xdbff) { /* [0xd800,0xdbff] */
        if (QSJ4C_json_message_read(parser->msg) != '\\' || QSJ4C_json_message_read(parser->msg) != 'u') {
            QSJ4C_first_pass_parser_set_error(parser, QSJ4C_ERROR_INVALID_UNICODE_SEQUENCE);
        }
        else {
            uint32_t low_surrogate = QSJ4C_first_pass_4digits(parser);
            /* check valid surrogate range */
            if ( low_surrogate < 0xdc00 || low_surrogate > 0xdfff) {
                QSJ4C_first_pass_parser_set_error(parser, QSJ4C_ERROR_INVALID_UNICODE_SEQUENCE);
            }
        }
        amount_utf8_chars = 4;
    } else { /* [0xdc00, 0xdfff] */
        /* invalid (low-surrogate before high-surrogate) */
        QSJ4C_first_pass_parser_set_error(parser, QSJ4C_ERROR_INVALID_UNICODE_SEQUENCE);
    }

    return amount_utf8_chars;
}

static void QSJ4C_first_pass_numeric_value( QSJ4C_First_pass_parser* parser ) {
    char json_char = QSJ4C_json_message_peek(parser->msg);

    if ( json_char == '-' ) {
        json_char = QSJ4C_json_message_forward_and_peek(parser->msg);
    } else if (json_char == '+') {
        if (parser->strict_parsing) {
            QSJ4C_first_pass_parser_set_error(parser, QSJ4C_ERROR_INVALID_NUMBER_FORMAT);
        } else {
            json_char = QSJ4C_json_message_forward_and_peek(parser->msg);
        }
    }

	if (!QSJ4C_is_digit(json_char)) {
		QSJ4C_first_pass_parser_set_error(parser, QSJ4C_ERROR_INVALID_NUMBER_FORMAT);
	} else if (json_char == '0' && parser->strict_parsing) {
        json_char = QSJ4C_json_message_forward_and_peek(parser->msg);
        /* next char is not allowed to be numeric! */
        if (QSJ4C_is_digit(json_char)) {
            QSJ4C_first_pass_parser_set_error(parser, QSJ4C_ERROR_INVALID_NUMBER_FORMAT);
        }
    }

    while (QSJ4C_is_digit(json_char)) {
        json_char = QSJ4C_json_message_forward_and_peek(parser->msg);
    }

    if (QSJ4C_is_float_separation_char(json_char)) {
        /* check the format! */
        if (json_char == '.') {
            json_char = QSJ4C_json_message_forward_and_peek(parser->msg);
            /* expect at least one digit! */
            if (!QSJ4C_is_digit(json_char)) {
                QSJ4C_first_pass_parser_set_error(parser, QSJ4C_ERROR_INVALID_NUMBER_FORMAT);
            }
            while (QSJ4C_is_digit(json_char)) {
                json_char = QSJ4C_json_message_forward_and_peek(parser->msg);
            }
        }
        if (json_char == 'E' || json_char == 'e') {
            /* next char can be a + or - */
            json_char = QSJ4C_json_message_forward_and_peek(parser->msg);
            if (json_char == '+' || json_char == '-') {
                json_char = QSJ4C_json_message_forward_and_peek(parser->msg);
            }
            /* expect at least one digit! */
            if (!QSJ4C_is_digit(json_char)) {
                QSJ4C_first_pass_parser_set_error(parser, QSJ4C_ERROR_INVALID_NUMBER_FORMAT);
            }
            /* forward until the end! */
            while (QSJ4C_is_digit(json_char)) {
                json_char = QSJ4C_json_message_forward_and_peek(parser->msg);
            }
        }
    }
}

static void QSJ4C_first_pass_constant( QSJ4C_First_pass_parser* parser, const char* str, size_t len ) {
    size_t i;
    for (i = 0; i < len; i++) {
		char c = QSJ4C_json_message_read(parser->msg);
        if (c != str[i]) {
            QSJ4C_first_pass_parser_set_error(parser, QSJ4C_ERROR_UNEXPECTED_CHAR);
        }
    }
}

static void QSJ4C_second_pass_parser_init( QSJ4C_Second_pass_parser* me, QSJ4C_First_pass_parser* parser ) {
    QSJ4C_Builder* builder = parser->builder;
    size_t required_object_storage = parser->amount_nodes * sizeof(QSJ4C_Value);
    size_t required_tempoary_storage = parser->storage_counter * sizeof(size_type);
    size_t copy_to_index = required_object_storage - required_tempoary_storage;
    size_t i = 0;
    size_t n = parser->string_storage_counter / 2;

	builder->buffer[copy_to_index] = builder->buffer[0];
    memmove(builder->buffer + copy_to_index, builder->buffer, required_tempoary_storage);
    me->json_char = parser->msg->json;
    me->builder = parser->builder;
    me->realloc_callback = parser->realloc_callback;
    me->curr_object_pos = copy_to_index;

    /* reset the builder to its original state! */
    QSJ4C_builder_init(builder, builder->buffer, builder->buffer_size);
    builder->cur_str_pos = required_object_storage;

    copy_to_index = builder->buffer_size - sizeof(size_type) * parser->string_storage_counter;

    /* Now reverse the string buffer part (as it is in reverse order) */
    for( i = 0; i < n; ++i )
    {
    	size_t a_index = copy_to_index + (i * 2);
    	size_t b_index = builder->buffer_size - ((i + 1) * 2);
    	/* prevent compiler to optimize away the swap using volatile */
    	volatile size_type* lhs = (size_type*)&builder->buffer[a_index];
    	volatile size_type* rhs = (size_type*)&builder->buffer[b_index];
    	size_type tmp = *lhs;
    	*lhs = *rhs;
    	*rhs = tmp;
    }

    me->curr_string_pos = copy_to_index;
}

static void QSJ4C_second_pass_process( QSJ4C_Second_pass_parser* me, QSJ4C_Value* result_ptr ) {
    /* skip those stupid whitespaces! */
    me->json_char = QSJ4C_skip_whitespaces_and_comments_second_pass(me->json_char);
    switch (*me->json_char) {
    case '{':
        ++me->json_char;
        QSJ4C_second_pass_object(me, result_ptr);
        break;
    case '[':
        ++me->json_char;
        QSJ4C_second_pass_array(me, result_ptr);
        break;
    case '"':
        ++me->json_char;
        QSJ4C_second_pass_string(me, result_ptr);
        break;
    case 't':
        QSJ4C_set_bool(result_ptr, true);
        me->json_char += QSJ4C_TRUE_STR_LEN; /* jump to the end of the constant */
        break;
    case 'f':
        QSJ4C_set_bool(result_ptr, false);
        me->json_char += QSJ4C_FALSE_STR_LEN; /* jump to the end of the constant */
        break;
    case 'n':
        QSJ4C_set_null(result_ptr);
        me->json_char += QSJ4C_NULL_STR_LEN; /* jump to the end of the constant */
        break;
    default: /* As first pass was successful, it can only be a numeric value */
        QSJ4C_second_pass_numeric_value(me, result_ptr);
        break;
    }
}

static void QSJ4C_second_pass_object( QSJ4C_Second_pass_parser* me, QSJ4C_Value* result_ptr  ) {
    size_type elements = QSJ4C_second_pass_fetch_stats_data(me);
    size_type index;
    QSJ4C_Member* top_ptr = (QSJ4C_Member*)(&me->builder->buffer[me->builder->cur_obj_pos]);


    /*
     * Do not use set_object as it would initialize memory and thus corrupt the buffer
     * that stores string sizes and integer types
     */
	QSJ4C_set_object_data(result_ptr, top_ptr, elements);
    me->builder->cur_obj_pos += sizeof(QSJ4C_Member) * elements;

    me->json_char = QSJ4C_skip_whitespaces_and_comments_second_pass(me->json_char);
    for (index = 0; index < elements; ++index) {
        if (*me->json_char == ',') {
            ++me->json_char;
            me->json_char = QSJ4C_skip_whitespaces_and_comments_second_pass(me->json_char);
        }
        ++me->json_char; /* skip the first " */
		QSJ4C_second_pass_string(me, (QSJ4C_Value*) QSJ4C_member_get_key(&top_ptr[index]));
        me->json_char = QSJ4C_skip_whitespaces_and_comments_second_pass(me->json_char);
        ++me->json_char; /* skip the : */
        me->json_char = QSJ4C_skip_whitespaces_and_comments_second_pass(me->json_char);
		QSJ4C_second_pass_process(me, (QSJ4C_Value*) QSJ4C_member_get_value(&top_ptr[index]));
        me->json_char = QSJ4C_skip_whitespaces_and_comments_second_pass(me->json_char);
    }

    while( *me->json_char != '}') {
        me->json_char += 1;
        me->json_char = QSJ4C_skip_whitespaces_and_comments_second_pass(me->json_char);
    }

    ++me->json_char; /* walk over the } */
}

static void QSJ4C_second_pass_array( QSJ4C_Second_pass_parser* me, QSJ4C_Value* result_ptr ) {
    size_type elements = QSJ4C_second_pass_fetch_stats_data(me);
    size_type index;
    QSJ4C_Value* top_ptr = (QSJ4C_Value*)(&me->builder->buffer[me->builder->cur_obj_pos]);

    /*
     * Do not use set_array as it would initialize memory and thus corrupt the buffer
     * that stores string sizes and integer types
     */
    QSJ4C_set_array_data(result_ptr, top_ptr, elements);

    me->builder->cur_obj_pos += sizeof(QSJ4C_Value) * elements;

    me->json_char = QSJ4C_skip_whitespaces_and_comments_second_pass(me->json_char);
    for (index = 0; index < elements; index++) {
        if (*me->json_char == ',') {
            ++me->json_char;
            me->json_char = QSJ4C_skip_whitespaces_and_comments_second_pass(
                    me->json_char);
        }
        QSJ4C_second_pass_process(me, &top_ptr[index]);
        me->json_char = QSJ4C_skip_whitespaces_and_comments_second_pass(me->json_char);
    }

    while (*me->json_char != ']') {
        me->json_char += 1;
        me->json_char = QSJ4C_skip_whitespaces_and_comments_second_pass(me->json_char);
    }
    ++me->json_char; /* walk over the ] */
}

static void QSJ4C_second_pass_string( QSJ4C_Second_pass_parser* me, QSJ4C_Value* result_ptr  ) {
    char* put_str = NULL;

	if (*me->json_char == '"') {
		/* empty string */
		QSJ4C_set_string_data(result_ptr, NULL, 0);
	} else {
	    size_type str_len = QSJ4C_second_pass_fetch_string_stats_data(me);
	    size_type pos = 0;

		put_str = (char*) &me->builder->buffer[me->builder->cur_str_pos];
		QSJ4C_set_string_data(result_ptr, put_str, str_len);
		me->builder->cur_str_pos += str_len + 1;

		while( pos < str_len ) {
			pos += QSJ4C_second_pass_read_char( me, &put_str[pos] );
			me->json_char += 1;
		}
		put_str[str_len] = '\0';
	}
	++me->json_char; /* skip " */
}

static size_type QSJ4C_second_pass_read_char( QSJ4C_Second_pass_parser* me, char* out ) {
	size_type written = 1;
    if (*me->json_char == '\\') {
        me->json_char += 1;
        switch (*me->json_char) {
        case '"':
        	*out = '"';
            break;
        case '\\':
        	*out = '\\';
            break;
        case '/':
        	*out = '/';
            break;
        case 'b':
        	*out = '\b';
            break;
        case 'f':
        	*out = '\f';
            break;
        case 'n':
        	*out = '\n';
            break;
        case 'r':
        	*out = '\r';
            break;
        case 't':
        	*out = '\t';
            break;
        default: /* is has to be u */
			written = QSJ4C_second_pass_unicode_sequence(me, out);
            break;
        }
	} else {
		*out = *me->json_char;
	}
    return written;
}

static size_type QSJ4C_second_pass_unicode_sequence( QSJ4C_Second_pass_parser* me, char* out )
{
    static const uint8_t FIRST_BYTE_TWO_BYTE_MARK_MASK = 0xC0;
    static const uint8_t FIRST_BYTE_TWO_BYTE_PAYLOAD_MASK = 0x1F;
    static const uint8_t FIRST_BYTE_THREE_BYTE_MARK_MASK = 0xE0;
    static const uint8_t FIRST_BYTE_THREE_BYTE_PAYLOAD_MASK = 0x0F;
    static const uint8_t FIRST_BYTE_FOUR_BYTE_MARK_MASK = 0xF0;
    static const uint8_t FIRST_BYTE_FOUR_BYTE_PAYLOAD_MASK = 0x07;
    static const uint8_t FOLLOW_BYTE_MARK_MASK = 0x80;
    static const uint8_t FOLLOW_BYTE_PAYLOAD_MASK = 0x3F;

    char* write_ptr = out;

    uint32_t utf16;
    me->json_char += 1;
    /* utf-8 conversion inspired by parson */
    utf16 = QSJ4C_second_pass_utf16(me);
    if (utf16 < 0x80) {
        *write_ptr = (char)utf16;
    } else if (utf16 < 0x800) {
    	*write_ptr = ((utf16 >> 6) & FIRST_BYTE_TWO_BYTE_PAYLOAD_MASK) | FIRST_BYTE_TWO_BYTE_MARK_MASK;
    	write_ptr++;
        *write_ptr = (utf16 & 0x3f) | FOLLOW_BYTE_MARK_MASK;
    } else if (utf16 < 0x010000) {
        *write_ptr = ((utf16 >> 12) & FIRST_BYTE_THREE_BYTE_PAYLOAD_MASK) | FIRST_BYTE_THREE_BYTE_MARK_MASK;
    	write_ptr++;
        *write_ptr = ((utf16 >> 6) & FOLLOW_BYTE_PAYLOAD_MASK) | FOLLOW_BYTE_MARK_MASK;
    	write_ptr++;
        *write_ptr = ((utf16) & FOLLOW_BYTE_PAYLOAD_MASK) | FOLLOW_BYTE_MARK_MASK;
    } else {
        *write_ptr = (((utf16 >> 18) & FIRST_BYTE_FOUR_BYTE_PAYLOAD_MASK) | FIRST_BYTE_FOUR_BYTE_MARK_MASK);
    	write_ptr++;
        *write_ptr = (((utf16 >> 12) & FOLLOW_BYTE_PAYLOAD_MASK) | FOLLOW_BYTE_MARK_MASK);
    	write_ptr++;
        *write_ptr = (((utf16 >> 6) & FOLLOW_BYTE_PAYLOAD_MASK) | FOLLOW_BYTE_MARK_MASK);
    	write_ptr++;
        *write_ptr = (((utf16) & FOLLOW_BYTE_PAYLOAD_MASK) | FOLLOW_BYTE_MARK_MASK);
    }
    return (write_ptr - out) + 1;
}

static uint32_t QSJ4C_second_pass_utf16( QSJ4C_Second_pass_parser* me ) {
    uint32_t value = QSJ4C_xdigit(me->json_char[0]) << 12 | QSJ4C_xdigit(me->json_char[1]) << 8 | QSJ4C_xdigit(me->json_char[2]) << 4 | QSJ4C_xdigit(me->json_char[3]);
    me->json_char += 4;
    if (value >= 0xd800 && value <= 0xdbff) {
        uint32_t low_surrogate;
        me->json_char += 2;
        low_surrogate = QSJ4C_xdigit(me->json_char[0]) << 12 | QSJ4C_xdigit(me->json_char[1]) << 8 | QSJ4C_xdigit(me->json_char[2]) << 4 | QSJ4C_xdigit(me->json_char[3]);
        value = ((((value-0xD800)&0x3FF)<<10)|((low_surrogate-0xDC00)&0x3FF))+0x010000;
        me->json_char += 4;
    }
    /* set pointer to last char */
    me->json_char -= 1;
    return value;
}

static void QSJ4C_second_pass_numeric_value( QSJ4C_Second_pass_parser* me, QSJ4C_Value* result_ptr ) {
    char* c = (char*)me->json_char;
    bool float_value = false;
    if (*me->json_char == '-') {
        int64_t i = QSJ4C_STRTOL(me->json_char, &c, 10);
        if (QSJ4C_is_float_separation_char(*c) || i < INT32_MIN) {
            float_value = true;
        } else {
            QSJ4C_set_int(result_ptr, i);
        }
    } else {
        uint64_t i = QSJ4C_STRTOUL(me->json_char, &c, 10);
        if (QSJ4C_is_float_separation_char(*c) || i > UINT32_MAX) {
            float_value = true;
        } else {
            QSJ4C_set_uint(result_ptr, i);
        }

    }
    if (float_value) {
        QSJ4C_set_float(result_ptr, QSJ4C_STRTOD(me->json_char, &c));
    }
    me->json_char = c;
}

static size_type QSJ4C_second_pass_fetch_stats_data( QSJ4C_Second_pass_parser* me ) {
    if (QSJ4C_UNLIKELY(me->curr_object_pos <= me->builder->cur_obj_pos - sizeof(size_type))) {
        /*
         * Corner case that the last entry is empty. In some cases this causes that
         * the statistics are already overwritten (see Corner-case tests)
         */
        return 0;
    }
    size_type data = *((size_type*)(me->builder->buffer + me->curr_object_pos));
    me->curr_object_pos += sizeof(size_type);
    return data;
}

static size_type QSJ4C_second_pass_fetch_string_stats_data( QSJ4C_Second_pass_parser* me ) {
    size_type data = *((size_type*)(me->builder->buffer + me->curr_string_pos));
    me->curr_string_pos += sizeof(size_type);
    return data;
}


static char QSJ4C_json_message_peek( QSJ4C_Json_message* msg ) {
    /* Also very unlikely to happen (only in case json is invalid) */
    return QSJ4C_UNLIKELY(msg->json_pos >= msg->json_len) ? '\0' : msg->json[msg->json_pos];
}

static char QSJ4C_json_message_read( QSJ4C_Json_message* msg ) {
    char result = QSJ4C_json_message_peek(msg);
    if (QSJ4C_UNLIKELY(result == '\0') && msg->json_pos < msg->json_len ) {
        msg->json_len = msg->json_pos;
    }
    msg->json_pos += 1;
    return result;
}

static void QSJ4C_json_message_forward( QSJ4C_Json_message* msg ) {
    msg->json_pos += 1;
}

static char QSJ4C_json_message_forward_and_peek( QSJ4C_Json_message* msg ) {
    msg->json_pos += 1;
    return QSJ4C_json_message_peek(msg);
}

static void QSJ4C_first_pass_skip_whitespaces_and_comments( QSJ4C_First_pass_parser* parser ) {
    char current_char = QSJ4C_json_message_peek(parser->msg);

    while (current_char != '\0') {
        switch (current_char) {
        case '\t':
        case '\n':
        case '\b':
        case '\r':
        case ' ':
            break;
        case '/': /* also skip comments! */
            QSJ4C_first_pass_skip_comment(parser);
            break;
        default:
            return;
        }
        current_char = QSJ4C_json_message_forward_and_peek(parser->msg);
    }
}

static void QSJ4C_first_pass_skip_comment( QSJ4C_First_pass_parser* parser )
{
    char current_char = QSJ4C_json_message_forward_and_peek(parser->msg);
    if (current_char == '*') {
        QSJ4C_json_message_forward(parser->msg);
        do {
            current_char = QSJ4C_json_message_read(parser->msg);
        } while (current_char != '\0' && !(current_char == '*' && QSJ4C_json_message_peek(parser->msg) == '/'));
    } else if (current_char == '/') {
        do {
            current_char = QSJ4C_json_message_forward_and_peek(parser->msg);
        } while (current_char != '\0' && current_char != '\n');
    } else {
    	QSJ4C_first_pass_parser_set_error(parser, QSJ4C_ERROR_UNEXPECTED_CHAR);
    }
}

static const char* QSJ4C_skip_whitespaces_and_comments_second_pass( const char* json ) {
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

static QSJ4C_Value* QSJ4C_create_error_description( QSJ4C_First_pass_parser* parser ) {
	uint32_t* ptr;
    QSJ4C_Value* document;
    /* not enough space to store error information in buffer... */
    if (parser->builder->buffer_size < sizeof(QSJ4C_Value)) {
        return NULL;
    }

    QSJ4C_builder_init(parser->builder, parser->builder->buffer, parser->builder->buffer_size);
    document = QSJ4C_builder_get_document(parser->builder);

    document->buff[0] = QSJ4C_TYPE_INVALID;
    document->buff[1] = parser->err_code;
    ptr = (uint32_t*)&document->buff[2];
    *ptr = parser->msg->json_len;

    parser->builder->cur_obj_pos = sizeof(QSJ4C_Value);

    return document;
}

static void* QSJ4C_allocate( QSJ4C_Builder* builder, QSJ4C_First_pass_parser* parser ) {
	size_t required_size = QSJ4C_calculate_max_buffer_parser(parser);
    size_t required_tempoary_storage = parser->string_storage_counter* sizeof(size_type);
    void *tmp = NULL;

    if (parser->realloc_callback == NULL) {
        QSJ4C_first_pass_parser_set_error(parser, QSJ4C_ERROR_STORAGE_BUFFER_TO_SMALL);
        return NULL;
    }

    tmp = parser->realloc_callback(builder->buffer, required_size);
    if (tmp == NULL) {
        QSJ4C_first_pass_parser_set_error(parser, QSJ4C_ERROR_ALLOCATION_ERROR);
        return NULL;
    }
    builder->buffer = tmp;

    if ( required_tempoary_storage > 0 ) {
    	size_t dst_index = required_size - required_tempoary_storage;
    	size_t src_index = builder->buffer_size - required_tempoary_storage;

        /* Now move the string storage to the new end! */
        memmove(builder->buffer + dst_index, builder->buffer + src_index, required_tempoary_storage);
    }

    builder->buffer_size = required_size;
    return tmp;
}

static bool QSJ4C_first_pass_needs_reallocation( QSJ4C_First_pass_parser* parser ) {
	return (parser->highest_storage_counter + parser->highest_string_storage_counter + 2) * sizeof(size_type) > parser->builder->buffer_size;
}


static size_type* QSJ4C_first_pass_fetch_stats_buffer( QSJ4C_First_pass_parser* parser, size_t storage_pos ) {
    QSJ4C_Builder* builder = parser->builder;
    size_t in_buffer_pos = storage_pos * sizeof(size_type);
	if (storage_pos > parser->highest_storage_counter) {
		parser->highest_storage_counter = storage_pos;
	    if (QSJ4C_first_pass_needs_reallocation(parser) && QSJ4C_allocate(builder, parser) == NULL) {
	    	return NULL;
	    }
	}

    return (size_type*)&builder->buffer[in_buffer_pos];
}

static size_type* QSJ4C_first_pass_fetch_string_stats_buffer( QSJ4C_First_pass_parser* parser, size_t storage_pos ) {
    QSJ4C_Builder* builder = parser->builder;
    size_t in_buffer_pos;

	if (storage_pos > parser->highest_string_storage_counter) {
		parser->highest_string_storage_counter = storage_pos;
	    if (QSJ4C_first_pass_needs_reallocation(parser) && QSJ4C_allocate(builder, parser) == NULL) {
	    	return NULL;
	    }
	}

	in_buffer_pos = parser->builder->buffer_size - (storage_pos + 1 ) * sizeof(size_type);
    return (size_type*)&builder->buffer[in_buffer_pos];
}

static bool QSJ4C_builder_validate_buffer( QSJ4C_Builder* builder ) {
    return builder->cur_obj_pos - 1 <= builder->cur_str_pos;
}

QSJ4C_Value* QSJ4C_builder_pop_values( QSJ4C_Builder* builder, size_type count ) {
    QSJ4C_Value* new_pointer;
    size_type i;
    if (count == 0) {
        return NULL;
    }
    new_pointer = (QSJ4C_Value*)(&builder->buffer[builder->cur_obj_pos]);
    builder->cur_obj_pos += count * sizeof(QSJ4C_Value);

    QSJ4C_ASSERT(QSJ4C_builder_validate_buffer(builder), {return NULL;});

    for (i = 0; i < count; i++) {
		QSJ4C_set_type(new_pointer + i, QSJ4C_TYPE_NULL);
    }
    return new_pointer;
}

uint8_t QSJ4C_get_compatibility_types( const QSJ4C_Value* value_ptr ) {
	return value_ptr->buff[1] & 0xF;
}

uint8_t QSJ4C_get_storage_type( QSJ4C_Value* value_ptr ) {
	return (value_ptr->buff[1] >> 4) & 0xF;
}

void QSJ4C_set_type(QSJ4C_Value* value_ptr, QSJ4C_TYPE type) {
	value_ptr->buff[0] = (uint8_t) type;
}

void QSJ4C_set_object_data(QSJ4C_Value* value_ptr, QSJ4C_Member* top_ptr, uint16_t count) {
	uint32_t offset = ((QSJ4C_Value*)top_ptr) - (value_ptr + 1);
	uint16_t* ptr;
	QSJ4C_set_type(value_ptr, QSJ4C_TYPE_OBJECT);
	if ( count > 0 ) {
		QSJ4C_ASSERT( offset <= 0xFFFFFF, {} );
		QSJ4C_write_uint24(&value_ptr->buff[1], offset);
	}
	ptr = (uint16_t*)&value_ptr->buff[4];
	*ptr = count;
}

void QSJ4C_set_array_data(QSJ4C_Value* value_ptr, QSJ4C_Value* top_ptr, uint16_t count) {
	uint32_t offset = top_ptr - (value_ptr + 1);
	uint16_t* ptr;
	QSJ4C_set_type(value_ptr, QSJ4C_TYPE_ARRAY);
	if ( count > 0 ) {
		QSJ4C_ASSERT( offset <= 0xFFFFFF, {} );
		QSJ4C_write_uint24(&value_ptr->buff[1], offset);
	}
	ptr = (uint16_t*)&value_ptr->buff[4];
	*ptr = count;
}

void QSJ4C_set_string_data(QSJ4C_Value* value_ptr, char* str_ptr, uint16_t count) {
	uint32_t offset = str_ptr - (char*) (value_ptr + 1);
	uint16_t* ptr;
	QSJ4C_set_type(value_ptr, QSJ4C_TYPE_STRING);
	if (str_ptr == NULL) {
		offset = 0;
	}
	QSJ4C_ASSERT( offset <= 0xFFFFFF, {} );
	QSJ4C_write_uint24(&value_ptr->buff[1], offset);
	ptr = (uint16_t*)&value_ptr->buff[4];
	*ptr = count;
}

static size_t QSJ4C_copy_simple_string(char* buffer, size_t buffer_length, const char* string, size_t string_length) {
    size_t index = 0;
    size_t copy_length = string_length > buffer_length ? (buffer_length) : (string_length);

    for (index = 0; index < copy_length; index++) {
        buffer[index] = string[index];
    }
    return index;
}

/*
 * This method does not only copy the string ... but also replaces all control chars.
 */
static size_t QSJ4C_copy_custom_string(char* buffer, size_t buffer_size, QSJ4C_Value* value_ptr) {
    size_t string_index = 0;
    size_t buffer_index = 0;
    static const char* replacement_buf[] = {
            "\\u0000", "\\u0001", "\\u0002", "\\u0003", "\\u0004", "\\u0005", "\\u0006", "\\u0007", "\\b",     "\\t",     "\\n",     "\\u000b", "\\f",     "\\r",     "\\u000e", "\\u000f",
            "\\u0010", "\\u0011", "\\u0012", "\\u0013", "\\u0014", "\\u0015", "\\u0016", "\\u0017", "\\u0018", "\\u0019", "\\u001a", "\\u001b", "\\u001c", "\\u001d", "\\u001e", "\\u001f"
    };
    static const uint8_t replacement_len[] = {
            6,         6,         6,         6,         6,         6,         6,         6,         2,         2,         2,         6,          2,        2,         6,         6,
            6,         6,         6,         6,         6,         6,         6,         6,         6,         6,         6,         6,          6,        6,         6,         6
    };

    const char* string = QSJ4C_get_string(value_ptr);
    size_type string_length = QSJ4C_get_string_length(value_ptr);

    buffer[buffer_index] = '"';
    buffer_index++;

    for( string_index = 0; string_index < string_length && buffer_index < buffer_size; ++string_index) {
        const char* replacement_string = NULL;
        uint8_t replacement_string_len = 0;
        uint8_t char_value = string[string_index];

        switch(char_value) {
        case '"':
            replacement_string = "\\\"";
            replacement_string_len = 2;
            break;
        case '\\':
            replacement_string = "\\\\";
            replacement_string_len = 2;
            break;
        case '/':
            replacement_string = "\\/";
            replacement_string_len = 2;
            break;

        default:
            if (QSJ4C_UNLIKELY(char_value < 32)) {
                replacement_string = replacement_buf[char_value];
                replacement_string_len = replacement_len[char_value];
            }
        }

        if (replacement_string != NULL) {
            buffer_index += QSJ4C_copy_simple_string(buffer + buffer_index, buffer_size - buffer_index, replacement_string, replacement_string_len);
        } else {
            buffer[buffer_index] = string[string_index];
            buffer_index++;
        }
    }

    if (buffer_index >= buffer_size) {
        return buffer_index;
    }
    buffer[buffer_index] = '"';
    buffer_index++;

    return buffer_index;
}

size_t QSJ4C_sprint_impl( QSJ4C_Value* value_ptr, char* buffer, size_t buffer_size, size_t index ) {
	size_t buffer_index = index;

    if (buffer_index >= buffer_size) {
        return buffer_index;
    }
	switch (QSJ4C_get_type(value_ptr)) {
    case QSJ4C_TYPE_OBJECT:
        buffer_index = QSJ4C_sprint_object(value_ptr, buffer, buffer_size, index);
        break;
    case QSJ4C_TYPE_ARRAY:
        buffer_index = QSJ4C_sprint_array(value_ptr, buffer, buffer_size, index);
        break;
    case QSJ4C_TYPE_NUMBER:
        buffer_index = QSJ4C_sprint_primitive(value_ptr, buffer, buffer_size, index);
        break;
    case QSJ4C_TYPE_BOOL:
    	buffer_index = QSJ4C_sprint_bool(value_ptr, buffer, buffer_size, index);
    	break;
    case QSJ4C_TYPE_NULL:
        buffer_index += QSJ4C_copy_simple_string(buffer + buffer_index, buffer_size - buffer_index, QSJ4C_NULL_STR, QSJ4C_NULL_STR_LEN);
        break;
    case QSJ4C_TYPE_STRING:
        buffer_index += QSJ4C_copy_custom_string(buffer + buffer_index, buffer_size - buffer_index, value_ptr);
        break;
    case QSJ4C_TYPE_INVALID:
        buffer_index += QSJ4C_SNPRINTF(buffer + buffer_index, buffer_size - buffer_index, "{\"error\":\"Unable to parse json message. Error (%d) at position %zu\"}", QSJ4C_error_get_errno(value_ptr), QSJ4C_error_get_json_pos(value_ptr));
        break;
    default:
        g_QSJ4C_err_function();
    }

    return buffer_index;
}


size_t QSJ4C_sprint_object( QSJ4C_Value* value_ptr, char* buffer, size_t buffer_size, size_t index ) {
	size_t buffer_index = index;
    size_type i;
	size_type n = QSJ4C_object_size(value_ptr);

    buffer[buffer_index] = '{';
    buffer_index++;
    for (i = 0; i < n; ++i) {
    	QSJ4C_Member* member = QSJ4C_object_get_member(value_ptr, i);
    	QSJ4C_Value* key = QSJ4C_member_get_key(member);
    	QSJ4C_Value* value = QSJ4C_member_get_value(member);
        if (buffer_index >= buffer_size) {
            return buffer_index;
        }
        if (!QSJ4C_is_null(key)) {
            if (i > 0) {
                buffer[buffer_index] = ',';
                buffer_index++;
            }
            buffer_index = QSJ4C_sprint_impl(key, buffer, buffer_size, buffer_index);
            if (buffer_index >= buffer_size) {
                return buffer_index;
            }
            buffer[buffer_index] = ':';
            buffer_index++;
            buffer_index = QSJ4C_sprint_impl(value, buffer, buffer_size, buffer_index);
        }
    }
    if (buffer_index >= buffer_size) {
        return buffer_index;
    }
    buffer[buffer_index] = '}';
    buffer_index++;
    return buffer_index;
}

size_t QSJ4C_sprint_array( QSJ4C_Value* value_ptr, char* buffer, size_t buffer_size, size_t index )
{
	size_t buffer_index = index;
    size_type i;
	size_type n = QSJ4C_array_size(value_ptr);

    buffer[buffer_index] = '[';
    buffer_index++;
    for (i = 0; i < n; i++) {
        if (buffer_index >= buffer_size) {
            return buffer_index;
        }
        if (i > 0) {
            buffer[buffer_index] = ',';
            buffer_index++;
        }
        buffer_index = QSJ4C_sprint_impl(QSJ4C_array_get(value_ptr, i), buffer, buffer_size, buffer_index);
    }
    if (buffer_index >= buffer_size) {
        return buffer_index;
    }
    buffer[buffer_index] = ']';
    buffer_index++;
    return buffer_index;
}

size_t QSJ4C_sprint_bool( QSJ4C_Value* value_ptr, char* buffer, size_t buffer_size, size_t index ) {
	bool value = QSJ4C_get_bool(value_ptr);
	size_t buffer_index = index;

	if (value) {
        buffer_index += QSJ4C_copy_simple_string(buffer + buffer_index, buffer_size - buffer_index, QSJ4C_TRUE_STR, QSJ4C_TRUE_STR_LEN);
    } else {
        buffer_index += QSJ4C_copy_simple_string(buffer + buffer_index, buffer_size - buffer_index, QSJ4C_FALSE_STR, QSJ4C_FALSE_STR_LEN);
	}

	return buffer_index;
}

size_t QSJ4C_sprint_primitive( QSJ4C_Value* value_ptr, char* buffer, size_t buffer_size, size_t index )
{
	size_t buffer_index = index;
    switch (QSJ4C_get_storage_type(value_ptr)) {
    case QSJ4C_PRIMITIVE_INT:
        buffer_index += QSJ4C_ITOSTRN(buffer + buffer_index, buffer_size - buffer_index, QSJ4C_get_int(value_ptr));
        break;
    case QSJ4C_PRIMITIVE_UINT:
        buffer_index += QSJ4C_UTOSTRN(buffer + buffer_index, buffer_size - buffer_index, QSJ4C_get_uint(value_ptr));
        break;
    default: /* it has to be float */
        buffer_index = QSJ4C_sprint_float(QSJ4C_get_float(value_ptr), buffer, buffer_size, buffer_index);
        break;
    }
    return buffer_index;
}

size_t QSJ4C_sprint_float( float f, char* buffer, size_t buffer_size, size_t index )
{
    /* printing floats inspired from cJSON */
	size_t buffer_index = index;
    bool exp = false;
    float absf = f < 0 ? -f: f;
    if ((f * 0) != 0) {
        buffer_index += QSJ4C_copy_simple_string(buffer + buffer_index, buffer_size - buffer_index, QSJ4C_NULL_STR, QSJ4C_NULL_STR_LEN);
    } else if ((absf < 1.0e-6) || (absf > 1.0e9)) {
        buffer_index += QSJ4C_SNPRINTF(buffer + buffer_index, buffer_size - buffer_index, "%e", f);
        exp = true;
    } else {
        buffer_index += QSJ4C_SNPRINTF(buffer + buffer_index, buffer_size - buffer_index, "%f", f);
    }

	if ( buffer_index >= buffer_size ) {
		buffer_index = buffer_size - 1;
	}
	if ( exp ) {
		/* verify that there is really an exponent available!*/
		size_t i = index;
		exp = false;
		for (; i < buffer_index; ++i) {
			if (buffer[i] == 'e') {
				exp = true;
				break;
			}
		}
	}

	if (!exp) {
		while (buffer[buffer_index - 1] == '0' && buffer[buffer_index - 2] != '.') {
			buffer_index--;
		}
	}

    return buffer_index;
}

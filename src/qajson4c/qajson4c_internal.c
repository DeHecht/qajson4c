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
    uint8_t integer_parse_base;

    bool fatal_fault;

    size_type curr_buffer_pos;
} QAJ4C_Second_pass_parser;


static void QAJ4C_std_err_function(void) {
	QAJ4C_raise(SIGABRT);
}

QAJ4C_fatal_error_fn QAJ4C_ERR_FUNCTION = &QAJ4C_std_err_function;

static void QAJ4C_first_pass_parser_init( QAJ4C_First_pass_parser* parser, QAJ4C_Builder* builder, QAJ4C_Json_message* msg, int opts, QAJ4C_realloc_fn realloc_callback );
static void QAJ4C_first_pass_parser_set_error( QAJ4C_First_pass_parser* parser, QAJ4C_ERROR_CODE error );
static void QAJ4C_first_pass_process( QAJ4C_First_pass_parser* parser, int depth );
static void QAJ4C_first_pass_object( QAJ4C_First_pass_parser* parser, int depth );
static void QAJ4C_first_pass_array( QAJ4C_First_pass_parser* parser, int depth );
static void QAJ4C_first_pass_string( QAJ4C_First_pass_parser* parser );
static void QAJ4C_first_pass_numeric_value( QAJ4C_First_pass_parser* parser );
static uint32_t QAJ4C_first_pass_4digits( QAJ4C_First_pass_parser* parser );
static int QAJ4C_first_pass_utf16( QAJ4C_First_pass_parser* parser );


static size_type* QAJ4C_first_pass_fetch_stats_buffer( QAJ4C_First_pass_parser* parser, size_type storage_pos );
static QAJ4C_Value* QAJ4C_create_error_description( QAJ4C_First_pass_parser* me );

size_t QAJ4C_calculate_max_buffer_parser( QAJ4C_First_pass_parser* parser );


static void QAJ4C_second_pass_parser_init( QAJ4C_Second_pass_parser* me, QAJ4C_First_pass_parser* parser);
static void QAJ4C_second_pass_process( QAJ4C_Second_pass_parser* me, QAJ4C_Value* result_ptr );
static void QAJ4C_second_pass_object( QAJ4C_Second_pass_parser* me, QAJ4C_Value* result_ptr );
static void QAJ4C_second_pass_array( QAJ4C_Second_pass_parser* me, QAJ4C_Value* result_ptr );
static void QAJ4C_second_pass_string( QAJ4C_Second_pass_parser* me, QAJ4C_Value* result_ptr );
static void QAJ4C_second_pass_numeric_value( QAJ4C_Second_pass_parser* me, QAJ4C_Value* result_ptr );
static uint32_t QAJ4C_second_pass_utf16( QAJ4C_Second_pass_parser* me );

static size_type QAJ4C_second_pass_fetch_stats_data( QAJ4C_Second_pass_parser* me );

static void QAJ4C_skip_whitespaces_and_comments( QAJ4C_Json_message* msg );
static const char* QAJ4C_skip_whitespaces_and_comments_second_pass( const char* json );


static char QAJ4C_json_message_peek( QAJ4C_Json_message* msg );
static char QAJ4C_json_message_read( QAJ4C_Json_message* msg );
static void QAJ4C_json_message_forward( QAJ4C_Json_message* msg );

static const char QAJ4C_NULL_STR[] = "null";
static const char QAJ4C_TRUE_STR[] = "true";
static const char QAJ4C_FALSE_STR[] = "false";
static const unsigned QAJ4C_NULL_STR_LEN = sizeof(QAJ4C_NULL_STR) / sizeof(QAJ4C_NULL_STR[0]);
static const unsigned QAJ4C_TRUE_STR_LEN = sizeof(QAJ4C_TRUE_STR) / sizeof(QAJ4C_TRUE_STR[0]);
static const unsigned QAJ4C_FALSE_STR_LEN = sizeof(QAJ4C_FALSE_STR) / sizeof(QAJ4C_FALSE_STR[0]);

static int QAJ4C_xdigit( char c ) {
	return (c > '9')? (c &~ 0x20) - 'A' + 10: (c - '0');
}


size_t QAJ4C_parse_generic(QAJ4C_Builder* builder, const char* json, size_t json_len, int opts, const QAJ4C_Value** result_ptr, QAJ4C_realloc_fn realloc_callback) {
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
		QAJ4C_skip_whitespaces_and_comments(parser.msg);
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

	if (second_parser.fatal_fault) {
		QAJ4C_first_pass_parser_set_error(&parser, QAJ4C_ERROR_FATAL_PARSER_ERROR);
    	*result_ptr = QAJ4C_create_error_description(&parser);
    	return builder->cur_obj_pos;
	}

    return builder->buffer_size;
}

size_t QAJ4C_calculate_max_buffer_parser( QAJ4C_First_pass_parser* parser ) {
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
	parser->optimize_object = (opts & QAJ4C_PARSE_OPTS_SORT_OBJECT_MEMBERS) != 0;
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
		parser->msg->json_len = parser->msg->json_pos;
	}
}

static void QAJ4C_first_pass_process( QAJ4C_First_pass_parser* parser, int depth) {
    QAJ4C_skip_whitespaces_and_comments(parser->msg);
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
    case '-':
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
    case 't':
    	QAJ4C_json_message_forward(parser->msg);
		if (QAJ4C_json_message_read(parser->msg) != 'r'
				|| QAJ4C_json_message_read(parser->msg) != 'u'
				|| QAJ4C_json_message_read(parser->msg) != 'e') {
    		QAJ4C_first_pass_parser_set_error(parser, QAJ4C_ERROR_UNEXPECTED_CHAR);
    	}
    	break;
    case 'f':
    	QAJ4C_json_message_forward(parser->msg);
		if (QAJ4C_json_message_read(parser->msg) != 'a'
				|| QAJ4C_json_message_read(parser->msg) != 'l'
				|| QAJ4C_json_message_read(parser->msg) != 's'
				|| QAJ4C_json_message_read(parser->msg) != 'e') {
    		QAJ4C_first_pass_parser_set_error(parser, QAJ4C_ERROR_UNEXPECTED_CHAR);
    	}
    	break;
    case 'n':
    	QAJ4C_json_message_forward(parser->msg);
		if (QAJ4C_json_message_read(parser->msg) != 'u'
				|| QAJ4C_json_message_read(parser->msg) != 'l'
				|| QAJ4C_json_message_read(parser->msg) != 'l') {
    		QAJ4C_first_pass_parser_set_error(parser, QAJ4C_ERROR_UNEXPECTED_CHAR);
    	}
        break;
    case '\0':
        QAJ4C_first_pass_parser_set_error(parser, QAJ4C_ERROR_BUFFER_TRUNCATED);
        break;
    default:
        QAJ4C_first_pass_parser_set_error(parser, QAJ4C_ERROR_UNEXPECTED_CHAR);
        break;
    }
}

static void QAJ4C_first_pass_object( QAJ4C_First_pass_parser* parser, int depth ) {
    char json_char;
    size_type member_count = 0;
    size_type storage_pos = parser->storage_counter++;

	if (parser->max_depth < depth) {
    	QAJ4C_first_pass_parser_set_error(parser, QAJ4C_ERROR_DEPTH_OVERFLOW);
        return;
    }

    QAJ4C_skip_whitespaces_and_comments(parser->msg);
    for (json_char = QAJ4C_json_message_read(parser->msg); json_char != '\0' && json_char != '}'; json_char = QAJ4C_json_message_read(parser->msg)) {
		if (member_count > 0) {
            if (json_char != ',') {
        		QAJ4C_first_pass_parser_set_error(parser, QAJ4C_ERROR_MISSING_COMMA);
                return;
            }
            QAJ4C_skip_whitespaces_and_comments(parser->msg);
            json_char = QAJ4C_json_message_read( parser->msg );
		}
        if (json_char == '"') {
            parser->amount_nodes++; /* count the string as node */
			QAJ4C_first_pass_string(parser);
            QAJ4C_skip_whitespaces_and_comments(parser->msg);
			json_char = QAJ4C_json_message_read(parser->msg);
            if (json_char != ':') {
        		QAJ4C_first_pass_parser_set_error(parser, QAJ4C_ERROR_MISSING_COLON);
                return;
            }
            QAJ4C_skip_whitespaces_and_comments(parser->msg);
            QAJ4C_first_pass_process(parser, depth + 1);
            ++member_count;
        } else if( json_char == '}' ) {
			if (parser->strict_parsing) {
				QAJ4C_first_pass_parser_set_error(parser, QAJ4C_ERROR_TRAILING_COMMA);
			}
        	break;
        } else {
			QAJ4C_first_pass_parser_set_error(parser, QAJ4C_ERROR_UNEXPECTED_CHAR);
        }

        QAJ4C_skip_whitespaces_and_comments(parser->msg);
    }

    if ( json_char == '\0' ) {
    	QAJ4C_first_pass_parser_set_error(parser, QAJ4C_ERROR_BUFFER_TRUNCATED);
    	return;
    }

    if (parser->builder != NULL) {
    	size_type* obj_data = QAJ4C_first_pass_fetch_stats_buffer( parser, storage_pos );
    	if ( obj_data != NULL ) {
    		*obj_data = member_count;
    	}
    }
}

static void QAJ4C_first_pass_array( QAJ4C_First_pass_parser* parser, int depth ) {
    char json_char;
    size_type member_count = 0;
    size_type storage_pos = parser->storage_counter++;

    if (parser->max_depth < depth) {
    	QAJ4C_first_pass_parser_set_error(parser, QAJ4C_ERROR_DEPTH_OVERFLOW);
        return;
    }

    QAJ4C_skip_whitespaces_and_comments(parser->msg);
    for (json_char = QAJ4C_json_message_peek(parser->msg); json_char != '\0' && json_char != ']'; json_char = QAJ4C_json_message_peek(parser->msg)) {
        if (member_count > 0) {
            if (json_char != ',') {
        		QAJ4C_first_pass_parser_set_error(parser, QAJ4C_ERROR_MISSING_COMMA);
                return;
            }
            QAJ4C_json_message_forward(parser->msg);
            QAJ4C_skip_whitespaces_and_comments(parser->msg);
            json_char = QAJ4C_json_message_peek( parser->msg );
        }
        if (json_char != ']') {
            QAJ4C_first_pass_process(parser, depth + 1);
            ++member_count;
		} else if (parser->strict_parsing) {
        	QAJ4C_first_pass_parser_set_error(parser, QAJ4C_ERROR_TRAILING_COMMA);
        }
        QAJ4C_skip_whitespaces_and_comments(parser->msg);
    }

    if ( json_char == '\0' ) {
    	QAJ4C_first_pass_parser_set_error(parser, QAJ4C_ERROR_BUFFER_TRUNCATED);
    	return;
    }

    QAJ4C_json_message_forward(parser->msg);

    if (parser->builder != NULL) {
    	size_type* obj_data = QAJ4C_first_pass_fetch_stats_buffer( parser, storage_pos );
    	if ( obj_data != NULL ) {
    		*obj_data = member_count;
    	}
    }
}

static void QAJ4C_first_pass_string( QAJ4C_First_pass_parser* parser ) {
    char json_char;
    bool escape_char = false;
    size_type chars = 0;

	for (json_char = QAJ4C_json_message_read(parser->msg); json_char != '\0' && (escape_char || json_char != '"'); json_char = QAJ4C_json_message_read(parser->msg)) {
		if (!escape_char) {
			if( json_char == '\\') {
				escape_char = true;
			} else if( ((uint8_t)json_char) < 32 ) {
        		QAJ4C_first_pass_parser_set_error(parser, QAJ4C_ERROR_UNEXPECTED_CHAR);
			}
			++chars;
		} else {
			switch( json_char ) {
			case 'u':
				chars += QAJ4C_first_pass_utf16( parser ) - 1;
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
			escape_char = false;
		}
	}

    if (json_char != '"') {
		QAJ4C_first_pass_parser_set_error(parser, QAJ4C_ERROR_BUFFER_TRUNCATED);
		return;
	}

    if (!parser->insitu_parsing && chars > QAJ4C_INLINE_STRING_SIZE) {
        parser->complete_string_length += chars + 1; /* count the \0 to the complete string length! */
    }
}

static uint32_t QAJ4C_first_pass_4digits( QAJ4C_First_pass_parser* parser ) {
    char json_char;
    uint32_t value = 0;
    int char_count = 0;
	for (json_char = QAJ4C_json_message_read(parser->msg); json_char != '\0'; json_char = QAJ4C_json_message_read(parser->msg)) {
		uint8_t xdigit = QAJ4C_xdigit(json_char);
		if (xdigit > 0xF) {
			QAJ4C_first_pass_parser_set_error(parser, QAJ4C_ERROR_INVALID_UNICODE_SEQUENCE);
			break;
		}

		value = value << 4 | QAJ4C_xdigit(json_char);
		if (++char_count == 4) {
			break;
		}
	}
	return value;
}

static int QAJ4C_first_pass_utf16( QAJ4C_First_pass_parser* parser) {
	int amount_utf8_chars = 0;
	uint32_t value = QAJ4C_first_pass_4digits(parser);

	if ( value < 0x80 ) {
		amount_utf8_chars = 1;
	} else if ( value < 0x800 ) {
		amount_utf8_chars = 2;
	} else if (value < 0xd800 || value > 0xdfff) {
		amount_utf8_chars = 3;
	} else if (value >= 0xd800 && value < 0xdbff) {
/*        val = (((val - 0xd800) << 10) | (0x03ff & (ucs(&p[i + 7]) - 0xdc00))) + 0x10000; */
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
	} else {
		/* invalid (low-surrogate before high-surrogate) */
		QAJ4C_first_pass_parser_set_error(parser, QAJ4C_ERROR_INVALID_UNICODE_SEQUENCE);
	}

	return amount_utf8_chars;
}

static void QAJ4C_first_pass_numeric_value( QAJ4C_First_pass_parser* parser ) {
    size_type num_type = 0; /* 1 => uint, 2 = int, 3 = double */
    char json_char = QAJ4C_json_message_peek(parser->msg);
    bool is_signed = false;

    // FIXME: Allow hex-notation in case not strictly parsing!

    if ( json_char == '-' ) {
    	is_signed = true;
        QAJ4C_json_message_forward(parser->msg);
    	json_char = QAJ4C_json_message_peek(parser->msg);
	} else if (json_char == '+' && !parser->strict_parsing) {
    	json_char = QAJ4C_json_message_read(parser->msg);
    }

    if( parser->strict_parsing && json_char == '0' ) {
    	/* next char is not allowed to be numeric! */
        QAJ4C_json_message_forward(parser->msg);
    	json_char = QAJ4C_json_message_peek(parser->msg);
		if (json_char >= '0' && json_char <= '9') {
			QAJ4C_first_pass_parser_set_error(parser, QAJ4C_ERROR_INVALID_NUMBER_FORMAT);
			return;
		}
    }

    while( json_char >= '0' && json_char <= '9' ) {
    	QAJ4C_json_message_forward(parser->msg);
    	json_char = QAJ4C_json_message_peek(parser->msg);
    }

    switch( json_char ) {
    case '.':
    case 'e':
    case 'E':
    	num_type = 3;
    	break;
    default:
    	num_type = is_signed ? 2 : 1;
    	break;
    }

	if (3 == num_type) {
		/* check the format! */
		if( json_char == '.' ) {
			QAJ4C_json_message_forward(parser->msg);
	    	json_char = QAJ4C_json_message_peek(parser->msg);
			/* expect at least one digit! */
			if (json_char < '0' || json_char > '9') {
				QAJ4C_first_pass_parser_set_error(parser, QAJ4C_ERROR_INVALID_NUMBER_FORMAT);
				return;
			}
		    while( json_char >= '0' && json_char <= '9' ) {
		    	QAJ4C_json_message_forward(parser->msg);
		    	json_char = QAJ4C_json_message_peek(parser->msg);
		    }
		}
		if( json_char == 'E' || json_char == 'e') {
			/* next char can be a + or - */
			QAJ4C_json_message_forward(parser->msg);
			json_char = QAJ4C_json_message_peek(parser->msg);
			if( json_char == '+' || json_char == '-') {
				QAJ4C_json_message_forward(parser->msg);
				json_char = QAJ4C_json_message_peek(parser->msg);
			}
			/* expect at least one digit! */
			if (json_char < '0' || json_char > '9') {
				QAJ4C_first_pass_parser_set_error(parser, QAJ4C_ERROR_INVALID_NUMBER_FORMAT);
				return;
			}
			/* forward until the end! */
		    while( json_char >= '0' && json_char <= '9' ) {
		    	QAJ4C_json_message_forward(parser->msg);
		    	json_char = QAJ4C_json_message_peek(parser->msg);
		    }
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
    me->integer_parse_base = parser->strict_parsing ? 10 : 0;
    me->optimize_object = parser->optimize_object;
    me->curr_buffer_pos = copy_to_index;
    me->fatal_fault = false;

    /* reset the builder to its original state! */
    QAJ4C_builder_init(builder, builder->buffer, builder->buffer_size);
}

static void QAJ4C_second_pass_process( QAJ4C_Second_pass_parser* me, QAJ4C_Value* result_ptr ) {
	if (me->fatal_fault) {
		return;
	}

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
    case '-':
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
        QAJ4C_second_pass_numeric_value(me, result_ptr);
        break;
    case 't':
        QAJ4C_set_bool(result_ptr, true);
        me->json_char += QAJ4C_TRUE_STR_LEN - 1; /* jump to the end of the constant */
        break;
    case 'f':
        QAJ4C_set_bool(result_ptr, false);
        me->json_char += QAJ4C_FALSE_STR_LEN - 1; /* jump to the end of the constant */
        break;
    case 'n':
        QAJ4C_set_null(result_ptr);
        me->json_char += QAJ4C_NULL_STR_LEN - 1; /* jump to the end of the constant */
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
        me->json_char = QAJ4C_skip_whitespaces_and_comments_second_pass(++me->json_char);
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
        me->json_char = QAJ4C_skip_whitespaces_and_comments_second_pass(++me->json_char);
    }
    ++me->json_char; /* walk over the ] */
}

static void QAJ4C_second_pass_string( QAJ4C_Second_pass_parser* me, QAJ4C_Value* result_ptr  ) {
    size_type chars = 0;
    char* put_str = NULL;

    /* determine the storage string size! */
    const char* c = me->json_char;
	while (*c != '\"') {
		if (*c == '\\') {
			++c;
			if (*c == 'u') {
				uint32_t value = QAJ4C_xdigit(c[1]) << 12 | QAJ4C_xdigit(c[2]) << 8 | QAJ4C_xdigit(c[3]) << 4 | QAJ4C_xdigit(c[4]);
				int fwd = 0;
				if (value < 0x80) {
					fwd = 1;
				} else if (value < 0x800) {
					fwd = 2;
				} else if (value < 0xd800 || value > 0xdfff) {
					fwd = 3;
				} else {
					fwd = 4;
					c += 6;
				}
				c += 4;
				chars += fwd - 1;
			}
		}
		++c;
		++chars;
	}

    if (me->insitu_parsing) {
        result_ptr->type = QAJ4C_STRING_REF_TYPE_CONSTANT;
        ((QAJ4C_String*)result_ptr)->count = chars;
        ((QAJ4C_String*)result_ptr)->s = me->json_char;
    } else if (QAJ4C_INLINE_STRING_SIZE >= chars) {
        result_ptr->type = QAJ4C_INLINE_STRING_TYPE_CONSTANT;
        ((QAJ4C_Short_string*)result_ptr)->count = chars;
    } else {
        result_ptr->type = QAJ4C_STRING_TYPE_CONSTANT;
        ((QAJ4C_String*)result_ptr)->count = chars;
        ((QAJ4C_String*)result_ptr)->s = QAJ4C_builder_pop_string(me->builder, chars + 1);
    }

    put_str = (char*)QAJ4C_get_string(result_ptr);

    while( *me->json_char != '"' ) {
        switch(*me->json_char) {
        case '\\':
            me->json_char += 1;
            switch(*me->json_char) {
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
            case 'u': {
                me->json_char += 1;
				/* utf-8 conversion inspired by parson */
				uint32_t utf16 = QAJ4C_second_pass_utf16(me);
				me->json_char--; /* FIXME: Function moves to far away */
				if (utf16 < 0x80) {
					*put_str = (char) utf16;
				} else if (utf16 < 0x800) {
					*put_str++ = ((utf16 >> 6) & 0x1f) | 0xc0;
					*put_str = (utf16 & 0x3f) | 0x80;
				} else if (utf16 < 0x010000) {
					*put_str++ = ((utf16 >> 12) & 0x0F) | 0xE0;
					*put_str++ = ((utf16 >> 6) & 0x3F) | 0x80;
					*put_str = ((utf16) & 0x3F) | 0x80;
				} else {
					*put_str++ = (((utf16 >> 18) & 0x07) | 0xF0);
					*put_str++ = (((utf16 >> 12) & 0x3F) | 0x80);
					*put_str++ = (((utf16 >> 6) & 0x3F) | 0x80);
					*put_str = (((utf16) & 0x3F) | 0x80);
				}
				break;
			}
			}
			break;
        default:
            *put_str = *me->json_char;
            break;
        }
        put_str += 1;
        me->json_char += 1;
    }
    *put_str = '\0';
    me->json_char += 1;
}

static uint32_t QAJ4C_second_pass_utf16( QAJ4C_Second_pass_parser* me ) {
	uint32_t value = QAJ4C_xdigit(me->json_char[0]) << 12 | QAJ4C_xdigit(me->json_char[1]) << 8 | QAJ4C_xdigit(me->json_char[2]) << 4 | QAJ4C_xdigit(me->json_char[3]);
	me->json_char += 4;
	if (value >= 0xd800 && value < 0xdbff) {
		me->json_char += 2;
		uint32_t low_surrogate = QAJ4C_xdigit(me->json_char[0]) << 12 | QAJ4C_xdigit(me->json_char[1]) << 8 | QAJ4C_xdigit(me->json_char[2]) << 4 | QAJ4C_xdigit(me->json_char[3]);
		value = ((((value-0xD800)&0x3FF)<<10)|((low_surrogate-0xDC00)&0x3FF))+0x010000;
		me->json_char += 4;
	}
	return value;
}


static void QAJ4C_second_pass_numeric_value( QAJ4C_Second_pass_parser* me, QAJ4C_Value* result_ptr  ) {
    char* c = (char*)me->json_char;
	bool double_value = false;
	if (*c == '-') {
		int64_t i = QAJ4C_strtol(c, &c, me->integer_parse_base);
		if ((i == INT64_MAX && errno == ERANGE) || *c == '.' || *c == 'E' || *c == 'e') {
			double_value = true;
		} else {
			QAJ4C_set_int64(result_ptr, i);
			me->json_char = c;
		}
	} else {
		uint64_t i = QAJ4C_strtoul(c, &c, me->integer_parse_base);
		if ((i == UINT64_MAX && errno == ERANGE) || *c == '.' || *c == 'E' || *c == 'e') {
			double_value = true;
		} else {
			QAJ4C_set_uint64(result_ptr, i);
			me->json_char = c;
		}

	}
	if (double_value) {
		QAJ4C_set_double(result_ptr, QAJ4C_strtod(me->json_char, (char**) &me->json_char));
	}
}

static size_type QAJ4C_second_pass_fetch_stats_data( QAJ4C_Second_pass_parser* me ) {
    size_type data = *((size_type*)(me->builder->buffer + me->curr_buffer_pos));
    me->curr_buffer_pos += sizeof(size_type);
    return data;
}

static char QAJ4C_json_message_peek( QAJ4C_Json_message* msg ) {
	/* Also very unlikely to happen (only in case json is invalid) */
	if ( msg->json_len > 0 && msg->json_pos >= msg->json_len ) {
		return '\0';
	}
	return msg->json[msg->json_pos];
}

static char QAJ4C_json_message_read( QAJ4C_Json_message* msg ) {
	char result = QAJ4C_json_message_peek( msg );
	++msg->json_pos;
	if ( result == '\0' ) {
		msg->json_len = msg->json_pos;
	}
	return result;
}

static void QAJ4C_json_message_forward( QAJ4C_Json_message* msg ) {
	if ( QAJ4C_json_message_peek( msg ) != 0 ) {
		++msg->json_pos;
	}
}

static void QAJ4C_skip_whitespaces_and_comments( QAJ4C_Json_message* msg ) {
	char current_char = QAJ4C_json_message_peek(msg);

	while (current_char != '\0') {
		switch (current_char) {
		case '\t':
		case '\n':
		case '\b':
		case '\r':
		case ' ':
			break;
			/* also skip comments! */
		case '/':
			QAJ4C_json_message_forward(msg);
			if (QAJ4C_json_message_peek(msg) == '*') {
				QAJ4C_json_message_forward(msg);
				do {
					current_char = QAJ4C_json_message_read(msg);
				} while (current_char != '\0' && !(current_char == '*' && QAJ4C_json_message_peek(msg) == '/'));
			} else if (QAJ4C_json_message_peek(msg) == '/') {
				QAJ4C_json_message_forward(msg);
				current_char = QAJ4C_json_message_peek(msg);
				while (current_char != '\0' && current_char != '\n') {
					QAJ4C_json_message_forward(msg);
					current_char = QAJ4C_json_message_peek(msg);
				}
			} else {
				return;
			}
			break;
		default:
			return;
		}
		QAJ4C_json_message_forward(msg);
		current_char = QAJ4C_json_message_peek(msg);
	}
}

static const char* QAJ4C_skip_whitespaces_and_comments_second_pass( const char* json ) {
	while (*json != '\0') {
		switch (*json) {
		case '\t':
		case '\n':
		case '\b':
		case '\r':
		case ' ':
			break;
		case '/':
			++json;
			/* It can either be a c-comment or a line comment*/
			if (*json == '*') {
				++json;
				while (!(*json == '*' && *(json + 1) == '/')) {
					++json;
				}
				++json;
			} else {
				while (*json != '\n') {
					++json;
				}

			}
			break;
		default:
			return json;
		}
		++json;
	}
	return json;
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
    err_info->json_pos = parser->msg->json_pos;

    ((QAJ4C_Error*)document)->info = err_info;

    return document;
}


static size_type* QAJ4C_first_pass_fetch_stats_buffer( QAJ4C_First_pass_parser* parser, size_type storage_pos )
{
	QAJ4C_Builder* builder = parser->builder;
	size_t in_buffer_pos = storage_pos * sizeof(size_type);
	if ( in_buffer_pos >= builder->buffer_size ) {
		void *tmp;
		size_t required_size = parser->amount_nodes * sizeof(QAJ4C_Value);
		if ( parser->realloc_callback == NULL ) {
			QAJ4C_first_pass_parser_set_error(parser, QAJ4C_ERROR_STORAGE_BUFFER_TO_SMALL);
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


static void QAJ4C_builder_validate_buffer( QAJ4C_Builder* builder ) {
    QAJ4C_ASSERT(builder->cur_obj_pos - 1 <= builder->cur_str_pos, {});
}

QAJ4C_Value* QAJ4C_builder_pop_values( QAJ4C_Builder* builder, size_type count ) {
    QAJ4C_Value* new_pointer;
    size_type i;
    if (count == 0) {
        return NULL;
    }
    new_pointer = (QAJ4C_Value*)(&builder->buffer[builder->cur_obj_pos]);
    builder->cur_obj_pos += count * sizeof(QAJ4C_Value);

    QAJ4C_builder_validate_buffer(builder);
    for (i = 0; i < count; i++) {
    	new_pointer->type = QAJ4C_NULL_TYPE_CONSTANT;
    }
    return new_pointer;
}

char* QAJ4C_builder_pop_string( QAJ4C_Builder* builder, size_type length ) {
    builder->cur_str_pos -= length * sizeof(char);
    QAJ4C_builder_validate_buffer(builder);
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
    QAJ4C_builder_validate_buffer(builder);

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
    result = QAJ4C_bsearch(&member, obj_ptr->top, obj_ptr->count, sizeof(QAJ4C_Member), QAJ4C_compare_members);
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
    size_type i;

    if (lhs_size != rhs_size) {
        return lhs_size - rhs_size;
    }

    for ( i = 0; i < lhs_size; ++i) {
        if (lhs_string[i] != rhs_string[i]) {
            return lhs_string[i] - rhs_string[i];
        }
    }
    return 0;
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

static size_t QAJ4C_copy_simple_string(char* buffer, size_t buffer_length, const char* string, size_t string_length) {
	size_t string_index = 0;
	size_t buffer_index = 0;
	if ( string_length > buffer_length ) {
		string_length = buffer_length;
	}

	for (string_index = 0; string_index < string_length; ++string_index) {
		buffer[buffer_index++] = string[string_index];
	}
	return buffer_index;

}


/*
 * This method does not only copy the string ... but also replaces all control chars.
 */
static size_t QAJ4C_copy_custom_string(char* buffer, size_t buffer_size, const QAJ4C_Value* value_ptr) {
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

	const char* string = QAJ4C_get_string(value_ptr);
    size_type string_length = QAJ4C_get_string_length(value_ptr);

	if (buffer_index >= buffer_size) return buffer_index;
	buffer[buffer_index++] = '"';

	for( string_index = 0; string_index < string_length && buffer_index <= buffer_size; ++string_index) {
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
			if (char_value < 32) {
				replacement_string = replacement_buf[char_value];
				replacement_string_len = replacement_len[char_value];
			}
		}

		if (replacement_string != NULL) {
			buffer_index += QAJ4C_copy_simple_string(buffer + buffer_index, buffer_size - buffer_index, replacement_string, replacement_string_len);
		} else {
			buffer[buffer_index++] = string[string_index];
		}
	}

	if (buffer_index >= buffer_size) return buffer_index;
	buffer[buffer_index++] = '"';

	return buffer_index;
}

size_t QAJ4C_sprint_impl( const QAJ4C_Value* value_ptr, char* buffer, size_t buffer_size, size_t index ) {
    size_type i;

	if (index >= buffer_size) return index;

    switch (QAJ4C_get_internal_type(value_ptr)) {
    case QAJ4C_OBJECT_SORTED:
    case QAJ4C_OBJECT: {
        QAJ4C_Member* top = ((QAJ4C_Object*)value_ptr)->top;
		if (index >= buffer_size) return index;
		buffer[index++] = '{';
        for (i = 0; i < ((QAJ4C_Object*)value_ptr)->count; i++) {
    		if (index >= buffer_size) return index;
            if (!QAJ4C_is_null(&top[i].key)) {
                if (i > 0) {
            		if (index >= buffer_size) return index;
            		buffer[index++] = ',';
                }
                index = QAJ4C_sprint_impl(&top[i].key, buffer, buffer_size, index);
        		if (index >= buffer_size) return index;
        		buffer[index++] = ':';
                index = QAJ4C_sprint_impl(&top[i].value, buffer, buffer_size, index);
            }
        }
		if (index >= buffer_size) return index;
		buffer[index++] = '}';
        break;
    }
    case QAJ4C_ARRAY: {
        QAJ4C_Value* top = ((QAJ4C_Array*)value_ptr)->top;
		if (index >= buffer_size) return index;
		buffer[index++] = '[';
        for (i = 0; i < ((QAJ4C_Array*)value_ptr)->count; i++) {
    		if (index >= buffer_size) return index;
            if (i > 0) {
        		if (index >= buffer_size) return index;
        		buffer[index++] = ',';
            }
            index = QAJ4C_sprint_impl(&top[i], buffer, buffer_size, index);
        }
		buffer[index++] = ']';
        break;
    }
    case QAJ4C_PRIMITIVE: {
        switch (QAJ4C_get_storage_type(value_ptr)) {
        case QAJ4C_PRIMITIVE_BOOL:
        	if (((QAJ4C_Primitive*)value_ptr)->data.b) {
        		index += QAJ4C_copy_simple_string(buffer + index, buffer_size - index, QAJ4C_TRUE_STR, QAJ4C_TRUE_STR_LEN - 1);
        	} else {
        		index += QAJ4C_copy_simple_string(buffer + index, buffer_size - index, QAJ4C_FALSE_STR, QAJ4C_FALSE_STR_LEN - 1);
        	}
            break;
        case QAJ4C_PRIMITIVE_INT:
        case QAJ4C_PRIMITIVE_INT64:
			index += QAJ4C_itostrn(buffer + index, buffer_size - index, ((QAJ4C_Primitive*) value_ptr)->data.i);
            break;
        case QAJ4C_PRIMITIVE_UINT:
        case QAJ4C_PRIMITIVE_UINT64:
			index += QAJ4C_utostrn(buffer + index, buffer_size - index, ((QAJ4C_Primitive*) value_ptr)->data.u);
            break;
        case QAJ4C_PRIMITIVE_DOUBLE: {
        	/* printing doubles inspired from cJSON */
        	double d = ((QAJ4C_Primitive*) value_ptr)->data.d;
        	double absd = d < 0 ? -d: d;
        	double delta = (int64_t)(d) - d;
        	double absDelta = delta < 0 ? -delta : delta;
			if ((d * 0) != 0) {
				index += QAJ4C_copy_simple_string(buffer + index, buffer_size - index, QAJ4C_NULL_STR, QAJ4C_NULL_STR_LEN - 1);
			} else if ((absDelta <= DBL_EPSILON) && (absd < 1.0e60)) {
				index += QAJ4C_snprintf(buffer + index, buffer_size - index, "%.1f", d);
			} else if ((absd < 1.0e-6) || (absd > 1.0e9)) {
				index += QAJ4C_snprintf(buffer + index, buffer_size - index, "%e", d);
			} else {
				index += QAJ4C_snprintf(buffer + index, buffer_size - index, "%f", d);
				while (buffer[index - 1] == '0') {
					index--;
				}
				if (buffer[index - 1] == '.') {
					index++;
				}
			}
        }
            break;
        default:
            QAJ4C_ASSERT(false, {return 0;});
        }
        break;
    }
    case QAJ4C_NULL: {
		index += QAJ4C_copy_simple_string(buffer + index, buffer_size - index, QAJ4C_NULL_STR, QAJ4C_NULL_STR_LEN - 1);
        break;
    }
    case QAJ4C_STRING_REF:
    case QAJ4C_INLINE_STRING:
    case QAJ4C_STRING: {
    	index += QAJ4C_copy_custom_string(buffer + index, buffer_size - index, value_ptr);
        break;
    }
    default:
        QAJ4C_ASSERT(false, {return 0;});
    }
    return index;
}


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

    size_type curr_buffer_pos;
} QAJ4C_Second_pass_parser;


QAJ4C_fatal_error_fn QAJ4C_ERR_FUNCTION;

static void QAJ4C_first_pass_parser_init( QAJ4C_First_pass_parser* parser, QAJ4C_Builder* builder, QAJ4C_Json_message* msg, int opts, QAJ4C_realloc_fn realloc_callback );
static void QAJ4C_first_pass_parser_set_error( QAJ4C_First_pass_parser* parser, QAJ4C_ERROR_CODE error );
static void QAJ4C_first_pass_process( QAJ4C_First_pass_parser* parser, int depth );
static void QAJ4C_first_pass_object( QAJ4C_First_pass_parser* parser, int depth );
static void QAJ4C_first_pass_array( QAJ4C_First_pass_parser* parser, int depth );
/** Analyze the string value (and store the size and if escaping is required) */
static void QAJ4C_first_pass_string( QAJ4C_First_pass_parser* parser );
/** Analyze the numeric value (and store the storage type in buffer) */
static void QAJ4C_first_pass_numeric_value( QAJ4C_First_pass_parser* parser );

static size_type* QAJ4C_first_pass_fetch_stats_buffer( QAJ4C_First_pass_parser* parser, size_type storage_pos );
static QAJ4C_Value* QAJ4C_create_error_description( QAJ4C_First_pass_parser* me );

static void QAJ4C_second_pass_parser_init( QAJ4C_Second_pass_parser* me, QAJ4C_First_pass_parser* parser);
static void QAJ4C_second_pass_process( QAJ4C_Second_pass_parser* me, QAJ4C_Value* result_ptr );
static void QAJ4C_second_pass_object( QAJ4C_Second_pass_parser* me, QAJ4C_Value* result_ptr );
static void QAJ4C_second_pass_array( QAJ4C_Second_pass_parser* me, QAJ4C_Value* result_ptr );
static void QAJ4C_second_pass_string( QAJ4C_Second_pass_parser* me, QAJ4C_Value* result_ptr );
static void QAJ4C_second_pass_numeric_value( QAJ4C_Second_pass_parser* me, QAJ4C_Value* result_ptr );

static size_type QAJ4C_second_pass_fetch_stats_data( QAJ4C_Second_pass_parser* me );

static void QAJ4C_skip_whitespaces_and_comments( QAJ4C_Json_message* msg );
static const char* QAJ4C_skip_whitespaces_and_comments_second_pass( const char* json );


static char QAJ4C_json_message_peek( QAJ4C_Json_message* msg );
static char QAJ4C_json_message_peek_ahead( QAJ4C_Json_message* msg, unsigned chars );
static char QAJ4C_json_message_read( QAJ4C_Json_message* msg );
static void QAJ4C_json_message_forward( QAJ4C_Json_message* msg );

static const char QAJ4C_NULL_STR[] = "null";
static const char QAJ4C_TRUE_STR[] = "true";
static const char QAJ4C_FALSE_STR[] = "false";
static const unsigned QAJ4C_NULL_STR_LEN = sizeof(QAJ4C_NULL_STR) / sizeof(QAJ4C_NULL_STR[0]);
static const unsigned QAJ4C_TRUE_STR_LEN = sizeof(QAJ4C_TRUE_STR) / sizeof(QAJ4C_TRUE_STR[0]);
static const unsigned QAJ4C_FALSE_STR_LEN = sizeof(QAJ4C_FALSE_STR) / sizeof(QAJ4C_FALSE_STR[0]);


QAJ4C_Value* QAJ4C_parse_generic( QAJ4C_Builder* builder, const char* json, size_t json_len, int opts, QAJ4C_realloc_fn realloc_callback) {
	QAJ4C_First_pass_parser parser;
	QAJ4C_Second_pass_parser second_parser;
	QAJ4C_Json_message msg;
    QAJ4C_Value* result = NULL;
    size_type required_size;
	msg.json = json;
	msg.json_len = json_len;
	msg.json_pos = 0;

	QAJ4C_first_pass_parser_init(&parser, builder, &msg, opts, realloc_callback);

    QAJ4C_first_pass_process(&parser, 0);

    required_size = parser.amount_nodes * sizeof(QAJ4C_Value) + parser.complete_string_length;
    if (required_size > parser.builder->buffer_size) {
        if (parser.realloc_callback != NULL) {
            void* tmp = parser.realloc_callback(parser.builder->buffer, required_size);
            if (tmp == NULL) {
                QAJ4C_first_pass_parser_set_error(&parser, QAJ4C_ERROR_ALLOCATION_ERROR);
            } else {
                QAJ4C_builder_init(parser.builder, tmp, required_size);
            }
        } else {
            QAJ4C_first_pass_parser_set_error(&parser, QAJ4C_ERROR_STORAGE_BUFFER_TO_SMALL);
        }
    }
    else
    {
        QAJ4C_builder_init(parser.builder, parser.builder->buffer, required_size);
    }

    if ( parser.err_code != QAJ4C_ERROR_NO_ERROR ) {
        return QAJ4C_create_error_description(&parser);
    }

    QAJ4C_second_pass_parser_init(&second_parser, &parser);
    result = QAJ4C_builder_pop_values(builder, 1);
    QAJ4C_second_pass_process(&second_parser, result);
    return result;
}

size_t QAJ4C_calculate_max_buffer_generic( const char* json, size_t json_len, int opts ) {
    QAJ4C_First_pass_parser parser;
    QAJ4C_Json_message msg;
    msg.json = json;
    msg.json_len = json_len;
    msg.json_pos = 0;

    QAJ4C_first_pass_parser_init(&parser, NULL, &msg, opts, NULL);
    QAJ4C_first_pass_process(&parser, 0);

    return parser.amount_nodes * sizeof(QAJ4C_Value) + parser.complete_string_length;
}

static void QAJ4C_first_pass_parser_init( QAJ4C_First_pass_parser* parser, QAJ4C_Builder* builder, QAJ4C_Json_message* msg, int opts, QAJ4C_realloc_fn realloc_callback ) {
	parser->msg = msg;

	parser->builder = builder;
	parser->realloc_callback = realloc_callback;

	parser->strict_parsing = (opts & QAJ4C_PARSE_OPTS_STRICT) != 0;
	parser->insitu_parsing = (opts & 1) != 0;

	parser->max_depth = 32;
	parser->amount_nodes = 0;
	parser->complete_string_length = 0;
	parser->storage_counter = 0;
	parser->err_code = QAJ4C_ERROR_NO_ERROR;
}

static void QAJ4C_first_pass_parser_set_error( QAJ4C_First_pass_parser* parser, QAJ4C_ERROR_CODE error ) {
	parser->err_code = error;
	/* set the length of the json message to the current position to avoid the parser will continue parsing */
	parser->msg->json_len = parser->msg->json_pos;
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
		}
        json_char = QAJ4C_json_message_peek( parser->msg );
        if( json_char != '}' ) {
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
		} else if (parser->strict_parsing) {
        	QAJ4C_first_pass_parser_set_error(parser, QAJ4C_ERROR_TRAILING_COMMA);
        }
        QAJ4C_skip_whitespaces_and_comments(parser->msg);
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
    for (json_char = QAJ4C_json_message_read(parser->msg); json_char != '\0' && json_char != ']'; json_char = QAJ4C_json_message_read(parser->msg)) {
        if (member_count > 0) {
            if (json_char != ',') {
        		QAJ4C_first_pass_parser_set_error(parser, QAJ4C_ERROR_MISSING_COMMA);
                return;
            }
            QAJ4C_skip_whitespaces_and_comments(parser->msg);
        }
        json_char = QAJ4C_json_message_peek( parser->msg );
        if (json_char != ']') {
            QAJ4C_first_pass_process(parser, depth + 1);
            ++member_count;
		} else if (parser->strict_parsing) {
        	QAJ4C_first_pass_parser_set_error(parser, QAJ4C_ERROR_TRAILING_COMMA);
        }
        QAJ4C_skip_whitespaces_and_comments(parser->msg);
    }

    if (parser->builder != NULL) {
    	size_type* obj_data = QAJ4C_first_pass_fetch_stats_buffer( parser, storage_pos );
    	if ( obj_data != NULL ) {
    		*obj_data = member_count;
    	}
    }
}

static void QAJ4C_first_pass_string( QAJ4C_First_pass_parser* parser ) {
    char json_char;
    size_type storage_pos = parser->storage_counter++;
    bool escape_char = false;
    size_type chars = 0;

	for (json_char = QAJ4C_json_message_read(parser->msg); json_char != '\0' && (escape_char || json_char != '"'); json_char = QAJ4C_json_message_read(parser->msg)) {
		if (!escape_char) {
			switch( json_char ) {
			case '\\':
				escape_char = true;
				break;
			}
			++chars;
		} else {
			switch( json_char ) {
			case 'u': /* we do not yet support this (so it does not count as escaped) */
				++chars;
				/* FIXME: check for 4 following hex digits! */
				break;
			case '\\':
			case '/':
			case 'b':
			case 'f':
			case 'n':
			case 'r':
			case 't':
				break;
			default:
        		QAJ4C_first_pass_parser_set_error(parser, QAJ4C_ERROR_INVALID_ESCAPE_SEQUENCE);
				return;
			}
			escape_char = true;
		}
	}

    if (json_char != '"') {
		QAJ4C_first_pass_parser_set_error(parser, QAJ4C_ERROR_BUFFER_TRUNCATED);
		return;
	}

    if (parser->builder != NULL) {
    	size_type* obj_data = QAJ4C_first_pass_fetch_stats_buffer( parser, storage_pos );
    	if ( obj_data != NULL ) {
    		*obj_data = chars;
    	}
    }
    if (!parser->insitu_parsing && chars > INLINE_STRING_SIZE) {
        parser->complete_string_length += chars + 1; /* count the \0 to the complete string length! */
    }
}

static void QAJ4C_first_pass_numeric_value( QAJ4C_First_pass_parser* parser ) {
    size_type storage_pos = parser->storage_counter++;
    size_type num_type = 0; /* 1 => uint, 2 = int, 3 = double */
    char json_char = QAJ4C_json_message_peek(parser->msg);
    bool is_signed = false;

    if ( json_char == '-' ) {
    	is_signed = true;
    	json_char = QAJ4C_json_message_read(parser->msg);
	} else if (json_char == '+' && !parser->strict_parsing) {
    	json_char = QAJ4C_json_message_read(parser->msg);
    }

    if( parser->strict_parsing && json_char == '0' ) {
    	/* next char is not allowed to be numeric! */
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
		    while( json_char >= '0' && json_char <= '9' ) {
		    	QAJ4C_json_message_forward(parser->msg);
		    	json_char = QAJ4C_json_message_peek(parser->msg);
		    }
		}
		if( json_char == 'E' || json_char == 'e') {
			/* next char has to be a + or - */
			json_char = QAJ4C_json_message_read(parser->msg);
			if( json_char != '+' && json_char != '-') {
				QAJ4C_first_pass_parser_set_error(parser, QAJ4C_ERROR_INVALID_NUMBER_FORMAT);
				return;
			}
			/* expect at least one digit! */
			json_char = QAJ4C_json_message_read(parser->msg);
			if (json_char < '0' && json_char > '9') {
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


    if (parser->builder != NULL) {
    	size_type* obj_data = QAJ4C_first_pass_fetch_stats_buffer( parser, storage_pos );
    	if ( obj_data != NULL ) {
    		*obj_data = num_type;
    	}
    }
}

static void QAJ4C_second_pass_parser_init( QAJ4C_Second_pass_parser* me, QAJ4C_First_pass_parser* parser ) {
    QAJ4C_Builder* builder = parser->builder;
    size_type required_object_storage = parser->amount_nodes * sizeof(QAJ4C_Value);
    size_type required_tempoary_storage = parser->storage_counter *  sizeof(size_type) + sizeof(size_type);
    size_type copy_to_index = required_object_storage - required_tempoary_storage;

    memmove(builder->buffer + copy_to_index, builder->buffer, required_tempoary_storage);
    me->json_char = parser->msg->json;
    me->builder = parser->builder;
    me->realloc_callback = parser->realloc_callback;
    me->insitu_parsing = parser->insitu_parsing;
    me->curr_buffer_pos = copy_to_index;

    /* reset the builder to its original state! */
    QAJ4C_builder_init(builder, builder->buffer, builder->buffer_size);
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

    QAJ4C_set_object(result_ptr, elements, me->builder);
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

    if (elements > 2) {
        QAJ4C_object_optimize(result_ptr);
    }
    ++me->json_char; /* walk over the } */
}

static void QAJ4C_second_pass_array( QAJ4C_Second_pass_parser* me, QAJ4C_Value* result_ptr ) {
    size_type elements = QAJ4C_second_pass_fetch_stats_data(me);
    size_type index;
    QAJ4C_Value* top;

    QAJ4C_set_array(result_ptr, elements, me->builder);
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
    size_type chars = QAJ4C_second_pass_fetch_stats_data(me);
    char* put_str = NULL;

    if (me->insitu_parsing) {
        result_ptr->type = STRING_REF_TYPE_CONSTANT;
        ((QAJ4C_String*)result_ptr)->count = chars;
        ((QAJ4C_String*)result_ptr)->s = me->json_char;
    } else if (INLINE_STRING_SIZE > chars) {
        result_ptr->type = INLINE_STRING_TYPE_CONSTANT;
        ((QAJ4C_Short_string*)result_ptr)->count = chars;
    } else {
        result_ptr->type = STRING_TYPE_CONSTANT;
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
            case 'u':
                *put_str = '\\';
                put_str += 1;
                *put_str = 'u'; /* not yet supported */
                break;
            }
            break;
        default:
            *put_str = *me->json_char;
            me->json_char += 1;
            break;
        }
        put_str += 1;
    }
    *put_str = '\0';
    me->json_char += 1;
}

static void QAJ4C_second_pass_numeric_value( QAJ4C_Second_pass_parser* me, QAJ4C_Value* result_ptr  ) {
    size_type number_type = QAJ4C_second_pass_fetch_stats_data(me);
    /* 1 => uint, 2 = int, 3 = double */
    switch (number_type) {
    case 1:
        QAJ4C_set_uint64(result_ptr, QAJ4C_strtoul(me->json_char, (char**)&me->json_char, 0));
        break;
    case 2:
        QAJ4C_set_int64(result_ptr, QAJ4C_strtol(me->json_char, (char**)&me->json_char, 0));
        break;
    case 3:
        QAJ4C_set_double(result_ptr, QAJ4C_strtod(me->json_char, (char**)&me->json_char));
        break;
    }
}

static size_type QAJ4C_second_pass_fetch_stats_data( QAJ4C_Second_pass_parser* me ) {
    size_type data = *((size_type*)&me->builder->buffer[me->curr_buffer_pos]);
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

static char QAJ4C_json_message_peek_ahead( QAJ4C_Json_message* msg, unsigned chars ) {
	unsigned idx = 0;

	for( idx = 0; idx < chars; idx++)
	{
		if ( msg->json_len > 0 && msg->json_pos + idx >= msg->json_len ) {
			return '\0';
		} else if ( msg->json[msg->json_pos + idx] == '\0' ) {
			return '\0';
		}
	}
	return msg->json[msg->json_pos + idx];
}


static char QAJ4C_json_message_read( QAJ4C_Json_message* msg ) {
	char result = QAJ4C_json_message_peek( msg );
	if ( result == '\0' ) {
		msg->json_len = msg->json_pos;
	}
	++msg->json_pos;
	return result;
}

static void QAJ4C_json_message_forward( QAJ4C_Json_message* msg ) {
	if ( QAJ4C_json_message_peek( msg ) != 0 ) {
		++msg->json_pos;
	}
}

static void QAJ4C_skip_whitespaces_and_comments( QAJ4C_Json_message* msg ) {
    bool in_comment = false;
	char current_char = QAJ4C_json_message_peek(msg);

    while (current_char != '\0') {
        if (!in_comment) {
			switch (current_char) {
            case '\t':
            case '\n':
            case '\b':
            case '\r':
            case ' ':
                break;
                /* also skip comments! */
            case '/':
                if (QAJ4C_json_message_peek_ahead(msg, 1) == '*') {
                    QAJ4C_json_message_forward(msg);
                    in_comment = true;
                } else {
                    return;
                }
                break;
            default:
                return;
            }
        } else {
			QAJ4C_json_message_forward(msg);
        	if ( current_char == '*' && QAJ4C_json_message_peek(msg) == '/') {
                QAJ4C_json_message_forward(msg);
                in_comment = false;
        	}
        }
        QAJ4C_json_message_forward(msg);
        current_char = QAJ4C_json_message_peek(msg);
    }
}

static const char* QAJ4C_skip_whitespaces_and_comments_second_pass( const char* json ) {
    bool in_comment = false;
    while ( *json != '\0') {
        if (!in_comment) {
            switch (*json) {
            case '\t':
            case '\n':
            case '\b':
            case '\r':
            case ' ':
                break;
            case '/': /* As we know the syntax is valid ... no need to check for comment start */
                in_comment = true;
                ++json;
                break;
            default:
                return json;
            }
        } else if( *json == '*' && *(json+1) == '/') {
            in_comment = false;
            ++json;
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
    document->type = ERROR_DESCRIPTION_TYPE_CONSTANT;

    err_info = (QAJ4C_Error_information*)(parser->builder->buffer + parser->builder->cur_obj_pos);
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
    QAJ4C_ASSERT(builder->cur_obj_pos - 1 <= builder->cur_str_pos);
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
    	new_pointer->type = NULL_TYPE_CONSTANT;
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
    	new_pointer[i].key.type = NULL_TYPE_CONSTANT;
		new_pointer[i].value.type = NULL_TYPE_CONSTANT;
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


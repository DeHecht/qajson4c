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


QAJ4C_fatal_error_fn QAJ4C_ERR_FUNCTION;

static void QAJ4C_First_pass_parser_init( QAJ4C_First_pass_parser* parser, QAJ4C_Builder* builder, QAJ4C_Json_message* msg, int opts, QAJ4C_realloc_fn realloc_callback );
static void QAJ4C_First_pass_parser_set_error( QAJ4C_First_pass_parser* parser, QAJ4C_ERROR_CODE error );
static void QAJ4C_first_pass_process( QAJ4C_First_pass_parser* parser, int depth );
static void QAJ4C_first_pass_object( QAJ4C_First_pass_parser* parser, int depth );
static void QAJ4C_first_pass_array( QAJ4C_First_pass_parser* parser, int depth );
/** Analyze the string value (and store the size and if escaping is required) */
static void QAJ4C_first_pass_string( QAJ4C_First_pass_parser* parser );
/** Analyze the numeric value (and store the storage type in buffer) */
static void QAJ4C_first_pass_numeric_value( QAJ4C_First_pass_parser* parser );

static size_type* QAJ4C_first_pass_fetch_stats_buffer( QAJ4C_First_pass_parser* parser, size_type storage_pos );

static void QAJ4C_skip_whitespaces_and_comments( QAJ4C_Json_message* msg );

static char QAJ4C_Json_message_peek( QAJ4C_Json_message* msg );
static char QAJ4C_Json_message_peek_ahead( QAJ4C_Json_message* msg, unsigned chars );
static char QAJ4C_Json_message_read( QAJ4C_Json_message* msg );
static void QAJ4C_Json_message_forward( QAJ4C_Json_message* msg );



QAJ4C_Value* QAJ4C_parse_generic( QAJ4C_Builder* builder, const char* json, size_t json_len, int opts, QAJ4C_realloc_fn realloc_callback) {
	QAJ4C_First_pass_parser parser;
	QAJ4C_Json_message msg;
	msg.json = json;
	msg.json_len = json_len;
	msg.json_pos = 0;

	QAJ4C_First_pass_parser_init(&parser, builder, &msg, opts, realloc_callback);

    QAJ4C_first_pass_process(&parser, 0);

    return NULL;
}

static void QAJ4C_First_pass_parser_init( QAJ4C_First_pass_parser* parser, QAJ4C_Builder* builder, QAJ4C_Json_message* msg, int opts, QAJ4C_realloc_fn realloc_callback ) {
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

static void QAJ4C_First_pass_parser_set_error( QAJ4C_First_pass_parser* parser, QAJ4C_ERROR_CODE error ) {
	parser->err_code = error;
	/* set the length of the json message to the current position to avoid the parser will continue parsing */
	parser->msg->json_len = parser->msg->json_pos;
}

static void QAJ4C_first_pass_process( QAJ4C_First_pass_parser* parser, int depth) {
    QAJ4C_skip_whitespaces_and_comments(parser->msg);
	switch (QAJ4C_Json_message_peek(parser->msg)) {
    case '\0':
    	QAJ4C_First_pass_parser_set_error(parser, QAJ4C_ERROR_BUFFER_TRUNCATED);
		break;
    case '{':
    	QAJ4C_Json_message_forward(parser->msg);
        QAJ4C_first_pass_object(parser, depth);
    	break;
    case '[':
    	QAJ4C_first_pass_array(parser, depth);
    	break;
    case '"':
    	QAJ4C_Json_message_forward(parser->msg);
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
    	QAJ4C_Json_message_forward(parser->msg);
		if (QAJ4C_Json_message_read(parser->msg) != 'r'
				|| QAJ4C_Json_message_read(parser->msg) != 'u'
				|| QAJ4C_Json_message_read(parser->msg) != 'e') {
    		QAJ4C_First_pass_parser_set_error(parser, QAJ4C_ERROR_UNEXPECTED_CHAR);
    	}
    	break;
    case 'f':
    	QAJ4C_Json_message_forward(parser->msg);
		if (QAJ4C_Json_message_read(parser->msg) != 'a'
				|| QAJ4C_Json_message_read(parser->msg) != 'l'
				|| QAJ4C_Json_message_read(parser->msg) != 's'
				|| QAJ4C_Json_message_read(parser->msg) != 'e') {
    		QAJ4C_First_pass_parser_set_error(parser, QAJ4C_ERROR_UNEXPECTED_CHAR);
    	}
    	break;
    case 'n':
    	QAJ4C_Json_message_forward(parser->msg);
		if (QAJ4C_Json_message_read(parser->msg) != 'u'
				|| QAJ4C_Json_message_read(parser->msg) != 'l'
				|| QAJ4C_Json_message_read(parser->msg) != 'l') {
    		QAJ4C_First_pass_parser_set_error(parser, QAJ4C_ERROR_UNEXPECTED_CHAR);
    	}
        break;
    }
}

static void QAJ4C_first_pass_object( QAJ4C_First_pass_parser* parser, int depth ) {
    char json_char;
    size_type member_count = 0;
    size_type storage_pos = parser->storage_counter++;

	if (parser->max_depth < depth) {
    	QAJ4C_First_pass_parser_set_error(parser, QAJ4C_ERROR_DEPTH_OVERFLOW);
        return;
    }

    parser->amount_nodes++;

    QAJ4C_skip_whitespaces_and_comments(parser->msg);
    for (json_char = QAJ4C_Json_message_read(parser->msg); json_char != '\0' && json_char != '}'; json_char = QAJ4C_Json_message_read(parser->msg)) {
		if (member_count > 0) {
            if (json_char != ',') {
        		QAJ4C_First_pass_parser_set_error(parser, QAJ4C_ERROR_MISSING_COMMA);
                return;
            }
            QAJ4C_skip_whitespaces_and_comments(parser->msg);
		}
        json_char = QAJ4C_Json_message_peek( parser->msg );
        if( json_char != '}' ) {
			QAJ4C_first_pass_string(parser);
            QAJ4C_skip_whitespaces_and_comments(parser->msg);
			json_char = QAJ4C_Json_message_read(parser->msg);
            if (json_char != ':') {
        		QAJ4C_First_pass_parser_set_error(parser, QAJ4C_ERROR_MISSING_COLON);
                return;
            }
            QAJ4C_skip_whitespaces_and_comments(parser->msg);
            QAJ4C_first_pass_process(parser, depth + 1);
            ++member_count;
		} else if (parser->strict_parsing) {
        	QAJ4C_First_pass_parser_set_error(parser, QAJ4C_ERROR_TRAILING_COMMA);
        }
        QAJ4C_skip_whitespaces_and_comments(parser->msg);
    }

    if (parser->builder->buffer != NULL) {
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
    	QAJ4C_First_pass_parser_set_error(parser, QAJ4C_ERROR_DEPTH_OVERFLOW);
        return;
    }

    parser->amount_nodes++;

    QAJ4C_skip_whitespaces_and_comments(parser->msg);
    for (json_char = QAJ4C_Json_message_read(parser->msg); json_char != '\0' && json_char != ']'; json_char = QAJ4C_Json_message_read(parser->msg)) {
        if (member_count > 0) {
            if (json_char != ',') {
        		QAJ4C_First_pass_parser_set_error(parser, QAJ4C_ERROR_MISSING_COMMA);
                return;
            }
            QAJ4C_skip_whitespaces_and_comments(parser->msg);
        }
        json_char = QAJ4C_Json_message_peek( parser->msg );
        if (json_char != ']') {
            QAJ4C_first_pass_process(parser, depth + 1);
            ++member_count;
		} else if (parser->strict_parsing) {
        	QAJ4C_First_pass_parser_set_error(parser, QAJ4C_ERROR_TRAILING_COMMA);
        }
        QAJ4C_skip_whitespaces_and_comments(parser->msg);
    }

    if (parser->builder->buffer != NULL) {
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

	for (json_char = QAJ4C_Json_message_read(parser->msg); json_char != '\0' && (escape_char || json_char != '"'); json_char = QAJ4C_Json_message_read(parser->msg)) {
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
        		QAJ4C_First_pass_parser_set_error(parser, QAJ4C_ERROR_INVALID_ESCAPE_SEQUENCE);
				return;
			}
			escape_char = true;
		}
	}

    if (json_char != '"') {
		QAJ4C_First_pass_parser_set_error(parser, QAJ4C_ERROR_BUFFER_TRUNCATED);
		return;
	}

    if (parser->builder->buffer != NULL) {
    	size_type* obj_data = QAJ4C_first_pass_fetch_stats_buffer( parser, storage_pos );
    	if ( obj_data != NULL ) {
    		*obj_data = chars;
    	}
    }
    parser->complete_string_length += chars + 1; /* the chars plus \0 */

}

static void QAJ4C_first_pass_numeric_value( QAJ4C_First_pass_parser* parser ) {
    size_type storage_pos = parser->storage_counter++;
    size_type num_type = 0; /* 1 => uint, 2 = int, 3 = double */
    char json_char = QAJ4C_Json_message_peek(parser->msg);
    bool is_signed = false;

    if ( json_char == '-' ) {
    	is_signed = true;
    	json_char = QAJ4C_Json_message_read(parser->msg);
	} else if (json_char == '+' && !parser->strict_parsing) {
    	json_char = QAJ4C_Json_message_read(parser->msg);
    }

    if( parser->strict_parsing && json_char == '0' ) {
    	/* next char is not allowed to be numeric! */
    	json_char = QAJ4C_Json_message_peek(parser->msg);
		if (json_char >= '0' && json_char <= '9') {
			QAJ4C_First_pass_parser_set_error(parser, QAJ4C_ERROR_INVALID_NUMBER_FORMAT);
			return;
		}
    }

    while( json_char >= '0' && json_char <= '9' ) {
    	QAJ4C_Json_message_forward(parser->msg);
    	json_char = QAJ4C_Json_message_peek(parser->msg);
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
			QAJ4C_Json_message_forward(parser->msg);
	    	json_char = QAJ4C_Json_message_peek(parser->msg);
		    while( json_char >= '0' && json_char <= '9' ) {
		    	QAJ4C_Json_message_forward(parser->msg);
		    	json_char = QAJ4C_Json_message_peek(parser->msg);
		    }
		}
		if( json_char == 'E' || json_char == 'e') {
			/* next char has to be a + or - */
			json_char = QAJ4C_Json_message_read(parser->msg);
			if( json_char != '+' && json_char != '-') {
				QAJ4C_First_pass_parser_set_error(parser, QAJ4C_ERROR_INVALID_NUMBER_FORMAT);
				return;
			}
			/* expect at least one digit! */
			json_char = QAJ4C_Json_message_read(parser->msg);
			if (json_char < '0' && json_char > '9') {
				QAJ4C_First_pass_parser_set_error(parser, QAJ4C_ERROR_INVALID_NUMBER_FORMAT);
				return;
			}
			/* forward until the end! */
		    while( json_char >= '0' && json_char <= '9' ) {
		    	QAJ4C_Json_message_forward(parser->msg);
		    	json_char = QAJ4C_Json_message_peek(parser->msg);
		    }
		}
	}


    if (parser->builder->buffer != NULL) {
    	size_type* obj_data = QAJ4C_first_pass_fetch_stats_buffer( parser, storage_pos );
    	if ( obj_data != NULL ) {
    		*obj_data = num_type;
    	}
    }
}


static char QAJ4C_Json_message_peek( QAJ4C_Json_message* msg ) {
	/* Also very unlikely to happen (only in case json is invalid) */
	if ( msg->json_len > 0 && msg->json_pos >= msg->json_len ) {
		return '\0';
	}
	return msg->json[msg->json_pos];
}

static char QAJ4C_Json_message_peek_ahead( QAJ4C_Json_message* msg, unsigned chars ) {
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


static char QAJ4C_Json_message_read( QAJ4C_Json_message* msg ) {
	char result = QAJ4C_Json_message_peek( msg );
	if ( result == '\0' ) {
		msg->json_len = msg->json_pos;
	}
	++msg->json_pos;
	return result;
}

static void QAJ4C_Json_message_forward( QAJ4C_Json_message* msg ) {
	if ( QAJ4C_Json_message_peek( msg ) != 0 ) {
		++msg->json_pos;
	}
}

static void QAJ4C_skip_whitespaces_and_comments( QAJ4C_Json_message* msg ) {
    bool skipping = true;
    bool in_comment = false;
	char current_char = QAJ4C_Json_message_peek(msg);

    while (skipping && current_char != '\0') {
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
                if (QAJ4C_Json_message_peek_ahead(msg, 1) == '*') {
                    in_comment = true;
                } else {
                    return;
                }
                break;
            default:
                return;
            }
        } else {
			QAJ4C_Json_message_forward(msg);
        	if ( current_char == '*' && QAJ4C_Json_message_peek(msg) == '/') {
                in_comment = false;
        	}
        }
        QAJ4C_Json_message_forward(msg);
        current_char = QAJ4C_Json_message_peek(msg);
    }
}

static size_type* QAJ4C_first_pass_fetch_stats_buffer( QAJ4C_First_pass_parser* parser, size_type storage_pos )
{
	QAJ4C_Builder* builder = parser->builder;
	size_t in_buffer_pos = storage_pos * sizeof(size_type);
	if ( in_buffer_pos >= builder->buffer_size ) {
		void *tmp;
		size_t required_size = parser->amount_nodes * sizeof(QAJ4C_Value);
		if ( parser->realloc_callback == NULL ) {
			QAJ4C_First_pass_parser_set_error(parser, QAJ4C_ERROR_STORAGE_BUFFER_TO_SMALL);
			return NULL;
		}

		tmp = parser->realloc_callback(builder->buffer, required_size);
		if (tmp == NULL) {
			QAJ4C_First_pass_parser_set_error(parser, QAJ4C_ERROR_ALLOCATION_ERROR);
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


/*
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <qajson4c/qajson4c.h>

#define INLINE_STRING_SIZE (sizeof(uintptr_t) + sizeof(size_type) - sizeof(uint8_t))

typedef uint32_t size_type;
typedef uint16_t half_size_type;

typedef enum QAJSON4C_Type {
    QAJSON4C_NULL = 0,
    QAJSON4C_UNSPECIFIED,
    QAJSON4C_OBJECT,
    QAJSON4C_ARRAY,
    QAJSON4C_STRING,
    QAJSON4C_INLINE_STRING,
    QAJSON4C_PRIMITIVE,
    QAJSON4C_ERROR_DESCRIPTION,
} QAJSON4C_Type;

typedef enum QAJSON4C_Primitive_type {
    QAJSON4C_PRIMITIVE_BOOL = (1 << 0),
    QAJSON4C_PRIMITIVE_INT = (1 << 1),
    QAJSON4C_PRIMITIVE_INT64 = (1 << 2),
    QAJSON4C_PRIMITIVE_UINT = (1 << 3),
    QAJSON4C_PRIMITIVE_UINT64 = (1 << 4),
    QAJSON4C_PRIMITIVE_DOUBLE = (1 << 5),
} QAJSON4C_Primitive_type;

typedef struct QAJSON4C_Object {
    QAJSON4C_Member* top;
    size_type count;
    size_type type;
} QAJSON4C_Object;

typedef struct QAJSON4C_Array {
    QAJSON4C_Value* top;
    size_type count;
    size_type type;
} QAJSON4C_Array;

typedef struct QAJSON4C_String {
    const char* s;
    size_type count;
    size_type type;
} QAJSON4C_String;

typedef struct QAJSON4C_Error_information {
    const char* json;
    size_type json_pos;
    size_type errno;
} QAJSON4C_Error_information;

typedef struct QAJSON4C_Error {
    QAJSON4C_Error_information* info;
    size_type padding;
    size_type type;
} QAJSON4C_Error;

/**
 * Instead of storing a pointer to a char* like in QAJSON4C_String we can inline the
 * char in the struct. This saves a) the dereferencing, b) additional memory.
 * As the string has to be small the count value can also be smaller, to additionally
 * store some more chars.
 * On 32-Bit => 6 chars + \0, on 64-Bit => 10 chars + \0.
 */
typedef struct QAJSON4C_Short_string {
    char s[INLINE_STRING_SIZE];
    uint8_t count;
    size_type type;
} QAJSON4C_Short_string;

typedef struct QAJSON4C_Primitive {
    union primitive {
        double d;
        uint64_t u;
        int64_t i;
        bool b;
    } data;
    half_size_type storage_type;
    half_size_type compatiblity_type;
    size_type type;
} QAJSON4C_Primitive; // always 8 Byte.

typedef struct QAJSON4C_Flag {
    const char padding[sizeof(uintptr_t) + sizeof(size_type)];
    size_type type;
} QAJSON4C_Flag;

typedef union QAJSON4C_Data {
    QAJSON4C_Object obj;
    QAJSON4C_Array array;
    QAJSON4C_String s;
    QAJSON4C_Short_string ss;
    QAJSON4C_Primitive p;
    QAJSON4C_Flag flag;
    QAJSON4C_Error err;
} QAJSON4C_Data;

struct QAJSON4C_Value {
    QAJSON4C_Data data;
};

struct QAJSON4C_Member {
    QAJSON4C_Value key;
    QAJSON4C_Value value;
};

struct QAJSON4C_Document {
    QAJSON4C_Value root_element;
};

typedef struct QAJSON4C_Parser {
    const char* json;
    size_type json_pos;

    char* buffer;
    size_type buffer_size;

    // the max depth is there to prevent stack overflow caused by a huge amount of nested
    // json objects/arrays.
    uint8_t max_depth;

    size_type curr_buffer_pos;
    size_type curr_buffer_str_pos;

    // for statistics
    size_type amount_elements;

    QAJSON4C_Builder* builder;
    bool insitu_parsing;

    bool error;
    QAJSON4C_ERROR_CODES errno;
} QAJSON4C_Parser;

static const char QAJSON4C_NULL_STR[] = "null";
static const char QAJSON4C_TRUE_STR[] = "true";
static const char QAJSON4C_FALSE_STR[] = "false";
static const unsigned QAJSON4C_NULL_STR_LEN = sizeof(QAJSON4C_NULL_STR)
        / sizeof(QAJSON4C_NULL_STR[0]);
static const unsigned QAJSON4C_TRUE_STR_LEN = sizeof(QAJSON4C_TRUE_STR)
        / sizeof(QAJSON4C_TRUE_STR[0]);
static const unsigned QAJSON4C_FALSE_STR_LEN = sizeof(QAJSON4C_FALSE_STR)
        / sizeof(QAJSON4C_FALSE_STR[0]);

static bool QAJSON4C_is_primitive( const QAJSON4C_Value* value );
static void QAJSON4C_parse_first_pass( QAJSON4C_Parser* parser );
static QAJSON4C_Document* QAJSON4C_parse_second_pass( QAJSON4C_Parser* parser );
static QAJSON4C_Document* QAJSON4C_create_error_description( QAJSON4C_Parser* parser );

static int QAJSON4C_strcmp( QAJSON4C_Value* lhs, QAJSON4C_Value* rhs );
static void QAJSON4C_skip_whitespaces_and_comments( QAJSON4C_Parser* parser );

static void QAJSON4C_first_pass_process( QAJSON4C_Parser* parser, uint8_t depth );
static void QAJSON4C_first_pass_object( QAJSON4C_Parser* parser, uint8_t depth );
static void QAJSON4C_first_pass_array( QAJSON4C_Parser* parser, uint8_t depth );

static void QAJSON4C_second_pass_process( QAJSON4C_Parser* parser, QAJSON4C_Value* result_ptr );

static void QAJSON4C_parse_array( QAJSON4C_Parser* parser, QAJSON4C_Value* result_ptr );
static void QAJSON4C_parse_object( QAJSON4C_Parser* parser, QAJSON4C_Value* result_ptr );
static void QAJSON4C_parse_string( QAJSON4C_Parser* parser, QAJSON4C_Value* result_ptr );
static void QAJSON4C_parse_constant( QAJSON4C_Parser* parser, QAJSON4C_Value* result_ptr );
static void QAJSON4C_parse_numeric_value( QAJSON4C_Parser* parser, QAJSON4C_Value* result_ptr );
static size_t QAJSON4C_print_value( const QAJSON4C_Value* value, char* buffer, size_t buffer_size, size_t index );

static QAJSON4C_Value* QAJSON4C_builder_pop_values( QAJSON4C_Builder* builder, size_type count );
static QAJSON4C_Member* QAJSON4C_builder_pop_members( QAJSON4C_Builder* builder, size_type count );
static char* QAJSON4C_builder_pop_string( QAJSON4C_Builder* builder, size_type length );

void QAJSON4C_print_stats() {
    printf("Sizeof QAJSON4C_Value: %lu\n", sizeof(QAJSON4C_Value));
    printf("Sizeof QAJSON4C_Array: %lu\n", sizeof(QAJSON4C_Array));
    printf("Sizeof QAJSON4C_String: %lu\n", sizeof(QAJSON4C_String));
    printf("Sizeof QAJSON4C_Short_string: %lu\n", sizeof(QAJSON4C_Short_string));
    printf("Sizeof QAJSON4C_Primitive: %lu\n", sizeof(QAJSON4C_Primitive));
    printf("Sizeof QAJSON4C_Error: %lu\n", sizeof(QAJSON4C_Error));
    printf("Sizeof QAJSON4C_Data: %lu\n", sizeof(QAJSON4C_Data));
    printf("Sizeof QAJSON4C_Member: %lu\n", sizeof(QAJSON4C_Member));
}

static QAJSON4C_Document* QAJSON4C_create_error_description( QAJSON4C_Parser* parser ) {
    // not enough space to store error information in buffer...
    if (parser->buffer_size < sizeof(QAJSON4C_Document) + sizeof(QAJSON4C_Error_information)) {
        return NULL;
    }

    QAJSON4C_Builder builder;
    QAJSON4C_builder_init(&builder, parser->buffer, parser->buffer_size);
    QAJSON4C_Document* document = QAJSON4C_builder_get_document(&builder);
    document->root_element.data.flag.type = QAJSON4C_ERROR_DESCRIPTION;

    QAJSON4C_Error_information* err_info = (QAJSON4C_Error_information*)(builder.buffer + builder.cur_obj_pos);
    err_info->errno = parser->errno;
    err_info->json = parser->json;
    err_info->json_pos = parser->json_pos;

    document->root_element.data.err.info = err_info;

    return document;
}

const QAJSON4C_Document* QAJSON4C_parse( const char* json, void* buffer, size_t buffer_size ) {
    QAJSON4C_Parser parser;
    parser.json = json;
    parser.buffer = buffer;
    parser.buffer_size = buffer_size;
    parser.insitu_parsing = false;

    QAJSON4C_parse_first_pass(&parser);
    if (parser.error) {
        return QAJSON4C_create_error_description(&parser);
    }
    return QAJSON4C_parse_second_pass(&parser);
}

const QAJSON4C_Document* QAJSON4C_parse_insitu( char* json, void* buffer, size_t buffer_size ) {
    QAJSON4C_Parser parser;
    parser.json = json;
    parser.buffer = buffer;
    parser.buffer_size = buffer_size;
    parser.insitu_parsing = true;

    QAJSON4C_parse_first_pass(&parser);
    if (parser.error) {
        return QAJSON4C_create_error_description(&parser);
    }
    return QAJSON4C_parse_second_pass(&parser);
}

unsigned QAJSON4C_calculate_max_buffer_size( const char* json ) {
    QAJSON4C_Parser parser;
    parser.json = json;
    parser.buffer = NULL;
    parser.buffer_size = 0;
    QAJSON4C_parse_first_pass(&parser);
    return sizeof(QAJSON4C_Document) + parser.amount_elements * sizeof(QAJSON4C_Value) + parser.curr_buffer_str_pos;
}

unsigned QAJSON4C_calculate_max_buffer_size_insitu( const char* json ) {
    QAJSON4C_Parser parser;
    parser.json = json;
    parser.buffer = NULL;
    parser.buffer_size = 0;
    QAJSON4C_parse_first_pass(&parser);
    return sizeof(QAJSON4C_Document) + parser.amount_elements * sizeof(QAJSON4C_Value);
}

static void QAJSON4C_first_pass_object( QAJSON4C_Parser* parser, uint8_t depth ) {
    if (parser->max_depth < depth) {
        parser->error = true;
        parser->errno = QAJSON4C_ERROR_DEPTH_OVERFLOW;
        return;
    }
    parser->amount_elements++;
    size_type old_pos = parser->curr_buffer_pos;

    size_type counter = 0;
    parser->curr_buffer_pos += sizeof(size_type);
    parser->json_pos++; // pass over the { char
    while (!parser->error && parser->json[parser->json_pos] != '}') {
        QAJSON4C_skip_whitespaces_and_comments(parser);
        if (counter > 0) {
            if (parser->json[parser->json_pos] != ',') {
                parser->error = true;
                parser->errno = QAJSON4C_ERROR_ARRAY_MISSING_COMMA;
                return;
            }
            parser->json_pos++; // pass over the , char
            QAJSON4C_skip_whitespaces_and_comments(parser);
        }
        if (parser->json[parser->json_pos] != '}') {
            QAJSON4C_parse_string(parser, NULL);
            if (parser->error) {
                return;
            }
            QAJSON4C_skip_whitespaces_and_comments(parser);
            if (parser->json[parser->json_pos] != ':') {
                parser->error = true;
                parser->errno = QAJSON4C_ERROR_OBJECT_MISSING_COLON;
                return;
            }
            parser->json_pos++; // pass over the : char
            QAJSON4C_skip_whitespaces_and_comments(parser);
            QAJSON4C_first_pass_process(parser, depth + 1);
            counter++;
        }
        QAJSON4C_skip_whitespaces_and_comments(parser);
    }
    parser->json_pos++; // pass over the } char
    if (parser->buffer != NULL) {
        *((size_type*)&parser->buffer[old_pos]) = counter;
    }
}

static void QAJSON4C_first_pass_array( QAJSON4C_Parser* parser, uint8_t depth ) {
    if (parser->max_depth < depth) {
        parser->error = true;
        parser->errno = QAJSON4C_ERROR_DEPTH_OVERFLOW;
        return;
    }
    parser->amount_elements++;
    size_type old_pos = parser->curr_buffer_pos;
    size_type counter = 0;
    parser->curr_buffer_pos += sizeof(size_type);
    parser->json_pos++; // pass over the [ char
    while (!parser->error && parser->json[parser->json_pos] != ']') {
        QAJSON4C_skip_whitespaces_and_comments(parser);
        if (counter > 0) {
            if (parser->json[parser->json_pos] != ',') {
                parser->error = true;
                parser->errno = QAJSON4C_ERROR_ARRAY_MISSING_COMMA;
                return;
            }
            parser->json_pos++; // pass over the , char
            QAJSON4C_skip_whitespaces_and_comments(parser);
        }
        if (parser->json[parser->json_pos] != ']') {
            QAJSON4C_first_pass_process(parser, depth + 1);
            counter++;
        }
        QAJSON4C_skip_whitespaces_and_comments(parser);
    }
    parser->json_pos++; // pass over the ] char
    if (parser->buffer != NULL) {
        *((size_type*)&parser->buffer[old_pos]) = counter;
    }
}

static void QAJSON4C_parse_numeric_value( QAJSON4C_Parser* parser, QAJSON4C_Value* result_ptr ) {
    parser->amount_elements++;
    bool use_uint = false;
    bool use_int = false;
    bool use_double = false;
    const char* start_pos = parser->json + parser->json_pos;
    char* pos = NULL;
    int64_t i_value = 0;
    uint64_t u_value = 0;
    double d_value = 0;
    int exp = 0;

    if (*start_pos == '-') {
        use_int = true;
        i_value = strtol(start_pos, &pos, 10);
    } else {
        use_uint = true;
        u_value = strtoul(start_pos, &pos, 10);
    }

    if (pos == start_pos) {
        parser->error = true;
        parser->errno = QAJSON4C_ERROR_INVALID_NUMBER_FORMAT;
        return;
    }

    if (*pos == '.') {
        use_int = false;
        use_uint = false;
        use_double = true;
        d_value = strtod(start_pos, &pos);
    }

    if (*pos == 'e' || *pos == 'E') {
        char* next = pos + 1;
        exp = strtol(next, &pos, 10);
    }

    if (exp != 0) {
        // FIXME: find a different way to solve this
        use_int = false;
        use_uint = false;
        use_double = true;
        d_value = strtod(start_pos, &pos);
    }

    if (result_ptr != NULL) {
        if (use_double) {
            QAJSON4C_set_double(result_ptr, d_value);
        } else if (use_uint) {
            QAJSON4C_set_uint64(result_ptr, u_value);
        } else if (use_int) {
            QAJSON4C_set_int64(result_ptr, i_value);
        }
    }
    parser->json_pos += pos - start_pos;
}

static void QAJSON4C_skip_whitespaces_and_comments( QAJSON4C_Parser* parser ) {
    bool skipping = true;
    bool in_comment = false;
    while (skipping && parser->json[parser->json_pos] != '\0') {
        if (!in_comment) {
            switch (parser->json[parser->json_pos]) {
            case '\t':
            case '\n':
            case '\b':
            case '\r':
            case ' ':
                break;
                // also skip comments!
            case '/':
                if (parser->json[parser->json_pos + 1] == '*') {
                    in_comment = true;
                } else {
                    return;
                }
                break;
            default:
                return;
            }
        } else {
            if (parser->json[parser->json_pos] == '*'
                    && parser->json[parser->json_pos + 1] == '/') {
                in_comment = false;
                parser->json_pos++;
            }
        }
        parser->json_pos++;
    }
}


static void QAJSON4C_parse_constant(QAJSON4C_Parser* parser, QAJSON4C_Value* result)
{
	parser->amount_elements++;
	const char* value_ptr = &parser->json[parser->json_pos];
	if (strncmp(QAJSON4C_NULL_STR, value_ptr, QAJSON4C_NULL_STR_LEN-1) == 0) {
		if (result != NULL) {
			result->data.flag.type = QAJSON4C_NULL;
		}
		parser->json_pos += QAJSON4C_NULL_STR_LEN-1;
	} else if (strncmp(QAJSON4C_FALSE_STR, value_ptr, QAJSON4C_FALSE_STR_LEN - 1) == 0
			|| strncmp(QAJSON4C_TRUE_STR, value_ptr, QAJSON4C_TRUE_STR_LEN -1) == 0) {
		bool constant_value = strncmp(QAJSON4C_TRUE_STR, value_ptr, QAJSON4C_TRUE_STR_LEN - 1) == 0;
		if (result != NULL) {
			result->data.flag.type = QAJSON4C_PRIMITIVE;
			result->data.p.storage_type = QAJSON4C_PRIMITIVE_BOOL;
			result->data.p.compatiblity_type = QAJSON4C_PRIMITIVE_BOOL;
			result->data.p.data.b = constant_value;
		}
		if( constant_value )
		{
			parser->json_pos += QAJSON4C_TRUE_STR_LEN-1;
		}
		else
		{
			parser->json_pos += QAJSON4C_FALSE_STR_LEN-1;
		}
	} else {
		parser->error = true;
		parser->errno = QAJSON4C_ERROR_UNEXPECTED_CHAR;
	}
}

static void QAJSON4C_parse_array(QAJSON4C_Parser* parser, QAJSON4C_Value* result_ptr) {
	if (parser->json[parser->json_pos] != '[') {
		parser->error = true;
		parser->errno = QAJSON4C_ERROR_UNEXPECTED_CHAR;
		return;
	}
	size_type elements = *((size_type*)&parser->buffer[parser->curr_buffer_pos]);
	parser->curr_buffer_pos += sizeof(size_type);

	QAJSON4C_set_array(result_ptr, elements, parser->builder);
	QAJSON4C_Value* top = result_ptr->data.array.top;

	parser->json_pos++; // pass over the [ char
	for (size_type index = 0; index < elements; index++) {
		QAJSON4C_skip_whitespaces_and_comments(parser);
		if(parser->json[parser->json_pos] == ',') {
			parser->json_pos++;
			QAJSON4C_skip_whitespaces_and_comments(parser);
		}
		QAJSON4C_second_pass_process(parser, &top[index]);
	}
	QAJSON4C_skip_whitespaces_and_comments(parser);

	// pass over a trailing , if available
	if( parser->json[parser->json_pos] == ',') {
		parser->json_pos++;
	}
	parser->json_pos++; // pass over the ] char

}

static void QAJSON4C_parse_object(QAJSON4C_Parser* parser, QAJSON4C_Value* result_ptr) {
	if (parser->json[parser->json_pos] != '{') {
		parser->error = true;
		parser->errno = QAJSON4C_ERROR_UNEXPECTED_CHAR;
		return;
	}

	// FIXME: implement a pop() method.
	size_type elements = *((size_type*)&parser->buffer[parser->curr_buffer_pos]);
	parser->curr_buffer_pos += sizeof(size_type);

	QAJSON4C_set_object(result_ptr, elements, parser->builder);
	QAJSON4C_Member* top = result_ptr->data.obj.top;

	parser->json_pos++; // pass over the { char

	for (size_type index = 0; index < elements; index++) {
		QAJSON4C_skip_whitespaces_and_comments(parser);
		if(parser->json[parser->json_pos] == ',') {
			parser->json_pos++;
			QAJSON4C_skip_whitespaces_and_comments(parser);
		}
		QAJSON4C_parse_string(parser, &top[index].key);
		QAJSON4C_skip_whitespaces_and_comments(parser);
		parser->json_pos++; // pass over the :
		QAJSON4C_skip_whitespaces_and_comments(parser);
		QAJSON4C_second_pass_process(parser, &top[index].value);
	}
	QAJSON4C_skip_whitespaces_and_comments(parser);
	// pass over a trailing , if available
	if( parser->json[parser->json_pos] == ',') {
		parser->json_pos++;
	}

	parser->json_pos++; // pass over the } char
}

/**
 * This method analyzes a string and moves the json_pos index to the character after
 * the ending ".
 */
static void QAJSON4C_parse_string(QAJSON4C_Parser* parser, QAJSON4C_Value* value) {
	if (parser->json[parser->json_pos] != '"') {
		parser->error = true;
		parser->errno = QAJSON4C_ERROR_INVALID_STRING_START;
		return;
	}
	parser->amount_elements++;
	parser->json_pos++; // move to first char in string
	size_type prev_pos = parser->json_pos;
	bool escape_char = false;
	bool in_string = true;
	// process until end of string and return the string size
	while (in_string && parser->json[parser->json_pos] != '\0') {
		if (!escape_char) {
			switch (parser->json[parser->json_pos]) {
			case '\\':
				escape_char = true;
				break;
			case '"':
				in_string = false;
				break;
			default:
				break;
			}
		} else {
			escape_char = false;
		}
		parser->json_pos++;
	}

	if (in_string) {
		parser->error = true;
		parser->errno = QAJSON4C_ERROR_BUFFER_TRUNCATED;
	} else {
		size_type size = parser->json_pos - prev_pos;
		if ( value != NULL ) {
			if ( parser->insitu_parsing ) {
				char* insitu_string = (char*)parser->json;
				insitu_string[parser->json_pos - 1] = '\0';
			    QAJSON4C_set_string(value, &parser->json[prev_pos], .ref=true, .len=size);
			} else {
                QAJSON4C_set_string(value, &parser->json[prev_pos], .builder=parser->builder, size);
			}
		} else if (size > INLINE_STRING_SIZE) {
			// we "may" not be able to store it inline in the DOM
			parser->curr_buffer_str_pos += size;
		}
	}

	// we have already proceed the " character within the while loop
}

static void QAJSON4C_first_pass_process(QAJSON4C_Parser* parser, uint8_t depth)
{
	QAJSON4C_skip_whitespaces_and_comments(parser);
	switch (parser->json[parser->json_pos]) {
    case '{':
       QAJSON4C_first_pass_object(parser, depth);
       break;
    case '[':
       QAJSON4C_first_pass_array(parser, depth);
       break;
    case '"':
       QAJSON4C_parse_string(parser, NULL);
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
       QAJSON4C_parse_numeric_value(parser, NULL);
       break;
    case 't':
    case 'f':
    case 'n':
       QAJSON4C_parse_constant(parser, NULL);
       break;
	}
}


static void QAJSON4C_parse_first_pass(QAJSON4C_Parser* parser) {
	// strategy. Walk through the json message and create counters how many members the detected arrays/objects have.
	// additionally count how many "memory" is required to store the DOM (exclusive and inclusive the strings).
	// With this information we can reserve the correct amount of memory for each object at once ...
	// the data should be stored on the end of the DOM "space" so it will be overridden by the DOM elements and thus
	// will not require additional memory.
	parser->json_pos = 0;
	parser->curr_buffer_pos = 0;
	parser->curr_buffer_str_pos = 0;
	parser->max_depth = 32;
	parser->error = false;
	parser->errno = 0;
	parser->amount_elements = 0;

	QAJSON4C_first_pass_process(parser, 0);
	if (!parser->error) {
		// skip whitespaces and comments
		QAJSON4C_skip_whitespaces_and_comments(parser);
		if (parser->json[parser->json_pos] != '\0') {
			parser->error = true;
			parser->errno = QAJSON4C_ERROR_UNEXPECTED_JSON_APPENDIX;
		}
	}
}

static QAJSON4C_Document* QAJSON4C_parse_second_pass(QAJSON4C_Parser* parser) {
	// prepare the layout and already create the document element!
	// then call the second_pass_process method

	if (parser->curr_buffer_pos > 0 )
	{
		size_type required_object_storage = sizeof(QAJSON4C_Document) + parser->amount_elements * sizeof(QAJSON4C_Value);
		size_type required_tempoary_storage = parser->curr_buffer_pos + sizeof(size_type);
        size_type copy_to_index = required_object_storage - required_tempoary_storage;

		// Currently the sizes of the objects are placed in memory starting at index 0 of the buffer.
		// To avoid, that the first object will override the valuable information gathered in the first
		// pass, the data is moved to the end of the required_object_storage. This will cause the last
		// object to be the first that will override its own valuable information. But at a moment where
		// the data is safe to override!
		memmove(parser->buffer + copy_to_index, parser->buffer, required_tempoary_storage);
		parser->curr_buffer_pos = copy_to_index;
	}
	// QAJSON4C_second_pass_process
	parser->json_pos = 0;
	parser->amount_elements = 0;

	QAJSON4C_Builder builder;
	QAJSON4C_builder_init(&builder, parser->buffer, parser->buffer_size);
	parser->builder = &builder;

	QAJSON4C_Document* document = QAJSON4C_builder_get_document(&builder);

	QAJSON4C_second_pass_process(parser, &document->root_element);
	return document;
}

void QAJSON4C_print(const QAJSON4C_Document* document, char* buffer, size_t buffer_size)
{
	size_t index = QAJSON4C_print_value(&document->root_element, buffer, buffer_size, 0);
	buffer[index] = '\0';
}

static size_t QAJSON4C_print_value( const QAJSON4C_Value* value, char* buffer, size_t buffer_size, size_t index ) {
    // FIXME: prevent a buffer overflow by checking buffer_size!
    switch (value->data.flag.type) {
    case QAJSON4C_OBJECT: {
        QAJSON4C_Member* top = value->data.obj.top;
        index += snprintf(buffer + index, buffer_size - index, "{");
        for (size_type i = 0; i < value->data.obj.count; i++) {
            if (!QAJSON4C_is_null(&top[i].key)) {
                if (i > 0) {
                    index += snprintf(buffer + index, buffer_size - index, ",");
                }

                index = QAJSON4C_print_value(&top[i].key, buffer, buffer_size, index);
                index += snprintf(buffer + index, buffer_size - index, ":");
                index = QAJSON4C_print_value(&top[i].value, buffer, buffer_size, index);
            }
        }
        index += snprintf(buffer + index, buffer_size - index, "}");
        break;
    }
    case QAJSON4C_ARRAY: {
        QAJSON4C_Value* top = value->data.array.top;
        index += snprintf(buffer + index, buffer_size - index, "[");
        for (size_type i = 0; i < value->data.obj.count; i++) {
            if (i > 0) {
                index += snprintf(buffer + index, buffer_size - index, ",");
            }
            index = QAJSON4C_print_value(&top[i], buffer, buffer_size, index);
        }
        index += snprintf(buffer + index, buffer_size - index, "]");
        break;
    }
    case QAJSON4C_PRIMITIVE: {
        switch (value->data.p.storage_type) {
        case QAJSON4C_PRIMITIVE_BOOL:
            index += snprintf(buffer + index, buffer_size - index, "%s", value->data.p.data.b ? "true" : "false");
            break;
        case QAJSON4C_PRIMITIVE_INT:
        case QAJSON4C_PRIMITIVE_INT64:
            index += snprintf(buffer + index, buffer_size - index, "%ld", value->data.p.data.i);
            break;
        case QAJSON4C_PRIMITIVE_UINT:
        case QAJSON4C_PRIMITIVE_UINT64:
            index += snprintf(buffer + index, buffer_size - index, "%lu", value->data.p.data.u);
            break;
        case QAJSON4C_PRIMITIVE_DOUBLE:
            index += snprintf(buffer + index, buffer_size - index, "%f", value->data.p.data.d);
            break;
        default:
            assert(false);
        }
        break;
    }
    case QAJSON4C_NULL: {
        index += snprintf(buffer + index, buffer_size - index, QAJSON4C_NULL_STR);
        break;
    }
    case QAJSON4C_INLINE_STRING:
    case QAJSON4C_STRING: {
    	buffer[index++] = '"';
    	const char* base = QAJSON4C_get_string(value);
    	// FIXME: use the stored string length instead of the strlen function.
    	for( size_type i = 0; i < strlen(base); i++) {
    		if ( base[i] == '"') {
    			buffer[index++] = '\\';
    		}
    		buffer[index++] = base[i];
    	}
    	buffer[index++] = '"';
        break;
    }
    default:
        assert(false);
    }
    return index;
}



static void QAJSON4C_second_pass_process(QAJSON4C_Parser* parser, QAJSON4C_Value* result_ptr)
{
	QAJSON4C_skip_whitespaces_and_comments(parser);
	switch (parser->json[parser->json_pos]) {
    case '{':
       QAJSON4C_parse_object(parser, result_ptr);
       break;
    case '[':
       QAJSON4C_parse_array(parser, result_ptr);
       break;
    case '"':
       QAJSON4C_parse_string(parser, result_ptr);
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
       QAJSON4C_parse_numeric_value(parser, result_ptr);
       break;
    case 't':
    case 'f':
    case 'n':
       QAJSON4C_parse_constant(parser, result_ptr);
       break;
	}

}


static int QAJSON4C_strcmp(QAJSON4C_Value* lhs, QAJSON4C_Value* rhs) {
	// first on string-length
	size_type lhs_size = QAJSON4C_get_string_length(lhs);
	size_type rhs_size = QAJSON4C_get_string_length(rhs);
	if (lhs_size != rhs_size) {
		return lhs_size - rhs_size;
	}
	const char* lhs_string = QAJSON4C_get_string(lhs);
	const char* rhs_string = QAJSON4C_get_string(rhs);

	for (size_type i = 0; i < lhs_size; ++i) {
		if (lhs_string[i] != rhs_string[i]) {
			return lhs_string[i] - rhs_string[i];
		}
	}
	return 0;
}

static bool QAJSON4C_is_primitive(const QAJSON4C_Value* value)
{
	return value != NULL && value->data.flag.type == QAJSON4C_PRIMITIVE;
}

/**
 * Getter and setter
 */

const QAJSON4C_Value* QAJSON4C_get_root_value(const QAJSON4C_Document* document) {
	return &document->root_element;
}

QAJSON4C_Value* QAJSON4C_get_root_value_rw(QAJSON4C_Document* document) {
	return &document->root_element;
}


bool QAJSON4C_is_string(const QAJSON4C_Value* value) {
	return value != NULL && (value->data.flag.type == QAJSON4C_INLINE_STRING || value->data.flag.type == QAJSON4C_STRING);
}

const char* QAJSON4C_get_string(const QAJSON4C_Value* value) {
	assert(QAJSON4C_is_string(value));
	if( value->data.flag.type == QAJSON4C_INLINE_STRING) {
		return value->data.ss.s;
	}
	return value->data.s.s;
}

unsigned QAJSON4C_get_string_length(const QAJSON4C_Value* value) {
	assert(QAJSON4C_is_string(value));
	if( value->data.flag.type == QAJSON4C_INLINE_STRING) {
		return value->data.ss.count;
	}
	return value->data.s.count;
}

bool QAJSON4C_is_object(const QAJSON4C_Value* value) {
	return value != NULL && value->data.flag.type == QAJSON4C_OBJECT;
}

bool QAJSON4C_is_array(const QAJSON4C_Value* value) {
	return value != NULL && value->data.flag.type == QAJSON4C_ARRAY;
}

bool QAJSON4C_is_int(const QAJSON4C_Value* value) {
   return QAJSON4C_is_primitive(value) && (value->data.p.compatiblity_type & QAJSON4C_PRIMITIVE_INT) != 0;
}

bool QAJSON4C_is_int64(const QAJSON4C_Value* value) {
   return QAJSON4C_is_primitive(value) && (value->data.p.compatiblity_type & QAJSON4C_PRIMITIVE_INT64) != 0;
}

bool QAJSON4C_is_uint(const QAJSON4C_Value* value) {
   return QAJSON4C_is_primitive(value) && (value->data.p.compatiblity_type & QAJSON4C_PRIMITIVE_UINT) != 0;
}

bool QAJSON4C_is_uint64(const QAJSON4C_Value* value) {
   return QAJSON4C_is_primitive(value) && (value->data.p.compatiblity_type & QAJSON4C_PRIMITIVE_UINT64) != 0;
}

bool QAJSON4C_is_real(const QAJSON4C_Value* value) {
   return QAJSON4C_is_primitive(value) && (value->data.p.compatiblity_type & QAJSON4C_PRIMITIVE_DOUBLE) != 0;
}

bool QAJSON4C_is_bool(const QAJSON4C_Value* value) {
   return QAJSON4C_is_primitive(value) && (value->data.p.compatiblity_type & QAJSON4C_PRIMITIVE_BOOL) != 0;
}

int32_t QAJSON4C_get_int(const QAJSON4C_Value* value) {
	assert(QAJSON4C_is_int(value));
	return (int32_t)value->data.p.data.i;
}

uint32_t QAJSON4C_get_uint(const QAJSON4C_Value* value) {
	assert(QAJSON4C_is_uint(value));
	return (uint32_t)value->data.p.data.u;
}

double QAJSON4C_get_real(const QAJSON4C_Value* value) {
	assert(QAJSON4C_is_real(value));
	switch(value->data.p.storage_type)
	{
	case QAJSON4C_PRIMITIVE_INT:
	case QAJSON4C_PRIMITIVE_INT64:
		return (double)value->data.p.data.i;
	case QAJSON4C_PRIMITIVE_UINT:
	case QAJSON4C_PRIMITIVE_UINT64:
      return (double)value->data.p.data.u;
	}
	return value->data.p.data.d;
}

bool QAJSON4C_get_bool(const QAJSON4C_Value* value) {
	assert(QAJSON4C_is_bool(value));
   return value->data.p.data.b;
}

bool QAJSON4C_is_null(const QAJSON4C_Value* value) {
	return value == NULL || value->data.flag.type == QAJSON4C_NULL;
}


bool QAJSON4C_is_error(const QAJSON4C_Value* value) {
	return value != NULL && value->data.flag.type == QAJSON4C_ERROR_DESCRIPTION;
}

const char* QAJSON4C_get_json(const QAJSON4C_Value* value) {
	assert(QAJSON4C_is_error(value));
	return value->data.err.info->json;
}

QAJSON4C_ERROR_CODES QAJSON4C_get_errno(const QAJSON4C_Value* value) {
	assert(QAJSON4C_is_error(value));
	return value->data.err.info->errno;
}

unsigned QAJSON4C_get_json_pos(const QAJSON4C_Value* value) {
	assert(QAJSON4C_is_error(value));
	return value->data.err.info->json_pos;
}

unsigned QAJSON4C_object_size(const QAJSON4C_Value* value) {
	assert(QAJSON4C_is_object(value));
	return value->data.obj.count;
}

QAJSON4C_Value* QAJSON4C_object_create_member_var(QAJSON4C_Value* value_ptr, string_ref_args args)
{
    // FIXME: Optimize this?
    assert(QAJSON4C_is_object(value_ptr));
    size_type count = value_ptr->data.obj.count;
    for (size_type i = 0; i < count; ++i) {
        QAJSON4C_Value* key_value = &value_ptr->data.obj.top[i].key;
        if (QAJSON4C_is_null(key_value)) {
            // we found some free space to place our ptr
            QAJSON4C_set_string_var(key_value, args);
            return &value_ptr->data.obj.top[i].value;
        }
    }
    return NULL;
}


const QAJSON4C_Member* QAJSON4C_object_get_member(const QAJSON4C_Value* value, unsigned index) {
	assert(QAJSON4C_is_object(value));
	return &value->data.obj.top[index];
}

const QAJSON4C_Value* QAJSON4C_member_get_key(const QAJSON4C_Member* member) {
	assert(member != NULL);
	return &member->key;
}

const QAJSON4C_Value* QAJSON4C_member_get_value(const QAJSON4C_Member* member) {
	assert(member != NULL);
	return &member->value;
}

const QAJSON4C_Value* QAJSON4C_object_get(const QAJSON4C_Value* value, const char* name) {
	assert(QAJSON4C_is_object(value));
	size_type count = value->data.obj.count;

	QAJSON4C_Value wrapper_value;
	QAJSON4C_set_string(&wrapper_value, name, .ref=true);

	QAJSON4C_Member* entry;
	for (size_type i = 0; i < count; ++i) {
		entry = value->data.obj.top + i;
		if (QAJSON4C_strcmp(&wrapper_value, &entry->key) == 0) {
			return &entry->value;
		}
	}
	return NULL; // assert?
}

const QAJSON4C_Value* QAJSON4C_array_get(const QAJSON4C_Value* value, unsigned index) {
	assert(QAJSON4C_is_array(value));
	assert(value->data.array.count > index);
	return value->data.array.top + index;
}

unsigned QAJSON4C_array_size(const QAJSON4C_Value* value) {
	assert(QAJSON4C_is_array(value));
	return value->data.array.count;
}

// Modifications on primitives is quite easy
void QAJSON4C_set_bool(QAJSON4C_Value* value_ptr, bool value)
{
	assert(value_ptr != NULL);
	value_ptr->data.flag.type = QAJSON4C_PRIMITIVE;
	value_ptr->data.p.storage_type = QAJSON4C_PRIMITIVE_BOOL;
	value_ptr->data.p.data.b = value;
}
void QAJSON4C_set_int(QAJSON4C_Value* value_ptr, int32_t value)
{
	QAJSON4C_set_int64(value_ptr, value);
}
void QAJSON4C_set_uint(QAJSON4C_Value* value_ptr, uint32_t value)
{
	QAJSON4C_set_uint64(value_ptr, value);
}

void QAJSON4C_set_int64(QAJSON4C_Value* value_ptr, int64_t value)
{
	value_ptr->data.flag.type = QAJSON4C_PRIMITIVE;
	value_ptr->data.p.data.i = value;
	if (value > INT32_MAX) {
		value_ptr->data.p.storage_type = QAJSON4C_PRIMITIVE_INT64;
		value_ptr->data.p.compatiblity_type = QAJSON4C_PRIMITIVE_INT64;
	} else {
		value_ptr->data.p.storage_type = QAJSON4C_PRIMITIVE_INT;
		value_ptr->data.p.compatiblity_type = QAJSON4C_PRIMITIVE_INT64 | QAJSON4C_PRIMITIVE_INT;
	}
	value_ptr->data.p.compatiblity_type |= QAJSON4C_PRIMITIVE_DOUBLE;
}
void QAJSON4C_set_uint64(QAJSON4C_Value* value_ptr, uint64_t value)
{
	value_ptr->data.flag.type = QAJSON4C_PRIMITIVE;
	value_ptr->data.p.data.u = value;

	if (value > UINT32_MAX) {
		value_ptr->data.p.storage_type = QAJSON4C_PRIMITIVE_UINT64;
		value_ptr->data.p.compatiblity_type = QAJSON4C_PRIMITIVE_UINT64;
	} else {
		value_ptr->data.p.storage_type = QAJSON4C_PRIMITIVE_UINT;
		value_ptr->data.p.compatiblity_type = QAJSON4C_PRIMITIVE_UINT64 | QAJSON4C_PRIMITIVE_UINT;
	}
	if (value <= INT64_MAX) {
		value_ptr->data.p.compatiblity_type |= QAJSON4C_PRIMITIVE_INT64;
	}
	if (value <= INT32_MAX) {
		value_ptr->data.p.compatiblity_type |= QAJSON4C_PRIMITIVE_INT;
	}
	value_ptr->data.p.compatiblity_type |= QAJSON4C_PRIMITIVE_DOUBLE;
}

void QAJSON4C_set_double(QAJSON4C_Value* value_ptr, double value) {
	value_ptr->data.flag.type = QAJSON4C_PRIMITIVE;
	value_ptr->data.p.data.d = value;
	value_ptr->data.p.storage_type = QAJSON4C_PRIMITIVE_DOUBLE;
}

void QAJSON4C_set_string_var( QAJSON4C_Value* value_ptr, string_ref_args args ) {
    assert( args.str != NULL );
    if ( args.len == 0 ) {
        args.len = strlen( args.str );
    }

    // check that in case a copy is requested the builder is specified!
    assert( args.ref || args.builder != NULL );

    if ( args.ref ) {
        value_ptr->data.flag.type = QAJSON4C_STRING;
        value_ptr->data.s.s = args.str;
        value_ptr->data.s.count = args.len;
    } else {
        if ( args.len <= INLINE_STRING_SIZE ) {
            value_ptr->data.flag.type = QAJSON4C_INLINE_STRING;
            value_ptr->data.ss.count = (uint8_t)args.len;
            memcpy( &value_ptr->data.ss.s, args.str, args.len );
            value_ptr->data.ss.s[args.len - 1] = '\0';
        } else {
            value_ptr->data.flag.type = QAJSON4C_STRING;
            value_ptr->data.s.count = (uint8_t)args.len;
            char* new_string = QAJSON4C_builder_pop_string( args.builder, args.len );
            memcpy( new_string, args.str, args.len );
            value_ptr->data.s.s = new_string;
            new_string[args.len - 1] = '\0';
        }
    }
}

// Modifications that require the builder as argument (as memory might has to be acquired in buffer)
void QAJSON4C_set_array( QAJSON4C_Value* value_ptr, unsigned count, QAJSON4C_Builder* builder ) {
    value_ptr->data.flag.type = QAJSON4C_ARRAY;
    value_ptr->data.array.top = QAJSON4C_builder_pop_values( builder, count );
    value_ptr->data.array.count = count;
}

QAJSON4C_Value* QAJSON4C_array_get_rw( QAJSON4C_Value* value_ptr, unsigned index ) {
    assert( QAJSON4C_is_array( value_ptr ) );
    return &value_ptr->data.array.top[index];
}

void QAJSON4C_set_object( QAJSON4C_Value* value_ptr, unsigned count, QAJSON4C_Builder* builder ) {
    value_ptr->data.flag.type = QAJSON4C_OBJECT;
    value_ptr->data.obj.top = QAJSON4C_builder_pop_members( builder, count );
    value_ptr->data.obj.count = count;
}

QAJSON4C_Member* QAJSON4C_object_get_member_rw( QAJSON4C_Value* value_ptr, unsigned index ) {
    assert( QAJSON4C_is_object( value_ptr ) );
    return &value_ptr->data.obj.top[index];
}

QAJSON4C_Value* QAJSON4C_member_get_key_rw( QAJSON4C_Member* member_ptr ) {
    return &member_ptr->key;
}

QAJSON4C_Value* QAJSON4C_member_get_value_rw( QAJSON4C_Member* member_ptr ) {
    return &member_ptr->value;
}

void QAJSON4C_builder_init( QAJSON4C_Builder* me, void* buff, size_t buff_size ) {
    me->buffer = buff;
    me->buffer_size = buff_size;

    // objects grow from front to end (and always contains a document
    me->cur_obj_pos = sizeof(QAJSON4C_Document);
    // strings from end to front
    me->cur_str_pos = buff_size - 1;
}

QAJSON4C_Document* QAJSON4C_builder_get_document( QAJSON4C_Builder* builder ) {
    return (QAJSON4C_Document*)builder->buffer;
}

static QAJSON4C_Value* QAJSON4C_builder_pop_values( QAJSON4C_Builder* builder, size_type count ) {
    QAJSON4C_Value* new_pointer = (QAJSON4C_Value*)&builder->buffer[builder->cur_obj_pos];
    builder->cur_obj_pos += count * sizeof(QAJSON4C_Value);
    assert( builder->cur_obj_pos < builder->cur_str_pos );
    for( size_type i = 0; i < count; i++ ) {
        new_pointer[i].data.flag.type = QAJSON4C_NULL;
    }
    return new_pointer;
}

static QAJSON4C_Member* QAJSON4C_builder_pop_members( QAJSON4C_Builder* builder, size_type count ) {
    QAJSON4C_Member* new_pointer = (QAJSON4C_Member*)&builder->buffer[builder->cur_obj_pos];
    builder->cur_obj_pos += count * sizeof(QAJSON4C_Member);
    assert( builder->cur_obj_pos < builder->cur_str_pos );

    for( size_type i = 0; i < count; i++ ) {
        new_pointer[i].key.data.flag.type = QAJSON4C_NULL;
        new_pointer[i].value.data.flag.type = QAJSON4C_NULL;
    }

    return new_pointer;
}

static char* QAJSON4C_builder_pop_string( QAJSON4C_Builder* builder, size_type length ) {
    // strings grow from back to the front!
    builder->cur_str_pos -= length * sizeof(char);
    assert( builder->cur_obj_pos < builder->cur_str_pos );
    return (char*)(&builder->buffer[builder->cur_str_pos]);

}

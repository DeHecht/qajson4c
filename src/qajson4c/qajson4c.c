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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>

#include <qajson4c/qajson4c.h>

#if (__STDC_VERSION__ >= 199901L)
#define QAJ4C_SNPRINTF snprintf
#define QAJ4C_ASSERT(arg) if (QAJ4C_ERR_FUNCTION && !(arg)) (*QAJ4C_ERR_FUNCTION)(__func__, #arg)
#define FMT_UINT64_T "ju"
#define FMT_INT64_T  "jd"
#else
#define QAJ4C_SNPRINTF(s, n, format, arg) sprintf(s, format, arg)
#define QAJ4C_ASSERT(arg) if (QAJ4C_ERR_FUNCTION && !(arg)) (*QAJ4C_ERR_FUNCTION)("???", #arg)
#ifdef __LP64__
#  define FMT_UINT64_T "lu"
#  define FMT_INT64_T  "ld"
#else
#  define FMT_UINT64_T "lu"
#  define FMT_INT64_T  "l"
#endif
#endif


#define INLINE_STRING_SIZE (sizeof(uintptr_t) + sizeof(size_type) - sizeof(uint8_t) * 2)

typedef uint32_t size_type;
typedef uint16_t half_size_type;

typedef enum QAJ4C_Type {
    QAJ4C_NULL = 0,
    QAJ4C_UNSPECIFIED,
    QAJ4C_OBJECT,
    QAJ4C_OBJECT_SORTED,
    QAJ4C_ARRAY,
    QAJ4C_STRING,
    QAJ4C_STRING_REF,
    QAJ4C_INLINE_STRING,
    QAJ4C_PRIMITIVE,
    QAJ4C_ERROR_DESCRIPTION
} QAJ4C_Type;

typedef enum QAJ4C_Primitive_type {
    QAJ4C_PRIMITIVE_BOOL = (1 << 0),
    QAJ4C_PRIMITIVE_INT = (1 << 1),
    QAJ4C_PRIMITIVE_INT64 = (1 << 2),
    QAJ4C_PRIMITIVE_UINT = (1 << 3),
    QAJ4C_PRIMITIVE_UINT64 = (1 << 4),
    QAJ4C_PRIMITIVE_DOUBLE = (1 << 5)
} QAJ4C_Primitive_type;

typedef struct QAJ4C_Object {
    QAJ4C_Member* top;
    size_type count;
} QAJ4C_Object;

typedef struct QAJ4C_Array {
    QAJ4C_Value* top;
    size_type count;
} QAJ4C_Array;

typedef struct QAJ4C_String {
    const char* s;
    size_type count;
} QAJ4C_String;

typedef struct QAJ4C_Error_information {
    const char* json;
    size_type json_pos;
    size_type err_no;
} QAJ4C_Error_information;

typedef struct QAJ4C_Error {
    QAJ4C_Error_information* info;
    size_type padding;
} QAJ4C_Error;

/**
 * Instead of storing a pointer to a char* like in QAJ4C_String we can QAJ4C_INLINE the
 * char in the struct. This saves a) the dereferencing, b) additional memory.
 * As the string has to be small the count value can also be smaller, to additionally
 * store some more chars.
 * On 32-Bit => 6 chars + \0, on 64-Bit => 10 chars + \0.
 */
typedef struct QAJ4C_Short_string {
    char s[INLINE_STRING_SIZE + 1];
    uint8_t count;
} QAJ4C_Short_string;

typedef struct QAJ4C_Primitive {
    union primitive {
        double d;
        uint64_t u;
        int64_t i;
        bool b;
    } data;
} QAJ4C_Primitive;

struct QAJ4C_Value {
    union {
        uintptr_t padding_ptr;
        uint32_t padding_min;
    } pad;
    size_type padding_2;
    size_type type;
}; /* minimal 3 * 4 Byte = 12, at 64 Bit 16 Byte */

struct QAJ4C_Member {
    QAJ4C_Value key;
    QAJ4C_Value value;
};

struct QAJ4C_Document {
    QAJ4C_Value root_element;
};

typedef struct QAJ4C_Parser {
    const char* json;
    size_type json_pos;
    size_type json_len;

    char* buffer;
    size_type buffer_size;

    int opts;

    /*
     * the max depth is there to prevent stack overflow caused by a huge amount of nested
     * json objects/arrays.
     */
    uint8_t max_depth;

    size_type curr_buffer_pos;
    size_type curr_buffer_str_pos;

    /* for statistics */
    size_type amount_elements;

    QAJ4C_Builder* builder;
    bool insitu_parsing;

    bool error;
    QAJ4C_ERROR_CODES err_no;

    QAJ4C_realloc_fn realloc_function;
} QAJ4C_Parser;

static bool QAJ4C_is_primitive( const QAJ4C_Value* value_ptr );
static void QAJ4C_parse_first_pass( QAJ4C_Parser* parser );
static QAJ4C_Document* QAJ4C_parse_second_pass( QAJ4C_Parser* parser );
static QAJ4C_Document* QAJ4C_create_error_description( QAJ4C_Parser* parser );

static int QAJ4C_strcmp( const QAJ4C_Value* lhs, const QAJ4C_Value* rhs );
static void QAJ4C_skip_whitespaces_and_comments( QAJ4C_Parser* parser );
static int QAJ4C_compare_members( const void* p1, const void * p2 );

static void QAJ4C_first_pass_process( QAJ4C_Parser* parser, uint8_t depth );
static void QAJ4C_first_pass_object( QAJ4C_Parser* parser, uint8_t depth );
static void QAJ4C_first_pass_array( QAJ4C_Parser* parser, uint8_t depth );

static void QAJ4C_second_pass_process( QAJ4C_Parser* parser, QAJ4C_Value* result_ptr );

static void QAJ4C_parse_array( QAJ4C_Parser* parser, QAJ4C_Value* result_ptr );
static void QAJ4C_parse_object( QAJ4C_Parser* parser, QAJ4C_Value* result_ptr );
static void QAJ4C_parse_string( QAJ4C_Parser* parser, QAJ4C_Value* result_ptr );
static void QAJ4C_parse_constant( QAJ4C_Parser* parser, QAJ4C_Value* result_ptr );
static void QAJ4C_parse_numeric_value( QAJ4C_Parser* parser, QAJ4C_Value* result_ptr );
static size_t QAJ4C_print_value( const QAJ4C_Value* value_ptr, char* buffer, size_t buffer_size, size_t index );
static bool QAJ4C_parser_realloc( QAJ4C_Parser* parser, size_type new_size );

static void QAJ4C_builder_validate_buffer( QAJ4C_Builder* builder );
static QAJ4C_Value* QAJ4C_builder_pop_values( QAJ4C_Builder* builder, size_type count );
static QAJ4C_Member* QAJ4C_builder_pop_members( QAJ4C_Builder* builder, size_type count );
static char* QAJ4C_builder_pop_string( QAJ4C_Builder* builder, size_type length );

static QAJ4C_INLINE QAJ4C_Type QAJ4C_get_type( const QAJ4C_Value* value_ptr ) {
    return value_ptr->type & 0xFF;
}

/*
 * This method will return less types as the get_type method (reducing all string types
 * to string or all object types to object)
 */
static QAJ4C_INLINE QAJ4C_Type QAJ4C_get_basic_type( const QAJ4C_Value* value_ptr ) {
    QAJ4C_Type type = QAJ4C_get_type ( value_ptr );
    switch( type )
    {
    case QAJ4C_OBJECT_SORTED:
        return QAJ4C_OBJECT;
    case QAJ4C_INLINE_STRING:
    case QAJ4C_STRING_REF:
        return QAJ4C_STRING;
    default:
        return type;
    }
}

static QAJ4C_INLINE void QAJ4C_set_type( QAJ4C_Value* value_ptr, QAJ4C_Type type ) {
    value_ptr->type = (value_ptr->type & 0xFFFFF00) | (type & 0xFF);
}

static QAJ4C_INLINE void QAJ4C_set_storage_type( QAJ4C_Value* value_ptr, uint8_t type ) {
    value_ptr->type = (value_ptr->type & 0xFF00FF) | (((size_type)type) << 8);
}

static QAJ4C_INLINE void QAJ4C_set_compatibility_types( QAJ4C_Value* value_ptr, uint8_t type ) {
    value_ptr->type = (value_ptr->type & 0xFFFF) | (((size_type)type) << 16);
}

static QAJ4C_INLINE void QAJ4C_add_compatibility_types( QAJ4C_Value* value_ptr, uint8_t type ) {
    value_ptr->type = value_ptr->type | (((size_type)type) << 16);
}

static QAJ4C_INLINE uint8_t QAJ4C_get_storage_type( const QAJ4C_Value* value_ptr ) {
    return (value_ptr->type >> 8) & 0xFF;
}

static QAJ4C_INLINE uint8_t QAJ4C_get_compatibility_types( const QAJ4C_Value* value_ptr ) {
    return (value_ptr->type >> 16) & 0xFF;
}

static QAJ4C_INLINE unsigned QAJ4C_do_calculate_max_buffer_size( const QAJ4C_Parser* parser ) {
	if (!parser->insitu_parsing) {
		return sizeof(QAJ4C_Document) + (parser->amount_elements) * sizeof(QAJ4C_Value) + parser->curr_buffer_str_pos;
	}
	return sizeof(QAJ4C_Document) + parser->amount_elements * sizeof(QAJ4C_Value);
}

static void QAJ4C_std_err_function( const char* function_name, const char* assertion_msg ) {
    fprintf(stderr, "%s: %s\n", function_name, assertion_msg);
    raise(SIGABRT);
}

const QAJ4C_Value* QAJ4C_object_get_unsorted(QAJ4C_Object* obj_ptr, QAJ4C_Value* str_ptr);
const QAJ4C_Value* QAJ4C_object_get_sorted(QAJ4C_Object* obj_ptr, QAJ4C_Value* str_ptr);

static const size_t QAJ4C_REALLOC_FACTOR = sizeof(QAJ4C_Value) / 2;
static const char QAJ4C_NULL_STR[] = "null";
static const char QAJ4C_TRUE_STR[] = "true";
static const char QAJ4C_FALSE_STR[] = "false";
static const unsigned QAJ4C_NULL_STR_LEN = sizeof(QAJ4C_NULL_STR) / sizeof(QAJ4C_NULL_STR[0]);
static const unsigned QAJ4C_TRUE_STR_LEN = sizeof(QAJ4C_TRUE_STR) / sizeof(QAJ4C_TRUE_STR[0]);
static const unsigned QAJ4C_FALSE_STR_LEN = sizeof(QAJ4C_FALSE_STR) / sizeof(QAJ4C_FALSE_STR[0]);
static QAJ4C_fatal_error_fn QAJ4C_ERR_FUNCTION = &QAJ4C_std_err_function;

static QAJ4C_Document* QAJ4C_create_error_description( QAJ4C_Parser* parser ) {
    QAJ4C_Builder builder;
    QAJ4C_Document* document;
    QAJ4C_Error_information* err_info;

    /* not enough space to store error information in buffer... */
    if (parser->buffer_size < sizeof(QAJ4C_Document) + sizeof(QAJ4C_Error_information)) {
        return NULL;
    }

    QAJ4C_builder_init(&builder, parser->buffer, parser->buffer_size);
    document = QAJ4C_builder_get_document(&builder);
    QAJ4C_set_type(&document->root_element, QAJ4C_ERROR_DESCRIPTION);

    err_info = (QAJ4C_Error_information*)(builder.buffer + builder.cur_obj_pos);
    err_info->err_no = parser->err_no;
    err_info->json = parser->json;
    err_info->json_pos = parser->json_pos;

    ((QAJ4C_Error*)document)->info = err_info;

    return document;
}

void QAJ4C_register_fatal_error_function( QAJ4C_fatal_error_fn function ) {
    QAJ4C_ERR_FUNCTION = *function;
}

const QAJ4C_Document* QAJ4C_parse( const char* json, void* buffer, size_t buffer_size ) {
	return QAJ4C_parse_opt(json, 0, 0, buffer, &buffer_size);
}

const QAJ4C_Document* QAJ4C_parse_opt( const char* json, size_t json_len, int opts, void* buffer, size_t* buffer_size ) {
    QAJ4C_Parser parser;
    parser.json = json;
    parser.json_len = json_len;
    parser.opts = opts;

    parser.buffer = buffer;
    parser.buffer_size = *buffer_size;
    parser.realloc_function = NULL;
    parser.insitu_parsing = ((opts & 1) == 1);

    QAJ4C_parse_first_pass(&parser);

    if (parser.error) {
        return QAJ4C_create_error_description(&parser);
    }

    parser.buffer_size = QAJ4C_do_calculate_max_buffer_size(&parser);
    if (*buffer_size < parser.buffer_size) {
        parser.error = true;
        parser.err_no = QAJ4C_ERROR_STORAGE_BUFFER_TO_SMALL;
        return QAJ4C_create_error_description(&parser);
    }

    *buffer_size = parser.buffer_size;

    return QAJ4C_parse_second_pass(&parser);
}


const QAJ4C_Document* QAJ4C_parse_insitu( char* json, void* buffer, size_t buffer_size ) {
	return QAJ4C_parse_opt_insitu(json, 0, 0, buffer, &buffer_size);
}

/*
 * It is kind of cheating, but internally we use the opt value 1 for insitu parsing.
 * For this reason the value 1 is reserved!
 *
 */
const QAJ4C_Document* QAJ4C_parse_opt_insitu( char* json, size_t json_len, int opts, void* buffer, size_t* buffer_size ) {
	return QAJ4C_parse_opt(json, json_len, opts | 1, buffer, buffer_size);
}


const QAJ4C_Document* QAJ4C_parse_dynamic( const char* json, QAJ4C_realloc_fn realloc_callback ) {
	return QAJ4C_parse_opt_dynamic( json, 0, 0, realloc_callback);
}

const QAJ4C_Document* QAJ4C_parse_opt_dynamic( const char* json, size_t json_len, int opts, QAJ4C_realloc_fn realloc_callback ) {
    static size_type min_size = sizeof(QAJ4C_Document) + sizeof(QAJ4C_Error_information);
    QAJ4C_Parser parser;
    size_t required_size;
    parser.json = json;
    parser.json_len = json_len;
    parser.opts = opts;

    parser.buffer = realloc_callback(NULL, min_size);
    parser.buffer_size = min_size;
    parser.realloc_function = realloc_callback;
    parser.insitu_parsing = false;

    if (parser.buffer == NULL) {
        /* allocation failed ... no parsing possible! */
        return NULL;
    }

    QAJ4C_parse_first_pass(&parser);
    if (parser.error) {
        return QAJ4C_create_error_description(&parser);
    }

    required_size = QAJ4C_do_calculate_max_buffer_size(&parser);
    if (required_size > parser.buffer_size && !QAJ4C_parser_realloc(&parser, required_size)) {
        return QAJ4C_create_error_description(&parser);
    }

    return QAJ4C_parse_second_pass(&parser);
}



unsigned QAJ4C_calculate_max_buffer_size( const char* json ) {
    QAJ4C_Parser parser;
    parser.json = json;
    parser.buffer = NULL;
    parser.buffer_size = 0;
    parser.insitu_parsing = false;
    QAJ4C_parse_first_pass(&parser);
    return QAJ4C_do_calculate_max_buffer_size(&parser);
}

unsigned QAJ4C_calculate_max_buffer_size_insitu( const char* json ) {
    QAJ4C_Parser parser;
    parser.json = json;
    parser.buffer = NULL;
    parser.buffer_size = 0;
    parser.insitu_parsing = true;
    QAJ4C_parse_first_pass(&parser);
    return QAJ4C_do_calculate_max_buffer_size(&parser);
}

static void QAJ4C_first_pass_object( QAJ4C_Parser* parser, uint8_t depth ) {
    size_type old_pos;
    size_type counter;

    if (parser->max_depth < depth) {
        parser->error = true;
        parser->err_no = QAJ4C_ERROR_DEPTH_OVERFLOW;
        return;
    }
    if (parser->buffer != NULL && parser->buffer_size <= parser->curr_buffer_pos) {
        if (!QAJ4C_parser_realloc(parser, parser->buffer_size * QAJ4C_REALLOC_FACTOR)) {
            return; /* memory issues (error set by parser_realloc)! */
        }
    }

    parser->amount_elements++;
    old_pos = parser->curr_buffer_pos;
    counter = 0;
    parser->curr_buffer_pos += sizeof(size_type);
    parser->json_pos++; /* pass over the { char */
    while (!parser->error && parser->json[parser->json_pos] != '}') {
        QAJ4C_skip_whitespaces_and_comments(parser);
        if (counter > 0) {
            if (parser->json[parser->json_pos] != ',') {
                parser->error = true;
                parser->err_no = QAJ4C_ERROR_ARRAY_MISSING_COMMA;
                return;
            }
            parser->json_pos++; /* pass over the , char */
            QAJ4C_skip_whitespaces_and_comments(parser);
        }
        if (parser->json[parser->json_pos] != '}') {
            QAJ4C_parse_string(parser, NULL);
            if (parser->error) {
                return;
            }
            QAJ4C_skip_whitespaces_and_comments(parser);
            if (parser->json[parser->json_pos] != ':') {
                parser->error = true;
                parser->err_no = QAJ4C_ERROR_OBJECT_MISSING_COLON;
                return;
            }
            parser->json_pos++; /* pass over the : char */
            QAJ4C_skip_whitespaces_and_comments(parser);
            QAJ4C_first_pass_process(parser, depth + 1);
            counter++;
        }
        QAJ4C_skip_whitespaces_and_comments(parser);
    }
    parser->json_pos++; /* pass over the } char */
    if (parser->buffer != NULL) {
        *((size_type*)&parser->buffer[old_pos]) = counter;
    }
}

static void QAJ4C_first_pass_array( QAJ4C_Parser* parser, uint8_t depth ) {
    size_type old_pos;
    size_type counter;

    if (parser->max_depth < depth) {
        parser->error = true;
        parser->err_no = QAJ4C_ERROR_DEPTH_OVERFLOW;
        return;
    }
    if (parser->buffer != NULL && parser->buffer_size <= parser->curr_buffer_pos) {
        if (!QAJ4C_parser_realloc(parser, parser->buffer_size * QAJ4C_REALLOC_FACTOR)) {
            return; /* memory issues (error set by parser_realloc)! */
        }
    }
    parser->amount_elements++;
    old_pos = parser->curr_buffer_pos;
    counter = 0;
    parser->curr_buffer_pos += sizeof(size_type);
    parser->json_pos++; /* pass over the [ char */
    while (!parser->error && parser->json[parser->json_pos] != ']') {
        QAJ4C_skip_whitespaces_and_comments(parser);
        if (counter > 0) {
            if (parser->json[parser->json_pos] != ',') {
                parser->error = true;
                parser->err_no = QAJ4C_ERROR_ARRAY_MISSING_COMMA;
                return;
            }
            parser->json_pos++; /* pass over the , char */
            QAJ4C_skip_whitespaces_and_comments(parser);
        }
        if (parser->json[parser->json_pos] != ']') {
            QAJ4C_first_pass_process(parser, depth + 1);
            counter++;
        }
        QAJ4C_skip_whitespaces_and_comments(parser);
    }
    parser->json_pos++; /* pass over the ] char */
    if (parser->buffer != NULL) {
        *((size_type*)&parser->buffer[old_pos]) = counter;
    }
}

static void QAJ4C_parse_numeric_value( QAJ4C_Parser* parser, QAJ4C_Value* result_ptr ) {
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
        i_value = strtol(start_pos, &pos, 0);
    } else {
        use_uint = true;
        u_value = strtoul(start_pos, &pos, 0);
    }

    if (pos == start_pos) {
        parser->error = true;
        parser->err_no = QAJ4C_ERROR_INVALID_NUMBER_FORMAT;
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
        /* FIXME: find a different way to solve this */
        use_int = false;
        use_uint = false;
        use_double = true;
        d_value = strtod(start_pos, &pos);
    }

    if (result_ptr != NULL) {
        if (use_double) {
            QAJ4C_set_double(result_ptr, d_value);
        } else if (use_uint) {
            QAJ4C_set_uint64(result_ptr, u_value);
        } else if (use_int) {
            QAJ4C_set_int64(result_ptr, i_value);
        }
    }
    parser->json_pos += pos - start_pos;
}

static void QAJ4C_skip_whitespaces_and_comments( QAJ4C_Parser* parser ) {
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
                /* also skip comments! */
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

static void QAJ4C_parse_constant( QAJ4C_Parser* parser, QAJ4C_Value* result ) {
    const char* value_ptr = &parser->json[parser->json_pos];
    parser->amount_elements++;

    if (strncmp(QAJ4C_NULL_STR, value_ptr, QAJ4C_NULL_STR_LEN - 1) == 0) {
        if (result != NULL) {
            QAJ4C_set_type(result, QAJ4C_NULL);
        }
        parser->json_pos += QAJ4C_NULL_STR_LEN - 1;
    } else if (strncmp(QAJ4C_FALSE_STR, value_ptr, QAJ4C_FALSE_STR_LEN - 1) == 0
            || strncmp(QAJ4C_TRUE_STR, value_ptr, QAJ4C_TRUE_STR_LEN - 1) == 0) {
        bool constant_value = strncmp(QAJ4C_TRUE_STR, value_ptr, QAJ4C_TRUE_STR_LEN - 1) == 0;
        if (result != NULL) {
            QAJ4C_set_bool(result, constant_value);
        }
        if (constant_value) {
            parser->json_pos += QAJ4C_TRUE_STR_LEN - 1;
        } else {
            parser->json_pos += QAJ4C_FALSE_STR_LEN - 1;
        }
    } else {
        parser->error = true;
        parser->err_no = QAJ4C_ERROR_UNEXPECTED_CHAR;
    }
}

static void QAJ4C_parse_array( QAJ4C_Parser* parser, QAJ4C_Value* result_ptr ) {
    size_type elements;
    QAJ4C_Value* top;
    size_type index;

    if (parser->json[parser->json_pos] != '[') {
        parser->error = true;
        parser->err_no = QAJ4C_ERROR_UNEXPECTED_CHAR;
        return;
    }
    elements = *((size_type*)&parser->buffer[parser->curr_buffer_pos]);
    parser->curr_buffer_pos += sizeof(size_type);

    QAJ4C_set_array(result_ptr, elements, parser->builder);
    top = ((QAJ4C_Array*)result_ptr)->top;

    parser->json_pos++; /* pass over the [ char */
    for (index = 0; index < elements; index++) {
        QAJ4C_skip_whitespaces_and_comments(parser);
        if (parser->json[parser->json_pos] == ',') {
            parser->json_pos++;
            QAJ4C_skip_whitespaces_and_comments(parser);
        }
        QAJ4C_second_pass_process(parser, &top[index]);
    }
    QAJ4C_skip_whitespaces_and_comments(parser);

    /* pass over a trailing , if available */
    if (parser->json[parser->json_pos] == ',') {
        parser->json_pos++;
        QAJ4C_skip_whitespaces_and_comments(parser);
    }
    parser->json_pos++; /* pass over the ] char */

}

static void QAJ4C_parse_object( QAJ4C_Parser* parser, QAJ4C_Value* result_ptr ) {
    size_type elements;
    QAJ4C_Member* top;
    size_type index;

    if (parser->json[parser->json_pos] != '{') {
        parser->error = true;
        parser->err_no = QAJ4C_ERROR_UNEXPECTED_CHAR;
        return;
    }

    elements = *((size_type*)&parser->buffer[parser->curr_buffer_pos]);
    parser->curr_buffer_pos += sizeof(size_type);

    QAJ4C_set_object(result_ptr, elements, parser->builder);
    top = ((QAJ4C_Object*)result_ptr)->top;

    parser->json_pos++; /* pass over the { char */

    for (index = 0; index < elements; index++) {
        QAJ4C_skip_whitespaces_and_comments(parser);
        if (parser->json[parser->json_pos] == ',') {
            parser->json_pos++;
            QAJ4C_skip_whitespaces_and_comments(parser);
        }
        QAJ4C_parse_string(parser, &top[index].key);
        QAJ4C_skip_whitespaces_and_comments(parser);
        parser->json_pos++; /* pass over the : */
        QAJ4C_skip_whitespaces_and_comments(parser);
        QAJ4C_second_pass_process(parser, &top[index].value);
    }
    QAJ4C_skip_whitespaces_and_comments(parser);
    /* pass over a trailing , if available */
    if (parser->json[parser->json_pos] == ',') {
        parser->json_pos++;
        QAJ4C_skip_whitespaces_and_comments(parser);
    }

    if ( elements > 1) {
        QAJ4C_object_optimize(result_ptr);
    }

    parser->json_pos++; /* pass over the } char */
}

/**
 * This method analyzes a string and moves the json_pos index to the character after
 * the ending ".
 */
static void QAJ4C_parse_string( QAJ4C_Parser* parser, QAJ4C_Value* value_ptr ) {
    size_type prev_pos;
    bool escape_char = false;
    bool in_string = true;

    if (parser->json[parser->json_pos] != '"') {
        parser->error = true;
        parser->err_no = QAJ4C_ERROR_INVALID_STRING_START;
        return;
    }
    parser->amount_elements++;
    parser->json_pos++; /* move to first char in string */
    prev_pos = parser->json_pos;

    /* process until end of string and return the string size */
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
        parser->err_no = QAJ4C_ERROR_BUFFER_TRUNCATED;
    } else {
        size_type size = parser->json_pos - prev_pos - 1;
        if (value_ptr != NULL) {
            if (parser->insitu_parsing) {
                char* insitu_string = (char*)parser->json;
                insitu_string[parser->json_pos - 1] = '\0';
                QAJ4C_set_string_ref2(value_ptr, &parser->json[prev_pos], size);
            } else {
			    QAJ4C_set_string_copy2(value_ptr, parser->builder, &parser->json[prev_pos], size);
            }
        } else if (size > INLINE_STRING_SIZE) {
            /* the determined string size + '\0' */
            parser->curr_buffer_str_pos += size + 1;
        }
    }
    /* we have already proceed the " character within the while loop */
}

static void QAJ4C_first_pass_process( QAJ4C_Parser* parser, uint8_t depth ) {
    QAJ4C_skip_whitespaces_and_comments(parser);
    switch (parser->json[parser->json_pos]) {
    case '\0':
        parser->error = true;
        parser->err_no = QAJ4C_ERROR_BUFFER_TRUNCATED;
        break;
    case '{':
        QAJ4C_first_pass_object(parser, depth);
        break;
    case '[':
        QAJ4C_first_pass_array(parser, depth);
        break;
    case '"':
        QAJ4C_parse_string(parser, NULL);
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
        QAJ4C_parse_numeric_value(parser, NULL);
        break;
    case 't':
    case 'f':
    case 'n':
        QAJ4C_parse_constant(parser, NULL);
        break;
    }
}

/*
 * Strategy:
 * Walk through the json message and create counters how many members the detected arrays/objects have.
 * additionally count how many "memory" is required to store the DOM (exclusive and inclusive the strings).
 * With this information we can reserve the correct amount of memory for each object at once ...
 * the data should be stored on the end of the DOM "space" so it will be overridden by the DOM elements and thus
 * will not require additional memory.
 */
static void QAJ4C_parse_first_pass( QAJ4C_Parser* parser ) {
    parser->json_pos = 0;
    parser->curr_buffer_pos = 0;
    parser->curr_buffer_str_pos = 0;
    parser->max_depth = 32;
    parser->error = false;
    parser->err_no = 0;
    parser->amount_elements = 0;

    QAJ4C_first_pass_process(parser, 0);
    if (!parser->error) {
        QAJ4C_skip_whitespaces_and_comments(parser);
        if (parser->json[parser->json_pos] != '\0') {
            parser->error = true;
            parser->err_no = QAJ4C_ERROR_UNEXPECTED_JSON_APPENDIX;
        }
    }
}

/*
 * prepare the layout and already create the document element!
 * then call the second_pass_process method
 */
static QAJ4C_Document* QAJ4C_parse_second_pass( QAJ4C_Parser* parser ) {
    QAJ4C_Builder builder;
    QAJ4C_Document* document;

    if (parser->curr_buffer_pos > 0) {
		size_type required_object_storage = sizeof(QAJ4C_Document) + parser->amount_elements * sizeof(QAJ4C_Value);
		size_type required_tempoary_storage = parser->curr_buffer_pos + sizeof(size_type);
        size_type copy_to_index = required_object_storage - required_tempoary_storage;

		/*
		 * Currently the sizes of the objects are placed in memory starting at index 0 of the buffer.
		 * To avoid, that the first object will override the valuable information gathered in the first
		 * pass, the data is moved to the end of the required_object_storage. This will cause the last
		 * object to be the first that will override its own valuable information. But at a moment where
		 * the data is safe to override!
		 */
        memmove(parser->buffer + copy_to_index, parser->buffer, required_tempoary_storage);
        parser->curr_buffer_pos = copy_to_index;
    }

    parser->json_pos = 0;
    parser->amount_elements = 0;

    QAJ4C_builder_init(&builder, parser->buffer, parser->buffer_size);
    parser->builder = &builder;

    document = QAJ4C_builder_get_document(&builder);

    QAJ4C_second_pass_process(parser, &document->root_element);
    return document;
}

size_t QAJ4C_sprint( const QAJ4C_Document* document, char* buffer, size_t buffer_size ) {
    size_t index = QAJ4C_print_value(&document->root_element, buffer, buffer_size, 0);
    buffer[index] = '\0';
    return index;
}

static size_t QAJ4C_copy_string(char* buffer, size_t buffer_length, size_t buffer_index, const char* string, size_t string_length) {
	size_t string_index;
	if ( buffer_index + string_length > buffer_length ) {
		string_length = buffer_length - buffer_index;
	}

	for (string_index = 0; string_index < string_length; ++string_index) {
		buffer[buffer_index++] = string[string_index];
	}
	return buffer_index;
}

static size_t QAJ4C_print_value( const QAJ4C_Value* value_ptr, char* buffer, size_t buffer_size, size_t index ) {
    size_type i;

	if (index >= buffer_size) return index;

    switch (QAJ4C_get_type(value_ptr)) {
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
                index = QAJ4C_print_value(&top[i].key, buffer, buffer_size, index);
        		if (index >= buffer_size) return index;
        		buffer[index++] = ':';
                index = QAJ4C_print_value(&top[i].value, buffer, buffer_size, index);
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
            index = QAJ4C_print_value(&top[i], buffer, buffer_size, index);
        }
		buffer[index++] = '}';
        break;
    }
    case QAJ4C_PRIMITIVE: {
        switch (QAJ4C_get_storage_type(value_ptr)) {
        case QAJ4C_PRIMITIVE_BOOL:
        	if (((QAJ4C_Primitive*)value_ptr)->data.b) {
        		index = QAJ4C_copy_string(buffer, buffer_size, index, QAJ4C_TRUE_STR, QAJ4C_TRUE_STR_LEN);
        	} else {
        		index = QAJ4C_copy_string(buffer, buffer_size, index, QAJ4C_FALSE_STR, QAJ4C_FALSE_STR_LEN);
        	}
            index += QAJ4C_SNPRINTF(buffer + index, buffer_size - index, "%s", ((QAJ4C_Primitive*)value_ptr)->data.b ? "true" : "false");
            break;
        case QAJ4C_PRIMITIVE_INT:
        case QAJ4C_PRIMITIVE_INT64:
            index += QAJ4C_SNPRINTF(buffer + index, buffer_size - index, "%"FMT_INT64_T, ((QAJ4C_Primitive*)value_ptr)->data.i);
            break;
        case QAJ4C_PRIMITIVE_UINT:
        case QAJ4C_PRIMITIVE_UINT64:
            index += QAJ4C_SNPRINTF(buffer + index, buffer_size - index, "%"FMT_UINT64_T, ((QAJ4C_Primitive*)value_ptr)->data.u);
            break;
        case QAJ4C_PRIMITIVE_DOUBLE:
            index += QAJ4C_SNPRINTF(buffer + index, buffer_size - index, "%f", ((QAJ4C_Primitive*)value_ptr)->data.d);
            break;
        default:
            QAJ4C_ASSERT(false);
        }
        break;
    }
    case QAJ4C_NULL: {
		index = QAJ4C_copy_string(buffer, buffer_size, index, QAJ4C_NULL_STR, QAJ4C_NULL_STR_LEN);
        break;
    }
    case QAJ4C_STRING_REF:
    case QAJ4C_INLINE_STRING:
    case QAJ4C_STRING: {
        const char* base = QAJ4C_get_string(value_ptr);
        size_type base_len = QAJ4C_get_string_length(value_ptr);

		if (index >= buffer_size) return index;
		buffer[index++] = '"';

    	for( i = 0; i < base_len && index >= buffer_size; ++i) {
    		if ( base[i] == '"') {
    			buffer[index++] = '\\';
    		}
    		buffer[index++] = base[i];
    	}

		if (index >= buffer_size) return index;
		buffer[index++] = '"';
        break;
    }
    default:
        QAJ4C_ASSERT(false);
    }
    return index;
}

static bool QAJ4C_parser_realloc( QAJ4C_Parser* parser, size_type required_size ) {
    void* tmp;
    if ( parser->realloc_function == NULL )
    {
        parser->error = true;
        parser->err_no = QAJ4C_ERROR_STORAGE_BUFFER_TO_SMALL;
        return false;
    }

    tmp = parser->realloc_function(parser->buffer, required_size);
    if (tmp == NULL) {
        parser->error = true;
        parser->err_no = QAJ4C_ERROR_ALLOCATION_ERROR;
        return false;
    }
    parser->buffer = tmp;
    parser->buffer_size = required_size;
    return true;
}

static void QAJ4C_second_pass_process( QAJ4C_Parser* parser, QAJ4C_Value* result_ptr ) {
    QAJ4C_skip_whitespaces_and_comments(parser);
    switch (parser->json[parser->json_pos]) {
    case '{':
        QAJ4C_parse_object(parser, result_ptr);
        break;
    case '[':
        QAJ4C_parse_array(parser, result_ptr);
        break;
    case '"':
        QAJ4C_parse_string(parser, result_ptr);
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
        QAJ4C_parse_numeric_value(parser, result_ptr);
        break;
    case 't':
    case 'f':
    case 'n':
        QAJ4C_parse_constant(parser, result_ptr);
        break;
    }
}

/*
 * The comparison will first check on the string size and then on the content as we
 * only require this for matching purposes. The string length is also stored within
 * the objects, so check is not expensive!
 */
static int QAJ4C_strcmp( const QAJ4C_Value* lhs, const QAJ4C_Value* rhs ) {
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
static int QAJ4C_compare_members( const void* p1, const void * p2 ) {
    const QAJ4C_Member* left = p1;
    const QAJ4C_Member* right = p2;

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

static bool QAJ4C_is_primitive( const QAJ4C_Value* value_ptr ) {
    return value_ptr != NULL && QAJ4C_get_type(value_ptr) == QAJ4C_PRIMITIVE;
}

/**
 * Getter and setter
 */

const QAJ4C_Value* QAJ4C_get_root_value( const QAJ4C_Document* document ) {
    return &document->root_element;
}

QAJ4C_Value* QAJ4C_get_root_value_rw( QAJ4C_Document* document ) {
    return &document->root_element;
}

bool QAJ4C_is_string( const QAJ4C_Value* value_ptr ) {
    return value_ptr != NULL && (QAJ4C_get_type(value_ptr) == QAJ4C_INLINE_STRING
                             || QAJ4C_get_type(value_ptr) == QAJ4C_STRING
                             || QAJ4C_get_type(value_ptr) == QAJ4C_STRING_REF);
}

const char* QAJ4C_get_string( const QAJ4C_Value* value_ptr ) {
    QAJ4C_ASSERT(QAJ4C_is_string(value_ptr));
    if (QAJ4C_get_type(value_ptr) == QAJ4C_INLINE_STRING) {
        return ((QAJ4C_Short_string*)value_ptr)->s;
    }
    return ((QAJ4C_String*)value_ptr)->s;
}

unsigned QAJ4C_get_string_length( const QAJ4C_Value* value_ptr ) {
    QAJ4C_ASSERT(QAJ4C_is_string(value_ptr));
    if (QAJ4C_get_type(value_ptr) == QAJ4C_INLINE_STRING) {
        return ((QAJ4C_Short_string*)value_ptr)->count;
    }
    return ((QAJ4C_String*)value_ptr)->count;
}

int QAJ4C_string_cmp2( const QAJ4C_Value* value_ptr, const char* str, size_t len ) {
    QAJ4C_Value wrapper_value;
    QAJ4C_ASSERT(QAJ4C_is_string(value_ptr));
    QAJ4C_set_string_ref2(&wrapper_value, str, len);
    return QAJ4C_strcmp(value_ptr, &wrapper_value);
}

bool QAJ4C_is_object( const QAJ4C_Value* value_ptr ) {
    return value_ptr != NULL && (QAJ4C_get_type(value_ptr) == QAJ4C_OBJECT || QAJ4C_get_type(value_ptr) == QAJ4C_OBJECT_SORTED);
}

bool QAJ4C_is_array( const QAJ4C_Value* value_ptr ) {
    return value_ptr != NULL && QAJ4C_get_type(value_ptr) == QAJ4C_ARRAY;
}

bool QAJ4C_is_int( const QAJ4C_Value* value_ptr ) {
    return QAJ4C_is_primitive(value_ptr) && (QAJ4C_get_compatibility_types(value_ptr) & QAJ4C_PRIMITIVE_INT) != 0;
}

bool QAJ4C_is_int64( const QAJ4C_Value* value_ptr ) {
    return QAJ4C_is_primitive(value_ptr) && (QAJ4C_get_compatibility_types(value_ptr) & QAJ4C_PRIMITIVE_INT64) != 0;
}

bool QAJ4C_is_uint( const QAJ4C_Value* value_ptr ) {
    return QAJ4C_is_primitive(value_ptr) && (QAJ4C_get_compatibility_types(value_ptr) & QAJ4C_PRIMITIVE_UINT) != 0;
}

bool QAJ4C_is_uint64( const QAJ4C_Value* value_ptr ) {
    return QAJ4C_is_primitive(value_ptr) && (QAJ4C_get_compatibility_types(value_ptr) & QAJ4C_PRIMITIVE_UINT64) != 0;
}

bool QAJ4C_is_double( const QAJ4C_Value* value_ptr ) {
    return QAJ4C_is_primitive(value_ptr) && (QAJ4C_get_compatibility_types(value_ptr) & QAJ4C_PRIMITIVE_DOUBLE) != 0;
}

bool QAJ4C_is_bool( const QAJ4C_Value* value_ptr ) {
    return QAJ4C_is_primitive(value_ptr) && (QAJ4C_get_compatibility_types(value_ptr) & QAJ4C_PRIMITIVE_BOOL) != 0;
}

int32_t QAJ4C_get_int( const QAJ4C_Value* value_ptr ) {
    QAJ4C_ASSERT(QAJ4C_is_int(value_ptr));
    return (int32_t)((QAJ4C_Primitive*)value_ptr)->data.i;
}

int64_t QAJ4C_get_int64( const QAJ4C_Value* value_ptr ) {
    QAJ4C_ASSERT(QAJ4C_is_int64(value_ptr));
    return ((QAJ4C_Primitive*)value_ptr)->data.i;
}

uint32_t QAJ4C_get_uint( const QAJ4C_Value* value_ptr ) {
    QAJ4C_ASSERT(QAJ4C_is_uint(value_ptr));
    return (uint32_t)((QAJ4C_Primitive*)value_ptr)->data.u;
}

uint64_t QAJ4C_get_uint64( const QAJ4C_Value* value_ptr ) {
    QAJ4C_ASSERT(QAJ4C_is_uint64(value_ptr));
    return ((QAJ4C_Primitive*)value_ptr)->data.u;
}

double QAJ4C_get_double( const QAJ4C_Value* value_ptr ) {
    QAJ4C_ASSERT(QAJ4C_is_double(value_ptr));
    switch (QAJ4C_get_storage_type(value_ptr)) {
    case QAJ4C_PRIMITIVE_INT:
    case QAJ4C_PRIMITIVE_INT64:
        return (double)((QAJ4C_Primitive*)value_ptr)->data.i;
    case QAJ4C_PRIMITIVE_UINT:
    case QAJ4C_PRIMITIVE_UINT64:
        return (double)((QAJ4C_Primitive*)value_ptr)->data.u;
    }
    return ((QAJ4C_Primitive*)value_ptr)->data.d;
}

bool QAJ4C_get_bool( const QAJ4C_Value* value_ptr ) {
    QAJ4C_ASSERT(QAJ4C_is_bool(value_ptr));
    return ((QAJ4C_Primitive*)value_ptr)->data.b;
}

bool QAJ4C_is_not_set( const QAJ4C_Value* value_ptr ) {
    return value_ptr == NULL;
}

bool QAJ4C_is_null( const QAJ4C_Value* value_ptr ) {
    return value_ptr == NULL || QAJ4C_get_type(value_ptr) == QAJ4C_NULL;
}

bool QAJ4C_is_error( const QAJ4C_Value* value_ptr ) {
    return value_ptr != NULL && QAJ4C_get_type(value_ptr) == QAJ4C_ERROR_DESCRIPTION;
}

const char* QAJ4C_error_get_json( const QAJ4C_Value* value_ptr ) {
    QAJ4C_ASSERT(QAJ4C_is_error(value_ptr));
    return ((QAJ4C_Error*)value_ptr)->info->json;
}

QAJ4C_ERROR_CODES QAJ4C_error_get_errno( const QAJ4C_Value* value_ptr ) {
    QAJ4C_ASSERT(QAJ4C_is_error(value_ptr));
    return ((QAJ4C_Error*)value_ptr)->info->err_no;
}

unsigned QAJ4C_error_get_json_pos( const QAJ4C_Value* value_ptr ) {
    QAJ4C_ASSERT(QAJ4C_is_error(value_ptr));
    return ((QAJ4C_Error*)value_ptr)->info->json_pos;
}

unsigned QAJ4C_object_size( const QAJ4C_Value* value_ptr ) {
    QAJ4C_ASSERT(QAJ4C_is_object(value_ptr));
    return ((QAJ4C_Object*)value_ptr)->count;
}

QAJ4C_Value* QAJ4C_object_create_member_by_ref2( QAJ4C_Value* value_ptr, const char* str, size_t len ) {
    size_type count = ((QAJ4C_Object*)value_ptr)->count;
    size_type i;
    QAJ4C_ASSERT(QAJ4C_is_object(value_ptr));
    for (i = 0; i < count; ++i) {
        QAJ4C_Value* key_value = &((QAJ4C_Object*)value_ptr)->top[i].key;
        if (QAJ4C_is_null(key_value)) {
            QAJ4C_set_type(value_ptr, QAJ4C_OBJECT); /* not sorted anymore */
            QAJ4C_set_string_ref2(key_value, str, len);
            return &((QAJ4C_Object*)value_ptr)->top[i].value;
        }
    }
    return NULL;
}

QAJ4C_Value* QAJ4C_object_create_member_by_copy2( QAJ4C_Value* value_ptr, QAJ4C_Builder* builder, const char* str, size_t len ) {
    size_type count = ((QAJ4C_Object*)value_ptr)->count;
    size_type i;
    QAJ4C_ASSERT(QAJ4C_is_object(value_ptr));
    for (i = 0; i < count; ++i) {
        QAJ4C_Value* key_value = &((QAJ4C_Object*)value_ptr)->top[i].key;
        if (QAJ4C_is_null(key_value)) {
            QAJ4C_set_type(value_ptr, QAJ4C_OBJECT); /* not sorted anymore */
            QAJ4C_set_string_copy2(key_value, builder, str, len);
            return &((QAJ4C_Object*)value_ptr)->top[i].value;
        }
    }
    return NULL;
}

const QAJ4C_Member* QAJ4C_object_get_member( const QAJ4C_Value* value_ptr, unsigned index ) {
    QAJ4C_ASSERT(QAJ4C_is_object(value_ptr));
    QAJ4C_ASSERT(((QAJ4C_Object* )value_ptr)->count > index);
    return &((QAJ4C_Object*)value_ptr)->top[index];
}

const QAJ4C_Value* QAJ4C_member_get_key( const QAJ4C_Member* member ) {
    QAJ4C_ASSERT(member != NULL);
    return &member->key;
}

const QAJ4C_Value* QAJ4C_member_get_value( const QAJ4C_Member* member ) {
    QAJ4C_ASSERT(member != NULL);
    return &member->value;
}

const QAJ4C_Value* QAJ4C_object_get2( const QAJ4C_Value* value_ptr, const char* str, size_t len ) {
    QAJ4C_Value wrapper_value;
    QAJ4C_ASSERT(QAJ4C_is_object(value_ptr));
    QAJ4C_set_string_ref2(&wrapper_value, str, len);

    if (QAJ4C_get_type(value_ptr) == QAJ4C_OBJECT_SORTED) {
        return QAJ4C_object_get_sorted((QAJ4C_Object*)value_ptr, &wrapper_value);
    }
    return QAJ4C_object_get_unsorted((QAJ4C_Object*)value_ptr, &wrapper_value);
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
    result = bsearch(&member, obj_ptr->top, obj_ptr->count, sizeof(QAJ4C_Member), QAJ4C_compare_members);
    if (result != NULL) {
        return &result->value;
    }
    return NULL;
}

const QAJ4C_Value* QAJ4C_array_get( const QAJ4C_Value* value_ptr, unsigned index ) {
    QAJ4C_ASSERT(QAJ4C_is_array(value_ptr));
    QAJ4C_ASSERT(index < ((QAJ4C_Array* )value_ptr)->count);
    return ((QAJ4C_Array*)value_ptr)->top + index;
}

unsigned QAJ4C_array_size( const QAJ4C_Value* value_ptr ) {
    QAJ4C_ASSERT(QAJ4C_is_array(value_ptr));
    return ((QAJ4C_Array*)value_ptr)->count;
}

void QAJ4C_set_bool( QAJ4C_Value* value_ptr, bool value ) {
    QAJ4C_ASSERT(value_ptr != NULL);
    QAJ4C_set_type(value_ptr, QAJ4C_PRIMITIVE);
    QAJ4C_set_storage_type(value_ptr, QAJ4C_PRIMITIVE_BOOL);
    QAJ4C_set_compatibility_types(value_ptr, QAJ4C_PRIMITIVE_BOOL);
    ((QAJ4C_Primitive*)value_ptr)->data.b = value;
}

void QAJ4C_set_int( QAJ4C_Value* value_ptr, int32_t value ) {
    QAJ4C_set_int64(value_ptr, value);
}
void QAJ4C_set_uint( QAJ4C_Value* value_ptr, uint32_t value ) {
    QAJ4C_set_uint64(value_ptr, value);
}

void QAJ4C_set_int64( QAJ4C_Value* value_ptr, int64_t value ) {
    QAJ4C_set_type(value_ptr, QAJ4C_PRIMITIVE);
    ((QAJ4C_Primitive*)value_ptr)->data.i = value;
    if (value > INT32_MAX) {
        QAJ4C_set_storage_type(value_ptr, QAJ4C_PRIMITIVE_INT64);
        QAJ4C_set_compatibility_types(value_ptr, QAJ4C_PRIMITIVE_INT64);
    } else {
        QAJ4C_set_storage_type(value_ptr, QAJ4C_PRIMITIVE_INT);
        QAJ4C_set_compatibility_types(value_ptr, QAJ4C_PRIMITIVE_INT64 | QAJ4C_PRIMITIVE_INT);
    }
    QAJ4C_add_compatibility_types(value_ptr, QAJ4C_PRIMITIVE_DOUBLE);
}

void QAJ4C_set_uint64( QAJ4C_Value* value_ptr, uint64_t value ) {
    QAJ4C_set_type(value_ptr, QAJ4C_PRIMITIVE);
    ((QAJ4C_Primitive*)value_ptr)->data.u = value;

    if (value > UINT32_MAX) {
        QAJ4C_set_storage_type(value_ptr, QAJ4C_PRIMITIVE_UINT64);
        QAJ4C_set_compatibility_types(value_ptr, QAJ4C_PRIMITIVE_UINT64);
    } else {
        QAJ4C_set_storage_type(value_ptr, QAJ4C_PRIMITIVE_UINT);
        QAJ4C_set_compatibility_types(value_ptr, QAJ4C_PRIMITIVE_UINT64 | QAJ4C_PRIMITIVE_UINT);
    }
    if (value <= INT64_MAX) {
        QAJ4C_add_compatibility_types(value_ptr, QAJ4C_PRIMITIVE_INT64);
    }
    if (value <= INT32_MAX) {
        QAJ4C_add_compatibility_types(value_ptr, QAJ4C_PRIMITIVE_INT);
    }
    QAJ4C_add_compatibility_types(value_ptr, QAJ4C_PRIMITIVE_DOUBLE);
}

void QAJ4C_set_double( QAJ4C_Value* value_ptr, double value ) {
    QAJ4C_set_type(value_ptr, QAJ4C_PRIMITIVE);
    ((QAJ4C_Primitive*)value_ptr)->data.d = value;
    QAJ4C_set_storage_type(value_ptr, QAJ4C_PRIMITIVE_DOUBLE);
    QAJ4C_set_compatibility_types(value_ptr, QAJ4C_PRIMITIVE_DOUBLE);
}

void QAJ4C_set_null( QAJ4C_Value* value_ptr) {
    QAJ4C_set_type(value_ptr, QAJ4C_NULL);
}

void QAJ4C_set_string_ref2( QAJ4C_Value* value_ptr, const char* str, size_t len ) {
    QAJ4C_set_type(value_ptr, QAJ4C_STRING_REF);
    ((QAJ4C_String*)value_ptr)->s = str;
    ((QAJ4C_String*)value_ptr)->count = len;
}

void QAJ4C_set_string_copy2( QAJ4C_Value* value_ptr, QAJ4C_Builder* builder, const char* str, size_t len ) {
    if (len <= INLINE_STRING_SIZE) {
        QAJ4C_set_type(value_ptr, QAJ4C_INLINE_STRING);
        ((QAJ4C_Short_string*)value_ptr)->count = (uint8_t)len;
        memcpy(&((QAJ4C_Short_string*)value_ptr)->s, str, len);
        ((QAJ4C_Short_string*)value_ptr)->s[len] = '\0';
    } else {
        char* new_string;
        QAJ4C_set_type(value_ptr, QAJ4C_STRING);
        ((QAJ4C_String*)value_ptr)->count = len;
        new_string = QAJ4C_builder_pop_string(builder, len + 1);
        memcpy(new_string, str, len);
        ((QAJ4C_String*)value_ptr)->s = new_string;
        new_string[len] = '\0';
    }
}

/*
 * Modifications that require the builder as argument (as memory might has to be acquired in buffer)
 */
void QAJ4C_set_array( QAJ4C_Value* value_ptr, unsigned count, QAJ4C_Builder* builder ) {
    QAJ4C_set_type(value_ptr, QAJ4C_ARRAY);
    ((QAJ4C_Array*)value_ptr)->top = QAJ4C_builder_pop_values(builder, count);
    ((QAJ4C_Array*)value_ptr)->count = count;
}

QAJ4C_Value* QAJ4C_array_get_rw( QAJ4C_Value* value_ptr, unsigned index ) {
    QAJ4C_ASSERT(QAJ4C_is_array(value_ptr));
    QAJ4C_ASSERT(index < ((QAJ4C_Array* )value_ptr)->count);
    return &((QAJ4C_Array*)value_ptr)->top[index];
}

void QAJ4C_set_object( QAJ4C_Value* value_ptr, unsigned count, QAJ4C_Builder* builder ) {
    QAJ4C_set_type(value_ptr, QAJ4C_OBJECT);
    ((QAJ4C_Object*)value_ptr)->top = QAJ4C_builder_pop_members(builder, count);
    ((QAJ4C_Object*)value_ptr)->count = count;
}

void QAJ4C_builder_init( QAJ4C_Builder* me, void* buff, size_t buff_size ) {
    me->buffer = buff;
    me->buffer_size = buff_size;

    /* objects grow from front to end (and always contains a document) */
    me->cur_obj_pos = sizeof(QAJ4C_Document);
    /* strings from end to front */
    me->cur_str_pos = buff_size - 1;
}

QAJ4C_Document* QAJ4C_builder_get_document( QAJ4C_Builder* builder ) {
    return (QAJ4C_Document*)builder->buffer;
}

static void QAJ4C_builder_validate_buffer( QAJ4C_Builder* builder ) {
    QAJ4C_ASSERT(builder->cur_obj_pos - 1 <= builder->cur_str_pos);
}

static QAJ4C_Value* QAJ4C_builder_pop_values( QAJ4C_Builder* builder, size_type count ) {
    QAJ4C_Value* new_pointer;
    size_type i;
    if (count == 0) {
        return NULL;
    }
    new_pointer = (QAJ4C_Value*)(&builder->buffer[builder->cur_obj_pos]);
    builder->cur_obj_pos += count * sizeof(QAJ4C_Value);

    QAJ4C_builder_validate_buffer(builder);
    for (i = 0; i < count; i++) {
        QAJ4C_set_type(&new_pointer[i], QAJ4C_NULL);
    }
    return new_pointer;
}

static QAJ4C_Member* QAJ4C_builder_pop_members( QAJ4C_Builder* builder, size_type count ) {
    QAJ4C_Member* new_pointer;
    size_type i;
    if (count == 0) {
        return NULL;
    }
    new_pointer = (QAJ4C_Member*)(&builder->buffer[builder->cur_obj_pos]);
    builder->cur_obj_pos += count * sizeof(QAJ4C_Member);
    QAJ4C_builder_validate_buffer(builder);

    for (i = 0; i < count; i++) {
        QAJ4C_set_type(&new_pointer[i].key, QAJ4C_NULL);
        QAJ4C_set_type(&new_pointer[i].value, QAJ4C_NULL);
    }

    return new_pointer;
}

static char* QAJ4C_builder_pop_string( QAJ4C_Builder* builder, size_type length ) {
    builder->cur_str_pos -= length * sizeof(char);
    QAJ4C_builder_validate_buffer(builder);
    return (char*)(&builder->buffer[builder->cur_str_pos]);
}

size_t QAJ4C_value_sizeof_as_document( const QAJ4C_Value* value_ptr ) {
    return sizeof(QAJ4C_Document) + QAJ4C_value_sizeof(value_ptr);
}

size_t QAJ4C_document_sizeof( const QAJ4C_Document* doc_ptr ) {
    return sizeof(QAJ4C_Document) + QAJ4C_value_sizeof(QAJ4C_get_root_value(doc_ptr));
}

size_t QAJ4C_value_sizeof( const QAJ4C_Value* value_ptr ) {
    size_t size = sizeof(QAJ4C_Value);
    size_type i;

    switch (QAJ4C_get_type(value_ptr)) {
    case QAJ4C_OBJECT_SORTED:
    case QAJ4C_OBJECT: {
        for (i = 0; i < QAJ4C_object_size(value_ptr); ++i) {
            const QAJ4C_Member* member = QAJ4C_object_get_member(value_ptr, i);
            size += QAJ4C_value_sizeof(&member->key);
            size += QAJ4C_value_sizeof(&member->value);
        }
        break;
    }
    case QAJ4C_ARRAY: {
        for (i = 0; i < QAJ4C_array_size(value_ptr); ++i) {
            const QAJ4C_Value* elem = QAJ4C_array_get(value_ptr, i);
            size += QAJ4C_value_sizeof(elem);
        }
        break;
    }
    case QAJ4C_STRING: {
        size += QAJ4C_get_string_length(value_ptr) + 1;
        break;
    }
    default:
        QAJ4C_ASSERT(QAJ4C_get_type(value_ptr) != QAJ4C_UNSPECIFIED);
    }
    return size;
}

void QAJ4C_object_optimize( QAJ4C_Value* value_ptr ) {
    QAJ4C_ASSERT(QAJ4C_is_object(value_ptr));
    if (QAJ4C_get_type(value_ptr) != QAJ4C_OBJECT_SORTED) {
        QAJ4C_Object* obj_ptr = (QAJ4C_Object*)value_ptr;
        qsort(obj_ptr->top, obj_ptr->count, sizeof(QAJ4C_Member), QAJ4C_compare_members);
        QAJ4C_set_type(value_ptr, QAJ4C_OBJECT_SORTED);
    }
}

void QAJ4C_copy( const QAJ4C_Value* src, QAJ4C_Value* dest, QAJ4C_Builder* builder ) {
    QAJ4C_Type lhs_type = QAJ4C_get_type(src);
    size_type i;

    switch (lhs_type) {
    case QAJ4C_NULL:
    case QAJ4C_STRING_REF:
    case QAJ4C_INLINE_STRING:
    case QAJ4C_PRIMITIVE:
        *dest = *src;
        break;
    case QAJ4C_STRING:
        QAJ4C_set_string_copy2(dest, builder, QAJ4C_get_string(src),
                               QAJ4C_get_string_length(src));
        break;
    case QAJ4C_OBJECT:
    case QAJ4C_OBJECT_SORTED: {
        QAJ4C_Object* dest_object;
        QAJ4C_set_object(dest, QAJ4C_object_size(src), builder);
        dest_object = (QAJ4C_Object*)dest;

        for (i = 0; i < QAJ4C_object_size(src); ++i) {
            const QAJ4C_Member* src_member = QAJ4C_object_get_member(src, i);
            QAJ4C_Member* dest_member = &dest_object->top[i];

            QAJ4C_copy(&src_member->key, &dest_member->key, builder);
            QAJ4C_copy(&src_member->value, &dest_member->value, builder);
        }
        break;
    }
    case QAJ4C_ARRAY: {
        QAJ4C_set_array(dest, QAJ4C_array_size(src), builder);
        for (i = 0; i < QAJ4C_array_size(src); ++i) {
            QAJ4C_copy(QAJ4C_array_get(src, i), QAJ4C_array_get_rw(dest, i), builder);
        }
        break;
    }
    default:
        break;
    }
}

bool QAJ4C_equals( const QAJ4C_Value* lhs, const QAJ4C_Value* rhs ) {
    QAJ4C_Type lhs_type = QAJ4C_get_basic_type(lhs);
    QAJ4C_Type rhs_type = QAJ4C_get_basic_type(rhs);
    size_type i;

    if (lhs_type != rhs_type) {
        return false;
    }
    switch (lhs_type) {
    case QAJ4C_NULL:
        return true;
    case QAJ4C_STRING:
        return QAJ4C_strcmp(lhs, rhs) == 0;
    case QAJ4C_PRIMITIVE:
        return ((QAJ4C_Primitive*)lhs)->data.i == ((QAJ4C_Primitive*)rhs)->data.i;
    case QAJ4C_OBJECT: {
        unsigned size_lhs = QAJ4C_object_size(lhs);
        if (size_lhs != QAJ4C_object_size(rhs)) {
            return false;
        }
        for (i = 0; i < size_lhs; ++i) {
            const QAJ4C_Member* member_lhs = QAJ4C_object_get_member(lhs, i);
            const QAJ4C_Value* key_lhs = QAJ4C_member_get_key(member_lhs);

            const QAJ4C_Member* member_rhs = QAJ4C_object_get_member(rhs, i);
            const QAJ4C_Value* key_rhs = QAJ4C_member_get_key(member_rhs);

            const QAJ4C_Value* lhs_value;
            const QAJ4C_Value* rhs_value;

            /*
             * If lhs is not fully filled the objects are only equal in case rhs has
             * the same fill amount
             */
            if (QAJ4C_is_null(key_lhs)) {
                return QAJ4C_is_null(key_rhs);
            }

            lhs_value = QAJ4C_member_get_value(member_lhs);
            rhs_value = QAJ4C_object_get2(rhs, QAJ4C_get_string(key_lhs),
                                          QAJ4C_get_string_length(key_lhs));
            if (!QAJ4C_equals(lhs_value, rhs_value)) {
                return false;
            }
        }
        return true;
    }
    case QAJ4C_ARRAY: {
        unsigned size_lhs = QAJ4C_array_size(lhs);
        if (size_lhs != QAJ4C_array_size(rhs)) {
            return false;
        }
        for (i = 0; i < size_lhs; ++i) {
            const QAJ4C_Value* value_lhs = QAJ4C_array_get(lhs, i);
            const QAJ4C_Value* value_rhs = QAJ4C_array_get(rhs, i);
            if (!QAJ4C_equals(value_lhs, value_rhs)) {
                return false;
            }
        }
        return true;
    }
    default:
        QAJ4C_ASSERT(false);
    }
    return false;
}

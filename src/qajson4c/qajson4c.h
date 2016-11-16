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

#ifndef QAJ4C_H_
#define QAJ4C_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

struct QAJ4C_Document;
typedef struct QAJ4C_Document QAJ4C_Document;

struct QAJ4C_Value;
typedef struct QAJ4C_Value QAJ4C_Value;

struct QAJ4C_Member;
typedef struct QAJ4C_Member QAJ4C_Member;

struct QAJ4C_Builder {
    uint8_t* buffer;
    size_t buffer_size;

    size_t cur_str_pos;
    size_t cur_obj_pos;
};
typedef struct QAJ4C_Builder QAJ4C_Builder;

typedef enum QAJ4C_ERROR_CODES {
    QAJ4C_ERROR_DEPTH_OVERFLOW = 1,
    QAJ4C_ERROR_UNEXPECTED_CHAR = 2,
    QAJ4C_ERROR_BUFFER_TRUNCATED = 3,
    QAJ4C_ERROR_INVALID_STRING_START = 4,
    QAJ4C_ERROR_INVALID_NUMBER_FORMAT = 5,
    QAJ4C_ERROR_UNEXPECTED_JSON_APPENDIX = 6,
    QAJ4C_ERROR_ARRAY_MISSING_COMMA = 7,
    QAJ4C_ERROR_OBJECT_MISSING_COLON = 8,
    QAJ4C_ERROR_FATAL_PARSER_ERROR = 9,
} QAJ4C_ERROR_CODES;

void QAJ4C_print_stats();

/**
 * This method will walk through the json message and analyue what buffer size would be required
 * to store the complete DOM.
 */
unsigned QAJ4C_calculate_max_buffer_size( const char* json );

/**
 * This method will walk through the json message and analyze the maximum required buffer size that
 * would be required to store the DOM (in case the strings do not have to be copied into the buffer)
 */
unsigned QAJ4C_calculate_max_buffer_size_insitu( const char* json );

const QAJ4C_Document* QAJ4C_parse( const char* json, void* buffer, size_t buffer_size );

const QAJ4C_Document* QAJ4C_parse_insitu( char* json, void* buffer, size_t buffer_size );

size_t QAJ4C_sprint( const QAJ4C_Document* document, char* buffer, size_t buffer_size );

const QAJ4C_Value* QAJ4C_get_root_value( const QAJ4C_Document* document );

bool QAJ4C_is_string( const QAJ4C_Value* value );
const char* QAJ4C_get_string( const QAJ4C_Value* value );
unsigned QAJ4C_get_string_length( const QAJ4C_Value* value );

int QAJ4C_string_cmp2( const QAJ4C_Value* value, const char* str, size_t len );

static inline bool QAJ4C_string_cmp( const QAJ4C_Value* value, const char* str ) {
    return QAJ4C_string_cmp2(value, str, strlen(str));
}

static inline bool QAJ4C_string_equals2( const QAJ4C_Value* value, const char* str, size_t len ) {
    return QAJ4C_string_cmp2(value, str, len) == 0;
}

static inline bool QAJ4C_string_equals( const QAJ4C_Value* value, const char* str ) {
    return QAJ4C_string_equals2(value, str, strlen(str));
}

bool QAJ4C_is_object( const QAJ4C_Value* value );
bool QAJ4C_is_array( const QAJ4C_Value* value );

bool QAJ4C_is_int( const QAJ4C_Value* value );
int32_t QAJ4C_get_int( const QAJ4C_Value* value );

bool QAJ4C_is_int64( const QAJ4C_Value* value );
int64_t QAJ4C_get_int64( const QAJ4C_Value* value );

bool QAJ4C_is_uint( const QAJ4C_Value* value );
uint32_t QAJ4C_get_uint( const QAJ4C_Value* value );

bool QAJ4C_is_uint64( const QAJ4C_Value* value );
uint64_t QAJ4C_get_uint64( const QAJ4C_Value* value );

bool QAJ4C_is_real( const QAJ4C_Value* value );
double QAJ4C_get_real( const QAJ4C_Value* value );

bool QAJ4C_is_bool( const QAJ4C_Value* value );
bool QAJ4C_get_bool( const QAJ4C_Value* value );

bool QAJ4C_is_null( const QAJ4C_Value* value );

bool QAJ4C_is_error( const QAJ4C_Value* value );
const char* QAJ4C_get_json( const QAJ4C_Value* value );
QAJ4C_ERROR_CODES QAJ4C_get_errno( const QAJ4C_Value* value );
unsigned QAJ4C_get_json_pos( const QAJ4C_Value* value );

unsigned QAJ4C_object_size( const QAJ4C_Value* value );
const QAJ4C_Member* QAJ4C_object_get_member( const QAJ4C_Value* value, unsigned index );
const QAJ4C_Value* QAJ4C_member_get_key( const QAJ4C_Member* member );
const QAJ4C_Value* QAJ4C_member_get_value( const QAJ4C_Member* member );
const QAJ4C_Value* QAJ4C_object_get2( const QAJ4C_Value* value, const char* str, size_t len );

static inline const QAJ4C_Value* QAJ4C_object_get( const QAJ4C_Value* value, const char* str ) {
    return QAJ4C_object_get2(value, str, strlen(str));
}

unsigned QAJ4C_array_size( const QAJ4C_Value* value );
const QAJ4C_Value* QAJ4C_array_get( const QAJ4C_Value* value, unsigned index );

void QAJ4C_builder_init( QAJ4C_Builder* me, void* buff, size_t buff_size );
QAJ4C_Document* QAJ4C_builder_get_document( QAJ4C_Builder* builder );

QAJ4C_Value* QAJ4C_get_root_value_rw( QAJ4C_Document* document );

// Modifications on primitives is quite easy

void QAJ4C_set_bool( QAJ4C_Value* value_ptr, bool value );
void QAJ4C_set_int( QAJ4C_Value* value_ptr, int32_t value );
void QAJ4C_set_int64( QAJ4C_Value* value_ptr, int64_t value );
void QAJ4C_set_uint( QAJ4C_Value* value_ptr, uint32_t value );
void QAJ4C_set_uint64( QAJ4C_Value* value_ptr, uint64_t value );
void QAJ4C_set_double( QAJ4C_Value* value_ptr, double value );

void QAJ4C_set_string_ref2( QAJ4C_Value* value_ptr, const char* str, size_t len );
static inline void QAJ4C_set_string_ref( QAJ4C_Value* value_ptr, const char* str ) {
    QAJ4C_set_string_ref2(value_ptr, str, strlen(str));
}

void QAJ4C_set_string_copy2( QAJ4C_Value* value_ptr, QAJ4C_Builder* builder, const char* str, size_t len );
static inline void QAJ4C_set_string_copy( QAJ4C_Value* value_ptr, QAJ4C_Builder* builder, const char* str ) {
    QAJ4C_set_string_copy2(value_ptr, builder, str, strlen(str));
}

void QAJ4C_set_array( QAJ4C_Value* value_ptr, unsigned count, QAJ4C_Builder* builder );
QAJ4C_Value* QAJ4C_array_get_rw( QAJ4C_Value* value_ptr, unsigned index );

void QAJ4C_set_object( QAJ4C_Value* value_ptr, unsigned count, QAJ4C_Builder* builder );

QAJ4C_Value* QAJ4C_object_create_member_by_ref2( QAJ4C_Value* value_ptr, const char* str, size_t len );
static inline QAJ4C_Value* QAJ4C_object_create_member_by_ref( QAJ4C_Value* value_ptr, const char* str ) {
    return QAJ4C_object_create_member_by_ref2(value_ptr, str, strlen(str));
}

QAJ4C_Value* QAJ4C_object_create_member_by_copy2( QAJ4C_Value* value_ptr, QAJ4C_Builder* builder, const char* str, size_t len );
static inline QAJ4C_Value* QAJ4C_object_create_member_by_copy( QAJ4C_Value* value_ptr, QAJ4C_Builder* builder, const char* str ) {
    return QAJ4C_object_create_member_by_copy2(value_ptr, builder, str, strlen(str));
}

#ifdef __cplusplus
}
#endif

#endif /* QAJ4C_H_ */

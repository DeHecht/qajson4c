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

/**
 * This file maps the api methods to the internal implementation.
 */


#include "qajson_stdwrap.h"
#include "qajson4c.h"
#include "qajson4c_internal.h"


void QSJ4C_register_fatal_error_function( QSJ4C_fatal_error_fn function ) {
    if (function == NULL) {
        g_QSJ4C_err_function = &QSJ4C_std_err_function;
    } else {
        g_QSJ4C_err_function = function;
    }
}

size_t QSJ4C_calculate_max_buffer_size_n( const char* json, size_t n ) {
    return QSJ4C_calculate_max_buffer_generic(json, n, 0);
}

size_t QSJ4C_calculate_max_buffer_size( const char* json ) {
    return QSJ4C_calculate_max_buffer_size_n(json, QSJ4C_STRLEN(json));
}

size_t QSJ4C_calculate_max_buffer_size_insitu_n( const char* json, size_t n ) {
    return QSJ4C_calculate_max_buffer_generic(json, n, 1);
}

size_t QSJ4C_calculate_max_buffer_size_insitu( const char* json ) {
    return QSJ4C_calculate_max_buffer_size_insitu_n(json, QSJ4C_STRLEN(json));
}

size_t QSJ4C_parse( const char* json, void* buffer, size_t buffer_size, QSJ4C_Value** result_ptr ) {
    return QSJ4C_parse_opt(json, SIZE_MAX, 0, buffer, buffer_size, result_ptr);
}

QSJ4C_Value* QSJ4C_parse_dynamic( const char* json, QSJ4C_realloc_fn realloc_callback ) {
    return QSJ4C_parse_opt_dynamic(json, SIZE_MAX, 0, realloc_callback);
}

size_t QSJ4C_parse_opt( const char* json, size_t json_len, int opts, void* buffer, size_t buffer_size, QSJ4C_Value** result_ptr ) {
    QSJ4C_Builder builder;
    QSJ4C_builder_init(&builder, buffer, buffer_size);
    return QSJ4C_parse_generic(&builder, json, json_len, opts, result_ptr, NULL);
}

QSJ4C_Value* QSJ4C_parse_opt_dynamic( const char* json, size_t json_len, int opts, QSJ4C_realloc_fn realloc_callback ) {
    static size_t MIN_SIZE = sizeof(QSJ4C_Value);
    void* buffer = realloc_callback( NULL, MIN_SIZE);
    QSJ4C_Builder builder;
    QSJ4C_Value* result = NULL;
    if (buffer == NULL) {
        return result;
    }

    QSJ4C_builder_init(&builder, buffer, MIN_SIZE);
    QSJ4C_parse_generic(&builder, json, json_len, opts, &result, realloc_callback);
    return result;
}

size_t QSJ4C_sprint( const QSJ4C_Value* value_ptr, char* buffer, size_t buffer_size ) {
    size_t index;
    if (QSJ4C_UNLIKELY(buffer_size == 0)) {
        return 0;
    }

    index = QSJ4C_sprint_impl( (QSJ4C_Value*)value_ptr, buffer, buffer_size, 0 );

    if (QSJ4C_UNLIKELY(index >= buffer_size)) {
        index = buffer_size - 1;
    }
    buffer[index] = '\0';
    return index + 1;
}

bool QSJ4C_is_string(const QSJ4C_Value* value_ptr) {
    return QSJ4C_get_type(value_ptr) == QSJ4C_TYPE_STRING;
}

const char* QSJ4C_get_string( const QSJ4C_Value* value_ptr ) {
    uint32_t string_index;
    QSJ4C_ASSERT(QSJ4C_is_string(value_ptr), {return "";});
	if (QSJ4C_get_string_length(value_ptr) == 0) {
		return "";
	}
	string_index = QSJ4C_read_uint24(&value_ptr->buff[1]);
    return ((const char*)(value_ptr + 1)) + string_index;
}

size_t QSJ4C_get_string_length( const QSJ4C_Value* value_ptr ){
    uint16_t* ptr;
    QSJ4C_ASSERT(QSJ4C_is_string(value_ptr), {return 0;});
    ptr = (uint16_t*)&value_ptr->buff[4];
    return *ptr;
}

int QSJ4C_string_cmp_n( const QSJ4C_Value* value_ptr, const char* str, size_t len ) {
	size_t i;
	size_t lhs_size;

    QSJ4C_ASSERT(QSJ4C_is_string(value_ptr), {return strcmp("", str);});

	const char* lhs_string = QSJ4C_get_string(value_ptr);
	lhs_size = QSJ4C_get_string_length(value_ptr);
	if ( lhs_size != len ) {
    	return lhs_size - len;
    }

	for (i = 0; i < len; ++i) {
        if (lhs_string[i] != str[i]) {
            return lhs_string[i] - str[i];
        }
	}

	return 0;
}

bool QSJ4C_string_cmp( const QSJ4C_Value* value_ptr, const char* str ) {
    return QSJ4C_string_cmp_n(value_ptr, str, QSJ4C_STRLEN(str));
}

bool QSJ4C_string_equals_n( const QSJ4C_Value* value_ptr, const char* str, size_t len ) {
    return QSJ4C_string_cmp_n(value_ptr, str, len) == 0;
}

bool QSJ4C_string_equals( const QSJ4C_Value* value_ptr, const char* str ) {
    return QSJ4C_string_equals_n(value_ptr, str, QSJ4C_STRLEN(str));
}

bool QSJ4C_is_object(const QSJ4C_Value* value_ptr) {
    return QSJ4C_get_type(value_ptr) == QSJ4C_TYPE_OBJECT;
}

bool QSJ4C_is_array( const QSJ4C_Value* value_ptr ){
    return QSJ4C_get_type(value_ptr) == QSJ4C_TYPE_ARRAY;
}

bool QSJ4C_is_int(const QSJ4C_Value* value_ptr) {
    return QSJ4C_get_type(value_ptr) == QSJ4C_TYPE_NUMBER && (QSJ4C_get_compatibility_types(value_ptr) & QSJ4C_PRIMITIVE_INT) != 0;
}

int32_t QSJ4C_get_int( const QSJ4C_Value* value_ptr ) {
	int32_t* ptr;
	QSJ4C_ASSERT(QSJ4C_is_int(value_ptr), {return 0;});
    ptr = (int32_t*)&value_ptr->buff[2];
    return *ptr;
}

bool QSJ4C_is_uint( const QSJ4C_Value* value_ptr ) {
    return QSJ4C_get_type(value_ptr) == QSJ4C_TYPE_NUMBER && (QSJ4C_get_compatibility_types(value_ptr) & QSJ4C_PRIMITIVE_UINT) != 0;
}

uint32_t QSJ4C_get_uint( const QSJ4C_Value* value_ptr ) {
	uint32_t* ptr;
	QSJ4C_ASSERT(QSJ4C_is_uint(value_ptr), {return 0;});
	ptr = (uint32_t*)&value_ptr->buff[2];
    return *ptr;
}

bool QSJ4C_is_float( const QSJ4C_Value* value_ptr ) {
    /* No need to check the compatibility type as float is always compatible! */
    return QSJ4C_get_type(value_ptr) == QSJ4C_TYPE_NUMBER;
}

float QSJ4C_get_float( const QSJ4C_Value* value_ptr ) {
	QSJ4C_Primitive_type type;
	float* ptr;
    QSJ4C_ASSERT(QSJ4C_is_float(value_ptr), {return 0.0;});

    type = value_ptr->buff[1];
	switch (type) {
	case QSJ4C_PRIMITIVE_INT:
		return (float) QSJ4C_get_int(value_ptr);
	case QSJ4C_PRIMITIVE_UINT:
		return (float) QSJ4C_get_uint(value_ptr);
	default:
		ptr = (float*) &value_ptr->buff[2];
		return *ptr;
	}
}

bool QSJ4C_is_bool( const QSJ4C_Value* value_ptr ) {
    return QSJ4C_get_type(value_ptr) == QSJ4C_TYPE_BOOL;
}

bool QSJ4C_get_bool( const QSJ4C_Value* value_ptr ) {
    QSJ4C_ASSERT(QSJ4C_is_bool(value_ptr), {return false;});
    return value_ptr->buff[1] != 0;
}

bool QSJ4C_is_not_set(const QSJ4C_Value* value_ptr) {
    return value_ptr == NULL;
}

bool QSJ4C_is_null( const QSJ4C_Value* value_ptr ) {
    return QSJ4C_get_type(value_ptr) == QSJ4C_TYPE_NULL;
}

QSJ4C_TYPE QSJ4C_get_type( const QSJ4C_Value* value_ptr ) {
    if ( value_ptr == NULL ) {
        return QSJ4C_TYPE_NULL;
    }
    return value_ptr->buff[0] & 0xFF;
}

bool QSJ4C_is_error( const QSJ4C_Value* value_ptr ) {
    return QSJ4C_get_type(value_ptr) == QSJ4C_TYPE_INVALID;
}

QSJ4C_ERROR_CODE QSJ4C_error_get_errno(const QSJ4C_Value* value_ptr) {
    QSJ4C_ASSERT(QSJ4C_is_error(value_ptr), {return 0;});
    return (QSJ4C_ERROR_CODE)value_ptr->buff[1];
}

size_t QSJ4C_error_get_json_pos(const QSJ4C_Value* value_ptr) {
	uint32_t* ptr;
    QSJ4C_ASSERT(QSJ4C_is_error(value_ptr), {return 0;});
	ptr = (uint32_t*)&value_ptr->buff[2];
    return *ptr;
}

size_t QSJ4C_object_size( const QSJ4C_Value* value_ptr ) {
	uint16_t* ptr;
    QSJ4C_ASSERT(QSJ4C_is_object(value_ptr), {return 0;});
	ptr = (uint16_t*)&value_ptr->buff[4];
    return *ptr;
}

QSJ4C_Member* QSJ4C_object_get_member( QSJ4C_Value* value_ptr, size_t index ) {
	uint32_t top_index;
    QSJ4C_ASSERT(QSJ4C_is_object(value_ptr) && QSJ4C_object_size(value_ptr) > index, {return NULL;});

	top_index = QSJ4C_read_uint24(&value_ptr->buff[1]);
    return (QSJ4C_Member*)(top_index + (value_ptr + 1)) + index;
}

QSJ4C_Value* QSJ4C_member_get_key(QSJ4C_Member* member) {
    QSJ4C_ASSERT(member != NULL, {return NULL;});
	return (QSJ4C_Value*) &member->buff[0];
}

QSJ4C_Value* QSJ4C_member_get_value(QSJ4C_Member* member) {
    QSJ4C_ASSERT(member != NULL, {return NULL;});
	return (QSJ4C_Value*) &member->buff[BUFF_SIZE];
}

QSJ4C_Value* QSJ4C_object_get_n(QSJ4C_Value* value_ptr, const char* str, size_t len) {
	size_type count;
	size_type i;
	QSJ4C_Member* top_ptr;
	QSJ4C_ASSERT(QSJ4C_is_object(value_ptr), {return NULL;});

	count = QSJ4C_object_size(value_ptr);
	top_ptr = QSJ4C_object_get_member(value_ptr, 0);
	for (i = 0; i < count; ++i) {
		QSJ4C_Value* key = QSJ4C_member_get_key(top_ptr + i);
		if (QSJ4C_string_cmp_n(key, str, len) == 0) {
			return QSJ4C_member_get_value(top_ptr + i);
		}
	}

	return NULL;
}

QSJ4C_Value* QSJ4C_object_get( QSJ4C_Value* value_ptr, const char* str ) {
    return QSJ4C_object_get_n( value_ptr, str, QSJ4C_STRLEN(str));
}

size_t QSJ4C_array_size( const QSJ4C_Value* value_ptr ) {
	uint16_t* ptr;
    QSJ4C_ASSERT(QSJ4C_is_array(value_ptr), {return 0;});
    ptr = (uint16_t*)&value_ptr->buff[4];
	return *ptr;
}

QSJ4C_Value* QSJ4C_array_get( QSJ4C_Value* value_ptr, size_t index ) {
	uint32_t top_index;
    QSJ4C_ASSERT(QSJ4C_is_array(value_ptr) && QSJ4C_array_size(value_ptr) > index, {return NULL;});
	top_index = QSJ4C_read_uint24(&value_ptr->buff[1]);
    return value_ptr + top_index + 1 + index;
}

void QSJ4C_builder_init( QSJ4C_Builder* me, void* buff, size_t buff_size ) {
    me->buffer = buff;
    me->buffer_size = buff_size;

    /* objects grow from front to end (and always contains a document) */
    me->cur_obj_pos = sizeof(QSJ4C_Value);
    /* strings from end to front */
    me->cur_str_pos = buff_size;
}

QSJ4C_Value* QSJ4C_builder_get_document( QSJ4C_Builder* builder ) {
    return (QSJ4C_Value*)builder->buffer;
}

void QSJ4C_set_bool( QSJ4C_Value* value_ptr, bool value ) {
	value_ptr->buff[0] = QSJ4C_TYPE_BOOL;
	value_ptr->buff[1] = value ? 1 : 0;
}

void QSJ4C_set_int( QSJ4C_Value* value_ptr, int32_t value ) {
	int32_t* ptr = (int32_t*)&value_ptr->buff[2];
	value_ptr->buff[0] = QSJ4C_TYPE_NUMBER;
	value_ptr->buff[1] = (QSJ4C_PRIMITIVE_INT << 4) | QSJ4C_PRIMITIVE_INT | QSJ4C_PRIMITIVE_FLOAT;
	*ptr = value;
	if (value >= 0) {
		value_ptr->buff[1] |= QSJ4C_PRIMITIVE_UINT;
	}
}

void QSJ4C_set_uint( QSJ4C_Value* value_ptr, uint32_t value ) {
	uint32_t* ptr = (uint32_t*)&value_ptr->buff[2];
	value_ptr->buff[0] = QSJ4C_TYPE_NUMBER;
	value_ptr->buff[1] = (QSJ4C_PRIMITIVE_UINT << 4) | QSJ4C_PRIMITIVE_UINT | QSJ4C_PRIMITIVE_FLOAT;
	*ptr = value;
	if (value <= INT32_MAX) {
		value_ptr->buff[1] |= QSJ4C_PRIMITIVE_INT;
	}
}

void QSJ4C_set_float( QSJ4C_Value* value_ptr, float value ) {
	float* ptr = (float*)&value_ptr->buff[2];
	value_ptr->buff[0] = QSJ4C_TYPE_NUMBER;
	value_ptr->buff[1] = (QSJ4C_PRIMITIVE_FLOAT << 4) | QSJ4C_PRIMITIVE_FLOAT;
	*ptr = value;
}

void QSJ4C_set_null( QSJ4C_Value* value_ptr ) {
	value_ptr->buff[0] = QSJ4C_TYPE_NULL;
}

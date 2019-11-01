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

/**
 * This file maps the api methods to the internal implementation.
 */


#include "qajson4c_types.h"
#include "internal/qajson_stdwrap.h"
#include "internal/types.h"

void QAJ4C_register_fatal_error_function( QAJ4C_fatal_error_fn function ) {
    if (function == NULL) {
        g_qaj4c_err_function = &QAJ4C_std_err_function;
    } else {
        g_qaj4c_err_function = function;
    }
}

const char* QAJ4C_get_string( const QAJ4C_Value* value_ptr ) {
    return QAJ4C_get_string_payload(value_ptr).str;
}

size_t QAJ4C_get_string_length( const QAJ4C_Value* value_ptr ){
    return QAJ4C_get_string_payload(value_ptr).size;
}

int QAJ4C_string_cmp( const QAJ4C_Value* value_ptr, const char* str, size_t len ) {
    QAJ4C_Value wrapper_value = {.type = QAJ4C_STRING_REF_TYPE_CONSTANT};
    QAJ4C_STRING_GET_PTR(&wrapper_value) = str;
    QAJ4C_STRING_GET_COUNT(&wrapper_value) = len;

    QAJ4C_ASSERT(QAJ4C_is_string(value_ptr), {return strcmp("", str);});

    return QAJ4C_strcmp(value_ptr, (QAJ4C_Value*)&wrapper_value);
}

bool QAJ4C_string_equals( const QAJ4C_Value* value_ptr, const char* str, size_t len ) {
    return QAJ4C_string_cmp(value_ptr, str, len) == 0;
}

bool QAJ4C_is_object( const QAJ4C_Value* value_ptr ) {
    return QAJ4C_get_type(value_ptr) == QAJ4C_TYPE_OBJECT;
}

bool QAJ4C_is_array( const QAJ4C_Value* value_ptr ){
    return QAJ4C_get_type(value_ptr) == QAJ4C_TYPE_ARRAY;
}

bool QAJ4C_is_string( const QAJ4C_Value* value_ptr ) {
    return QAJ4C_get_type(value_ptr) == QAJ4C_TYPE_STRING;
}

bool QAJ4C_is_int( const QAJ4C_Value* value_ptr ) {
    return QAJ4C_get_type(value_ptr) == QAJ4C_TYPE_NUMBER && (QAJ4C_get_compatibility_types(value_ptr) & QAJ4C_PRIMITIVE_INT) != 0;
}

int32_t QAJ4C_get_int( const QAJ4C_Value* value_ptr ) {
    QAJ4C_ASSERT(QAJ4C_is_int(value_ptr), {return 0;});
    return QAJ4C_GET_VALUE(value_ptr, int64_t);
}

bool QAJ4C_is_int64( const QAJ4C_Value* value_ptr ) {
    return QAJ4C_get_type(value_ptr) == QAJ4C_TYPE_NUMBER && (QAJ4C_get_compatibility_types(value_ptr) & QAJ4C_PRIMITIVE_INT64) != 0;
}

int64_t QAJ4C_get_int64( const QAJ4C_Value* value_ptr ) {
    QAJ4C_ASSERT(QAJ4C_is_int64(value_ptr), {return 0;});
    return QAJ4C_GET_VALUE(value_ptr, int64_t);
}

bool QAJ4C_is_uint( const QAJ4C_Value* value_ptr ) {
    return QAJ4C_get_type(value_ptr) == QAJ4C_TYPE_NUMBER && (QAJ4C_get_compatibility_types(value_ptr) & QAJ4C_PRIMITIVE_UINT) != 0;
}

uint32_t QAJ4C_get_uint( const QAJ4C_Value* value_ptr ) {
    QAJ4C_ASSERT(QAJ4C_is_uint(value_ptr), {return 0;});
    return QAJ4C_GET_VALUE(value_ptr, uint64_t);
}

bool QAJ4C_is_uint64( const QAJ4C_Value* value_ptr ) {
    return QAJ4C_get_type(value_ptr) == QAJ4C_TYPE_NUMBER && (QAJ4C_get_compatibility_types(value_ptr) & QAJ4C_PRIMITIVE_UINT64) != 0;
}

uint64_t QAJ4C_get_uint64( const QAJ4C_Value* value_ptr ) {
    QAJ4C_ASSERT(QAJ4C_is_uint64(value_ptr), {return 0;});
    return QAJ4C_GET_VALUE(value_ptr, uint64_t);
}

bool QAJ4C_is_double( const QAJ4C_Value* value_ptr ) {
    /* No need to check the compatibility type as double is always compatible! */
    return QAJ4C_get_type(value_ptr) == QAJ4C_TYPE_NUMBER;
}

double QAJ4C_get_double( const QAJ4C_Value* value_ptr ) {
    QAJ4C_ASSERT(QAJ4C_is_double(value_ptr), {return 0.0;});

    switch (QAJ4C_get_storage_type(value_ptr)) {
    case QAJ4C_PRIMITIVE_INT:
    case QAJ4C_PRIMITIVE_INT64:
        return QAJ4C_GET_VALUE(value_ptr, int64_t);
    case QAJ4C_PRIMITIVE_UINT:
    case QAJ4C_PRIMITIVE_UINT64:
        return QAJ4C_GET_VALUE(value_ptr, int64_t);
    default:
        return QAJ4C_GET_VALUE(value_ptr, double);
    }
}

bool QAJ4C_is_bool( const QAJ4C_Value* value_ptr ) {
    return QAJ4C_get_type(value_ptr) == QAJ4C_TYPE_BOOL;
}

bool QAJ4C_get_bool( const QAJ4C_Value* value_ptr ) {
    QAJ4C_ASSERT(QAJ4C_is_bool(value_ptr), {return false;});
    return QAJ4C_GET_VALUE(value_ptr, bool);
}

bool QAJ4C_is_not_set( const QAJ4C_Value* value_ptr ) {
    return value_ptr == NULL;
}

bool QAJ4C_is_null( const QAJ4C_Value* value_ptr ) {
    return QAJ4C_get_internal_type(value_ptr) == QAJ4C_NULL;
}

QAJ4C_TYPE QAJ4C_get_type( const QAJ4C_Value* value_ptr ) {
    if ( value_ptr == NULL ) {
        return QAJ4C_TYPE_NULL;
    }
    return value_ptr->type & 0xFF;
}

bool QAJ4C_is_error( const QAJ4C_Value* value_ptr ) {
    return QAJ4C_get_internal_type(value_ptr) == QAJ4C_ERROR_DESCRIPTION;
}

size_t QAJ4C_object_size( const QAJ4C_Value* value_ptr ) {
    QAJ4C_ASSERT(QAJ4C_is_object(value_ptr), {return 0;});
    return QAJ4C_OBJECT_GET_COUNT(value_ptr);
}

const QAJ4C_Member* QAJ4C_object_get_member( const QAJ4C_Value* value_ptr, size_t index ) {
    QAJ4C_ASSERT(QAJ4C_is_object(value_ptr) && QAJ4C_object_size(value_ptr) > index, {return NULL;});
    return QAJ4C_OBJECT_GET_PTR(value_ptr);
}

const QAJ4C_Value* QAJ4C_member_get_key( const QAJ4C_Member* member ) {
    QAJ4C_ASSERT(member != NULL, {return NULL;});
    return &member->key;
}

const QAJ4C_Value* QAJ4C_member_get_value( const QAJ4C_Member* member ) {
    QAJ4C_ASSERT(member != NULL, {return NULL;});
    return &member->value;
}

const QAJ4C_Value* QAJ4C_object_get( const QAJ4C_Value* value_ptr, const char* str, size_t len ) {
    QAJ4C_Member member;
    QAJ4C_Member* result;

    QAJ4C_ASSERT(QAJ4C_get_internal_type(value_ptr) == QAJ4C_OBJECT, {return NULL;});
    member.key.type = QAJ4C_STRING_REF_TYPE_CONSTANT;
    QAJ4C_STRING_GET_PTR(&member.key) = str;
    QAJ4C_STRING_GET_COUNT(&member.key) = len;

    result = QAJ4C_BSEARCH(&member, QAJ4C_OBJECT_GET_PTR(value_ptr), QAJ4C_OBJECT_GET_COUNT(value_ptr), sizeof(QAJ4C_Member), QAJ4C_compare_members);
    if (result != NULL) {
        return &result->value;
    }
    return NULL;
}

size_t QAJ4C_array_size( const QAJ4C_Value* value_ptr ) {
    QAJ4C_ASSERT(QAJ4C_is_array(value_ptr), {return 0;});
    return QAJ4C_ARRAY_GET_COUNT(value_ptr);
}

const QAJ4C_Value* QAJ4C_array_get( const QAJ4C_Value* value_ptr, size_t index ) {
    QAJ4C_ASSERT(QAJ4C_is_array(value_ptr) && QAJ4C_array_size(value_ptr) > index, {return NULL;});
    return QAJ4C_ARRAY_GET_PTR(value_ptr) + index;
}

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

void QAJ4C_register_fatal_error_function( QAJ4C_fatal_error_fn function ) {
    if (function == NULL) {
        g_qaj4c_err_function = &QAJ4C_std_err_function;
    } else {
        g_qaj4c_err_function = function;
    }
}

size_t QAJ4C_calculate_max_buffer_size_n( const char* json, size_t n ) {
    return QAJ4C_calculate_max_buffer_generic(json, n, 0);
}

size_t QAJ4C_calculate_max_buffer_size( const char* json ) {
    return QAJ4C_calculate_max_buffer_size_n(json, QAJ4C_STRLEN(json));
}

size_t QAJ4C_calculate_max_buffer_size_insitu_n( const char* json, size_t n ) {
    return QAJ4C_calculate_max_buffer_generic(json, n, 1);
}

size_t QAJ4C_calculate_max_buffer_size_insitu( const char* json ) {
    return QAJ4C_calculate_max_buffer_size_insitu_n(json, QAJ4C_STRLEN(json));
}

size_t QAJ4C_parse( const char* json, void* buffer, size_t buffer_size, const QAJ4C_Value** result_ptr ) {
    return QAJ4C_parse_opt(json, SIZE_MAX, 0, buffer, buffer_size, result_ptr);
}

const QAJ4C_Value* QAJ4C_parse_dynamic( const char* json, QAJ4C_realloc_fn realloc_callback ) {
    return QAJ4C_parse_opt_dynamic(json, SIZE_MAX, 0, realloc_callback);
}

size_t QAJ4C_parse_insitu( char* json, void* buffer, size_t buffer_size, const QAJ4C_Value** result_ptr ) {
    return QAJ4C_parse_opt_insitu(json, SIZE_MAX, 0, buffer, buffer_size, result_ptr);
}

size_t QAJ4C_parse_opt( const char* json, size_t json_len, int opts, void* buffer, size_t buffer_size, const QAJ4C_Value** result_ptr ) {
    QAJ4C_Builder builder;
    QAJ4C_builder_init(&builder, buffer, buffer_size);
    return QAJ4C_parse_generic(&builder, json, json_len, opts, result_ptr, NULL);
}

const QAJ4C_Value* QAJ4C_parse_opt_dynamic( const char* json, size_t json_len, int opts, QAJ4C_realloc_fn realloc_callback ) {
    static size_type MIN_SIZE = sizeof(QAJ4C_Value) + sizeof(QAJ4C_Error_information);
    void* buffer = realloc_callback( NULL, MIN_SIZE);
    QAJ4C_Builder builder;
    const QAJ4C_Value* result = NULL;
    if (buffer == NULL) {
        return result;
    }

    QAJ4C_builder_init(&builder, buffer, MIN_SIZE);
    QAJ4C_parse_generic(&builder, json, json_len, opts, &result, realloc_callback);
    return result;
}

size_t QAJ4C_parse_opt_insitu( char* json, size_t json_len, int opts, void* buffer, size_t buffer_size, const QAJ4C_Value** result_ptr ) {
    return QAJ4C_parse_opt(json, json_len, opts | 1, buffer, buffer_size, result_ptr);
}

size_t QAJ4C_sprint( const QAJ4C_Value* value_ptr, char* buffer, size_t buffer_size ) {
    size_t index;
    if (QAJ4C_UNLIKELY(buffer_size == 0)) {
        return 0;
    }

    index = QAJ4C_sprint_impl( value_ptr, buffer, buffer_size, 0 );

    if (QAJ4C_UNLIKELY(index >= buffer_size)) {
        index = buffer_size - 1;
    }
    buffer[index] = '\0';
    return index + 1;
}

bool QAJ4C_print_callback( const QAJ4C_Value* value_ptr, QAJ4C_print_callback_fn callback, void* ptr ) {
    if (QAJ4C_UNLIKELY(callback == NULL)) {
        return false;
    }
    return QAJ4C_print_callback_impl(value_ptr, callback, ptr);
}

bool QAJ4C_print_buffer_callback( const QAJ4C_Value* value_ptr, QAJ4C_print_buffer_callback_fn callback, void* ptr )
{
    if (QAJ4C_UNLIKELY(callback == NULL)) {
        return false;
    }
    return QAJ4C_print_buffer_callback_impl(value_ptr, callback, ptr);
}

bool QAJ4C_is_string( const QAJ4C_Value* value_ptr ) {
    return QAJ4C_get_type(value_ptr) == QAJ4C_TYPE_STRING;
}

const char* QAJ4C_get_string( const QAJ4C_Value* value_ptr ) {
    QAJ4C_ASSERT(QAJ4C_is_string(value_ptr), {return "";});

    if (QAJ4C_get_internal_type(value_ptr) == QAJ4C_INLINE_STRING) {
        return ((QAJ4C_Short_string*) value_ptr)->s;
    }
    return ((QAJ4C_String*) value_ptr)->s;
}

size_t QAJ4C_get_string_length( const QAJ4C_Value* value_ptr ){
    QAJ4C_ASSERT(QAJ4C_is_string(value_ptr), {return 0;});
    if (QAJ4C_get_internal_type(value_ptr) == QAJ4C_INLINE_STRING) {
        return ((QAJ4C_Short_string*) value_ptr)->count;
    }
    return ((QAJ4C_String*) value_ptr)->count;
}

int QAJ4C_string_cmp_n( const QAJ4C_Value* value_ptr, const char* str, size_t len ) {
    QAJ4C_Value wrapper_value;

    QAJ4C_ASSERT(QAJ4C_is_string(value_ptr), {return strcmp("", str);});

    QAJ4C_set_string_ref_n(&wrapper_value, str, len);
    return QAJ4C_strcmp(value_ptr, &wrapper_value);
}

bool QAJ4C_string_cmp( const QAJ4C_Value* value_ptr, const char* str ) {
    return QAJ4C_string_cmp_n(value_ptr, str, QAJ4C_STRLEN(str));
}

bool QAJ4C_string_equals_n( const QAJ4C_Value* value_ptr, const char* str, size_t len ) {
    return QAJ4C_string_cmp_n(value_ptr, str, len) == 0;
}

bool QAJ4C_string_equals( const QAJ4C_Value* value_ptr, const char* str )
{
    return QAJ4C_string_equals_n(value_ptr, str, QAJ4C_STRLEN(str));
}

bool QAJ4C_is_object( const QAJ4C_Value* value_ptr ) {
    return QAJ4C_get_type(value_ptr) == QAJ4C_TYPE_OBJECT;
}

bool QAJ4C_is_array( const QAJ4C_Value* value_ptr ){
    return QAJ4C_get_type(value_ptr) == QAJ4C_TYPE_ARRAY;
}

bool QAJ4C_is_int( const QAJ4C_Value* value_ptr ) {
    return QAJ4C_get_type(value_ptr) == QAJ4C_TYPE_NUMBER && (QAJ4C_get_compatibility_types(value_ptr) & QAJ4C_PRIMITIVE_INT) != 0;
}

int32_t QAJ4C_get_int( const QAJ4C_Value* value_ptr ) {
    QAJ4C_ASSERT(QAJ4C_is_int(value_ptr), {return 0;});
    return (int32_t) ((QAJ4C_Primitive*) value_ptr)->data.i;
}

bool QAJ4C_is_int64( const QAJ4C_Value* value_ptr ) {
    return QAJ4C_get_type(value_ptr) == QAJ4C_TYPE_NUMBER && (QAJ4C_get_compatibility_types(value_ptr) & QAJ4C_PRIMITIVE_INT64) != 0;
}

int64_t QAJ4C_get_int64( const QAJ4C_Value* value_ptr ) {
    QAJ4C_ASSERT(QAJ4C_is_int64(value_ptr), {return 0;});
    return ((QAJ4C_Primitive*) value_ptr)->data.i;
}

bool QAJ4C_is_uint( const QAJ4C_Value* value_ptr ) {
    return QAJ4C_get_type(value_ptr) == QAJ4C_TYPE_NUMBER && (QAJ4C_get_compatibility_types(value_ptr) & QAJ4C_PRIMITIVE_UINT) != 0;
}

uint32_t QAJ4C_get_uint( const QAJ4C_Value* value_ptr ) {
    QAJ4C_ASSERT(QAJ4C_is_uint(value_ptr), {return 0;});
    return (uint32_t) ((QAJ4C_Primitive*) value_ptr)->data.u;
}

bool QAJ4C_is_uint64( const QAJ4C_Value* value_ptr ) {
    return QAJ4C_get_type(value_ptr) == QAJ4C_TYPE_NUMBER && (QAJ4C_get_compatibility_types(value_ptr) & QAJ4C_PRIMITIVE_UINT64) != 0;
}

uint64_t QAJ4C_get_uint64( const QAJ4C_Value* value_ptr ) {
    QAJ4C_ASSERT(QAJ4C_is_uint64(value_ptr), {return 0;});
    return ((QAJ4C_Primitive*) value_ptr)->data.u;
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
        return (double) ((QAJ4C_Primitive*) value_ptr)->data.i;
    case QAJ4C_PRIMITIVE_UINT:
    case QAJ4C_PRIMITIVE_UINT64:
        return (double) ((QAJ4C_Primitive*) value_ptr)->data.u;
    default:
        return ((QAJ4C_Primitive*) value_ptr)->data.d;
    }
}

bool QAJ4C_is_bool( const QAJ4C_Value* value_ptr ) {
    return QAJ4C_get_type(value_ptr) == QAJ4C_TYPE_BOOL;
}

bool QAJ4C_get_bool( const QAJ4C_Value* value_ptr ) {
    QAJ4C_ASSERT(QAJ4C_is_bool(value_ptr), {return false;});
    return ((QAJ4C_Primitive*) value_ptr)->data.b;
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

const char* QAJ4C_error_get_json( const QAJ4C_Value* value_ptr ) {
    QAJ4C_ASSERT(QAJ4C_is_error(value_ptr), {return "";});
    return ((QAJ4C_Error*) value_ptr)->info->json;
}

QAJ4C_ERROR_CODE QAJ4C_error_get_errno( const QAJ4C_Value* value_ptr ) {
    QAJ4C_ASSERT(QAJ4C_is_error(value_ptr), {return 0;});
    return ((QAJ4C_Error*) value_ptr)->info->err_no;
}

size_t QAJ4C_error_get_json_pos( const QAJ4C_Value* value_ptr ) {
    QAJ4C_ASSERT(QAJ4C_is_error(value_ptr), {return 0;});
    return ((QAJ4C_Error*) value_ptr)->info->json_pos;
}

size_t QAJ4C_object_size( const QAJ4C_Value* value_ptr ) {
    QAJ4C_ASSERT(QAJ4C_is_object(value_ptr), {return 0;});
    return ((QAJ4C_Object*) value_ptr)->count;
}

const QAJ4C_Member* QAJ4C_object_get_member( const QAJ4C_Value* value_ptr, size_t index ) {
    QAJ4C_ASSERT(QAJ4C_is_object(value_ptr) && QAJ4C_object_size(value_ptr) > index, {return NULL;});
    return &((QAJ4C_Object*) value_ptr)->top[index];
}

const QAJ4C_Value* QAJ4C_member_get_key( const QAJ4C_Member* member ) {
    QAJ4C_ASSERT(member != NULL, {return NULL;});
    return &member->key;
}

const QAJ4C_Value* QAJ4C_member_get_value( const QAJ4C_Member* member ) {
    QAJ4C_ASSERT(member != NULL, {return NULL;});
    return &member->value;
}

const QAJ4C_Value* QAJ4C_object_get_n( const QAJ4C_Value* value_ptr, const char* str, size_t len ) {
    QAJ4C_Value wrapper_value;
    QAJ4C_ASSERT(QAJ4C_is_object(value_ptr), {return NULL;});

    QAJ4C_set_string_ref_n(&wrapper_value, str, len);
    if (QAJ4C_get_internal_type(value_ptr) == QAJ4C_OBJECT_SORTED) {
        return QAJ4C_object_get_sorted((QAJ4C_Object*) value_ptr, &wrapper_value);
    }
    return QAJ4C_object_get_unsorted((QAJ4C_Object*) value_ptr, &wrapper_value);
}

const QAJ4C_Value* QAJ4C_object_get( const QAJ4C_Value* value_ptr, const char* str ) {
    return QAJ4C_object_get_n( value_ptr, str, QAJ4C_STRLEN(str));
}

size_t QAJ4C_array_size( const QAJ4C_Value* value_ptr ) {
    QAJ4C_ASSERT(QAJ4C_is_array(value_ptr), {return 0;});
    return ((QAJ4C_Array*) value_ptr)->count;
}

const QAJ4C_Value* QAJ4C_array_get( const QAJ4C_Value* value_ptr, size_t index ) {
    QAJ4C_ASSERT(QAJ4C_is_array(value_ptr) && QAJ4C_array_size(value_ptr) > index, {return NULL;});
    return ((QAJ4C_Array*) value_ptr)->top + index;
}

QAJ4C_Builder QAJ4C_builder_create( void* buff, size_t buff_size )
{
   QAJ4C_Builder result;
   QAJ4C_builder_init( &result, buff, buff_size );
   return result;
}

void QAJ4C_builder_init( QAJ4C_Builder* me, void* buff, size_t buff_size ) {
    me->buffer = buff;
    me->buffer_size = buff_size;

    /* objects grow from front to end (and always contains a document) */
    me->cur_obj_pos = sizeof(QAJ4C_Value);
    /* strings from end to front */
    me->cur_str_pos = buff_size;
}

void QAJ4C_builder_reset( QAJ4C_Builder* me )
{
   QAJ4C_builder_init( me, me->buffer, me->buffer_size );
}

QAJ4C_Value* QAJ4C_builder_get_document( QAJ4C_Builder* builder ) {
    return (QAJ4C_Value*)builder->buffer;
}

void QAJ4C_set_bool( QAJ4C_Value* value_ptr, bool value ) {
    value_ptr->type = QAJ4C_BOOL_TYPE_CONSTANT;
    ((QAJ4C_Primitive*) value_ptr)->data.b = value;
}

void QAJ4C_set_int( QAJ4C_Value* value_ptr, int32_t value ) {
    QAJ4C_set_int64(value_ptr, value);
}

void QAJ4C_set_int64( QAJ4C_Value* value_ptr, int64_t value ) {
    if (value >= 0) {
        QAJ4C_set_uint64(value_ptr, value);
        return;
    }
    if ( value < INT32_MIN ) {
        value_ptr->type = QAJ4C_INT64_TYPE_CONSTANT;
    } else {
        value_ptr->type = QAJ4C_INT32_TYPE_CONSTANT;
    }
    ((QAJ4C_Primitive*) value_ptr)->data.i = value;
}

void QAJ4C_set_uint( QAJ4C_Value* value_ptr, uint32_t value ) {
    QAJ4C_set_uint64(value_ptr, value);
}

void QAJ4C_set_uint64( QAJ4C_Value* value_ptr, uint64_t value ) {
    if (value <= UINT32_MAX) {
        if (value <= INT32_MAX) {
            value_ptr->type = QAJ4C_UINT32_TYPE_INT32_COMPAT_CONSTANT;
        } else {
            value_ptr->type = QAJ4C_UINT32_TYPE_CONSTANT;
        }
    } else {
        if (value <= INT64_MAX) {
            value_ptr->type = QAJ4C_UINT64_TYPE_INT64_COMPAT_CONSTANT;
        } else {
            value_ptr->type = QAJ4C_UINT64_TYPE_CONSTANT;
        }
    }
    ((QAJ4C_Primitive*) value_ptr)->data.u = value;
}

void QAJ4C_set_double( QAJ4C_Value* value_ptr, double value ) {
    value_ptr->type = QAJ4C_DOUBLE_TYPE_CONSTANT;
    ((QAJ4C_Primitive*) value_ptr)->data.d = value;
}

void QAJ4C_set_null( QAJ4C_Value* value_ptr ) {
    value_ptr->type = QAJ4C_NULL_TYPE_CONSTANT;
}

void QAJ4C_set_string_ref_n( QAJ4C_Value* value_ptr, const char* str, size_t len ) {
    value_ptr->type = QAJ4C_STRING_REF_TYPE_CONSTANT;
    ((QAJ4C_String*)value_ptr)->s = str;
    ((QAJ4C_String*)value_ptr)->count = len;
}

void QAJ4C_set_string_ref( QAJ4C_Value* value_ptr, const char* str ) {
    QAJ4C_set_string_ref_n(value_ptr, str, QAJ4C_STRLEN(str));
}

void QAJ4C_set_string_copy_n( QAJ4C_Value* value_ptr, const char* str, size_t len, QAJ4C_Builder* builder ) {
    if (len <= QAJ4C_INLINE_STRING_SIZE) {
        value_ptr->type = QAJ4C_INLINE_STRING_TYPE_CONSTANT;
        ((QAJ4C_Short_string*)value_ptr)->count = (uint8_t)len;
        QAJ4C_MEMCPY(&((QAJ4C_Short_string*)value_ptr)->s, str, len);
        ((QAJ4C_Short_string*)value_ptr)->s[len] = '\0';
    } else {
        char* new_string;
        value_ptr->type = QAJ4C_STRING_TYPE_CONSTANT;
        QAJ4C_ASSERT(builder != NULL, {((QAJ4C_String*)value_ptr)->count = 0; ((QAJ4C_String*)value_ptr)->s = ""; return;});
        new_string = QAJ4C_builder_pop_string(builder, len + 1);
        QAJ4C_ASSERT(new_string != NULL, {((QAJ4C_String*)value_ptr)->count = 0; ((QAJ4C_String*)value_ptr)->s = ""; return;});
        ((QAJ4C_String*)value_ptr)->count = len;
        QAJ4C_MEMCPY(new_string, str, len);
        ((QAJ4C_String*)value_ptr)->s = new_string;
        new_string[len] = '\0';
    }
}

void QAJ4C_set_string_copy( QAJ4C_Value* value_ptr, const char* str, QAJ4C_Builder* builder ) {
    QAJ4C_set_string_copy_n(value_ptr, str, QAJ4C_STRLEN(str), builder);
}

void QAJ4C_set_array( QAJ4C_Value* value_ptr, size_t count, QAJ4C_Builder* builder ) {
    value_ptr->type = QAJ4C_ARRAY_TYPE_CONSTANT;
    ((QAJ4C_Array*)value_ptr)->top = QAJ4C_builder_pop_values(builder, count);
    if (QAJ4C_UNLIKELY(((QAJ4C_Array*)value_ptr)->top==NULL)) {
        ((QAJ4C_Array*)value_ptr)->count = 0;
    } else {
        ((QAJ4C_Array*)value_ptr)->count = count;
    }
}

QAJ4C_Value* QAJ4C_array_get_rw( QAJ4C_Value* value_ptr, size_t index ) {
    QAJ4C_ASSERT(QAJ4C_is_array(value_ptr) && QAJ4C_array_size(value_ptr) > index, {return NULL;});
    return ((QAJ4C_Array*) value_ptr)->top + index;
}

void QAJ4C_set_object( QAJ4C_Value* value_ptr, size_t count, QAJ4C_Builder* builder ) {
    value_ptr->type = QAJ4C_OBJECT_TYPE_CONSTANT;
    ((QAJ4C_Object*)value_ptr)->top = QAJ4C_builder_pop_members(builder, count);
    if (QAJ4C_UNLIKELY(((QAJ4C_Object*)value_ptr)->top==NULL)) {
        ((QAJ4C_Object*)value_ptr)->count = 0;
    } else {
        ((QAJ4C_Object*)value_ptr)->count = count;
    }
}

QAJ4C_Value* QAJ4C_object_create_member_by_ref_n( QAJ4C_Value* value_ptr, const char* str, size_t len ) {
    size_type count;
    size_type i;

    QAJ4C_ASSERT(QAJ4C_is_object(value_ptr), {return NULL;});
    count = ((QAJ4C_Object*) value_ptr)->count;

    for (i = 0; i < count; ++i) {
        QAJ4C_Value* key_value = &((QAJ4C_Object*) value_ptr)->top[i].key;
        if (QAJ4C_is_null(key_value)) {
            value_ptr->type = QAJ4C_OBJECT_TYPE_CONSTANT; /* not sorted anymore */
            QAJ4C_set_string_ref_n(key_value, str, len);
            return &((QAJ4C_Object*) value_ptr)->top[i].value;
        } else if (QAJ4C_string_equals_n(key_value, str, len)) {
            /* adding the same key twice voilates the json rules! */
            break;
        }
    }
    g_qaj4c_err_function();
    return NULL;
}

QAJ4C_Value* QAJ4C_object_create_member_by_ref( QAJ4C_Value* value_ptr, const char* str ) {
    return QAJ4C_object_create_member_by_ref_n(value_ptr, str, QAJ4C_STRLEN(str));
}

QAJ4C_Value* QAJ4C_object_create_member_by_copy_n( QAJ4C_Value* value_ptr, const char* str, size_t len, QAJ4C_Builder* builder ) {
    size_type count;
    size_type i;

    QAJ4C_ASSERT(QAJ4C_is_object(value_ptr), {return NULL;});
    count = ((QAJ4C_Object*) value_ptr)->count;

    for (i = 0; i < count; ++i) {
        QAJ4C_Value* key_value = &((QAJ4C_Object*) value_ptr)->top[i].key;
        if (QAJ4C_is_null(key_value)) {
            value_ptr->type = QAJ4C_OBJECT_TYPE_CONSTANT; /* not sorted anymore */
            QAJ4C_set_string_copy_n(key_value, str, len, builder);
            return &((QAJ4C_Object*) value_ptr)->top[i].value;
        } else if (QAJ4C_string_equals_n(key_value, str, len)) {
            /* adding the same key twice voilates the json rules! */
            break;
        }
    }
    g_qaj4c_err_function();
    return NULL;
}

QAJ4C_Value* QAJ4C_object_create_member_by_copy( QAJ4C_Value* value_ptr, const char* str, QAJ4C_Builder* builder ) {
    return QAJ4C_object_create_member_by_copy_n(value_ptr, str, QAJ4C_STRLEN(str), builder);
}

void QAJ4C_object_optimize( QAJ4C_Value* value_ptr ) {
    QAJ4C_ASSERT(QAJ4C_is_object(value_ptr), {return;});

    if (QAJ4C_get_internal_type(value_ptr) != QAJ4C_OBJECT_SORTED) {
        QAJ4C_Object* obj_ptr = (QAJ4C_Object*)value_ptr;
        QAJ4C_QSORT(obj_ptr->top, obj_ptr->count, sizeof(QAJ4C_Member), QAJ4C_compare_members);
        value_ptr->type = QAJ4C_OBJECT_SORTED_TYPE_CONSTANT;
    }
}


QAJ4C_Object_builder QAJ4C_object_builder_init( QAJ4C_Value* value_ptr, size_t member_count, bool deduplicate, QAJ4C_Builder* builder )
{
    QAJ4C_Object_builder object_builder = {value_ptr, deduplicate, 0, member_count};
    QAJ4C_Object* object_ptr = (QAJ4C_Object*)value_ptr;
    QAJ4C_set_object(value_ptr, member_count, builder);
    object_ptr->count = 0; /* The size management is performed by the builder */
    return object_builder;
}

QAJ4C_Value* QAJ4C_object_builder_create_member_by_ref_n( QAJ4C_Object_builder* value_ptr, const char* str, size_t len )
{
    QAJ4C_Value* return_value = NULL;
    QAJ4C_Object* object_ptr = ((QAJ4C_Object*)value_ptr->object);

    QAJ4C_ASSERT(value_ptr != NULL && value_ptr->pos < value_ptr->count, {return return_value;});

    if (value_ptr->strict) {
        return_value = (QAJ4C_Value*)QAJ4C_object_get_n(value_ptr->object, str, len);
    }

    if (return_value == NULL) {
        QAJ4C_Member* member = &object_ptr->top[value_ptr->pos];
        QAJ4C_set_string_ref_n(&member->key, str, len);
        return_value = &member->value;

        value_ptr->pos += 1;
        object_ptr->count = value_ptr->pos;
    }

    return return_value;
}

QAJ4C_Value* QAJ4C_object_builder_create_member_by_ref( QAJ4C_Object_builder* value_ptr, const char* str )
{
    return QAJ4C_object_builder_create_member_by_ref_n(value_ptr, str, QAJ4C_STRLEN(str));
}

QAJ4C_Value* QAJ4C_object_builder_create_member_by_copy_n( QAJ4C_Object_builder* value_ptr, const char* str, size_t len, QAJ4C_Builder* builder )
{
    QAJ4C_Value* return_value = NULL;
    QAJ4C_Object* object_ptr = ((QAJ4C_Object*)value_ptr->object);

    QAJ4C_ASSERT(value_ptr != NULL && value_ptr->pos < value_ptr->count, {return return_value;});

    if (value_ptr->strict) {
        return_value = (QAJ4C_Value*)QAJ4C_object_get_n(value_ptr->object, str, len);
    }

    if (return_value == NULL) {
        QAJ4C_Member* member = &object_ptr->top[value_ptr->pos];
        QAJ4C_set_string_copy_n(&member->key, str, len, builder);
        return_value = &member->value;

        value_ptr->pos += 1;
        object_ptr->count = value_ptr->pos;
    }

    return return_value;
}

QAJ4C_Value* QAJ4C_object_builder_create_member_by_copy( QAJ4C_Object_builder* value_ptr, const char* str, QAJ4C_Builder* builder )
{
    return QAJ4C_object_builder_create_member_by_copy_n(value_ptr, str, QAJ4C_STRLEN(str), builder);
}

void QAJ4C_copy( const QAJ4C_Value* src, QAJ4C_Value* dest, QAJ4C_Builder* builder ) {
    QAJ4C_INTERNAL_TYPE lhs_type = QAJ4C_get_internal_type(src);
    size_type i;
    size_type n;

    switch (lhs_type) {
    case QAJ4C_NULL:
    case QAJ4C_STRING_REF:
    case QAJ4C_INLINE_STRING:
    case QAJ4C_PRIMITIVE:
        *dest = *src;
        break;
    case QAJ4C_STRING:
        QAJ4C_set_string_copy_n(dest, QAJ4C_get_string(src), QAJ4C_get_string_length(src), builder);
        break;
    case QAJ4C_OBJECT:
    case QAJ4C_OBJECT_SORTED:
        n = QAJ4C_object_size(src);
        QAJ4C_set_object(dest, n, builder);

        for (i = 0; i < n; ++i) {
            const QAJ4C_Member* src_member = QAJ4C_object_get_member(src, i);
            QAJ4C_Member* dest_member = &((QAJ4C_Object*)dest)->top[i];

            QAJ4C_copy(&src_member->key, &dest_member->key, builder);
            QAJ4C_copy(&src_member->value, &dest_member->value, builder);
        }
        break;
    case QAJ4C_ARRAY:
        n = QAJ4C_array_size(src);
        QAJ4C_set_array(dest, n, builder);
        for (i = 0; i < n; ++i) {
            QAJ4C_copy(QAJ4C_array_get(src, i), QAJ4C_array_get_rw(dest, i), builder);
        }
        break;
    default:
        g_qaj4c_err_function();
        break;
    }
}

bool QAJ4C_equals( const QAJ4C_Value* lhs, const QAJ4C_Value* rhs ) {
    QAJ4C_TYPE lhs_type = QAJ4C_get_type(lhs);
    QAJ4C_TYPE rhs_type = QAJ4C_get_type(rhs);
    size_type i;
    size_type n;

    if (lhs_type != rhs_type) {
        return false;
    }
    switch (lhs_type) {
    case QAJ4C_TYPE_NULL:
        return true;
    case QAJ4C_TYPE_STRING:
        return QAJ4C_strcmp(lhs, rhs) == 0;
    case QAJ4C_TYPE_BOOL:
        return ((QAJ4C_Primitive*)lhs)->data.b == ((QAJ4C_Primitive*)rhs)->data.b;
    case QAJ4C_TYPE_NUMBER:
        return ((QAJ4C_Primitive*)lhs)->data.i == ((QAJ4C_Primitive*)rhs)->data.i;
    case QAJ4C_TYPE_OBJECT:
        n = QAJ4C_object_size(lhs);
        if (n != QAJ4C_object_size(rhs)) {
            return false;
        }
        for (i = 0; i < n; ++i) {
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
            rhs_value = QAJ4C_object_get_n(rhs, QAJ4C_get_string(key_lhs),
                                          QAJ4C_get_string_length(key_lhs));
            if (!QAJ4C_equals(lhs_value, rhs_value)) {
                return false;
            }
        }
        return true;
    case QAJ4C_TYPE_ARRAY:
        n = QAJ4C_array_size(lhs);
        if (n != QAJ4C_array_size(rhs)) {
            return false;
        }
        for (i = 0; i < n; ++i) {
            const QAJ4C_Value* value_lhs = QAJ4C_array_get(lhs, i);
            const QAJ4C_Value* value_rhs = QAJ4C_array_get(rhs, i);
            if (!QAJ4C_equals(value_lhs, value_rhs)) {
                return false;
            }
        }
        return true;
    default:
        g_qaj4c_err_function();
        break;
    }
    return false;
}


size_t QAJ4C_value_sizeof( const QAJ4C_Value* value_ptr ) {
    size_t size = sizeof(QAJ4C_Value);
    size_type i;
    size_type n;

    switch (QAJ4C_get_internal_type(value_ptr)) {
    case QAJ4C_OBJECT_SORTED:
    case QAJ4C_OBJECT:
        n = QAJ4C_object_size(value_ptr);
        for (i = 0; i < n; ++i) {
            const QAJ4C_Member* member = QAJ4C_object_get_member(value_ptr, i);
            size += QAJ4C_value_sizeof(&member->key);
            size += QAJ4C_value_sizeof(&member->value);
        }
        break;
    case QAJ4C_ARRAY:
        n = QAJ4C_array_size(value_ptr);
        for (i = 0; i < n; ++i) {
            const QAJ4C_Value* elem = QAJ4C_array_get(value_ptr, i);
            size += QAJ4C_value_sizeof(elem);
        }
        break;
    case QAJ4C_STRING:
        size += QAJ4C_get_string_length(value_ptr) + 1;
        break;
    case QAJ4C_ERROR_DESCRIPTION:
        size += sizeof(QAJ4C_Error_information);
        break;
    default:
        QAJ4C_ASSERT(QAJ4C_get_internal_type(value_ptr) != QAJ4C_UNSPECIFIED, {});
        break;
    }
    return size;
}



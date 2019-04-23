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

#include "qajson4c_builder.h"
#include "internal/types.h"

static bool QAJ4C_builder_validate_buffer( QAJ4C_Builder* builder );
static QAJ4C_Value* QAJ4C_builder_pop_values( QAJ4C_Builder* builder, size_type count );
static char* QAJ4C_builder_pop_string( QAJ4C_Builder* builder, size_type length );
static QAJ4C_Member* QAJ4C_builder_pop_members( QAJ4C_Builder* builder, size_type count );

QAJ4C_Builder QAJ4C_builder_create( void* buff, size_t buff_size ) {
    QAJ4C_Builder result;
    result.buffer = buff;
    result.buffer_size = buff_size;

    QAJ4C_builder_reset(&result);
    return result;
}

void QAJ4C_builder_reset( QAJ4C_Builder* me ) {
    /* objects grow from front to end (and always contains a document) */
    me->obj_ptr = (QAJ4C_Value*)me->buffer;
    /* strings from end to front */
    me->str_ptr = ((char*)me->buffer) + me->buffer_size;
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

void QAJ4C_set_string_ref( QAJ4C_Value* value_ptr, const char* str, size_t len ) {
    value_ptr->type = QAJ4C_STRING_REF_TYPE_CONSTANT;
    ((QAJ4C_String*)value_ptr)->s = str;
    ((QAJ4C_String*)value_ptr)->count = len;
}

void QAJ4C_set_string_copy( QAJ4C_Value* value_ptr, const char* str, size_t len, QAJ4C_Builder* builder ) {
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

QAJ4C_Array_builder QAJ4C_array_builder_create( QAJ4C_Value* value_ptr, size_t capacity, QAJ4C_Builder* builder ) {
    QAJ4C_Array_builder array_builder;
    QAJ4C_Array* array_ptr = (QAJ4C_Array*)value_ptr;
    value_ptr->type = QAJ4C_ARRAY_TYPE_CONSTANT;
    array_ptr->count = 0; /* The size management is performed by the builder */
    array_builder.array = value_ptr;
    array_builder.capacity = capacity;
    array_ptr->top = QAJ4C_builder_pop_values( builder, capacity );
    return array_builder;
}

QAJ4C_Value* QAJ4C_array_builder_next( QAJ4C_Array_builder* builder ) {
    QAJ4C_Array* array_ptr = (QAJ4C_Array*)builder->array;
    QAJ4C_ASSERT(array_ptr != NULL && array_ptr->count < builder->capacity, {return NULL;});
    QAJ4C_Value* result = array_ptr->top + array_ptr->count;
    array_ptr->count += 1;
    return result;
}

QAJ4C_Object_builder QAJ4C_object_builder_create( QAJ4C_Value* value_ptr, size_t member_capacity, bool deduplicate, QAJ4C_Builder* builder )
{
    QAJ4C_Object_builder object_builder;
    QAJ4C_Object* object_ptr = (QAJ4C_Object*)value_ptr;
    value_ptr->type = QAJ4C_OBJECT_SORTED_TYPE_CONSTANT;
    object_ptr->count = 0; /* The size management is performed by the builder */

    object_builder.strict = deduplicate;
    object_builder.object = value_ptr;
    object_builder.capacity = member_capacity;
    object_ptr->top = QAJ4C_builder_pop_members( builder, member_capacity );
    return object_builder;
}

QAJ4C_Value* QAJ4C_object_builder_create_member_by_ref( QAJ4C_Object_builder* object_builder, const char* str, size_t len )
{
    QAJ4C_Value* return_value = NULL;
    QAJ4C_Object* object_ptr = ((QAJ4C_Object*)object_builder->object);

    QAJ4C_ASSERT(object_ptr != NULL && object_ptr->count < object_builder->capacity, {return return_value;});

    if (object_builder->strict) {
        return_value = (QAJ4C_Value*)QAJ4C_object_get_n(object_builder->object, str, len);
    }

    if (return_value == NULL) {
        QAJ4C_Member* member = &object_ptr->top[object_ptr->count];
        QAJ4C_set_string_ref(&member->key, str, len);
        return_value = &member->value;
        object_ptr->count += 1;
    }

    if (object_builder->strict && object_ptr->count > 1) {
        // FIXME: sort if the last appended value does not match the sorting!
    }

    return return_value;
}

QAJ4C_Value* QAJ4C_object_builder_create_member_by_copy( QAJ4C_Object_builder* object_builder, const char* str, size_t len, QAJ4C_Builder* builder )
{
    QAJ4C_Value* return_value = NULL;
    QAJ4C_Object* object_ptr = ((QAJ4C_Object*)object_builder->object);

    QAJ4C_ASSERT(object_ptr != NULL && object_ptr->count < object_builder->capacity, {return return_value;});

    if (object_builder->strict) {
        return_value = (QAJ4C_Value*)QAJ4C_object_get_n(object_builder->object, str, len);
    }

    if (return_value == NULL) {
        QAJ4C_Member* member = &object_ptr->top[object_ptr->count];
        QAJ4C_set_string_copy(&member->key, str, len, builder);
        return_value = &member->value;
        object_ptr->count += 1;
    }

    if (object_builder->strict) {
        // FIXME: sort if the last appended value does not match the sorting!
    }

    return return_value;
}

void QAJ4C_object_optimize( QAJ4C_Value* value_ptr ) {
    QAJ4C_ASSERT(QAJ4C_is_object(value_ptr), {return;});

    if (QAJ4C_get_internal_type(value_ptr) != QAJ4C_OBJECT_SORTED) {
        QAJ4C_Object* obj_ptr = (QAJ4C_Object*)value_ptr;
        QAJ4C_QSORT(obj_ptr->top, obj_ptr->count, sizeof(QAJ4C_Member), QAJ4C_compare_members);
        value_ptr->type = QAJ4C_OBJECT_SORTED_TYPE_CONSTANT;
    }
}

void QAJ4C_copy( const QAJ4C_Value* src, QAJ4C_Value* dest, QAJ4C_Builder* builder ) {
    /* FIXME: Remove recursion */
    QAJ4C_INTERNAL_TYPE lhs_type = QAJ4C_get_internal_type(src);

    switch (lhs_type) {
    case QAJ4C_NULL:
    case QAJ4C_STRING_REF:
    case QAJ4C_INLINE_STRING:
    case QAJ4C_PRIMITIVE:
        *dest = *src;
        break;
    case QAJ4C_STRING:
        QAJ4C_set_string_copy(dest, QAJ4C_get_string(src), QAJ4C_get_string_length(src), builder);
        break;
    case QAJ4C_OBJECT:
    case QAJ4C_OBJECT_SORTED:
        // FIXME: Implement copying the object
        break;
    case QAJ4C_ARRAY:
        // FIXME: Implement copying the array
        break;
    default:
        g_qaj4c_err_function();
        break;
    }
}

static bool QAJ4C_builder_validate_buffer( QAJ4C_Builder* builder ) {
    // FIXME: Altered implementation
    return (void*)builder->obj_ptr <= (void*)(builder->str_ptr + 1);
}

static QAJ4C_Value* QAJ4C_builder_pop_values( QAJ4C_Builder* builder, size_type count ) {
    QAJ4C_Value* new_pointer;
    if (count == 0) {
        return NULL;
    }
    new_pointer = builder->obj_ptr;
    builder->obj_ptr += count;
    QAJ4C_ASSERT(QAJ4C_builder_validate_buffer(builder), {return NULL;});
    return new_pointer;
}

static char* QAJ4C_builder_pop_string( QAJ4C_Builder* builder, size_type length ) {
    builder->str_ptr -= length;
    QAJ4C_ASSERT(QAJ4C_builder_validate_buffer(builder), {return NULL;});
    return builder->str_ptr;
}

static QAJ4C_Member* QAJ4C_builder_pop_members( QAJ4C_Builder* builder, size_type count ) {
    return (QAJ4C_Member*)QAJ4C_builder_pop_values(builder, count * 2);
}


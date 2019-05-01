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

typedef struct QAJ4C_builder_stack_entry {
    const QAJ4C_Value* src_value;
    QAJ4C_Value* dst_value;
    fast_size_type index;
    fast_size_type length;
} QAJ4C_builder_stack_entry;

typedef struct QAJ4C_builder_stack {
    QAJ4C_builder_stack_entry info[QAJ4C_STACK_SIZE];
    QAJ4C_builder_stack_entry* it;
} QAJ4C_builder_stack;

static QAJ4C_Value* QAJ4C_builder_pop_values( QAJ4C_Builder* builder, size_type count );
static char* QAJ4C_builder_pop_string( QAJ4C_Builder* builder, size_type length );
static QAJ4C_Member* QAJ4C_builder_pop_members( QAJ4C_Builder* builder, size_type count );
static bool stack_up_copy(QAJ4C_builder_stack* stack, const QAJ4C_Value* lhs, QAJ4C_Value* rhs, QAJ4C_Builder* builder);

QAJ4C_Builder QAJ4C_builder_create( void* buff, size_t buff_size ) {
    QAJ4C_Builder result;
    result.buffer = buff;
    result.buffer_size = buff_size;

    QAJ4C_builder_reset(&result);
    return result;
}

void QAJ4C_builder_reset( QAJ4C_Builder* me ) {
    /* objects grow from front to end (and always contains a document) */
    me->obj_ptr = me->buffer == NULL ? NULL : (((QAJ4C_Value*)me->buffer) + 1);
    /* strings from end to front */
    me->str_ptr = me->buffer == NULL ? NULL : (((char*)me->buffer) + me->buffer_size);
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

QAJ4C_Array_builder QAJ4C_array_builder_create( size_t capacity, QAJ4C_Builder* builder ) {
    QAJ4C_Array_builder array_builder = {
            .top = QAJ4C_builder_pop_values( builder, capacity ),
            .index = 0,
            .capacity = capacity
    };

    if (QAJ4C_UNLIKELY(array_builder.top == NULL)) {
        array_builder.capacity = 0;
    }

    return array_builder;
}

QAJ4C_Value* QAJ4C_array_builder_next( QAJ4C_Array_builder* array_builder ) {
    QAJ4C_Value* result;
    QAJ4C_ASSERT(array_builder != NULL && array_builder->index < array_builder->capacity, {return NULL;});
    result = array_builder->top + array_builder->index;
    array_builder->index += 1;
    return result;
}

void QAJ4C_set_array( QAJ4C_Value* value_ptr, QAJ4C_Array_builder* array_builder ) {
    QAJ4C_Array* array_ptr = (QAJ4C_Array*)value_ptr;
    value_ptr->type = QAJ4C_ARRAY_TYPE_CONSTANT;
    array_ptr->top = array_builder->top;
    array_ptr->count = array_builder->index;
}

QAJ4C_Object_builder QAJ4C_object_builder_create( size_t member_capacity, QAJ4C_Builder* builder ) {
    QAJ4C_Object_builder object_builder = {
        .top = QAJ4C_builder_pop_members( builder, member_capacity ),
        .index = 0,
        .capacity = member_capacity
    };

    if (QAJ4C_UNLIKELY(object_builder.top == NULL)) {
        object_builder.capacity = 0;
    }

    return object_builder;
}

QAJ4C_Value* QAJ4C_object_builder_create_member_by_ref( QAJ4C_Object_builder* object_builder, const char* str, size_t len ) {
    QAJ4C_Member* member;
    QAJ4C_ASSERT(object_builder != NULL && object_builder->index < object_builder->capacity, {return NULL;});
    member = object_builder->top + object_builder->index;
    QAJ4C_set_string_ref(&member->key, str, len);
    object_builder->index += 1;
    return &member->value;
}

QAJ4C_Value* QAJ4C_object_builder_create_member_by_copy( QAJ4C_Object_builder* object_builder, const char* str, size_t len, QAJ4C_Builder* builder ) {
    QAJ4C_Member* member;
    QAJ4C_ASSERT(object_builder != NULL && object_builder->index < object_builder->capacity, {return NULL;});
    member = object_builder->top + object_builder->index;
    QAJ4C_set_string_copy(&member->key, str, len, builder);
    object_builder->index += 1;
    return &member->value;
}

void QAJ4C_set_object( QAJ4C_Value* value_ptr, QAJ4C_Object_builder* object_builder ) {
    QAJ4C_Object* obj_ptr = (QAJ4C_Object*)value_ptr;
    value_ptr->type = QAJ4C_OBJECT_TYPE_CONSTANT;
    obj_ptr->top = object_builder->top;
    obj_ptr->count = object_builder->index;
    QAJ4C_object_optimize(obj_ptr);
}

void QAJ4C_copy( const QAJ4C_Value* src, QAJ4C_Value* dest, QAJ4C_Builder* builder ) {
    QAJ4C_builder_stack stack;
    stack.it = stack.info;
    stack.it->index = 0;
    stack.it->length = 1;
    stack.it->src_value = src;
    stack.it->dst_value = dest;

    while (stack.it > stack.info || stack.it->index < stack.it->length) {
        const QAJ4C_Value* src_value = stack.it->src_value + stack.it->index;
        QAJ4C_Value* dst_value = stack.it->dst_value + stack.it->index;
        QAJ4C_INTERNAL_TYPE src_type = QAJ4C_get_internal_type(src_value);
        stack.it->index += 1;

        switch (src_type) {
        case QAJ4C_NULL:
        case QAJ4C_STRING_REF:
        case QAJ4C_INLINE_STRING:
        case QAJ4C_PRIMITIVE:
            *dst_value = *src_value;
            break;
        case QAJ4C_STRING:
            QAJ4C_set_string_copy(dst_value, QAJ4C_get_string(src_value), QAJ4C_get_string_length(src_value), builder);
            break;
        case QAJ4C_OBJECT:
        case QAJ4C_ARRAY:
            stack_up_copy(&stack, src_value, dst_value, builder);
            break;
        default:
            g_qaj4c_err_function();
            return;
        }
        while ( stack.it > stack.info && stack.it->index >= stack.it->length )
        {
            stack.it -= 1;
        }
    }
}

static QAJ4C_Value* QAJ4C_builder_pop_values( QAJ4C_Builder* builder, size_type count ) {
    QAJ4C_Value* new_pointer;
    if (count == 0) {
        return NULL;
    }
    // FIXME: Test if the boundary check is correctly implemented
    QAJ4C_ASSERT(builder != NULL && (void*)(builder->obj_ptr + count) < (void*)(builder->str_ptr + 1), {return NULL;});
    new_pointer = builder->obj_ptr;
    builder->obj_ptr += count;
    return new_pointer;
}

static char* QAJ4C_builder_pop_string( QAJ4C_Builder* builder, size_type length ) {
    size_t count = builder->str_ptr - (char*)(builder->obj_ptr);
    // FIXME: Test if the boundary check is correctly implemented
    QAJ4C_ASSERT(builder != NULL && count >= length, {return NULL;});
    builder->str_ptr -= length;
    return builder->str_ptr;
}

static QAJ4C_Member* QAJ4C_builder_pop_members( QAJ4C_Builder* builder, size_type count ) {
    return (QAJ4C_Member*)QAJ4C_builder_pop_values(builder, count * 2);
}

static bool stack_up_copy(QAJ4C_builder_stack* stack, const QAJ4C_Value* lhs, QAJ4C_Value* rhs, QAJ4C_Builder* builder) {
    const QAJ4C_Array* lhs_array = (const QAJ4C_Array*)lhs;
    QAJ4C_Array* rhs_array = (QAJ4C_Array*)rhs;
    size_type value_count = QAJ4C_get_type(lhs) == QAJ4C_TYPE_OBJECT ? lhs_array->count * 2 : lhs_array->count;

    rhs->type = lhs->type;
    rhs_array->count = lhs_array->count;
    rhs_array->top = QAJ4C_builder_pop_values(builder, value_count);

    QAJ4C_ASSERT(stack->it < stack->info + QAJ4C_ARRAY_COUNT(stack->info) - 1 && rhs_array->top != NULL, {rhs_array->count = 0; return false;});

    stack->it += 1;
    stack->it->index = 0;
    stack->it->src_value = lhs_array->top;
    stack->it->length = value_count;
    stack->it->dst_value = rhs_array->top;

    return true;
}


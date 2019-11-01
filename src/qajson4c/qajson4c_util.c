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

#include "qajson4c_util.h"
#include "internal/types.h"

typedef struct QAJ4C_util_stack_entry {
    const QAJ4C_Value* lhs_value;
    const QAJ4C_Value* rhs_value;
    fast_size_type index;
    fast_size_type length;
} QAJ4C_util_stack_entry;

typedef struct QAJ4C_util_stack {
    QAJ4C_util_stack_entry info[QAJ4C_STACK_SIZE];
    QAJ4C_util_stack_entry* it;
} QAJ4C_util_stack;

static bool stack_up_equal(QAJ4C_util_stack* stack, const QAJ4C_Value* lhs, const QAJ4C_Value* rhs) {
    size_type lhs_count = QAJ4C_ARRAY_GET_COUNT(lhs);
    bool result = lhs_count == QAJ4C_ARRAY_GET_COUNT(rhs);

    QAJ4C_ASSERT(stack->it < stack->info + QAJ4C_ARRAY_COUNT(stack->info) - 1, result = false;);

    if (result) {
        stack->it += 1;
        stack->it->index = 0;
        stack->it->length = lhs_count * (QAJ4C_get_type(lhs) == QAJ4C_TYPE_OBJECT ? 2 : 1);
        stack->it->lhs_value = QAJ4C_ARRAY_GET_PTR(lhs);
        stack->it->rhs_value = QAJ4C_ARRAY_GET_PTR(rhs);
    }

    return result;
}

static void stack_up_sizeof(QAJ4C_util_stack* stack, const QAJ4C_Value* lhs) {
    QAJ4C_ASSERT(stack->it < stack->info + QAJ4C_ARRAY_COUNT(stack->info) - 1, return;);

    stack->it += 1;
    stack->it->index = 0;
    stack->it->length = QAJ4C_ARRAY_GET_COUNT(lhs) * (QAJ4C_get_type(lhs) == QAJ4C_TYPE_OBJECT ? 2 : 1);
    stack->it->lhs_value = QAJ4C_ARRAY_GET_PTR(lhs);
}

bool QAJ4C_equals( const QAJ4C_Value* lhs, const QAJ4C_Value* rhs ) {
    bool result = true;
    QAJ4C_util_stack stack;
    stack.it = stack.info;
    stack.it->lhs_value = lhs;
    stack.it->rhs_value = rhs;
    stack.it->index = 0;
    stack.it->length = 1;

    while (result && (stack.it > stack.info || stack.it->index < stack.it->length)) {
        const QAJ4C_Value* lhs_value = stack.it->lhs_value + stack.it->index;
        const QAJ4C_Value* rhs_value = stack.it->rhs_value + stack.it->index;
        QAJ4C_TYPE lhs_type = QAJ4C_get_type(lhs_value);
        QAJ4C_TYPE rhs_type = QAJ4C_get_type(rhs_value);
        stack.it->index += 1;

        if (lhs_type != rhs_type) {
            result = false;
        } else {
            switch (lhs_type) {
            case QAJ4C_TYPE_NULL:
                break; // type equality is value equality
            case QAJ4C_TYPE_STRING:
                result = QAJ4C_strcmp(lhs_value, rhs_value) == 0;
                break;
            case QAJ4C_TYPE_BOOL:
                result = QAJ4C_GET_VALUE(lhs_value, bool) == QAJ4C_GET_VALUE(rhs_value, bool);
                break;
            case QAJ4C_TYPE_NUMBER:
                // FIXME: This is not correctly for doubles!!
                result = QAJ4C_GET_VALUE(lhs_value, int64_t) == QAJ4C_GET_VALUE(rhs_value, int64_t);
                break;
            case QAJ4C_TYPE_OBJECT:
            case QAJ4C_TYPE_ARRAY:
                result = stack_up_equal(&stack, lhs_value, rhs_value);
                break;
            default:
                g_qaj4c_err_function();
                result = false;
                break;
            }
        }
        while ( stack.it > stack.info && stack.it->index >= stack.it->length )
        {
            stack.it -= 1;
        }
    }

    return result;
}

size_t QAJ4C_value_sizeof( const QAJ4C_Value* root_ptr ) {
    size_t size = 0;

    QAJ4C_util_stack stack;
    stack.it = stack.info;

    stack.it->lhs_value = root_ptr;
    stack.it->index = 0;
    stack.it->length = 1;

    while (stack.it > stack.info || stack.it->index < stack.it->length) {
        const QAJ4C_Value* lhs_value = stack.it->lhs_value + stack.it->index;
        stack.it->index += 1;

        size += sizeof(QAJ4C_Value);

        switch (QAJ4C_get_internal_type(lhs_value)) {
        case QAJ4C_OBJECT:
        case QAJ4C_ARRAY:
            stack_up_sizeof(&stack, lhs_value);
            break;
        case QAJ4C_STRING:
            size += QAJ4C_get_string_length(lhs_value) + 1;
            break;
        case QAJ4C_ERROR_DESCRIPTION:
            size += sizeof(QAJ4C_Error_information);
            break;
        default:
            break;
        }

        while ( stack.it > stack.info && stack.it->index >= stack.it->length )
        {
            stack.it -= 1;
        }
    }
    return size;
}

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

bool QAJ4C_equals( const QAJ4C_Value* lhs, const QAJ4C_Value* rhs ) {
    /* FIXME: Remove recursion */
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

size_t QAJ4C_value_sizeof( const QAJ4C_Value* root_ptr ) {
    size_t size = 0;
    size_type i;
    size_type n = 1;

    for (i = 0; i < n; ++i) {
        const QAJ4C_Value* value_ptr = root_ptr + i;
        switch (QAJ4C_get_internal_type(value_ptr)) {
        case QAJ4C_OBJECT_SORTED:
        case QAJ4C_OBJECT:
            n = QAJ4C_MAX(n, ((QAJ4C_Array* )value_ptr)->top + ((QAJ4C_Array* )value_ptr)->count * 2 - root_ptr);
            break;
        case QAJ4C_ARRAY:
            n = QAJ4C_MAX(n, ((QAJ4C_Array* )value_ptr)->top + ((QAJ4C_Array* )value_ptr)->count - root_ptr);
            break;
        case QAJ4C_STRING:
            size += QAJ4C_get_string_length(value_ptr) + 1;
            break;
        default:
            break;
        }
    }
    return n * sizeof(QAJ4C_Value) + size;
}

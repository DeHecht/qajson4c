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

#include "types.h"

#include "qajson_stdwrap.h"
#include "first_pass.h"
#include "second_pass.h"

QAJ4C_fatal_error_fn g_qaj4c_err_function = NULL;

void QAJ4C_std_err_function( void ) {
    QAJ4C_RAISE(SIGABRT);
}

QAJ4C_INTERNAL_TYPE QAJ4C_get_internal_type( const QAJ4C_Value* value_ptr ) {
    if (value_ptr == NULL) {
        return QAJ4C_NULL;
    }
    return (value_ptr->type >> 8) & 0xFF;
}

uint8_t QAJ4C_get_compatibility_types( const QAJ4C_Value* value_ptr ) {
    return (value_ptr->type >> 16) & 0xFF;
}

uint8_t QAJ4C_get_storage_type( const QAJ4C_Value* value_ptr ) {
    return (value_ptr->type >> 24) & 0xFF;
}

QAJ4C_String_payload QAJ4C_get_string_payload( const QAJ4C_Value* value_ptr ) {
    QAJ4C_String_payload payload;
    bool fatal = false;
    QAJ4C_ASSERT(QAJ4C_is_string(value_ptr), {fatal = true;});

    if (QAJ4C_UNLIKELY(fatal)) {
        payload.str = "";
        payload.size = 0;
    } else if (QAJ4C_get_internal_type(value_ptr) == QAJ4C_STRING_INLINE) {
        payload.str = ((QAJ4C_Short_string*)value_ptr)->s;
        payload.size = ((QAJ4C_Short_string*)value_ptr)->count;
    } else {
        payload.str = ((QAJ4C_String*)value_ptr)->s;
        payload.size = ((QAJ4C_String*)value_ptr)->count;
    }
    return payload;
}

void QAJ4C_object_optimize( QAJ4C_Object* value_ptr ) {
    QAJ4C_QSORT(value_ptr->top, value_ptr->count, sizeof(QAJ4C_Member), QAJ4C_compare_members);
}

bool QAJ4C_object_has_duplicate( QAJ4C_Object* value_ptr ) {
    bool duplicate = false;
    fast_size_type n = value_ptr->count;
    if ( n > 1 ) {
        for (fast_size_type i = 1; i < n; ++i) {
            if (QAJ4C_strcmp(&value_ptr->top[i - 1].key, &value_ptr->top[i].key) == 0) {
                duplicate = true;
            }
        }
    }
    return duplicate;
}

/*
 * The comparison will first check on the string size and then on the content as we
 * only require this for matching purposes. The string length is also stored within
 * the objects, so check is not expensive!
 */
int QAJ4C_strcmp( const QAJ4C_Value* lhs, const QAJ4C_Value* rhs ) {
    QAJ4C_String_payload lhs_payload = QAJ4C_get_string_payload(lhs);
    QAJ4C_String_payload rhs_payload = QAJ4C_get_string_payload(rhs);

    if (lhs_payload.size != rhs_payload.size) {
        return lhs_payload.size - rhs_payload.size;
    }
    return QAJ4C_MEMCMP(lhs_payload.str, rhs_payload.str, lhs_payload.size);
}

/*
 * The API does not provide any way a member key could be something else, than
 * a string so there is no need to check this type.
 */
int QAJ4C_compare_members( const void* lhs, const void* rhs ) {
    const QAJ4C_Member* left = lhs;
    const QAJ4C_Member* right = rhs;

    return QAJ4C_strcmp(&left->key, &right->key);
}

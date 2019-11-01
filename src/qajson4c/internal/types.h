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

#ifndef QAJSON4C_INTERNAL_H_
#define QAJSON4C_INTERNAL_H_

#include "qajson_stdwrap.h"
#include "../qajson4c_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Define unlikely macro to be used in asserts
 *  Basically because the compiler should compile for the non-failure scenario.
 **/
#if __GNUC__ >= 3
#define QAJ4C_UNLIKELY(expr) __builtin_expect(expr, 0)
#else
#define QAJ4C_UNLIKELY(expr) expr
#endif

#ifndef QAJ4C_STACK_SIZE
#define QAJ4C_STACK_SIZE (32)
#endif

#define QAJ4C_STRUCT_SIZE (sizeof(size_type) + sizeof(uintptr_t))

#define QAJ4C_GET_VALUE(value_ptr, type) QAJ4C_GET_VALUE_OFFSET(value_ptr, type, 0)
#define QAJ4C_SET_VALUE(value_ptr, type, value) QAJ4C_GET_VALUE(value_ptr, type) = value
#define QAJ4C_GET_VALUE_OFFSET(value_ptr, type, offset) (*((type*)((value_ptr)->data+offset)))

#define QAJ4C_ARRAY_GET_PTR(value_ptr) QAJ4C_GET_VALUE(value_ptr, QAJ4C_Value*)
#define QAJ4C_ARRAY_GET_COUNT(value_ptr) QAJ4C_GET_VALUE_OFFSET(value_ptr, size_type, sizeof(QAJ4C_Value*))
#define QAJ4C_OBJECT_GET_PTR(value_ptr) QAJ4C_GET_VALUE(value_ptr, QAJ4C_Member*)
#define QAJ4C_OBJECT_GET_COUNT(value_ptr) QAJ4C_GET_VALUE_OFFSET(value_ptr, size_type, sizeof(QAJ4C_Member*))
#define QAJ4C_STRING_GET_PTR(value_ptr) QAJ4C_GET_VALUE(value_ptr, const char*)
#define QAJ4C_STRING_GET_COUNT(value_ptr) QAJ4C_GET_VALUE_OFFSET(value_ptr, size_type, sizeof(char*))
#define QAJ4C_ISTRING_GET_PTR(value_ptr) &QAJ4C_GET_VALUE_OFFSET(value_ptr, char, sizeof(uint8_t))
#define QAJ4C_ISTRING_GET_COUNT(value_ptr) QAJ4C_GET_VALUE(value_ptr, uint8_t)
#define QAJ4C_ERROR_GET_PTR(value_ptr) QAJ4C_GET_VALUE(value_ptr, QAJ4C_Error_information*)

#define QAJ4C_ASSERT(arg, alt) if (QAJ4C_UNLIKELY(!(arg))) do { g_qaj4c_err_function(); alt } while(0)

#define QAJ4C_MIN(lhs, rhs) ((lhs<=rhs)?(lhs):(rhs))
#define QAJ4C_MAX(lhs, rhs) ((lhs>=rhs)?(lhs):(rhs))

#define QAJ4C_INLINE_STRING_SIZE (sizeof(uintptr_t) + sizeof(size_type) - sizeof(uint8_t) * 2)

#define QAJ4C_NULL_TYPE_CONSTANT   ((QAJ4C_NULL << 8) | QAJ4C_TYPE_NULL)
#define QAJ4C_OBJECT_TYPE_CONSTANT ((QAJ4C_OBJECT << 8) | QAJ4C_TYPE_OBJECT)
#define QAJ4C_OBJECT_RAW_TYPE_CONSTANT ((QAJ4C_OBJECT_RAW << 8) | QAJ4C_TYPE_OBJECT)
#define QAJ4C_ARRAY_TYPE_CONSTANT  ((QAJ4C_ARRAY << 8) | QAJ4C_TYPE_ARRAY)
#define QAJ4C_STRING_TYPE_CONSTANT ((QAJ4C_STRING << 8) | QAJ4C_TYPE_STRING)
#define QAJ4C_STRING_REF_TYPE_CONSTANT ((QAJ4C_STRING_REF << 8) | QAJ4C_TYPE_STRING)
#define QAJ4C_STRING_INLINE_TYPE_CONSTANT ((QAJ4C_STRING_INLINE << 8) | QAJ4C_TYPE_STRING)
#define QAJ4C_ERROR_DESCRIPTION_TYPE_CONSTANT ((QAJ4C_ERROR_DESCRIPTION << 8) | QAJ4C_TYPE_INVALID)

#define QAJ4C_NUMBER_TYPE_CONSTANT ((QAJ4C_PRIMITIVE << 8) | QAJ4C_TYPE_NUMBER)

#define QAJ4C_BOOL_TYPE_CONSTANT   ((QAJ4C_PRIMITIVE_BOOL << 24) | (QAJ4C_PRIMITIVE_BOOL << 16) | (QAJ4C_PRIMITIVE << 8) |  QAJ4C_TYPE_BOOL)
#define QAJ4C_UINT64_TYPE_CONSTANT ((QAJ4C_PRIMITIVE_UINT64 << 24) | ((QAJ4C_PRIMITIVE_UINT64 | QAJ4C_PRIMITIVE_DOUBLE) << 16) | (QAJ4C_NUMBER_TYPE_CONSTANT))
#define QAJ4C_UINT64_TYPE_INT64_COMPAT_CONSTANT ((QAJ4C_PRIMITIVE_UINT64 << 24) | ((QAJ4C_PRIMITIVE_UINT64 | QAJ4C_PRIMITIVE_DOUBLE | QAJ4C_PRIMITIVE_INT64) << 16) | (QAJ4C_NUMBER_TYPE_CONSTANT))
#define QAJ4C_UINT32_TYPE_CONSTANT ((QAJ4C_PRIMITIVE_UINT << 24) | ((QAJ4C_PRIMITIVE_UINT64 | QAJ4C_PRIMITIVE_UINT | QAJ4C_PRIMITIVE_INT64 | QAJ4C_PRIMITIVE_DOUBLE) << 16) | (QAJ4C_NUMBER_TYPE_CONSTANT))
#define QAJ4C_UINT32_TYPE_INT32_COMPAT_CONSTANT ((QAJ4C_PRIMITIVE_UINT << 24) | ((QAJ4C_PRIMITIVE_UINT64 | QAJ4C_PRIMITIVE_UINT | QAJ4C_PRIMITIVE_INT64 | QAJ4C_PRIMITIVE_DOUBLE | QAJ4C_PRIMITIVE_INT) << 16) | (QAJ4C_NUMBER_TYPE_CONSTANT))
#define QAJ4C_INT64_TYPE_CONSTANT ((QAJ4C_PRIMITIVE_INT64 << 24) | ((QAJ4C_PRIMITIVE_INT64 | QAJ4C_PRIMITIVE_DOUBLE) << 16) | (QAJ4C_NUMBER_TYPE_CONSTANT))
#define QAJ4C_INT32_TYPE_CONSTANT ((QAJ4C_PRIMITIVE_INT << 24) | ((QAJ4C_PRIMITIVE_INT | QAJ4C_PRIMITIVE_INT64 | QAJ4C_PRIMITIVE_DOUBLE) << 16) | (QAJ4C_NUMBER_TYPE_CONSTANT))
#define QAJ4C_DOUBLE_TYPE_CONSTANT (((QAJ4C_PRIMITIVE_DOUBLE << 24) | (QAJ4C_PRIMITIVE_DOUBLE << 16)) | (QAJ4C_NUMBER_TYPE_CONSTANT))

#define QAJ4C_ARRAY_COUNT(a) (sizeof(a) / sizeof(a[0]))

typedef uint32_t size_type;
typedef uint_fast32_t fast_size_type;

typedef enum QAJ4C_INTERNAL_TYPE {
    QAJ4C_NULL = 0,
    QAJ4C_UNSPECIFIED,
    QAJ4C_OBJECT,
    QAJ4C_OBJECT_RAW,
    QAJ4C_ARRAY,
    QAJ4C_STRING,
    QAJ4C_STRING_REF,
    QAJ4C_STRING_INLINE,
    QAJ4C_PRIMITIVE,
    QAJ4C_ERROR_DESCRIPTION
} QAJ4C_INTERNAL_TYPE;

typedef enum QAJ4C_Primitive_type {
    QAJ4C_PRIMITIVE_BOOL = (1 << 0),
    QAJ4C_PRIMITIVE_INT = (1 << 1),
    QAJ4C_PRIMITIVE_INT64 = (1 << 2),
    QAJ4C_PRIMITIVE_UINT = (1 << 3),
    QAJ4C_PRIMITIVE_UINT64 = (1 << 4),
    QAJ4C_PRIMITIVE_DOUBLE = (1 << 5)
} QAJ4C_Primitive_type;

struct QAJ4C_Value {
    size_type type;
    uint8_t data[QAJ4C_STRUCT_SIZE];
};

typedef struct QAJ4C_Error_information {
    const char* json;
    size_type json_pos;
    size_type err_no;
} QAJ4C_Error_information;

struct QAJ4C_Member {
    QAJ4C_Value key;
    QAJ4C_Value value;
};

typedef struct QAJ4C_Json_message {
    const char* begin;
    const char* end;
    const char* pos;
} QAJ4C_Json_message;

typedef struct QAJ4C_String_payload {
    const char* str;
    fast_size_type size;
} QAJ4C_String_payload;

extern QAJ4C_fatal_error_fn g_qaj4c_err_function;

void QAJ4C_std_err_function( void );

uint8_t QAJ4C_get_storage_type( const QAJ4C_Value* value_ptr );
uint8_t QAJ4C_get_compatibility_types( const QAJ4C_Value* value_ptr );
QAJ4C_INTERNAL_TYPE QAJ4C_get_internal_type( const QAJ4C_Value* value_ptr );

QAJ4C_String_payload QAJ4C_get_string_payload( const QAJ4C_Value* value_ptr );

void QAJ4C_object_optimize( QAJ4C_Value* value_ptr );
bool QAJ4C_object_has_duplicate( QAJ4C_Value* value_ptr );

int QAJ4C_strcmp( const QAJ4C_Value* lhs, const QAJ4C_Value* rhs );
int QAJ4C_compare_members( const void* lhs, const void * rhs );
int QAJ4C_compare_members_stable( const void* lhs, const void * rhs );


#ifdef __cplusplus
}
#endif

#endif /* QAJSON4C_INTERNAL_H_ */

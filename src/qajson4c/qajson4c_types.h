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

#ifndef QAJ4C_TYPES_H_
#define QAJ4C_TYPES_H_

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Generic value opaque data type holding the JSON DOM.
 */
struct QAJ4C_Value;

typedef struct QAJ4C_Value QAJ4C_Value;

/**
 * Opaque data type that can be used to iterate over
 * members within a json object.
 */
struct QAJ4C_Member;

typedef struct QAJ4C_Member QAJ4C_Member;

/**
 * This method will get called in case of a fatal error
 * (array access on not array QAJ4C_Value).
 */
typedef void (*QAJ4C_fatal_error_fn)( void );

/**
 * Enumeration that represents all data types known to json.
 */
typedef enum QAJ4C_TYPE {
    QAJ4C_TYPE_NULL = 0, /*!< Type for NULL-pointer and json null */
    QAJ4C_TYPE_OBJECT,   /*!< Type for json objects */
    QAJ4C_TYPE_ARRAY,    /*!< Type for json arrays */
    QAJ4C_TYPE_STRING,   /*!< Type for json strings */
    QAJ4C_TYPE_NUMBER,   /*!< Type for doubles, signed and unsigned integers */
    QAJ4C_TYPE_BOOL,     /*!< Type for json booleans */
    QAJ4C_TYPE_INVALID   /*!< Type for non-json compliant type (like parsing errors) */
} QAJ4C_TYPE;

#define QAJ4C_S(string) string, strlen(string)

void QAJ4C_register_fatal_error_function( QAJ4C_fatal_error_fn function );

/**
 * This method will return true, in case the value can be read out as string.
 * @note This method is NULL-safe!
 */
bool QAJ4C_is_string( const QAJ4C_Value* value_ptr );

/**
 * This method will return the starting pointer of the c-string.
 *
 * @note This method should only be invoked on fields that are known to
 * be string. It is usually a good idea to call the QAJ4C_is_string method first!
 * @note With UTF-8 the \0 character may also be placed within the string,
 * thus in case you expect UTF-8 it might be a good idea to also read the
 * QAJ4C_get_string_length method.
 */

const char* QAJ4C_get_string( const QAJ4C_Value* value_ptr );

/**
 * This method will return string length of the string.
 *
 * @note With UTF-8 the \0 character may also be placed within the string.
 * For this reason all string operations are also available with a defined
 * string size.
 */
size_t QAJ4C_get_string_length( const QAJ4C_Value* value_ptr );

/**
 * This method will return true, in case the value can be read out as object.
 * @note This method is NULL-safe!
 */
bool QAJ4C_is_object( const QAJ4C_Value* value_ptr );

/**
 * This method will return true, in case the value can be read out as array.
 * @note This method is NULL-safe!
 */
bool QAJ4C_is_array( const QAJ4C_Value* value_ptr );

/**
 * This method will return true, in case the value can be read out as int32_t.
 * @note This method is NULL-safe!
 */
bool QAJ4C_is_int( const QAJ4C_Value* value_ptr );

/**
 * This method will return the value's int32_t value.
 *
 * @note This method should only be invoked on fields that are known to
 * be int32_t. It is usually a good idea to call the QAJ4C_is_int method first!
 */
int32_t QAJ4C_get_int( const QAJ4C_Value* value_ptr );

/**
 * This method will return true, in case the value can be read out as int64_t.
 * @note This method is NULL-safe!
 */
bool QAJ4C_is_int64( const QAJ4C_Value* value_ptr );

/**
 * This method will return the value's int64_t value.
 *
 * @note This method should only be invoked on fields that are known to
 * be int64_t. It is usually a good idea to call the QAJ4C_is_int64 method first!
 */
int64_t QAJ4C_get_int64( const QAJ4C_Value* value_ptr );

/**
 * This method will return true, in case the value can be read out as uint32_t.
 * @note This method is NULL-safe!
 */
bool QAJ4C_is_uint( const QAJ4C_Value* value_ptr );

/**
 * This method will return the value's uint32_t value.
 *
 * @note This method should only be invoked on fields that are known to
 * be uint32_t. It is usually a good idea to call the QAJ4C_is_uint method first!
 */
uint32_t QAJ4C_get_uint( const QAJ4C_Value* value_ptr );

/**
 * This method will return true, in case the value can be read out as uint64_t.
 * @note This method is NULL-safe!
 */
bool QAJ4C_is_uint64( const QAJ4C_Value* value_ptr );

/**
 * This method will return the value's uint64_t value.
 *
 * @note This method should only be invoked on fields that are known to
 * be int64_t. It is usually a good idea to call the QAJ4C_is_uint64 method first!
 */
uint64_t QAJ4C_get_uint64( const QAJ4C_Value* value_ptr );

/**
 * This method will return true, in case the value can be read out as double.
 *
 * @note this will return true for all parsed numeric values.
 * @note This method is NULL-safe!
 */
bool QAJ4C_is_double( const QAJ4C_Value* value_ptr );

/**
 * This method will return the value's double value.
 *
 * @note This method should only be invoked on fields that are known to
 * be double. It is usually a good idea to call the QAJ4C_is_double method first!
 */
double QAJ4C_get_double( const QAJ4C_Value* value_ptr );

/**
 * This method will return true, in case the type can be read out as boolean.
 * @note This method is NULL-safe!
 */
bool QAJ4C_is_bool( const QAJ4C_Value* value_ptr );

/**
 * This method will read the bool value of the 'value'.
 *
 * @note This method should only be invoked on fields that are known to
 * be boolean. It is usually a good idea to call the QAJ4C_is_bool method first!
 */
bool QAJ4C_get_bool( const QAJ4C_Value* value_ptr );

/**
 * This method will return true, in case the value pointer is not set (thus the NULL-pointer).
 * @note This method is NULL-safe!
 */
bool QAJ4C_is_not_set( const QAJ4C_Value* value_ptr );

/**
 * This method will return true, in case the value pointer is null
 * but also in case the value's type is null.
 * @note This method is NULL-safe!
 */
bool QAJ4C_is_null( const QAJ4C_Value* value_ptr );

/**
 * This method will return the type of the "value".
 *
 * @note if value_ptr is NULL the method will return QAJ4C_TYPE_NULL.
 */
QAJ4C_TYPE QAJ4C_get_type( const QAJ4C_Value* value_ptr );

/**
 * This method will return position within the json message where the
 * parse error was triggered.
 *
 * @note This method will fail in case the value is not an error!
 */
size_t QAJ4C_error_get_json_pos( const QAJ4C_Value* value_ptr );


/**
 * In case the value is an object, this method will retrieve the object member count.
 */
size_t QAJ4C_object_size( const QAJ4C_Value* value_ptr );

/**
 * In case the value is an object, this method will get the member (containing of key and value).
 * with the given index.
 *
 * @note the object is internally organized as c-array. This way random access on index is
 * cheap.
 */
const QAJ4C_Member* QAJ4C_object_get_member( const QAJ4C_Value* value_ptr, size_t index );

/**
 * This method will return the key's value of the member.
 */
const QAJ4C_Value* QAJ4C_member_get_key( const QAJ4C_Member* member );

/**
 * This method will return the value's value of the member.
 */
const QAJ4C_Value* QAJ4C_member_get_value( const QAJ4C_Member* member );

/**
 * In case the value is an object this method will retrieve a member by name with the
 * given size and return the value of the member.
 */
const QAJ4C_Value* QAJ4C_object_get( const QAJ4C_Value* value_ptr, const char* str, size_t len );

/**
 * In case the value is an array, this method will return the array size.
 */
size_t QAJ4C_array_size( const QAJ4C_Value* value_ptr );

/**
 * In case the value is an array, this method will return the value at the given
 * index of this array.
 *
 * @note a QAJ4C_array is organized as a c-array internally, thus random access
 * is possible with low cost.
 */
const QAJ4C_Value* QAJ4C_array_get( const QAJ4C_Value* value_ptr, size_t index );


#ifdef __cplusplus
}
#endif

#endif /* QAJ4C_TYPES_H_ */

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

#ifndef QSJ4C_H_
#define QSJ4C_H_

#ifndef NO_STDLIB

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Generic value opaque data type holding the JSON DOM.
 */
struct QSJ4C_Value;

typedef struct QSJ4C_Value QSJ4C_Value;

/**
 * Opaque data type that can be used to iterate over
 * members within a json object.
 */
struct QSJ4C_Member;

typedef struct QSJ4C_Member QSJ4C_Member;

struct QSJ4C_Builder {
    uint8_t* buffer;
    size_t buffer_size;

    size_t cur_str_pos;
    size_t cur_obj_pos;
};
typedef struct QSJ4C_Builder QSJ4C_Builder;

/**
 * This method will get called in case of a fatal error
 * (array access on not array QSJ4C_Value).
 */
typedef void (*QSJ4C_fatal_error_fn)( void );

/**
 * This type defines a realloc like method that can be handed over with the according parse
 * function.
 */
typedef void* (*QSJ4C_realloc_fn)( void *ptr, size_t size );

/**
 * Error codes that can be expected from the qa json parser.
 */
typedef enum QSJ4C_ERROR_CODE {
    QSJ4C_ERROR_INVALID_ERROR_CODE = 0,       /*!<  This error code indicates a fatal fault within the library! */
    QSJ4C_ERROR_NO_ERROR = 1,                 /*!<  Flag that determines that no error occurred! */
    QSJ4C_ERROR_DEPTH_OVERFLOW = 2,           /*!<  The amount of nested elements exceed the limit */
    QSJ4C_ERROR_UNEXPECTED_CHAR = 3,          /*!<  Unexpected char was processed */
    QSJ4C_ERROR_BUFFER_TRUNCATED = 4,         /*!<  Json message was incomplete (depcrecated) */
    QSJ4C_ERROR_JSON_MESSAGE_TRUNCATED = 4,   /*!<  Json message was incomplete */
    QSJ4C_ERROR_INVALID_STRING_START = 5,     /*!<  String value was expected but did not start with '"' */
    QSJ4C_ERROR_INVALID_NUMBER_FORMAT = 6,    /*!<  Numeric values was expected but had invalid format */
    QSJ4C_ERROR_UNEXPECTED_JSON_APPENDIX = 7, /*!<  Json message did not stop at the end of the buffer */
    QSJ4C_ERROR_MISSING_COMMA = 8,            /*!<  Array elements were not separated by comma */
    QSJ4C_ERROR_MISSING_COLON = 9,            /*!<  Object entry misses ':' after key declaration */
    QSJ4C_ERROR_FATAL_PARSER_ERROR = 10,      /*!<  A fatal error occurred (no other classification possible) */
    QSJ4C_ERROR_STORAGE_BUFFER_TO_SMALL = 11, /*!<  DOM storage buffer is too small to store DOM. */
    QSJ4C_ERROR_ALLOCATION_ERROR = 12,        /*!<  Realloc failed (parse_dynamic only). */
    QSJ4C_ERROR_TRAILING_COMMA = 13,          /*!<  Trailing comma is detected in an object/array detected (strict parsing only)*/
    QSJ4C_ERROR_INVALID_ESCAPE_SEQUENCE = 14, /*!<  String escaped character is invalid. (e.g. \x) */
    QSJ4C_ERROR_INVALID_UNICODE_SEQUENCE = 15 /*!<  The unicode sequence cannot be translated to a valid UTF-8 character */

} QSJ4C_ERROR_CODE;

/**
 * Enumeration that represents all data types known to json.
 */
typedef enum QSJ4C_TYPE {
    QSJ4C_TYPE_NULL = 0, /*!< Type for NULL-pointer and json null */
    QSJ4C_TYPE_OBJECT,   /*!< Type for json objects */
    QSJ4C_TYPE_ARRAY,    /*!< Type for json arrays */
    QSJ4C_TYPE_STRING,   /*!< Type for json strings */
    QSJ4C_TYPE_NUMBER,   /*!< Type for floats, signed and unsigned integers */
    QSJ4C_TYPE_BOOL,     /*!< Type for json booleans */
    QSJ4C_TYPE_INVALID   /*!< Type for non-json compliant type (like parsing errors) */
} QSJ4C_TYPE;

/**
 * Enumeration that holds all parsing options.
 */
typedef enum QSJ4C_PARSE_OPTS {
    /* enum value 1 is reserved! */
    QSJ4C_PARSE_OPTS_STRICT = 2, /*!< Enables the strict mode. */
    QSJ4C_PARSE_OPTS_DONT_SORT_OBJECT_MEMBERS = 4 /*!< Disables sorting objects for faster value by key access. */
} QSJ4C_PARSE_OPTS;

/**
 * With this method a fatal error handler can be registered to have a custom
 * way of handling invalid access behavior (like integer access on a string).
 *
 * The default behavior is print the an error message to stderr.
 */
void QSJ4C_register_fatal_error_function( QSJ4C_fatal_error_fn function );

/**
 * This method will walk through the json message (with a given size) and analyze what buffer
 * size would be required to store the complete DOM.
 */
size_t QSJ4C_calculate_max_buffer_size_n( const char* json, size_t n );

/**
 * This method will walk through the json message and analyze what buffer size would be required
 * to store the complete DOM.
 */
size_t QSJ4C_calculate_max_buffer_size( const char* json );

/**
 * This method will parse the json message and will use the handed over buffer to store the DOM
 * and the strings.
 *
 * In case the parse fails the document's root value will contain an error value. Usually, a
 * object is expected so you should in each case check the document's root value with QSJ4C_is_object.
 *
 * @return the amount of data written to the buffer
 */
size_t QSJ4C_parse( const char* json, void* buffer, size_t buffer_size, QSJ4C_Value** result_ptr );

/**
 * This method will parse the json message without a handed over buffer but with a realloc
 * callback method. The realloc method will called each time allocated buffer is insufficient.
 *
 * @returns NULL, in case no memory could ever get allocated at all. In all other cases
 * a valid instance (that may contain an error instead of parsed content).
 */
QSJ4C_Value* QSJ4C_parse_dynamic( const char* json, QSJ4C_realloc_fn realloc_callback );

/**
 * This method will parse the json message and will use the handed over buffer to store the DOM
 * and the strings. Additionally the method will modify the buffer_size to the actual amount written
 * to the buffer and will accept parsing options.
 *
 * @note In case the json string is null terminated, json_len can also be set to SIZE_MAX (or -1).
 *
 * @return the amount of data written to the buffer
 */
size_t QSJ4C_parse_opt( const char* json, size_t json_len, int opts, void* buffer, size_t buffer_size, QSJ4C_Value** result_ptr );

/**
 * This method will parse the json message  without a handed over buffer but with a realloc
 * callback method. The realloc method will called each time allocated buffer is insufficient.
 * Additionally will accept parsing options.
 *
 * @note In case the json string is null terminated, json_len can also be set to SIZE_MAX (or -1).
 */
QSJ4C_Value* QSJ4C_parse_opt_dynamic( const char* json, size_t json_len, int opts, QSJ4C_realloc_fn realloc_callback );

/**
 * This method prints the DOM as JSON in the handed over buffer.
 *
 * @return the amount of data written to the buffer, including the '\0' character.
 */
size_t QSJ4C_sprint( const QSJ4C_Value* value_ptr, char* buffer, size_t buffer_size );

/**
 * This method will return true, in case the value can be read out as string.
 * @note This method is NULL-safe!
 */
bool QSJ4C_is_string( const QSJ4C_Value* value_ptr );

/**
 * This method will return the starting pointer of the c-string.
 *
 * @note This method should only be invoked on fields that are known to
 * be string. It is usually a good idea to call the QSJ4C_is_string method first!
 * @note With UTF-8 the \0 character may also be placed within the string,
 * thus in case you expect UTF-8 it might be a good idea to also read the
 * QSJ4C_get_string_length method.
 */

const char* QSJ4C_get_string( const QSJ4C_Value* value_ptr );

/**
 * This method will return string length of the string.
 *
 * @note With UTF-8 the \0 character may also be placed within the string.
 * For this reason all string operations are also available with a defined
 * string size.
 */
size_t QSJ4C_get_string_length( const QSJ4C_Value* value_ptr );

/**
 * This method will compare the value's string value with the handed over string
 * with the given length.
 *
 * @note: The length of the string will be compared first ... so the results will
 * most likely differ to the c library strcmp.
 */
int QSJ4C_string_cmp_n( const QSJ4C_Value* value_ptr, const char* str, size_t len );

/**
 * This method will compare the value's string value with the handed over string
 * using strlen to determine the strings size.
 *
 * @note: The length of the string will be compared first ... so the results will
 * most likely differ to the c library strcmp.
 */
bool QSJ4C_string_cmp( const QSJ4C_Value* value_ptr, const char* str );

/**
 * This method will return true, in case the handed over string with the given size
 * is equal to the value's string.
 */
bool QSJ4C_string_equals_n( const QSJ4C_Value* value_ptr, const char* str, size_t len );

/**
 * This method will return true, in case the handed over string is equal to the
 * value's string. The string's length is determined using strlen.
 */
bool QSJ4C_string_equals( const QSJ4C_Value* value_ptr, const char* str );

/**
 * This method will return true, in case the value can be read out as object.
 * @note This method is NULL-safe!
 */
bool QSJ4C_is_object( const QSJ4C_Value* value_ptr );

/**
 * This method will return true, in case the value can be read out as array.
 * @note This method is NULL-safe!
 */
bool QSJ4C_is_array( const QSJ4C_Value* value_ptr );

/**
 * This method will return true, in case the value can be read out as int32_t.
 * @note This method is NULL-safe!
 */
bool QSJ4C_is_int( const QSJ4C_Value* value_ptr );

/**
 * This method will return the value's int32_t value.
 *
 * @note This method should only be invoked on fields that are known to
 * be int32_t. It is usually a good idea to call the QSJ4C_is_int method first!
 */
int32_t QSJ4C_get_int( const QSJ4C_Value* value_ptr );

/**
 * This method will return true, in case the value can be read out as uint32_t.
 * @note This method is NULL-safe!
 */
bool QSJ4C_is_uint( const QSJ4C_Value* value_ptr );

/**
 * This method will return the value's uint32_t value.
 *
 * @note This method should only be invoked on fields that are known to
 * be uint32_t. It is usually a good idea to call the QSJ4C_is_uint method first!
 */
uint32_t QSJ4C_get_uint( const QSJ4C_Value* value_ptr );

/**
 * This method will return true, in case the value can be read out as float.
 *
 * @note this will return true for all parsed numeric values.
 * @note This method is NULL-safe!
 */
bool QSJ4C_is_float( const QSJ4C_Value* value_ptr );

/**
 * This method will return the value's float value.
 *
 * @note This method should only be invoked on fields that are known to
 * be float. It is usually a good idea to call the QSJ4C_is_float method first!
 */
float QSJ4C_get_float( const QSJ4C_Value* value_ptr );

/**
 * This method will return true, in case the type can be read out as boolean.
 * @note This method is NULL-safe!
 */
bool QSJ4C_is_bool( const QSJ4C_Value* value_ptr );

/**
 * This method will read the bool value of the 'value'.
 *
 * @note This method should only be invoked on fields that are known to
 * be boolean. It is usually a good idea to call the QSJ4C_is_bool method first!
 */
bool QSJ4C_get_bool( const QSJ4C_Value* value_ptr );

/**
 * This method will return true, in case the value pointer is not set (thus the NULL-pointer).
 * @note This method is NULL-safe!
 */
bool QSJ4C_is_not_set( const QSJ4C_Value* value_ptr);

/**
 * This method will return true, in case the value pointer is null
 * but also in case the value's type is null.
 * @note This method is NULL-safe!
 */
bool QSJ4C_is_null( const QSJ4C_Value* value_ptr );

/**
 * This method will return the type of the "value".
 *
 * @note if value_ptr is NULL the method will return QSJ4C_TYPE_NULL.
 */
QSJ4C_TYPE QSJ4C_get_type( const QSJ4C_Value* value_ptr );

/**
 * Checks if the value is an error (not a valid json type)
 *
 * @note In case the parse method will fail and the buffer size is sufficient.
 * The root value will contain an error value, that can be 'queried' for
 * detailed error information.
 * @note This method is NULL-safe!
 */
bool QSJ4C_is_error( const QSJ4C_Value* value_ptr );

/**
 * This method will return an error code specifying the reason why
 * the parse method failed.
 *
 * @note This method will fail in case the value is not an error!
 */
QSJ4C_ERROR_CODE QSJ4C_error_get_errno( const QSJ4C_Value* value_ptr );

/**
 * This method will return position within the json message where the
 * parse error was triggered.
 *
 * @note This method will fail in case the value is not an error!
 */
size_t QSJ4C_error_get_json_pos( const QSJ4C_Value* value_ptr );


/**
 * In case the value is an object, this method will retrieve the object member count.
 */
size_t QSJ4C_object_size( const QSJ4C_Value* value_ptr );

/**
 * In case the value is an object, this method will get the member (containing of key and value).
 * with the given index.
 *
 * @note the object is internally organized as c-array. This way random access on index is
 * cheap.
 */
QSJ4C_Member* QSJ4C_object_get_member( QSJ4C_Value* value_ptr, size_t index );

/**
 * This method will return the key's value of the member.
 */
QSJ4C_Value* QSJ4C_member_get_key( QSJ4C_Member* member );

/**
 * This method will return the value's value of the member.
 */
QSJ4C_Value* QSJ4C_member_get_value( QSJ4C_Member* member );

/**
 * In case the value is an object this method will retrieve a member by name with the
 * given size and return the value of the member.
 */
QSJ4C_Value* QSJ4C_object_get_n( QSJ4C_Value* value_ptr, const char* str, size_t len );

/**
 * In case the value is an object this method will retrieve a member by name (using strlen
 * to determine the size) and return the value of the member.
 */
QSJ4C_Value* QSJ4C_object_get( QSJ4C_Value* value_ptr, const char* str );

/**
 * In case the value is an array, this method will return the array size.
 */
size_t QSJ4C_array_size( const QSJ4C_Value* value_ptr );

/**
 * In case the value is an array, this method will return the value at the given
 * index of this array.
 *
 * @note a QSJ4C_array is organized as a c-array internally, thus random access
 * is possible with low cost.
 */
QSJ4C_Value* QSJ4C_array_get( QSJ4C_Value* value_ptr, size_t index );


#ifdef __cplusplus
}
#endif

#endif /* QSJ4C_H_ */

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

#ifndef QAJ4C_H_
#define QAJ4C_H_

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
struct QAJ4C_Value;

typedef struct QAJ4C_Value QAJ4C_Value;

/**
 * Opaque data type that can be used to iterate over
 * members within a json object.
 */
struct QAJ4C_Member;

typedef struct QAJ4C_Member QAJ4C_Member;

struct QAJ4C_Builder {
    uint8_t* buffer;
    size_t buffer_size;

    size_t cur_str_pos;
    size_t cur_obj_pos;
};
typedef struct QAJ4C_Builder QAJ4C_Builder;

/**
 * This method will get called in case of a fatal error
 * (array access on not array QAJ4C_Value).
 */
typedef void (*QAJ4C_fatal_error_fn)( const char* function_name, const char* assertion_msg );

/**
 * This type defines a realloc like method that can be handed over with the according parse
 * function.
 */
typedef void* (*QAJ4C_realloc_fn)( void *ptr, size_t size );

/**
 * Error codes that can be expected from the qa json parser.
 */
typedef enum QAJ4C_ERROR_CODE {
	QAJ4C_ERROR_INVALID_ERROR_CODE = 0,
	QAJ4C_ERROR_NO_ERROR = 1,
    QAJ4C_ERROR_DEPTH_OVERFLOW = 2,
    QAJ4C_ERROR_UNEXPECTED_CHAR = 3,
    QAJ4C_ERROR_BUFFER_TRUNCATED = 4,
    QAJ4C_ERROR_INVALID_STRING_START = 5,
    QAJ4C_ERROR_INVALID_NUMBER_FORMAT = 6,
    QAJ4C_ERROR_UNEXPECTED_JSON_APPENDIX = 7,
    QAJ4C_ERROR_MISSING_COMMA = 8,
    QAJ4C_ERROR_MISSING_COLON = 9,
    QAJ4C_ERROR_FATAL_PARSER_ERROR = 10,
    QAJ4C_ERROR_STORAGE_BUFFER_TO_SMALL = 11,
    QAJ4C_ERROR_ALLOCATION_ERROR = 12,
	QAJ4C_ERROR_TRAILING_COMMA = 13,
	QAJ4C_ERROR_INVALID_ESCAPE_SEQUENCE = 14

} QAJ4C_ERROR_CODE;

/**
 * Enumeration that represents all data types known to json.
 */
typedef enum QAJ4C_TYPE {
    QAJ4C_TYPE_NULL = 0,
    QAJ4C_TYPE_OBJECT,
    QAJ4C_TYPE_ARRAY,
    QAJ4C_TYPE_STRING,
    QAJ4C_TYPE_NUMBER,
    QAJ4C_TYPE_BOOL,
	QAJ4C_TYPE_INVALID
} QAJ4C_TYPE;

/**
 * Enumeration that holds all parsing options.
 */
typedef enum QAJ4C_PARSE_OPTS {
	/* enum value 1 is reserved! */
	QAJ4C_PARSE_OPTS_STRICT = 2
} QAJ4C_PARSE_OPTS;

/**
 * With this method a fatal error handler can be registered to have a custom
 * way of handling invalid access behavior (like integer access on a string).
 *
 * The default behavior is print the an error message to stderr.
 */
void QAJ4C_register_fatal_error_function( QAJ4C_fatal_error_fn function );

/**
 * This method will walk through the json message (with a given size) and analyze what buffer
 * size would be required to store the complete DOM.
 */
size_t QAJ4C_calculate_max_buffer_size_n( const char* json, size_t n );

/**
 * This method will walk through the json message and analyze what buffer size would be required
 * to store the complete DOM.
 */
size_t QAJ4C_calculate_max_buffer_size( const char* json );

/**
 * This method will walk through the json message (with a given size and analyze the maximum
 * required buffer size that would be required to store the DOM (in case the strings do not
 * have to be copied into the buffer)
 */
size_t QAJ4C_calculate_max_buffer_size_insitu_n( const char* json, size_t n );

/**
 * This method will walk through the json message and analyze the maximum required buffer size that
 * would be required to store the DOM (in case the strings do not have to be copied into the buffer)
 */
size_t QAJ4C_calculate_max_buffer_size_insitu( const char* json );

/**
 * This method will parse the json message and will use the handed over buffer to store the DOM
 * and the strings.
 *
 * In case the parse fails the document's root value will contain an error value. Usually, a
 * object is expected so you should in each case check the document's root value with QAJ4C_is_object.
 */
const QAJ4C_Value* QAJ4C_parse( const char* json, void* buffer, size_t buffer_size );

/**
 * This method will parse the json message without a handed over buffer but with a realloc
 * callback method. The realloc method will called each time allocated buffer is insufficient.
 *
 * @returns NULL, in case no memory could ever get allocated at all. In all other cases
 * a valid instance (that may contain an error instead of parsed content).
 */
const QAJ4C_Value* QAJ4C_parse_dynamic( const char* json, QAJ4C_realloc_fn realloc_callback );

/**
 * This method will parse the json message and will use the handed over buffer to store the DOM.
 * The strings will not be copied within the buffer and will be referenced to the json message.
 * Just like strtok, the json message will be adjusted in place.
 *
 * In case the parse fails the document's root value will contain an error value. Usually, a
 * object is expected so you should in each case check the document's root value with QAJ4C_is_object.
 */
const QAJ4C_Value* QAJ4C_parse_insitu( char* json, void* buffer, size_t buffer_size );

/**
 * This method will parse the json message and will use the handed over buffer to store the DOM
 * and the strings. Additionally the method will modify the buffer_size to the actual amount written
 * to the buffer and will accept parsing options.
 *
 * @note In case the json_len is set to 0, the size of the message is autodetected.
 */
const QAJ4C_Value* QAJ4C_parse_opt( const char* json, size_t json_len, int opts, void* buffer, size_t* buffer_size );

/**
 * This method will parse the json message  without a handed over buffer but with a realloc
 * callback method. The realloc method will called each time allocated buffer is insufficient.
 * Additionally will accept parsing options.
 *
 * @note In case the json_len is set to 0, the size of the message is autodetected.
 */
const QAJ4C_Value* QAJ4C_parse_opt_dynamic( const char* json, size_t json_len, int opts, QAJ4C_realloc_fn realloc_callback );

/**
 * This method will parse the json message and will use the handed over buffer to store the DOM.
 * The strings will not be copied within the buffer and will be referenced to the json message.
 * Just like strtok, the json message will be adjusted in place. Additionally the method will modify
 * the buffer_size to the actual amount written to the buffer and will accept parsing options.
 *
 * In case the parse fails the document's root value will contain an error value. Usually, a
 * object is expected so you should in each case check the document's root value with QAJ4C_is_object.
 *
 * @note In case the json_len is set to 0, the size of the message is autodetected.
 */
const QAJ4C_Value* QAJ4C_parse_opt_insitu( char* json, size_t json_len, int opts, void* buffer, size_t* buffer_size );

/**
 * This method prints the DOM as JSON in the handed over buffer.
 */
size_t QAJ4C_sprint( const QAJ4C_Value* document, char* buffer, size_t buffer_size );

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
 * This method will compare the value's string value with the handed over string
 * with the given length.
 *
 * @note: The length of the string will be compared first ... so the results will
 * most likely differ to the c library strcmp.
 */
int QAJ4C_string_cmp_n( const QAJ4C_Value* value_ptr, const char* str, size_t len );

/**
 * This method will compare the value's string value with the handed over string
 * using strlen to determine the strings size.
 *
 * @note: The length of the string will be compared first ... so the results will
 * most likely differ to the c library strcmp.
 */
bool QAJ4C_string_cmp( const QAJ4C_Value* value_ptr, const char* str );

/**
 * This method will return true, in case the handed over string with the given size
 * is equal to the value's string.
 */
bool QAJ4C_string_equals_n( const QAJ4C_Value* value_ptr, const char* str, size_t len );

/**
 * This method will return true, in case the handed over string is equal to the
 * value's string. The string's length is determined using strlen.
 */
bool QAJ4C_string_equals( const QAJ4C_Value* value_ptr, const char* str );

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
bool QAJ4C_is_not_set( const QAJ4C_Value* value_ptr);

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
 * Checks if the value is an error (not a valid json type)
 *
 * @note In case the parse method will fail and the buffer size is sufficient.
 * The root value will contain an error value, that can be 'queried' for
 * detailed error information.
 * @note This method is NULL-safe!
 */
bool QAJ4C_is_error( const QAJ4C_Value* value_ptr );

/**
 * This method will return the json string that caused the error.
 *
 * @note This method will fail in case the value is not an error!
 */
const char* QAJ4C_error_get_json( const QAJ4C_Value* value_ptr );

/**
 * This method will return an error code specifying the reason why
 * the parse method failed.
 *
 * @note This method will fail in case the value is not an error!
 */
QAJ4C_ERROR_CODE QAJ4C_error_get_errno( const QAJ4C_Value* value_ptr );

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
const QAJ4C_Value* QAJ4C_object_get_n( const QAJ4C_Value* value_ptr, const char* str, size_t len );

/**
 * In case the value is an object this method will retrieve a member by name (using strlen
 * to determine the size) and return the value of the member.
 */
const QAJ4C_Value* QAJ4C_object_get( const QAJ4C_Value* value_ptr, const char* str );

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

/**
 * Initializes the builder with the given buffer.
 */
void QAJ4C_builder_init( QAJ4C_Builder* me, void* buff, size_t buff_size );

/**
 * This method will retrieve the document from the builder.
 * Returns NULL in case the buffer has insufficient size.
 */
QAJ4C_Value* QAJ4C_builder_get_document( QAJ4C_Builder* builder );

/**
 * This method will set the value to a boolean with the handed over value.
 */
void QAJ4C_set_bool( QAJ4C_Value* value_ptr, bool value );

/**
 * This method will set the value to a int32_t with the handed over value.
 */
void QAJ4C_set_int( QAJ4C_Value* value_ptr, int32_t value );

/**
 * This method will set the value to a int64_t with the handed over value.
 */
void QAJ4C_set_int64( QAJ4C_Value* value_ptr, int64_t value );

/**
 * This method will set the value to a uint32_t with the handed over value.
 */
void QAJ4C_set_uint( QAJ4C_Value* value_ptr, uint32_t value );

/**
 * This method will set the value to a uint64_t with the handed over value.
 */
void QAJ4C_set_uint64( QAJ4C_Value* value_ptr, uint64_t value );

/**
 * This method will set the value to a double with the handed over value.
 */
void QAJ4C_set_double( QAJ4C_Value* value_ptr, double value );

/**
 * This method will set the value to the JSON null type.
 */
void QAJ4C_set_null( QAJ4C_Value* value_ptr );

/**
 * This method will set the value to string with a pointer to the handed over
 * string with the given size.
 *
 * @note As the string is a reference, the lifetime of the string must at least
 * be as long as the lifetime of the DOM object.
 */
void QAJ4C_set_string_ref_n( QAJ4C_Value* value_ptr, const char* str, size_t len );

/**
 * This method will set the value to string with a pointer to the handed over
 * string using strlen() to determine the string size.
 *
 * @note As the string is a reference, the lifetime of the string must at least
 * be as long as the lifetime of the DOM object.
 */
void QAJ4C_set_string_ref( QAJ4C_Value* value_ptr, const char* str );

/**
 * This method will copy the handed over string with the given string size
 * using the builder.
 */
void QAJ4C_set_string_copy_n( QAJ4C_Value* value_ptr, const char* str, size_t len, QAJ4C_Builder* builder );

/**
 * This method will copy the handed over string using the builder. The string
 * size is determined by using strlen.
 */
void QAJ4C_set_string_copy( QAJ4C_Value* value_ptr, const char* str, QAJ4C_Builder* builder );

/**
 * This method will set the values type to array with the given value count. The
 * memory allocation will be performed on the builder.
 *
 * @note objects and arrays cannot be resized. Thus in case invoking this function twice
 * on the same value will cause the memory allocated for the old "members" to be wasted!
 */
void QAJ4C_set_array( QAJ4C_Value* value_ptr, size_t count, QAJ4C_Builder* builder );

/**
 * Will retrieve the entry of the array at the given index.
 *
 * @note a QAJ4C_array is organized as a c-array internally, thus random access
 * is possible with low cost.
 */
QAJ4C_Value* QAJ4C_array_get_rw( QAJ4C_Value* value_ptr, size_t index );

/**
 * This method will set the values type to object with the given member count. The
 * memory allocation will be performed on the builder.
 *
 * @note objects and arrays cannot be resized. Thus in case invoking this function twice
 * on the same value will cause the memory allocated for the old "members" to be wasted!
 */
void QAJ4C_set_object( QAJ4C_Value* value_ptr, size_t count, QAJ4C_Builder* builder );

/**
 * This method will optimize the current content on of the object (for faster DOM access).
 * Adding new members will require to call optimize again.
 *
 * @note invoking optimize on an already optimized object will leave the object untouched.
 * @note the behavior is undefined in case keys are present multiple times.
 */
void QAJ4C_object_optimize( QAJ4C_Value* value_ptr );

/**
 * This method creates a member within the object using the reference of the handed over string.
 * This way the string does not have to be copied over to the buffer, but the string has to stay
 * valid until the DOM is not required anymore.
 */
QAJ4C_Value* QAJ4C_object_create_member_by_ref_n( QAJ4C_Value* value_ptr, const char* str, size_t len );

/**
 * This method creates a member within the object using the reference of the handed over string.
 * This way the string does not have to be copied over to the buffer, but the string has to stay
 * valid until the DOM is not required anymore.
 *
 * @note This is a shortcut version of the QAJ4C_object_create_member_by_ref2 method, using
 * strlen to calculate the string length.
 */
QAJ4C_Value* QAJ4C_object_create_member_by_ref( QAJ4C_Value* value_ptr, const char* str );

/**
 * This method creates a member within the object doing a copy of the handed over string.
 * The allocation will be performed on the handed over builder.
 */
QAJ4C_Value* QAJ4C_object_create_member_by_copy_n( QAJ4C_Value* value_ptr, const char* str, size_t len, QAJ4C_Builder* builder );

/**
 * This method creates a member within the object doing a copy of the handed over string.
 * The allocation will be performed on the handed over builder.
 *
 * @note This is a shortcut version of the QAJ4C_object_create_member_by_copy2 method, using
 * strlen to calculate the string length.
 */
QAJ4C_Value* QAJ4C_object_create_member_by_copy( QAJ4C_Value* value_ptr, const char* str, QAJ4C_Builder* builder );

/**
 * This method creates a deep copy of the source value to the destination value using the
 * handed over builder.
 */
void QAJ4C_copy( const QAJ4C_Value* src, QAJ4C_Value* dest, QAJ4C_Builder* builder );

/**
 * This method checks if two elements are equal to each other.
 */
bool QAJ4C_equals( const QAJ4C_Value* lhs, const QAJ4C_Value* rhs );

/**
 * This method calculates the size of the QAJ4C_Value DOM.
 *
 * @note QAJ4C_Value cannot be allocated stand alone so in case you intend to extract
 * a value into a new document you need to use QAJ4C_value_sizeof_as_document instead.
 */
size_t QAJ4C_value_sizeof( const QAJ4C_Value* value_ptr );

#ifdef __cplusplus
}
#endif

#endif /* QAJ4C_H_ */

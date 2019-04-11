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

#ifndef QAJ4C_PARSE_H_
#define QAJ4C_PARSE_H_

#include "qajson4c_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * This type defines a realloc like method that can be handed over with the according parse
 * function.
 */
typedef void* (*QAJ4C_realloc_fn)( void *ptr, size_t size );

/**
 * Error codes that can be expected from the qa json parser.
 */
typedef enum QAJ4C_ERROR_CODE {
    QAJ4C_ERROR_INVALID_ERROR_CODE = 0,       /*!<  This error code indicates a fatal fault within the library! */
    QAJ4C_ERROR_NO_ERROR = 1,                 /*!<  Flag that determines that no error occurred! */
    QAJ4C_ERROR_DEPTH_OVERFLOW = 2,           /*!<  The amount of nested elements exceed the limit */
    QAJ4C_ERROR_UNEXPECTED_CHAR = 3,          /*!<  Unexpected char was processed */
    QAJ4C_ERROR_BUFFER_TRUNCATED = 4,         /*!<  Json message was incomplete (depcrecated) */
    QAJ4C_ERROR_JSON_MESSAGE_TRUNCATED = 4,   /*!<  Json message was incomplete */
    QAJ4C_ERROR_INVALID_STRING_START = 5,     /*!<  String value was expected but did not start with '"' */
    QAJ4C_ERROR_INVALID_NUMBER_FORMAT = 6,    /*!<  Numeric values was expected but had invalid format */
    QAJ4C_ERROR_UNEXPECTED_JSON_APPENDIX = 7, /*!<  Json message did not stop at the end of the buffer */
    QAJ4C_ERROR_MISSING_COMMA = 8,            /*!<  Array elements were not separated by comma */
    QAJ4C_ERROR_MISSING_COLON = 9,            /*!<  Object entry misses ':' after key declaration */
    QAJ4C_ERROR_FATAL_PARSER_ERROR = 10,      /*!<  A fatal error occurred (no other classification possible) */
    QAJ4C_ERROR_STORAGE_BUFFER_TO_SMALL = 11, /*!<  DOM storage buffer is too small to store DOM. */
    QAJ4C_ERROR_ALLOCATION_ERROR = 12,        /*!<  Realloc failed (parse_dynamic only). */
    QAJ4C_ERROR_TRAILING_COMMA = 13,          /*!<  Trailing comma is detected in an object/array detected (strict parsing only)*/
    QAJ4C_ERROR_INVALID_ESCAPE_SEQUENCE = 14, /*!<  String escaped character is invalid. (e.g. \x) */
    QAJ4C_ERROR_INVALID_UNICODE_SEQUENCE = 15 /*!<  The unicode sequence cannot be translated to a valid UTF-8 character */

} QAJ4C_ERROR_CODE;

/**
 * Enumeration that holds all parsing options.
 */
typedef enum QAJ4C_PARSE_OPTS {
    /* enum value 1 is reserved! */
    QAJ4C_PARSE_OPTS_STRICT = 2, /*!< Enables the strict mode. */
    QAJ4C_PARSE_OPTS_DONT_SORT_OBJECT_MEMBERS = 4 /*!< Disables sorting objects for faster value by key access. */
} QAJ4C_PARSE_OPTS;

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
 *
 * @return the amount of data written to the buffer
 */
size_t QAJ4C_parse( const char* json, void* buffer, size_t buffer_size, const QAJ4C_Value** result_ptr );

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
 *
 * @return the amount of data written to the buffer
 */
size_t QAJ4C_parse_insitu( char* json, void* buffer, size_t buffer_size, const QAJ4C_Value** result_ptr );

/**
 * This method will parse the json message and will use the handed over buffer to store the DOM
 * and the strings. Additionally the method will modify the buffer_size to the actual amount written
 * to the buffer and will accept parsing options.
 *
 * @note In case the json string is null terminated, json_len can also be set to SIZE_MAX (or -1).
 *
 * @return the amount of data written to the buffer
 */
size_t QAJ4C_parse_opt( const char* json, size_t json_len, int opts, void* buffer, size_t buffer_size, const QAJ4C_Value** result_ptr );

/**
 * This method will parse the json message  without a handed over buffer but with a realloc
 * callback method. The realloc method will called each time allocated buffer is insufficient.
 * Additionally will accept parsing options.
 *
 * @note In case the json string is null terminated, json_len can also be set to SIZE_MAX (or -1).
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
 * @note In case the json string is null terminated, json_len can also be set to SIZE_MAX (or -1).
 *
 * @return the amount of data written to the buffer
 */
size_t QAJ4C_parse_opt_insitu( char* json, size_t json_len, int opts, void* buffer, size_t buffer_size, const QAJ4C_Value** result_ptr );

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

#ifdef __cplusplus
}
#endif

#endif /* QAJ4C_PARSE_H_ */

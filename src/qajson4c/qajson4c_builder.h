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

#ifndef QAJ4C_BUILDER_H_
#define QAJ4C_BUILDER_H_

#include "qajson4c_types.h"

#ifdef __cplusplus
extern "C" {
#endif

struct QAJ4C_Builder {
    uint8_t* buffer;
    size_t buffer_size;

    size_t cur_str_pos;
    size_t cur_obj_pos;
};
typedef struct QAJ4C_Builder QAJ4C_Builder;

struct QAJ4C_Object_builder {
    QAJ4C_Value* object;
    bool strict;
    size_t pos;
    size_t count;
};
typedef struct QAJ4C_Object_builder QAJ4C_Object_builder;

/**
 * Creates the builder with the given buffer.
 */
QAJ4C_Builder QAJ4C_builder_create( void* buff, size_t buff_size );

/**
 * Initializes the builder with the given buffer.
 */
void QAJ4C_builder_init( QAJ4C_Builder* me, void* buff, size_t buff_size );

/**
 * Creates the builder with the given buffer.
 */
void QAJ4C_builder_reset( QAJ4C_Builder* me );

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
 * @note This is a shortcut version of the QAJ4C_object_create_member_by_ref_n method, using
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
 * @note This is a shortcut version of the QAJ4C_object_create_member_by_copy_n method, using
 * strlen to calculate the string length.
 */
QAJ4C_Value* QAJ4C_object_create_member_by_copy( QAJ4C_Value* value_ptr, const char* str, QAJ4C_Builder* builder );

/**
 * This method creates a QAJ4C_Object_builder to fill the data of an QAJ4C_Object with a given size.
 *
 * @param value_ptr the value pointer to allocate the object.
 * @param member_count the amount of members
 * @param deduplicate, if set to true, the builder will ensure that no duplicate key strings are used
 *                     and will then return the already present value so the application can overwrite it.
 *                     If set to false, new members are appended without lookup overhead.
 * @return the builder instance.
 */
QAJ4C_Object_builder QAJ4C_object_builder_init( QAJ4C_Value* value_ptr, size_t member_count, bool deduplicate, QAJ4C_Builder* builder );

/**
 * This method creates a member within the object using the reference of the handed over string.
 * This way the string does not have to be copied over to the buffer, but the string has to stay
 * valid until the DOM is not required anymore.
 */
QAJ4C_Value* QAJ4C_object_builder_create_member_by_ref_n( QAJ4C_Object_builder* value_ptr, const char* str, size_t len );

/**
 * This method creates a member within the object using the reference of the handed over string.
 * This way the string does not have to be copied over to the buffer, but the string has to stay
 * valid until the DOM is not required anymore.
 *
 * @note This is a shortcut version of the QAJ4C_object_create_member_by_ref_n method, using
 * strlen to calculate the string length.
 */
QAJ4C_Value* QAJ4C_object_builder_create_member_by_ref( QAJ4C_Object_builder* value_ptr, const char* str );

/**
 * This method creates a member within the object doing a copy of the handed over string.
 * The allocation will be performed on the handed over builder.
 */
QAJ4C_Value* QAJ4C_object_builder_create_member_by_copy_n( QAJ4C_Object_builder* value_ptr, const char* str, size_t len, QAJ4C_Builder* builder );

/**
 * This method creates a member within the object doing a copy of the handed over string.
 * The allocation will be performed on the handed over builder.
 *
 * @note This is a shortcut version of the QAJ4C_object_create_member_by_copy_n method, using
 * strlen to calculate the string length.
 */
QAJ4C_Value* QAJ4C_object_builder_create_member_by_copy( QAJ4C_Object_builder* value_ptr, const char* str, QAJ4C_Builder* builder );

#ifdef __cplusplus
}
#endif

#endif /* QAJ4C_BUILDER_H_ */

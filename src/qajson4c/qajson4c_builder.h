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
    void* buffer;
    size_t buffer_size;

    char* str_ptr;
    QAJ4C_Value* obj_ptr;
};
typedef struct QAJ4C_Builder QAJ4C_Builder;

struct QAJ4C_Object_builder {
    QAJ4C_Member* top;
    size_t index;
    size_t capacity;
};
typedef struct QAJ4C_Object_builder QAJ4C_Object_builder;

struct QAJ4C_Array_builder {
    QAJ4C_Value* top;
    size_t index;
    size_t capacity;
};
typedef struct QAJ4C_Array_builder QAJ4C_Array_builder;

/**
 * Creates the builder with the given buffer.
 */
QAJ4C_Builder QAJ4C_builder_create( void* buff, size_t buff_size );

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
 * string using strlen() to determine the string size.
 *
 * @note As the string is a reference, the lifetime of the string must at least
 * be as long as the lifetime of the DOM object.
 */
void QAJ4C_set_string_ref( QAJ4C_Value* value_ptr, const char* str, size_t len );

/**
 * This method will copy the handed over string using the builder. The string
 * size is determined by using strlen.
 */
void QAJ4C_set_string_copy( QAJ4C_Value* value_ptr, const char* str, size_t len, QAJ4C_Builder* builder );

/**
 * This method creates an array builder with a given (maximum) capacity. It is not possible
 * to exceed the capacity later on. It is possible to append less values than the capacity
 * to the builder. Not used capacity will be skipped when printing.
 */
QAJ4C_Array_builder QAJ4C_array_builder_create( size_t capacity, QAJ4C_Builder* builder );

/**
 * This method will increase the array length and return a pointer to the value or NULL in case
 * the capacity has been exceeded. Fails in case the capacity is exceeded.
 * @param initialize when set to true, the value will be initialized to NULL
 */
QAJ4C_Value* QAJ4C_array_builder_next( QAJ4C_Array_builder* array_builder );

/**
 * Assigns the value_ptr refering to the array_builders content.
 */
void QAJ4C_set_array( QAJ4C_Value* value_ptr, QAJ4C_Array_builder* array_builder );

/**
 * This method creates a QAJ4C_Object_builder to fill the data of an QAJ4C_Object with a given size.
 *
 * @param member_capacity the (maximum) amount of members
 * @return the builder instance for memory allocation.
 */
QAJ4C_Object_builder QAJ4C_object_builder_create( size_t member_capacity, QAJ4C_Builder* builder );

/**
 * This method creates a member using a string reference. This string will not be copied over so the callee
 * has to ensure the string does not go out of scope causing a dangling char*.
 *
 * @note the object builder does not check if a member with the given key is already present. So each create
 *       call will cause a member to be added.
 * @return the pointer to the members value (so it can be assigned).
 */
QAJ4C_Value* QAJ4C_object_builder_create_member_by_ref( QAJ4C_Object_builder* object_builder, const char* str, size_t len );

/**
 * This method creates a member with performing a string copy. The required memory is fetched by the builder, so the builders
 * buffer may not get out of scope or overridden.
 *
 * @note the object builder does not check if a member with the given key is already present. So each create
 *       call will cause a member to be added.
 * @return the pointer to the members value (so it can be assigned).
 */
QAJ4C_Value* QAJ4C_object_builder_create_member_by_copy( QAJ4C_Object_builder* object_builder, const char* str, size_t len, QAJ4C_Builder* builder );

/**
 * Assigns the value_ptr referring to the object_builders content.
 * @param skip_post_processing For printing reasons it is possible to skip post processing.
 *                             Not skipping will sort the members by hash and fail in case
 *                             duplicate keys are available.
 * @attention When skipping post processing QAJ4C_object_get and QAJ4C_equals cannot be
 *            called with this object.
 * @attention With skipping post processing set to false value pointers might not point
 */
void QAJ4C_set_object( QAJ4C_Value* value_ptr, QAJ4C_Object_builder* object_builder, bool skip_post_processing );

/**
 * This method creates a deep copy of the source value to the destination value using the
 * handed over builder.
 */
void QAJ4C_copy( const QAJ4C_Value* src, QAJ4C_Value* dest, QAJ4C_Builder* builder );

#ifdef __cplusplus
}
#endif

#endif /* QAJ4C_BUILDER_H_ */

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

#ifndef QAJSON4C_INTERNAL_H_
#define QAJSON4C_INTERNAL_H_

#include "qajson_stdwrap.h"

/** Define unlikely macro to be used in asserts
 *  Basically because the compiler should compile for the non-failure scenario.
 **/
#if __GNUC__ >= 3
#define QSJ4C_UNLIKELY(expr) __builtin_expect(expr, 0)
#else
#define QSJ4C_UNLIKELY(expr) expr
#endif

#define QSJ4C_ASSERT(arg, alt) if (QSJ4C_UNLIKELY(!(arg))) do { g_QSJ4C_err_function(); alt } while(0)

#define QSJ4C_MIN(lhs, rhs) ((lhs<=rhs)?(lhs):(rhs))
#define QSJ4C_MAX(lhs, rhs) ((lhs>=rhs)?(lhs):(rhs))

#ifdef __cplusplus
extern "C" {
#endif


#define BUFF_SIZE 6

typedef uint16_t size_type;

typedef enum QSJ4C_Primitive_type {
    QSJ4C_PRIMITIVE_INT = (1 << 0),
    QSJ4C_PRIMITIVE_UINT = (1 << 1),
    QSJ4C_PRIMITIVE_FLOAT = (1 << 2)
} QSJ4C_Primitive_type;

/**
 * Simplistic setup
 *  - Byte[0] => Bit 7-6 => Primitive-type, Bit 5-0 => QSJ4C_TYPE
 */
struct QSJ4C_Value {
	uint8_t buff[BUFF_SIZE];
};

///**
// * Byte[1] => reserved!
// * Byte[2,3] => "Pointer" to top QSJ4C_Member (BUFF_SIZE aligned)
// * Byte[4,5] => Member count (0-65535)
// */
//typedef QSJ4C_Value QSJ4C_Object;
//
///**
// * Byte[1] => reserved!
// * Byte[2,3] => "Pointer" to top QSJ4C_Value (BUFF_SIZE aligned)
// * Byte[4,5] => Member count (0-65535)
// */
//typedef QSJ4C_Value QSJ4C_Array;
//
///**
// * Byte[1] => High byte of "Pointer" to string start (uint8_t)
// * Byt€[2,3] => Low word of "Pointer" to string start (uint16_t)
// * Byte[4,5] => String length (0-65535)
// */
//typedef QSJ4C_Value QSJ4C_String;
//
///**
// * Byte[1] => Error code
// * Byte[2-5] => json position (2^32)
// */
//typedef QSJ4C_Value QSJ4C_Error_information;
//
///**
// * Byte[1] => primitive type
// * Byte[2-5] => 32 Bit numeric value (float, uint32_t, int32_t)
// */
//typedef QSJ4C_Value QSJ4C_Number;
//
///**
// * Byte[1] => 1 ==> true, 0 ==> false
// * Byte[2-5] reserved
// */
//typedef QSJ4C_Value QSJ4C_Bool;

struct QSJ4C_Member {
	uint8_t buff[BUFF_SIZE * 2];
};

extern QSJ4C_fatal_error_fn g_QSJ4C_err_function;

void QSJ4C_std_err_function( void );
size_t QSJ4C_parse_generic(QSJ4C_Builder* builder, const char* json, size_t json_len, int opts, QSJ4C_Value** result_ptr, QSJ4C_realloc_fn realloc_callback);
size_t QSJ4C_calculate_max_buffer_generic( const char* json, size_t json_len, int opts );

QSJ4C_Value* QSJ4C_builder_pop_values( QSJ4C_Builder* builder, size_type count );
char* QSJ4C_builder_pop_string( QSJ4C_Builder* builder, size_type length );
QSJ4C_Member* QSJ4C_builder_pop_members( QSJ4C_Builder* builder, size_type count );

QSJ4C_Value* QSJ4C_object_get_sorted(QSJ4C_Value* obj_ptr, QSJ4C_Value* str_ptr );
QSJ4C_Value* QSJ4C_object_get_unsorted( QSJ4C_Value* obj_ptr, QSJ4C_Value* str_ptr );

int QSJ4C_strcmp( const QSJ4C_Value* lhs, const QSJ4C_Value* rhs );
int QSJ4C_compare_members( const void* lhs, const void * rhs );

uint8_t QSJ4C_get_compatibility_types( const QSJ4C_Value* value_ptr );
void QSJ4C_set_type( QSJ4C_Value* value_ptr, QSJ4C_TYPE type );

/**
 * Sets the storage relevant data for objects.
 */
void QSJ4C_set_object_data( QSJ4C_Value* value_ptr, QSJ4C_Member* top_ptr, uint16_t count );

/**
 * Sets the storage relevant data for arrays.
 */
void QSJ4C_set_array_data( QSJ4C_Value* value_ptr, QSJ4C_Value* top_ptr, uint16_t count );

/**
 * Sets the storage relevant data for strings.
 */
void QSJ4C_set_string_data( QSJ4C_Value* value_ptr, char* str_ptr, uint16_t count );

/**
 * Initializes the builder with the given buffer.
 */
void QSJ4C_builder_init( QSJ4C_Builder* me, void* buff, size_t buff_size );

/**
 * This method will retrieve the document from the builder.
 * Returns NULL in case the buffer has insufficient size.
 */
QSJ4C_Value* QSJ4C_builder_get_document( QSJ4C_Builder* builder );

/**
 * This method will set the value to a boolean with the handed over value.
 */
void QSJ4C_set_bool( QSJ4C_Value* value_ptr, bool value );

/**
 * This method will set the value to a int32_t with the handed over value.
 */
void QSJ4C_set_int( QSJ4C_Value* value_ptr, int32_t value );

/**
 * This method will set the value to a uint32_t with the handed over value.
 */
void QSJ4C_set_uint( QSJ4C_Value* value_ptr, uint32_t value );

/**
 * This method will set the value to a float with the handed over value.
 */
void QSJ4C_set_float( QSJ4C_Value* value_ptr, float value );

/**
 * This method will set the value to the JSON null type.
 */
void QSJ4C_set_null( QSJ4C_Value* value_ptr );

size_t QSJ4C_sprint_impl( QSJ4C_Value* value_ptr, char* buffer, size_t buffer_size, size_t index );

uint32_t QSJ4C_read_uint24(const uint8_t* buff);

void QSJ4C_write_uint24(uint8_t* buff, uint32_t value);

#ifdef __cplusplus
}
#endif

#endif /* QAJSON4C_INTERNAL_H_ */

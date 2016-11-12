/*
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

#ifndef QAJSON4C_H_
#define QAJSON4C_H_

#ifdef __cplusplus
// Helper macros for easier string handling.
#define QAJSON4C_set_string(val_ptr,...) QAJSON4C_set_string_var(val_ptr, {__VA_ARGS__})
#define QAJSON4C_object_create_member(val_ptr,...) QAJSON4C_object_create_member_var(val_ptr, {__VA_ARGS__})
#define QAJSON4C_object_get(val_ptr,...) QAJSON4C_object_get_var(val_ptr, {__VA_ARGS__})
#define QAJSON4C_string_equals(val_ptr,...) QAJSON4C_string_equals_var(val_ptr, {__VA_ARGS__})
#define QAJSON4C_string_cmp(val_ptr,...) QAJSON4C_string_cmp_var(val_ptr, {__VA_ARGS__})
extern "C" {
#else
// Helper macros for easier string handling.
#define QAJSON4C_set_string(val_ptr,...) QAJSON4C_set_string_var(val_ptr, (string_ref_args){__VA_ARGS__})
#define QAJSON4C_object_create_member(val_ptr,...) QAJSON4C_object_create_member_var(val_ptr, (string_ref_args){__VA_ARGS__})
#define QAJSON4C_object_get(val_ptr,...) QAJSON4C_object_get_var(val_ptr, (string_cmp_args){__VA_ARGS__})
#define QAJSON4C_string_equals(val_ptr,...) QAJSON4C_string_equals_var(val_ptr, (string_cmp_args){__VA_ARGS__})
#define QAJSON4C_string_cmp(val_ptr,...) QAJSON4C_string_cmp_var(val_ptr, (string_cmp_args){__VA_ARGS__})
#endif

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>


struct QAJSON4C_Document;
typedef struct QAJSON4C_Document QAJSON4C_Document;

struct QAJSON4C_Value;
typedef struct QAJSON4C_Value QAJSON4C_Value;

struct QAJSON4C_Member;
typedef struct QAJSON4C_Member QAJSON4C_Member;

struct QAJSON4C_Builder {
	uint8_t* buffer;
	size_t buffer_size;

	size_t cur_str_pos;
	size_t cur_obj_pos;
};
typedef struct QAJSON4C_Builder QAJSON4C_Builder;

typedef struct {
    const char* str;
    bool ref;
    QAJSON4C_Builder* builder;
    uint32_t len;
} string_ref_args;

typedef struct {
    const char* str;
    uint32_t len;
} string_cmp_args;

typedef enum QAJSON4C_ERROR_CODES {
	QAJSON4C_ERROR_DEPTH_OVERFLOW = 1,
	QAJSON4C_ERROR_UNEXPECTED_CHAR = 2,
	QAJSON4C_ERROR_BUFFER_TRUNCATED = 3,
	QAJSON4C_ERROR_INVALID_STRING_START = 4,
	QAJSON4C_ERROR_INVALID_NUMBER_FORMAT = 5,
	QAJSON4C_ERROR_UNEXPECTED_JSON_APPENDIX = 6,
	QAJSON4C_ERROR_ARRAY_MISSING_COMMA = 7,
	QAJSON4C_ERROR_OBJECT_MISSING_COLON = 8,
	QAJSON4C_ERROR_FATAL_PARSER_ERROR = 9,
} QAJSON4C_ERROR_CODES;

void QAJSON4C_print_stats();

/**
 * This method will walk through the json message and analyue what buffer size would be required
 * to store the complete DOM.
 */
unsigned QAJSON4C_calculate_max_buffer_size(const char* json);

/**
 * This method will walk through the json message and analyze the maximum required buffer size that
 * would be required to store the DOM (in case the strings do not have to be copied into the buffer)
 */
unsigned QAJSON4C_calculate_max_buffer_size_insitu(const char* json);

const QAJSON4C_Document* QAJSON4C_parse(const char* json, void* buffer, size_t buffer_size);

const QAJSON4C_Document* QAJSON4C_parse_insitu(char* json, void* buffer, size_t buffer_size);

size_t QAJSON4C_sprint(const QAJSON4C_Document* document, char* buffer, size_t buffer_size);

const QAJSON4C_Value* QAJSON4C_get_root_value(const QAJSON4C_Document* document);

bool QAJSON4C_is_string(const QAJSON4C_Value* value);
const char* QAJSON4C_get_string(const QAJSON4C_Value* value);
unsigned QAJSON4C_get_string_length(const QAJSON4C_Value* value);

bool QAJSON4C_string_equals_var(const QAJSON4C_Value* value, string_cmp_args args);
int QAJSON4C_string_cmp_var(const QAJSON4C_Value* value, string_cmp_args args);

bool QAJSON4C_is_object(const QAJSON4C_Value* value);
bool QAJSON4C_is_array(const QAJSON4C_Value* value);

bool QAJSON4C_is_int(const QAJSON4C_Value* value);
int32_t QAJSON4C_get_int(const QAJSON4C_Value* value);

bool QAJSON4C_is_int64(const QAJSON4C_Value* value);
int64_t QAJSON4C_get_int64(const QAJSON4C_Value* value);

bool QAJSON4C_is_uint(const QAJSON4C_Value* value);
uint32_t QAJSON4C_get_uint(const QAJSON4C_Value* value);

bool QAJSON4C_is_uint64(const QAJSON4C_Value* value);
uint64_t QAJSON4C_get_uint64(const QAJSON4C_Value* value);

bool QAJSON4C_is_real(const QAJSON4C_Value* value);
double QAJSON4C_get_real(const QAJSON4C_Value* value);

bool QAJSON4C_is_bool(const QAJSON4C_Value* value);
bool QAJSON4C_get_bool(const QAJSON4C_Value* value);

bool QAJSON4C_is_null(const QAJSON4C_Value* value);

bool QAJSON4C_is_error(const QAJSON4C_Value* value);
const char* QAJSON4C_get_json(const QAJSON4C_Value* value);
QAJSON4C_ERROR_CODES QAJSON4C_get_errno(const QAJSON4C_Value* value);
unsigned QAJSON4C_get_json_pos(const QAJSON4C_Value* value);

unsigned QAJSON4C_object_size(const QAJSON4C_Value* value);
const QAJSON4C_Member* QAJSON4C_object_get_member(const QAJSON4C_Value* value, unsigned index);
const QAJSON4C_Value* QAJSON4C_member_get_key(const QAJSON4C_Member* member);
const QAJSON4C_Value* QAJSON4C_member_get_value(const QAJSON4C_Member* member);
const QAJSON4C_Value* QAJSON4C_object_get_var(const QAJSON4C_Value* value, string_cmp_args args);

unsigned QAJSON4C_array_size(const QAJSON4C_Value* value);
const QAJSON4C_Value* QAJSON4C_array_get(const QAJSON4C_Value* value, unsigned index);

void QAJSON4C_builder_init(QAJSON4C_Builder* me, void* buff, size_t buff_size);
QAJSON4C_Document* QAJSON4C_builder_get_document(QAJSON4C_Builder* builder);

QAJSON4C_Value* QAJSON4C_get_root_value_rw(QAJSON4C_Document* document);


// Modifications on primitives is quite easy

void QAJSON4C_set_bool(QAJSON4C_Value* value_ptr, bool value);
void QAJSON4C_set_int(QAJSON4C_Value* value_ptr, int32_t value);
void QAJSON4C_set_int64(QAJSON4C_Value* value_ptr, int64_t value);
void QAJSON4C_set_uint(QAJSON4C_Value* value_ptr, uint32_t value);
void QAJSON4C_set_uint64(QAJSON4C_Value* value_ptr, uint64_t value);
void QAJSON4C_set_double(QAJSON4C_Value* value_ptr, double value);

/**
 * This method creates transforms the value into a string. If you find the steps required
 * to prepare for this call too much, you can simply use the wrapping MACRO
 * "QAJSON4C_set_string" which simply wrapps the call so one can use default parameter values.
 *
 * @example // Following will create a string by reference on the string, strlen is used.
 *          QAJSON4C_set_string(value_ptr, "id", true);
 *          QAJSON4C_set_string(value_ptr, "id", .ref=true);
 *
 * @example // Following will create a string as copy on the string, strlen is used to
 *          // determine the strings length.
 *          QAJSON4C_set_string(value_ptr, "id", .builder=my_builder_ptr);
 *
 * @example // Following will create a string by reference on the string with a specified
 *          // string length. Note the dot character before the len.
 *          QAJSON4C_set_string(value_ptr, "id\0abc", .ref=true, .len=6);
 *
 * @see QAJSON4C_set_string macro
 *
 * @note in case copy is set to true, the builder has to be specified!
 */
void QAJSON4C_set_string_var(QAJSON4C_Value* value_ptr, string_ref_args args);

void QAJSON4C_set_array(QAJSON4C_Value* value_ptr, unsigned count, QAJSON4C_Builder* builder);
QAJSON4C_Value* QAJSON4C_array_get_rw(QAJSON4C_Value* value_ptr, unsigned index);

void QAJSON4C_set_object(QAJSON4C_Value* value_ptr, unsigned count, QAJSON4C_Builder* builder);

/**
 * This method creates a new member in the object and returns the value that has to be set
 * accordingly. If you find the steps required to prepare for this call too much, you
 * can simply use the wrapping MACRO "QAJSON4C_object_create_member" which simply wrapps
 * the call.
 *
 * @example // Following will create a member with a copy on the string, strlen is used to
 *          // determine the strings length.
 *          QAJSON4C_Value* value = QAJSON4C_object_create_member(obj_ptr, "id", .builder=my_builder_ptr);
 *
 * @example // Following will create a member with a reference on the string, strlen is used.
 *          QAJSON4C_Value* value = QAJSON4C_object_create_member(obj_ptr, "id", true);
 *          QAJSON4C_Value* value = QAJSON4C_object_create_member(obj_ptr, "id", .ref=true);
 *
 * @example // Following will create a member with a reference on the string with a specified
 *          // string length. Note the dot character before the len.
 *          QAJSON4C_Value* value = QAJSON4C_object_create_member(obj_ptr, "id\0abc", .ref=true, .len=6);
 *
 * @see QAJSON4C_object_create_member macro
 */
QAJSON4C_Value* QAJSON4C_object_create_member_var(QAJSON4C_Value* value_ptr, string_ref_args args);

#ifdef __cplusplus
}
#endif



#endif /* QAJSON4C_H_ */

/*
 * qajson4c_internal.h
 */

#ifndef QAJSON4C_INTERNAL_H_
#define QAJSON4C_INTERNAL_H_

#include "qajson_stdwrap.h"

/** Define unlikely macro to be used in asserts
 *  Basically because the compiler should compile for the non-failure scenario.
 **/
#if __GNUC__ >= 3
#define QAJ4C_UNLIKELY(expr) __builtin_expect(expr, 0)
#else
#define QAJ4C_UNLIKELY(expr) expr
#endif

#define QAJ4C_ASSERT(arg, alt) if (QAJ4C_UNLIKELY(!(arg))) do { (*QAJ4C_ERR_FUNCTION)(); alt } while(0)

#define QAJ4C_MIN(lhs, rhs) ((lhs<=rhs)?(lhs):(rhs))
#define QAJ4C_MAX(lhs, rhs) ((lhs>=rhs)?(lhs):(rhs))

#define QAJ4C_INLINE_STRING_SIZE (sizeof(uintptr_t) + sizeof(size_type) - sizeof(uint8_t) * 2)

#define QAJ4C_NULL_TYPE_CONSTANT   ((QAJ4C_NULL << 8) | QAJ4C_TYPE_NULL)
#define QAJ4C_OBJECT_TYPE_CONSTANT ((QAJ4C_OBJECT << 8) |  QAJ4C_TYPE_OBJECT)
#define QAJ4C_OBJECT_SORTED_TYPE_CONSTANT ((QAJ4C_OBJECT_SORTED << 8) | QAJ4C_TYPE_OBJECT)
#define QAJ4C_ARRAY_TYPE_CONSTANT  ((QAJ4C_ARRAY << 8) | QAJ4C_TYPE_ARRAY)
#define QAJ4C_STRING_TYPE_CONSTANT ((QAJ4C_STRING << 8) | QAJ4C_TYPE_STRING)
#define QAJ4C_STRING_REF_TYPE_CONSTANT ((QAJ4C_STRING_REF << 8) | QAJ4C_TYPE_STRING)
#define QAJ4C_INLINE_STRING_TYPE_CONSTANT ((QAJ4C_INLINE_STRING << 8) | QAJ4C_TYPE_STRING)
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

#ifdef __cplusplus
extern "C" {
#endif

typedef uint32_t size_type;
typedef uint16_t half_size_type;

typedef enum QAJ4C_INTERNAL_TYPE {
    QAJ4C_NULL = 0,
    QAJ4C_UNSPECIFIED,
    QAJ4C_OBJECT,
    QAJ4C_OBJECT_SORTED,
    QAJ4C_ARRAY,
    QAJ4C_STRING,
    QAJ4C_STRING_REF,
    QAJ4C_INLINE_STRING,
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

typedef struct QAJ4C_Object {
    QAJ4C_Member* top;
    size_type count;
    char padding[sizeof(size_type)];
} QAJ4C_ALIGN QAJ4C_Object;

typedef struct QAJ4C_Array {
    QAJ4C_Value* top;
    size_type count;
    char padding[sizeof(size_type)];
} QAJ4C_ALIGN QAJ4C_Array;

typedef struct QAJ4C_String {
    const char* s;
    size_type count;
    char padding[sizeof(size_type)];
} QAJ4C_ALIGN QAJ4C_String;

typedef struct QAJ4C_Error_information {
    const char* json;
    size_type json_pos;
    size_type err_no;
} QAJ4C_Error_information;

typedef struct QAJ4C_Error {
    QAJ4C_Error_information* info;
    char padding[sizeof(size_type) * 2];
} QAJ4C_ALIGN QAJ4C_Error;

/**
 * Instead of storing a pointer to a char* like in QAJ4C_String we can QAJ4C_INLINE the
 * char in the struct. This saves a) the dereferencing, b) additional memory.
 * As the string has to be small the count value can also be smaller, to additionally
 * store some more chars.
 * On 32-Bit => 6 chars + \0, on 64-Bit => 10 chars + \0.
 */
typedef struct QAJ4C_Short_string {
    char s[QAJ4C_INLINE_STRING_SIZE + 1];
    uint8_t count;
    char padding[sizeof(size_type)];
} QAJ4C_ALIGN QAJ4C_Short_string;

typedef struct QAJ4C_Primitive {
    union primitive {
        double d;
        uint64_t u;
        int64_t i;
        bool b;
    } data;
    /* Padding on 64 Bit => 8, 32 Bit => 4 */
    char padding[sizeof(uintptr_t) + sizeof(size_type) * 2 - sizeof(uint64_t)];

} QAJ4C_ALIGN QAJ4C_Primitive;

struct QAJ4C_Value {
	char padding[QAJ4C_MAX(sizeof(uint32_t), sizeof(uintptr_t)) + sizeof(size_type)];
    size_type type;
} QAJ4C_ALIGN; /* minimal 3 * 4 Byte = 12, at 64 Bit 16 Byte */

struct QAJ4C_Member {
    QAJ4C_Value key;
    QAJ4C_Value value;
};

extern QAJ4C_fatal_error_fn QAJ4C_ERR_FUNCTION;


size_t QAJ4C_parse_generic(QAJ4C_Builder* builder, const char* json, size_t json_len, int opts, const QAJ4C_Value** result_ptr, QAJ4C_realloc_fn realloc_callback);
size_t QAJ4C_calculate_max_buffer_generic( const char* json, size_t json_len, int opts );

QAJ4C_Value* QAJ4C_builder_pop_values( QAJ4C_Builder* builder, size_type count );
char* QAJ4C_builder_pop_string( QAJ4C_Builder* builder, size_type length );
QAJ4C_Member* QAJ4C_builder_pop_members( QAJ4C_Builder* builder, size_type count );

uint8_t QAJ4C_get_storage_type( const QAJ4C_Value* value_ptr );
uint8_t QAJ4C_get_compatibility_types( const QAJ4C_Value* value_ptr );
QAJ4C_INTERNAL_TYPE QAJ4C_get_internal_type( const QAJ4C_Value* value_ptr );


const QAJ4C_Value* QAJ4C_object_get_unsorted( QAJ4C_Object* obj_ptr, QAJ4C_Value* str_ptr );
const QAJ4C_Value* QAJ4C_object_get_sorted( QAJ4C_Object* obj_ptr, QAJ4C_Value* str_ptr );


int QAJ4C_strcmp( const QAJ4C_Value* lhs, const QAJ4C_Value* rhs );
int QAJ4C_compare_members( const void* lhs, const void * rhs );

size_t QAJ4C_sprint_impl( const QAJ4C_Value* value_ptr, char* buffer, size_t buffer_size, size_t index );


#ifdef __cplusplus
}
#endif

#endif /* QAJSON4C_INTERNAL_H_ */

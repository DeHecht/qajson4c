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

#include "qajson4c_print.h"

#include "internal/types.h"

typedef struct QAJ4C_Buffer_printer {
    char* buffer;
    fast_size_type index;
    fast_size_type len;
} QAJ4C_Buffer_printer;

typedef struct QAJ4C_callback_printer {
    void* external_ptr;
    QAJ4C_print_callback_fn callback;
} QAJ4C_callback_printer;

typedef struct QAJ4C_Print_stack_entry {
    const QAJ4C_Value* top;
    fast_size_type index;
    fast_size_type size;
    QAJ4C_TYPE type;
} QAJ4C_Print_stack_entry;

typedef struct QAJ4C_Print_stack {
    QAJ4C_Print_stack_entry info[QAJ4C_STACK_SIZE];
    QAJ4C_Print_stack_entry* it;
} QAJ4C_Print_stack;


static bool QAJ4C_std_sprint_function( void *ptr, const char* buffer, size_t size );
static bool QAJ4C_std_print_callback_function( void *ptr, const char* buffer, size_t size );

static bool QAJ4C_print_callback_stack_up( QAJ4C_Print_stack* stack, const QAJ4C_Value* value_ptr, QAJ4C_print_buffer_callback_fn callback, void *ptr );
static bool QAJ4C_print_callback_string( const char *string, QAJ4C_print_buffer_callback_fn callback, void *ptr );
static bool QAJ4C_print_callback_error( const QAJ4C_Error* value_ptr, QAJ4C_print_buffer_callback_fn callback, void *ptr );
static bool QAJ4C_print_callback_number( const QAJ4C_Value* value_ptr, QAJ4C_print_buffer_callback_fn callback, void *ptr );
static bool QAJ4C_print_callback_double( double d, QAJ4C_print_buffer_callback_fn callback, void *ptr );
static bool QAJ4C_print_callback_uint64( uint64_t value, QAJ4C_print_buffer_callback_fn callback, void *ptr );
static bool QAJ4C_print_callback_int64( int64_t value, QAJ4C_print_buffer_callback_fn callback, void *ptr );

static bool QAJ4C_is_digit( char c ) {
    return (uint8_t)(c - '0') < 10;
}

static bool QAJ4C_is_valid_second_double_char( char c ) {
    return QAJ4C_is_digit(c) || c == '.' || c == 'e' || c == 'E' || c == '\0';
}

size_t QAJ4C_sprint( const QAJ4C_Value* value_ptr, char* buffer, size_t buffer_size ) {
    if (QAJ4C_UNLIKELY(buffer_size == 0)) {
            return 0;
    }
    QAJ4C_Buffer_printer printer = {buffer, 0, buffer_size - 1};
    QAJ4C_print_buffer_callback(value_ptr, QAJ4C_std_sprint_function, &printer);
    buffer[printer.index] = '\0'; // ensure \0 termination
    return printer.index + 1;
}

bool QAJ4C_print_callback( const QAJ4C_Value* value_ptr, QAJ4C_print_callback_fn callback, void* ptr ) {
    QAJ4C_callback_printer printer = {ptr, callback};
    if (QAJ4C_UNLIKELY(callback == NULL)) {
        return false;
    }
    return QAJ4C_print_buffer_callback(value_ptr, QAJ4C_std_print_callback_function, &printer);
}

bool QAJ4C_print_buffer_callback( const QAJ4C_Value* value_ptr, QAJ4C_print_buffer_callback_fn callback, void* ptr ) {
    if (QAJ4C_UNLIKELY(callback == NULL)) {
        return false;
    }

    QAJ4C_Print_stack stack;
    bool result = true;
    stack.it = stack.info;
    stack.it->top = value_ptr;
    stack.it->index = 0;
    stack.it->size = 1;
    stack.it->type = QAJ4C_TYPE_INVALID;

    while (result && stack.it >= stack.info) {
        const QAJ4C_Value* pos_ptr = stack.it->top + stack.it->index;
        if (stack.it->index > 0) {
            if (stack.it->type == QAJ4C_TYPE_ARRAY || (stack.it->type == QAJ4C_TYPE_OBJECT && (stack.it->index & 0x1) == 0)) {
                result = callback(ptr, QAJ4C_S(","));
            } else {
                result = callback(ptr, QAJ4C_S(":"));
            }
        }
        stack.it->index += 1;

        if (result) {
            switch (QAJ4C_get_type(pos_ptr)) {
            case QAJ4C_TYPE_NULL:
                result = callback(ptr, QAJ4C_S("null"));
                break;
            case QAJ4C_TYPE_OBJECT:
            case QAJ4C_TYPE_ARRAY:
                result = QAJ4C_print_callback_stack_up(&stack, pos_ptr, callback, ptr);
                break;
            case QAJ4C_TYPE_STRING:
                result = QAJ4C_print_callback_string(QAJ4C_get_string(pos_ptr), callback, ptr);
                break;
            case QAJ4C_TYPE_NUMBER:
                result = QAJ4C_print_callback_number(pos_ptr, callback, ptr);
                break;
            case QAJ4C_TYPE_BOOL:
                if (((const QAJ4C_Primitive*)pos_ptr)->data.b) {
                    result = callback(ptr, QAJ4C_S("true"));
                } else {
                    result = callback(ptr, QAJ4C_S("false"));
                }
                break;
            case QAJ4C_TYPE_INVALID:
                result = QAJ4C_print_callback_error((const QAJ4C_Error*)pos_ptr, callback, ptr);
                break;
            default:
                g_qaj4c_err_function();
            }
            while (stack.it >= stack.info && stack.it->index >= stack.it->size) {
                if ( stack.it->type == QAJ4C_TYPE_OBJECT ) {
                    result = result && callback( ptr, QAJ4C_S("}"));
                } else if ( stack.it->type == QAJ4C_TYPE_ARRAY ) {
                    result = result && callback( ptr, QAJ4C_S("]"));
                }
                // end current object / array
                stack.it -= 1;
            }
        }
    }
    return result;
}

static bool QAJ4C_std_sprint_function( void *ptr, const char* buffer, size_t size ) {
    QAJ4C_Buffer_printer* printer = (QAJ4C_Buffer_printer*)ptr;
    size_t bytes_left = printer->len - printer->index;
    size_t copy_bytes = (bytes_left > size) ? size : bytes_left;
    QAJ4C_MEMCPY(printer->buffer + printer->index, buffer, copy_bytes);
    printer->index += copy_bytes;
    return copy_bytes == size;
}

static bool QAJ4C_std_print_callback_function( void *ptr, const char* buffer, size_t size ) {
    bool result = true;
    size_t pos = 0;
    QAJ4C_callback_printer* callback_printer = (QAJ4C_callback_printer*)ptr;
    while (result && pos < size) {
        result = callback_printer->callback(callback_printer->external_ptr, buffer[pos]);
        pos += 1;
    }
    return result;
}

static bool QAJ4C_print_callback_stack_up( QAJ4C_Print_stack* stack, const QAJ4C_Value* value_ptr, QAJ4C_print_buffer_callback_fn callback, void *ptr ) {
    const QAJ4C_Array* array_ptr = (const QAJ4C_Array*) value_ptr;
    QAJ4C_TYPE value_type = QAJ4C_get_type(value_ptr);
    bool success = false;
    bool is_object = value_type == QAJ4C_TYPE_OBJECT;
    char append_char = is_object ? '{' : '[';

    if (stack->it < stack->info + QAJ4C_STACK_SIZE - 1) {
        stack->it += 1;
        stack->it->index = 0;
        stack->it->type = value_type;
        stack->it->top = array_ptr->top;
        stack->it->size = is_object ? array_ptr->count * 2 : array_ptr->count;
        success = callback(ptr, &append_char, 1);
    }
    return success;
}

static bool QAJ4C_print_callback_string( const char *string, QAJ4C_print_buffer_callback_fn callback, void *ptr ) {
    static const char* replacement_buf[] = {
            "\\u0000", "\\u0001", "\\u0002", "\\u0003", "\\u0004", "\\u0005", "\\u0006", "\\u0007", "\\b",     "\\t",     "\\n",     "\\u000b", "\\f",     "\\r",     "\\u000e", "\\u000f",
            "\\u0010", "\\u0011", "\\u0012", "\\u0013", "\\u0014", "\\u0015", "\\u0016", "\\u0017", "\\u0018", "\\u0019", "\\u001a", "\\u001b", "\\u001c", "\\u001d", "\\u001e", "\\u001f",
            "", "", "\\\"", "", "", "", "", "", "", "", "", "", "\\\\", "", "", "\\/"
    };
    const char *p = string;
    size_t size = 0;
    bool result = callback(ptr, "\"", 1);

    while( result && p[size] != '\0' ) {
        uint8_t c = p[size];
        if (QAJ4C_UNLIKELY(c < 32 || c == '"' || c == '/' || c == '\\')) {
            if ( c >= 32 ) {
                /* set the char in the 2x range so we can use the replacement buffer to replace the string. */
                c = (c & 0xF) | 0x20;
            }
            const char* replacement_string = replacement_buf[c];
            result = result && callback(ptr, p, size); /* flush the string until now */
            result = result && callback(ptr, replacement_string, strlen(replacement_string));
            p = p + size + 1;
            size = 0;
        } else {
            size += 1;
        }
    }

    if (size > 0) {
        result = result && callback(ptr, p, size);
    }

    return result && callback(ptr, "\"", 1);
}

static bool QAJ4C_print_callback_error( const QAJ4C_Error* value_ptr, QAJ4C_print_buffer_callback_fn callback, void *ptr ) {
    static const char ERR_MSG[] = "{\"error\":\"Unable to parse json message. Error (";
    static const char ERR_MSG_2[] = ") at position ";
    static const char ERR_MSG_3[] = "\"}";

    return callback(ptr, QAJ4C_S(ERR_MSG))
            && QAJ4C_print_callback_uint64(value_ptr->info->err_no, callback, ptr)
            && callback(ptr, QAJ4C_S(ERR_MSG_2))
            && QAJ4C_print_callback_uint64(value_ptr->info->json_pos, callback, ptr)
            && callback(ptr, QAJ4C_S(ERR_MSG_3));
}

static bool QAJ4C_print_callback_number( const QAJ4C_Value* value_ptr, QAJ4C_print_buffer_callback_fn callback, void *ptr )
{
    bool result = true;
    switch (QAJ4C_get_storage_type(value_ptr)) {
    case QAJ4C_PRIMITIVE_INT:
    case QAJ4C_PRIMITIVE_INT64:
        result = QAJ4C_print_callback_int64(((QAJ4C_Primitive*)value_ptr)->data.i, callback, ptr);
        break;
    case QAJ4C_PRIMITIVE_UINT:
    case QAJ4C_PRIMITIVE_UINT64:
        result = QAJ4C_print_callback_uint64(((QAJ4C_Primitive*)value_ptr)->data.u, callback, ptr);
        break;
    default: /* it has to be double */
        result = QAJ4C_print_callback_double(((QAJ4C_Primitive*)value_ptr)->data.d, callback, ptr);
        break;
    }
    return result;
}

static bool QAJ4C_print_callback_double( double d, QAJ4C_print_buffer_callback_fn callback, void *ptr )
{
    static int BUFFER_SIZE = 32;
    char buffer[BUFFER_SIZE];
    bool result = true;

    int printf_result = QAJ4C_SNPRINTF(buffer, BUFFER_SIZE, "%1.10g", d);

    if (printf_result > 0 && printf_result < BUFFER_SIZE && QAJ4C_is_valid_second_double_char(buffer[1]))
    {
       result = callback(ptr, buffer, printf_result);
    }
    else
    {
        result = callback(ptr, QAJ4C_S("null"));
    }
    return result;
}

static char* QAJ4C_do_print_uint64( uint64_t value, char* buffer, size_t size )
{
    uint64_t number = value;
    char* pos_ptr = buffer + size;
    if (number == 0) {
        pos_ptr -= 1;
        *pos_ptr = '0';
    } else {
        while (number != 0) {
            pos_ptr -= 1;
            *pos_ptr = '0' + number % 10;
            number /= 10;
        }
    }

    return pos_ptr;
}

static bool QAJ4C_print_callback_uint64( uint64_t value, QAJ4C_print_buffer_callback_fn callback, void *ptr )
{
    static int BUFFER_SIZE = 32;
    char buffer[BUFFER_SIZE];
    char* pos_ptr = QAJ4C_do_print_uint64(value, buffer, BUFFER_SIZE);
    return callback(ptr, pos_ptr, (buffer + BUFFER_SIZE) - pos_ptr);
}

static bool QAJ4C_print_callback_int64( int64_t value, QAJ4C_print_buffer_callback_fn callback, void *ptr )
{
    static int BUFFER_SIZE = 32;
    char buffer[BUFFER_SIZE];

    /* this callback is only called with a negative number as it otherwise has been classified uint64 */
    char* pos_ptr = QAJ4C_do_print_uint64(-value, buffer + 1, BUFFER_SIZE - 1);

    pos_ptr -= 1;
    *pos_ptr = '-';
    return callback(ptr, pos_ptr, (buffer + BUFFER_SIZE) - pos_ptr);
}


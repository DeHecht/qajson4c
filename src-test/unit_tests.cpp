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

#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>
#include <wait.h>
#include <assert.h>
#include <signal.h>
#include <string.h>
#include <time.h>

#include <qajson4c/qajson4c.h>
#include <qajson4c/qajson4c_internal.h>

/**
 * This has been copied from gtest
 */
#if defined(__GNUC__) && !defined(COMPILER_ICC)
# define UNITTESTS_ATTRIBUTE_UNUSED_ __attribute__ ((unused))
#else
# define UNITTESTS_ATTRIBUTE_UNUSED_
#endif

#define ARRAY_COUNT(x)  (sizeof(x) / sizeof(x[0]))

#define DEBUGGING true

typedef struct QAJ4C_TEST_DEF {
    const char* subject_name;
    const char* test_name;
    void (*test_func)( void );
    struct QAJ4C_TEST_DEF* next;
    bool status;
    time_t execution;
} QAJ4C_TEST_DEF;

QAJ4C_TEST_DEF* TOP_POINTER = NULL;
QAJ4C_TEST_DEF* LAST_POINTER = NULL;

static int tests_total = 0;
static int tests_failed = 0;
static int tests_passed = 0;

static QAJ4C_TEST_DEF* create_test( const char* subject, const char* test, void (*test_func)() ) {
    QAJ4C_TEST_DEF* tmp = (QAJ4C_TEST_DEF*)malloc(sizeof(QAJ4C_TEST_DEF));
    tmp->next = NULL;
    tmp->test_func = test_func;
    tmp->subject_name = subject;
    tmp->test_name = test;
    tmp->status = false;
    tmp->execution = 0;

    // sort the tests correctly!

    if (TOP_POINTER == NULL) {
        TOP_POINTER = tmp;
    } else {
        if (strcmp(TOP_POINTER->subject_name, subject) > 0) {
            tmp->next = TOP_POINTER;
            TOP_POINTER = tmp;
        } else {
            QAJ4C_TEST_DEF* current = TOP_POINTER;
            while (current->next != NULL && strcmp(current->next->subject_name, subject) <= 0) {
                current = current->next;
            }
            if ( current->next == NULL ) {
                current->next = tmp;
            } else {
                tmp->next = current->next;
                current->next = tmp;
            }
        }
    }
    tests_total++;
    return tmp;
}

int fork_and_run( QAJ4C_TEST_DEF* test ) {
    if ( DEBUGGING ) {
        test->test_func();
    }
    else
    {
        test->execution = -time(NULL);
        pid_t pid = fork();
        if ( pid == 0 ) {
           test->test_func();
           exit(0);
        }
        else {
            int status;
            pid_t wait_pid;
            while ((wait_pid = wait(&status)) > 0) {
                if (WIFEXITED(status)) {
                    printf("Exit status of %s.%s was %d\n", test->subject_name,
                           test->test_name, WEXITSTATUS(status));
                } else if (WIFSIGNALED(status)) {
                    printf("Signaled status of %s.%s was %d\n", test->subject_name,
                           test->test_name, WTERMSIG(status));
                } else {
                    printf("Process %d seems to be stopped!\n", (int)wait_pid);
                    continue;
                }

                if ( WIFEXITED(status) && WEXITSTATUS(status) == 0) {
                    tests_passed++;
                } else {
                    tests_failed++;
                }
            }
            test->execution += time(NULL);
        }
    }

    return 0;
}

/**
 * Ensure the library will crash in case an error occurs.
 */
void custom_error_function() {
    raise(6);
}

#define STATIC_FWRITE(str, file) fwrite(str, sizeof(char), ARRAY_COUNT(str), file)

int main( int argc, char **argv ) {
    QAJ4C_register_fatal_error_function(custom_error_function);

    QAJ4C_TEST_DEF* curr_test = TOP_POINTER;
    while (curr_test != NULL) {
        fork_and_run(curr_test);
        curr_test = curr_test->next;
    }

    printf("%d tests of %d total tests passed (%d failed)\n", tests_passed, tests_total, tests_failed);
}

#define TEST(a, b) \
   void a## _## b## _test(); \
   UNITTESTS_ATTRIBUTE_UNUSED_ QAJ4C_TEST_DEF* a## _## b## _test_ptr = create_test(#a, #b, a## _## b## _test); \
   void a## _## b## _test()


TEST(BufferSizeTests, ParseObjectWithOneNumericMember) {
    const char json[] = R"({"id":1})";

    size_t normal_required_buffer_size = QAJ4C_calculate_max_buffer_size_n(json, ARRAY_COUNT(json));
    size_t insitu_required_buffer_size = QAJ4C_calculate_max_buffer_size_insitu_n(json, ARRAY_COUNT(json));
    assert(normal_required_buffer_size == insitu_required_buffer_size);

    // one object + one member
    assert(normal_required_buffer_size == (sizeof(QAJ4C_Value) + sizeof(QAJ4C_Member)));
}

TEST(Statistics, CheckSizes) {

	assert(sizeof(double) == 8);
	assert(sizeof(int64_t) == 8);
	assert(sizeof(uint64_t) == 8);

	assert(sizeof(QAJ4C_Value) == sizeof(QAJ4C_Object));
	assert(sizeof(QAJ4C_Value) == sizeof(QAJ4C_Array));
	assert(sizeof(QAJ4C_Value) == sizeof(QAJ4C_String));
	assert(sizeof(QAJ4C_Value) == sizeof(QAJ4C_Short_string));
	assert(sizeof(QAJ4C_Value) == sizeof(QAJ4C_Primitive));
	assert(sizeof(QAJ4C_Value) == sizeof(QAJ4C_Error));

	assert(sizeof(QAJ4C_Member) == 2 * sizeof(QAJ4C_Value));
}

TEST(SimpleParsingTests, ParseObjectWithOneNumericMember) {
    const char json[] = R"({"id":1})";

    const QAJ4C_Value* value = QAJ4C_parse_opt_dynamic(json, ARRAY_COUNT(json), 0, realloc);
    assert(QAJ4C_is_object(value));
    assert(QAJ4C_object_size(value) == 1);

    const QAJ4C_Value* object_entry = QAJ4C_object_get(value, "id");
    assert(object_entry != NULL);
    assert(QAJ4C_is_uint(object_entry));
    assert(QAJ4C_get_uint(object_entry) == 1);
    free((void*)value);
}

TEST(SimpleParsingTests, ParseObjectWithOneNumericMemberWhitespaces) {
    const char json[] = R"({ "id" : 1 })";

    const QAJ4C_Value* value = QAJ4C_parse_opt_dynamic(json, ARRAY_COUNT(json), 0, realloc);
    assert(QAJ4C_is_object(value));
    assert(QAJ4C_object_size(value) == 1);

    const QAJ4C_Value* object_entry = QAJ4C_object_get(value, "id");
    assert(object_entry != NULL);
    assert(QAJ4C_is_uint(object_entry));
    assert(QAJ4C_get_uint(object_entry) == 1);
    free((void*)value);
}


TEST(BufferSizeTests, ParseObjectWithOneStringVeryLongMember) {
    const char json[] = R"({"name":"blahblubbhubbeldipup"})";

    size_t normal_required_buffer_size = QAJ4C_calculate_max_buffer_size_n(json, ARRAY_COUNT(json));
    size_t insitu_required_buffer_size = QAJ4C_calculate_max_buffer_size_insitu_n(json, ARRAY_COUNT(json));
    assert(normal_required_buffer_size > insitu_required_buffer_size);

    // one object + one member
    assert(insitu_required_buffer_size == (sizeof(QAJ4C_Value) + sizeof(QAJ4C_Member)));
}

TEST(SimpleParsingTests, ParseObjectWithOneStringMember) {
    const char json[] = R"({"name":"blah"})";
    const QAJ4C_Value* value = QAJ4C_parse_opt_dynamic(json, ARRAY_COUNT(json), 0, realloc);
    assert(QAJ4C_is_object(value));
    assert(QAJ4C_object_size(value) == 1);

    const QAJ4C_Value* object_entry = QAJ4C_object_get(value, "name");
    assert(QAJ4C_is_string(object_entry));
    assert(strcmp(QAJ4C_get_string(object_entry), "blah") == 0);
    free((void*)value);
}

TEST(SimpleParsingTests, ParseObjectWithOneStringVeryLongMember) {
    const char json[] = R"({"name":"blahblubbhubbeldipup"})";
    const QAJ4C_Value* value = QAJ4C_parse_opt_dynamic(json, ARRAY_COUNT(json), 0, realloc);
    assert(QAJ4C_is_object(value));
    assert(QAJ4C_object_size(value) == 1);

    const QAJ4C_Value* object_entry = QAJ4C_object_get(value, "name");
    assert(QAJ4C_is_string(object_entry));
    assert(strcmp(QAJ4C_get_string(object_entry), "blahblubbhubbeldipup") == 0);
    free((void*)value);
}

TEST(SimpleParsingTests, ParseObjectWithOneStringVeryLongMemberInsitu) {
    char json[] = R"({"name":"blahblubbhubbeldipup"})";
    char buffer[64];

    size_t required = ARRAY_COUNT(buffer);

    const QAJ4C_Value* value = QAJ4C_parse_opt_insitu(json, ARRAY_COUNT(json), 0, (void*)buffer, &required);
    assert(QAJ4C_is_object(value));
    assert(QAJ4C_object_size(value) == 1);
    assert(required == sizeof(QAJ4C_Value) + sizeof(QAJ4C_Member));

    const QAJ4C_Value* object_entry = QAJ4C_object_get(value, "name");
    assert(QAJ4C_is_string(object_entry));
    assert(strcmp(QAJ4C_get_string(object_entry), "blahblubbhubbeldipup") == 0);
}

TEST(SimpleParsingTests, ParseStringWithNewLine) {
    char json[] = R"(["Hello\nWorld"])";
    const QAJ4C_Value* value = QAJ4C_parse_opt_dynamic(json, ARRAY_COUNT(json), 0, realloc);
    assert(QAJ4C_is_array(value));
    assert(QAJ4C_array_size(value) == 1);
    const QAJ4C_Value* array_entry = QAJ4C_array_get(value, 0);
    assert(QAJ4C_is_string(array_entry));

    // Now compare with the non-raw string (as the \n should be interpeted)
    assert(strcmp(QAJ4C_get_string(array_entry), "Hello\nWorld") == 0);
}

TEST(SimpleParsingTests, ParseStringWithEscapedQuotes) {
    char json[] = R"(["Hello\"World"])";
    const QAJ4C_Value* value = QAJ4C_parse_opt_dynamic(json, ARRAY_COUNT(json), 0, realloc);
    assert(QAJ4C_is_array(value));
    assert(QAJ4C_array_size(value) == 1);
    const QAJ4C_Value* array_entry = QAJ4C_array_get(value, 0);
    assert(QAJ4C_is_string(array_entry));

    // Now compare with the non-raw string (as the \n should be interpeted)
    assert(strcmp(QAJ4C_get_string(array_entry), "Hello\"World") == 0);

}

TEST(SimpleParsingTests, ParseEmptyObject) {
    const char json[] = R"({})";
    const QAJ4C_Value* value = QAJ4C_parse_opt_dynamic(json, ARRAY_COUNT(json), 0, realloc);
    assert(QAJ4C_is_object(value));
    assert(QAJ4C_object_size(value) == 0);
    free((void*)value);
}

TEST(SimpleParsingTests, ParseEmptyObjectWhitespaces) {
    const char json[] = R"({ })";
    const QAJ4C_Value* value = QAJ4C_parse_opt_dynamic(json, ARRAY_COUNT(json), 0, realloc);
    assert(QAJ4C_is_object(value));
    assert(QAJ4C_object_size(value) == 0);
    free((void*)value);
}

TEST(SimpleParsingTests, ParseEmptyArray) {
    const char json[] = R"([])";
    const QAJ4C_Value* value = QAJ4C_parse_opt_dynamic(json, ARRAY_COUNT(json), 0, realloc);
    assert(QAJ4C_is_array(value));
    assert(QAJ4C_array_size(value) == 0);
    free((void*)value);
}

TEST(SimpleParsingTests, ParseEmptyArrayWhitespaces) {
    const char json[] = R"([ ])";
    const QAJ4C_Value* value = QAJ4C_parse_opt_dynamic(json, ARRAY_COUNT(json), 0, realloc);
    assert(QAJ4C_is_array(value));
    assert(QAJ4C_array_size(value) == 0);
    free((void*)value);
}


TEST(SimpleParsingTests, ParseNumberArray) {
    const char json[] = R"([1,2,3,-4,5,6])";
    const QAJ4C_Value* value = QAJ4C_parse_opt_dynamic(json, ARRAY_COUNT(json), 0, realloc);
    assert(QAJ4C_is_array(value));
    assert(QAJ4C_array_size(value) == 6);

    assert(1 == QAJ4C_get_uint(QAJ4C_array_get(value, 0)));
    assert(2 == QAJ4C_get_uint(QAJ4C_array_get(value, 1)));
    assert(3 == QAJ4C_get_uint(QAJ4C_array_get(value, 2)));
    assert(-4 == QAJ4C_get_int(QAJ4C_array_get(value, 3)));
    assert(5 == QAJ4C_get_uint(QAJ4C_array_get(value, 4)));
    assert(6 == QAJ4C_get_uint(QAJ4C_array_get(value, 5)));

    free((void*)value);
}

TEST(SimpleParsingTests, ParseNumberArrayWithComments) {
    const char json[] = R"([/* HO */1, /* HO **/ 2, /**/ 3, /***/4,5,6])";
    const QAJ4C_Value* value = QAJ4C_parse_opt_dynamic(json, ARRAY_COUNT(json), 0, realloc);
    assert(QAJ4C_is_array(value));
    assert(QAJ4C_array_size(value) == 6);

    assert(1 == QAJ4C_get_uint(QAJ4C_array_get(value, 0)));
    assert(2 == QAJ4C_get_uint(QAJ4C_array_get(value, 1)));
    assert(3 == QAJ4C_get_uint(QAJ4C_array_get(value, 2)));
    assert(4 == QAJ4C_get_uint(QAJ4C_array_get(value, 3)));
    assert(5 == QAJ4C_get_uint(QAJ4C_array_get(value, 4)));
    assert(6 == QAJ4C_get_uint(QAJ4C_array_get(value, 5)));

    free((void*)value);
}

TEST(SimpleParsingTests, ParseNumberArrayWithLineComments) {
	const char json[] = R"([// HO 
1, // HO 
 2, //
 3, //
4,5,6])";

    const QAJ4C_Value* value = QAJ4C_parse_opt_dynamic(json, ARRAY_COUNT(json), 0, realloc);
    assert(QAJ4C_is_array(value));
    assert(QAJ4C_array_size(value) == 6);

    assert(1 == QAJ4C_get_uint(QAJ4C_array_get(value, 0)));
    assert(2 == QAJ4C_get_uint(QAJ4C_array_get(value, 1)));
    assert(3 == QAJ4C_get_uint(QAJ4C_array_get(value, 2)));
    assert(4 == QAJ4C_get_uint(QAJ4C_array_get(value, 3)));
    assert(5 == QAJ4C_get_uint(QAJ4C_array_get(value, 4)));
    assert(6 == QAJ4C_get_uint(QAJ4C_array_get(value, 5)));

    free((void*)value);
}

TEST(SimpleParsingTests, ParseMultilayeredObject) {
    const char json[] = R"({"id":1,"data":{"name":"foo","param":12}})";
    const QAJ4C_Value* value = QAJ4C_parse_opt_dynamic(json, ARRAY_COUNT(json), 0, realloc);

    assert(QAJ4C_is_object(value));
    assert(QAJ4C_object_size(value) == 2);

    const QAJ4C_Value* id_node = QAJ4C_object_get(value, "id");
    assert(QAJ4C_is_uint(id_node));
    assert(QAJ4C_get_uint(id_node) == 1);

    const QAJ4C_Value* data_node = QAJ4C_object_get(value, "data");
    assert(QAJ4C_is_object(data_node));
    assert(QAJ4C_object_size(data_node) == 2);

    const QAJ4C_Value* data_name_node = QAJ4C_object_get(data_node, "name");
    assert(QAJ4C_is_string(data_name_node));
    assert(strcmp(QAJ4C_get_string(data_name_node), "foo") == 0);

    const QAJ4C_Value* data_param_node = QAJ4C_object_get(data_node, "param");
    assert(QAJ4C_is_uint(data_param_node));
    assert(QAJ4C_get_uint(data_param_node) == 12);

    free((void*)value);
}

TEST(SimpleParsingTests, ParseNumberArrayTrailingComma) {
    const char json[] = R"([1,2,])";
    const QAJ4C_Value* value = QAJ4C_parse_opt_dynamic(json, ARRAY_COUNT(json), 0, realloc);
    assert(QAJ4C_is_array(value));
    assert(QAJ4C_array_size(value) == 2);

    assert(1 == QAJ4C_get_uint(QAJ4C_array_get(value, 0)));
    assert(2 == QAJ4C_get_uint(QAJ4C_array_get(value, 1)));

    free((void*)value);
}

TEST(SimpleParsingTests, ParseNumberArrayWhitespaces) {
    const char json[] = R"([ 1 , 2  , ])";
    const QAJ4C_Value* value = QAJ4C_parse_opt_dynamic(json, ARRAY_COUNT(json), 0, realloc);
    assert(QAJ4C_is_array(value));
    assert(QAJ4C_array_size(value) == 2);

    assert(1 == QAJ4C_get_uint(QAJ4C_array_get(value, 0)));
    assert(2 == QAJ4C_get_uint(QAJ4C_array_get(value, 1)));

    free((void*)value);
}

TEST(SimpleParsingTests, ParseObjectArrayObjectCombination) {
    const char json[] = R"({"services":[{"id":1},{"id":2},{"id":3}]})";
    const QAJ4C_Value* value = QAJ4C_parse_opt_dynamic(json, ARRAY_COUNT(json), 0, realloc);
    assert(QAJ4C_is_object(value));
    assert(QAJ4C_object_size(value) == 1);

    const QAJ4C_Value* services_node = QAJ4C_object_get(value, "services");
    assert(QAJ4C_is_array(services_node));
    assert(QAJ4C_array_size(services_node) == 3);

    free((void*)value);
}

/**
 * In this test case a scenario where the memory was corrupted internally is
 * reconstructed.
 */
TEST(SimpleParsingTests, ParseMemoryCornerCase) {
    const char json[] = R"({"b":[1],"c":"d"})";
    const QAJ4C_Value* value = QAJ4C_parse_opt_dynamic(json, ARRAY_COUNT(json), 0, realloc);
    assert(!QAJ4C_is_error(value));
    free((void*)value);
}

TEST(ErrorHandlingTests, ParseIncompleteObject) {
    const char json[] = R"({)";
    const QAJ4C_Value* value = QAJ4C_parse_opt_dynamic(json, ARRAY_COUNT(json), 0, realloc);
    assert(QAJ4C_is_error(value));
    assert(QAJ4C_error_get_errno(value) == QAJ4C_ERROR_BUFFER_TRUNCATED);
    free((void*)value);
}

TEST(ErrorHandlingTests, ParseIncompleteArray) {
    const char json[] = R"([)";
    const QAJ4C_Value* value = QAJ4C_parse_opt_dynamic(json, ARRAY_COUNT(json), 0, realloc);
    assert(QAJ4C_is_error(value));
    assert(QAJ4C_error_get_errno(value) == QAJ4C_ERROR_BUFFER_TRUNCATED);
    free((void*)value);
}

TEST(ErrorHandlingTests, ParseIncompleteString) {
    const char json[] = R"(")";
    const QAJ4C_Value* value = QAJ4C_parse_opt_dynamic(json, ARRAY_COUNT(json), 0, realloc);
    assert(QAJ4C_is_error(value));
    assert(QAJ4C_error_get_errno(value) == QAJ4C_ERROR_BUFFER_TRUNCATED);
    free((void*)value);
}

TEST(ErrorHandlingTests, ParseObjectKeyWithoutStartingQuotes) {
    const char json[] = R"({id":1})";
    const QAJ4C_Value* value = QAJ4C_parse_opt_dynamic(json, ARRAY_COUNT(json), 0, realloc);
    assert(QAJ4C_is_error(value));
    assert(QAJ4C_error_get_errno(value) == QAJ4C_ERROR_UNEXPECTED_CHAR);
    free((void*)value);
}


TEST(ErrorHandlingTests, ParseBombasticArray) {
	char json[] = "[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]";
    const QAJ4C_Value* value = QAJ4C_parse_opt_dynamic(json, ARRAY_COUNT(json), 0, realloc);
    assert(QAJ4C_is_error(value));
    assert(QAJ4C_error_get_errno(value) == QAJ4C_ERROR_DEPTH_OVERFLOW);
    free((void*)value);
}

TEST(ErrorHandlingTests, ParseBombasticObject) {
	char json[] = R"({"a":{"a":{"a":{"a":{"a":{"a":{"a":{"a":{"a":{"a":{"a":{"a":{"a":{"a":{"a":{"a":{"a":{"a":{"a":{"a":{"a":{"a":{"a":{"a":{"a":{"a":{"a":{"a":{"a":{"a":{"a":{"a":{"a":{"a":{}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}})";
    const QAJ4C_Value* value = QAJ4C_parse_opt_dynamic(json, ARRAY_COUNT(json), 0, realloc);
    assert(QAJ4C_is_error(value));
    assert(QAJ4C_error_get_errno(value) == QAJ4C_ERROR_DEPTH_OVERFLOW);
    free((void*)value);
}


TEST(PrintTests, PrintEmtpyObject) {
	char json[] = "{}";
    const QAJ4C_Value* value = QAJ4C_parse_opt_dynamic(json, ARRAY_COUNT(json), 0, realloc);

    assert(!QAJ4C_is_error(value));

    char output[ARRAY_COUNT(json)];
    size_t out = QAJ4C_sprint(value, output, ARRAY_COUNT(output));
    assert(ARRAY_COUNT(output) == out);
	assert(strcmp(json, output) == 0);

    free((void*)value);
}

TEST(PrintTests, PrintNumericArray) {
    const char json[] = R"([1,2,3,4,5,6])";
    const QAJ4C_Value* value = QAJ4C_parse_opt_dynamic(json, ARRAY_COUNT(json), 0, realloc);

    assert(!QAJ4C_is_error(value));

    char output[ARRAY_COUNT(json)];
    size_t out = QAJ4C_sprint(value, output, ARRAY_COUNT(output));
    assert(ARRAY_COUNT(output) == out);
    assert(strcmp(json, output) == 0);

    free((void*)value);
}

TEST(PrintTests, PrintStringArray) {
    const char json[] = R"(["1","22","333","4444","55555","666666"])";
    const QAJ4C_Value* value = QAJ4C_parse_opt_dynamic(json, ARRAY_COUNT(json), 0, realloc);

    assert(!QAJ4C_is_error(value));

    char output[ARRAY_COUNT(json)];
    size_t out = QAJ4C_sprint(value, output, ARRAY_COUNT(output));
    assert(ARRAY_COUNT(output) == out);
    assert(strcmp(json, output) == 0);

    free((void*)value);
}

TEST(PrintTests, PrintMultiLayerObject) {
    const char json[] = R"({"id":1,"data":{"name":"foo","param":12}})";
    const QAJ4C_Value* value = QAJ4C_parse_opt_dynamic(json, ARRAY_COUNT(json), 0, realloc);

    assert(!QAJ4C_is_error(value));

    char output[ARRAY_COUNT(json)];
    size_t out = QAJ4C_sprint(value, output, ARRAY_COUNT(output));

    assert(ARRAY_COUNT(output) == out);
    assert(strcmp(json, output) == 0);

    free((void*)value);

}


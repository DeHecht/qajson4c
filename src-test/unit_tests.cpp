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
#include <assert.h>
#include <signal.h>
#include <string.h>
#include <time.h>

#ifndef _WIN32
#include <wait.h>
#endif

#include "qajson4c/qajson4c.h"
#include "qajson4c/qajson4c_internal.h"

/**
 * This has been copied from gtest
 */
#if defined(__GNUC__) && !defined(COMPILER_ICC)
# define UNITTESTS_ATTRIBUTE_UNUSED_ __attribute__ ((unused))
#else
# define UNITTESTS_ATTRIBUTE_UNUSED_
#endif

#define ARRAY_COUNT(x)  (sizeof(x) / sizeof(x[0]))

static bool DEBUGGING = false;

typedef struct QAJ4C_TEST_DEF {
    const char* subject_name;
    const char* test_name;
    void (*test_func)( void );
    struct QAJ4C_TEST_DEF* next;
    char* failure_message;
    char* error_message;
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
    tmp->failure_message = NULL;
    tmp->error_message = NULL;
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
    char buff[256];
    buff[0] = '\0';

    QAJ4C_register_fatal_error_function(QAJ4C_ERR_FUNCTION);

#ifndef _WIN32
    if ( DEBUGGING ) {
#endif
        test->test_func();
        tests_passed++;
#ifndef _WIN32
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
                if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
                    tests_passed++;
                } else {
                    if (WIFEXITED(status)) {
                        snprintf(buff, ARRAY_COUNT(buff), "Test exited with code %d", WEXITSTATUS(status));
                        test->failure_message = strdup(buff);
                    } else if (WIFSIGNALED(status)) {
                        snprintf(buff, ARRAY_COUNT(buff), "Test exited with signal %d", WTERMSIG(status));
                        test->error_message = strdup(buff);
                    } else {
                        continue;
                    }
                    tests_failed++;
                }
            }
            test->execution += time(NULL);
        }
    }
#endif

    return 0;
}

/**
 * Ensure the library will crash in case an error occurs.
 */
void custom_error_function() {
    raise(6);
}

#define STATIC_FWRITE(str, file) fwrite(str, sizeof(char), ARRAY_COUNT(str), file)

struct test_suite_stats {
    size_t fails = 0;
    size_t pass = 0;
    size_t errors = 0;
    size_t total_time = 0;

    void reset() {
        fails = 0;
        pass = 0;
        errors = 0;
        total_time = 0;
    }
};

void flush_test_suite( FILE* fp, const char* suite_name, char* buff, size_t buff_size, test_suite_stats* stats) {
    char testsuite_buffer[1024];
    int w = snprintf(testsuite_buffer, ARRAY_COUNT(testsuite_buffer), R"(<testsuite errors="%zu" failures="%zu" name="%s" skips="%zu" tests="%zu" time="%zu">%c)", stats->errors, stats->fails, suite_name, (size_t)0, (size_t)(stats->errors + stats->fails + stats->pass), stats->total_time, '\n');
    fwrite(testsuite_buffer, w, sizeof(char), fp);
    fwrite(buff, sizeof(char), buff_size , fp);
    fwrite("</testsuite>\n", sizeof(char), ARRAY_COUNT("</testsuite>\n") - 1, fp);
    fflush(fp);
}

int main( int argc, char **argv ) {
    // output=xml:gtestresults.xml
    static const char* gtest_xml_notation = "xml:";

    const char* filename = "test.xml";
    for ( int i = 0; i < argc; ++i ) {
        char* candidate = NULL;
        if ( strstr(argv[i], "--debug") == argv[i] ) {
            DEBUGGING = true;
        } else if( strstr(argv[i], "output=") != NULL) {
            candidate = (strstr(argv[i], "output=") + strlen("output="));
        } else if(strstr(argv[i], "output") != NULL) {
            if (i + 1 < argc) {
                candidate = argv[i+1];
            }
        }
        if( candidate != NULL ) {
            if ( strncmp(candidate, gtest_xml_notation, strlen(gtest_xml_notation)) == 0) {
                filename = candidate + strlen(gtest_xml_notation);
            }
            else{
                filename = candidate;
            }
        }
    }

    QAJ4C_register_fatal_error_function(custom_error_function);

    const char* all_suites_start = "<testsuites name=\"AllTests\">\n";
    char all_suites_end[] = "</testsuites>\n";
    const char* current_testsuite_name = NULL;
    test_suite_stats test_suite_stats;


    FILE* out = fopen(filename, "w");
    fwrite(all_suites_start, sizeof(char), strlen(all_suites_start), out);
    fflush(out);
    uint32_t testsuite_content_buffer_size = 4096;
    char* testsuite_content_buffer = (char*)realloc(0, testsuite_content_buffer_size);
    int pos = 0;
    char testsuite_buffer[1024];

    QAJ4C_TEST_DEF* curr_test = TOP_POINTER;
    while (curr_test != NULL) {
        fork_and_run(curr_test);

        if (current_testsuite_name == NULL) {
            current_testsuite_name = curr_test->subject_name;
        } else if (strcmp(current_testsuite_name, curr_test->subject_name) != 0) {
            flush_test_suite(out, current_testsuite_name, testsuite_content_buffer, pos, &test_suite_stats);
            pos = 0;
            current_testsuite_name = curr_test->subject_name;
            test_suite_stats.reset();
        }
        pos += snprintf(testsuite_content_buffer + pos, testsuite_content_buffer_size, R"(<testcase classname="%s" name="%s" time="%zu">)", curr_test->subject_name, curr_test->test_name, curr_test->execution);
        if ( curr_test->failure_message != NULL ) {
            pos += snprintf(testsuite_content_buffer + pos, testsuite_content_buffer_size, R"(<failure message="test failure">%s</failure>)", curr_test->failure_message);
            test_suite_stats.fails += 1;
        } else if( curr_test->error_message != NULL ) {
            pos += snprintf(testsuite_content_buffer + pos, testsuite_content_buffer_size, R"(<error message="test failure">%s</error>)", curr_test->error_message);
            test_suite_stats.errors += 1;
        } else {
            test_suite_stats.pass += 1;
        }
        pos += snprintf(testsuite_content_buffer + pos, testsuite_content_buffer_size, R"(</testcase>)");

        test_suite_stats.total_time += curr_test->execution;

        if (testsuite_content_buffer_size - pos < 1024) {
            testsuite_content_buffer = (char*)realloc(testsuite_content_buffer, testsuite_content_buffer_size * 2);
            testsuite_content_buffer_size *= 2;
        }
        free( curr_test->error_message );
        free( curr_test->failure_message );

        auto tmp = curr_test;
        curr_test = curr_test->next;
        free ( tmp );
    }

    flush_test_suite(out, current_testsuite_name, testsuite_content_buffer, pos, &test_suite_stats);

    printf("%d tests of %d total tests passed (%d failed)\n", tests_passed, tests_total, tests_failed);

    fwrite("</testsuites>", sizeof(char), ARRAY_COUNT("</testsuites>") - 1, out);

    fclose(out);
    free(testsuite_content_buffer);
    exit(tests_failed > 0);
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

TEST(SimpleParsingTests, ParseObjectWithOneNumericMemberNoOptions) {
    const char json[] = R"({"id":1})";

    const QAJ4C_Value* value = QAJ4C_parse_dynamic(json, realloc);
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
    assert(QAJ4C_string_equals(object_entry, "blah"));
    free((void*)value);
}

TEST(SimpleParsingTests, ParseObjectWithOneStringVeryLongMember) {
    const char json[] = R"({"name":"blahblubbhubbeldipup"})";
    const QAJ4C_Value* value = QAJ4C_parse_opt_dynamic(json, ARRAY_COUNT(json), 0, realloc);
    assert(QAJ4C_is_object(value));
    assert(QAJ4C_object_size(value) == 1);

    const QAJ4C_Value* object_entry = QAJ4C_object_get(value, "name");
    assert(QAJ4C_is_string(object_entry));
    assert(QAJ4C_string_equals(object_entry, "blahblubbhubbeldipup"));
    free((void*)value);
}

TEST(SimpleParsingTests, ParseObjectWithOneStringVeryLongMemberInsitu) {
    char json[] = R"({"name":"blahblubbhubbeldipup"})";
    char buffer[64];
    const QAJ4C_Value* value;

    size_t required = ARRAY_COUNT(buffer);

    required = QAJ4C_parse_opt_insitu(json, ARRAY_COUNT(json), 0, (void*)buffer, required, &value);
    assert(QAJ4C_is_object(value));
    assert(QAJ4C_object_size(value) == 1);
    assert(required == sizeof(QAJ4C_Value) + sizeof(QAJ4C_Member));

    const QAJ4C_Value* object_entry = QAJ4C_object_get(value, "name");
    assert(QAJ4C_is_string(object_entry));
    assert(QAJ4C_string_equals(object_entry, "blahblubbhubbeldipup"));
}

TEST(SimpleParsingTests, ParseStringWithNewLine) {
    char json[] = R"(["Hello\nWorld"])";
    const QAJ4C_Value* value = QAJ4C_parse_opt_dynamic(json, ARRAY_COUNT(json), 0, realloc);
    assert(QAJ4C_is_array(value));
    assert(QAJ4C_array_size(value) == 1);
    const QAJ4C_Value* array_entry = QAJ4C_array_get(value, 0);
    assert(QAJ4C_is_string(array_entry));

    // Now compare with the non-raw string (as the \n should be interpeted)
    assert(QAJ4C_string_equals(array_entry, "Hello\nWorld"));
    free((void*)value);
}

TEST(SimpleParsingTests, ParseStringWithNewLineUnicode) {
    char json[] = R"(["Hello\u000AWorld"])";
    char expected[] = "Hello\nWorld";
    const QAJ4C_Value* value = QAJ4C_parse_opt_dynamic(json, ARRAY_COUNT(json), 0, realloc);
    assert(QAJ4C_is_array(value));
    assert(QAJ4C_array_size(value) == 1);
    const QAJ4C_Value* array_entry = QAJ4C_array_get(value, 0);
    assert(QAJ4C_is_string(array_entry));

    // Now compare with the non-raw string (as the \n should be interpeted)
    assert(QAJ4C_get_string_length(array_entry) == strlen(expected));
    assert(QAJ4C_string_equals(array_entry, expected));

    free((void*)value);
}

TEST(SimpleParsingTests, ParseStringWithEndOfStringUnicode) {
    char json[] = R"(["Hello\u0000World"])";
    const QAJ4C_Value* value = QAJ4C_parse_opt_dynamic(json, ARRAY_COUNT(json), 0, realloc);
    assert(QAJ4C_is_array(value));
    assert(QAJ4C_array_size(value) == 1);
    const QAJ4C_Value* array_entry = QAJ4C_array_get(value, 0);
    assert(QAJ4C_is_string(array_entry));

    // Now compare with the non-raw string (as the \n should be interpeted)

    const char* result_str = QAJ4C_get_string(array_entry);

    const char expected[] = "Hello\0World";
    size_t expected_len = ARRAY_COUNT(expected) - 1; // strlen will not work
    assert(expected_len == QAJ4C_get_string_length(array_entry));
    assert(QAJ4C_string_equals_n(array_entry, expected, expected_len));

    free((void*)value);
}

TEST(SimpleParsingTests, ParseDollarSign) {
    const char json[] = R"(["\u0024"])";
    const char expected[] = "$";
    const QAJ4C_Value* value = QAJ4C_parse_opt_dynamic(json, ARRAY_COUNT(json), 0, realloc);
    assert(QAJ4C_is_array(value));
    assert(QAJ4C_array_size(value) == 1);
    const QAJ4C_Value* array_entry = QAJ4C_array_get(value, 0);
    assert(QAJ4C_is_string(array_entry));

    assert(QAJ4C_string_equals(array_entry, expected));
    assert(QAJ4C_get_string_length(array_entry) == strlen(expected));

    free((void*)value);
}

TEST(SimpleParsingTests, ParseJapaneseTeaSign) {

    const char json[] = R"(["\u8336"])";
    const char expected[] = "茶";
    const QAJ4C_Value* value = QAJ4C_parse_opt_dynamic(json, ARRAY_COUNT(json), 0, realloc);
    assert(QAJ4C_is_array(value));
    assert(QAJ4C_array_size(value) == 1);
    const QAJ4C_Value* array_entry = QAJ4C_array_get(value, 0);
    assert(QAJ4C_is_string(array_entry));

    assert(QAJ4C_string_equals(array_entry, expected));
    assert(QAJ4C_get_string_length(array_entry) == strlen(expected));

    free((void*)value);
}

TEST(SimpleParsingTests, ParseYenSign) {

    const char json[] = R"(["\u00A5"])";
    const char expected[] = "¥";
    const QAJ4C_Value* value = QAJ4C_parse_opt_dynamic(json, ARRAY_COUNT(json), 0, realloc);
    assert(QAJ4C_is_array(value));
    assert(QAJ4C_array_size(value) == 1);
    const QAJ4C_Value* array_entry = QAJ4C_array_get(value, 0);
    assert(QAJ4C_is_string(array_entry));

    assert(QAJ4C_string_equals(array_entry, expected));
    assert(QAJ4C_get_string_length(array_entry) == strlen(expected));

    free((void*)value);
}

TEST(SimpleParsingTests, ParseBigUtf16) {
    const char json[] = R"(["\uD834\uDD1E"])";
    const char expected[] = "𝄞";
    const QAJ4C_Value* value = QAJ4C_parse_opt_dynamic(json, ARRAY_COUNT(json), 0, realloc);
    assert(QAJ4C_is_array(value));
    assert(QAJ4C_array_size(value) == 1);
    const QAJ4C_Value* array_entry = QAJ4C_array_get(value, 0);
    assert(QAJ4C_is_string(array_entry));

    assert(QAJ4C_string_equals(array_entry, expected));
    assert(QAJ4C_get_string_length(array_entry) == strlen(expected));

    free((void*)value);
}

TEST(SimpleParsingTests, ParseStringWithEscapedQuotes) {
    char json[] = R"(["Hello\"World"])";
    const QAJ4C_Value* value = QAJ4C_parse_opt_dynamic(json, ARRAY_COUNT(json), 0, realloc);
    assert(QAJ4C_is_array(value));
    assert(QAJ4C_array_size(value) == 1);
    const QAJ4C_Value* array_entry = QAJ4C_array_get(value, 0);
    assert(QAJ4C_is_string(array_entry));

    // Now compare with the non-raw string (as the \n should be interpeted)
    assert(QAJ4C_string_equals(array_entry, "Hello\"World"));
    free((void*)value);
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
    const char json[] = R"([1,2,3,-4,5,+6])";
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

TEST(SimpleParsingTests, ParseArraysWithinArray) {
    const char json[] = "[[],[],[],[],[],[],[],[],[]]";
    const QAJ4C_Value* value = QAJ4C_parse_opt_dynamic(json, ARRAY_COUNT(json), 0, realloc);
    assert(QAJ4C_is_array(value));
    assert(QAJ4C_array_size(value) == 9);

    assert(0 == QAJ4C_array_size(QAJ4C_array_get(value, 0)));
    assert(0 == QAJ4C_array_size(QAJ4C_array_get(value, 1)));
    assert(0 == QAJ4C_array_size(QAJ4C_array_get(value, 2)));
    assert(0 == QAJ4C_array_size(QAJ4C_array_get(value, 3)));
    assert(0 == QAJ4C_array_size(QAJ4C_array_get(value, 4)));
    assert(0 == QAJ4C_array_size(QAJ4C_array_get(value, 5)));
    assert(0 == QAJ4C_array_size(QAJ4C_array_get(value, 6)));
    assert(0 == QAJ4C_array_size(QAJ4C_array_get(value, 7)));
    assert(0 == QAJ4C_array_size(QAJ4C_array_get(value, 8)));

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

TEST(SimpleParsingTests, ParseObjectTrailingComma) {
    const char json[] = R"({"id":1,"name":"foo",})";
    const QAJ4C_Value* value = QAJ4C_parse_opt_dynamic(json, ARRAY_COUNT(json), 0, realloc);
    assert(QAJ4C_is_object(value));
    assert(QAJ4C_object_size(value) == 2);

    free((void*)value);
}

TEST(SimpleParsingTests, ParseObjectCheckOptimized) {
    const char json[] = R"({"id":1,"name":"foo","age":39,"job":null,"role":"admin"})";
    const QAJ4C_Value* value = QAJ4C_parse_opt_dynamic(json, ARRAY_COUNT(json), 0, realloc);
    assert(QAJ4C_is_object(value));
    assert(QAJ4C_object_size(value) == 5);
    assert(QAJ4C_get_internal_type(value) == QAJ4C_OBJECT_SORTED);

    free((void*)value);
}

TEST(SimpleParsingTests, ParseObjectCheckNonOptimized) {
    const char json[] = R"({"id":1,"name":"foo","age":39,"job":null,"role":"admin"})";
    const QAJ4C_Value* value = QAJ4C_parse_opt_dynamic(json, ARRAY_COUNT(json), QAJ4C_PARSE_OPTS_DONT_SORT_OBJECT_MEMBERS, realloc);
    assert(QAJ4C_is_object(value));
    assert(QAJ4C_object_size(value) == 5);
    assert(QAJ4C_get_internal_type(value) == QAJ4C_OBJECT);

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


TEST(SimpleParsingTests, ParseLineCommentInString) {
    const char json[] = R"(["Hallo//Welt"])";
    const QAJ4C_Value* value = QAJ4C_parse_opt_dynamic(json, ARRAY_COUNT(json), 0, realloc);
    assert(!QAJ4C_is_error(value));
    free((void*)value);
}

TEST(SimpleParsingTests, ParseDoubleValues) {
    const char json[] = R"([0.123456789e-12, 1.234567890E+34, 23456789012E66, -9876.543210])";
    const QAJ4C_Value* value = QAJ4C_parse_opt_dynamic(json, ARRAY_COUNT(json), 0, realloc);
    assert(QAJ4C_is_array(value));
    assert(QAJ4C_is_double(QAJ4C_array_get(value, 0)));
    assert(QAJ4C_is_double(QAJ4C_array_get(value, 1)));
    assert(QAJ4C_is_double(QAJ4C_array_get(value, 2)));
    assert(QAJ4C_is_double(QAJ4C_array_get(value, 3)));
    free((void*)value);
}

TEST(SimpleParsingTests, ParseNumericEValues) {
    const char json[] = R"([1e1,0.1e1])";
    const QAJ4C_Value* value = QAJ4C_parse_opt_dynamic(json, ARRAY_COUNT(json), 0, realloc);
    assert(QAJ4C_is_array(value));
    assert(QAJ4C_is_double(QAJ4C_array_get(value, 0)));
    assert(QAJ4C_is_double(QAJ4C_array_get(value, 1)));
    free((void*)value);
}

TEST(SimpleParsingTests, ParseUint64Max) {
    const char json[] = R"([18446744073709551615])";
    const QAJ4C_Value* value = QAJ4C_parse_opt_dynamic(json, ARRAY_COUNT(json), 0, realloc);
    assert(QAJ4C_is_array(value));
    const QAJ4C_Value* array_value = QAJ4C_array_get(value, 0);

    assert(!QAJ4C_is_uint(array_value));
    assert(!QAJ4C_is_int(array_value));
    assert(!QAJ4C_is_int64(array_value));
    assert(QAJ4C_is_uint64(array_value));
    assert(QAJ4C_is_double(array_value));

    assert(UINT64_MAX == QAJ4C_get_uint64(array_value));
    free((void*)value);
}

TEST(SimpleParsingTests, ParseUint64MaxPlus1) {
    const char json[] = R"([18446744073709551616])";
    const QAJ4C_Value* value = QAJ4C_parse_opt_dynamic(json, ARRAY_COUNT(json), 0, realloc);
    assert(QAJ4C_is_array(value));
    assert(!QAJ4C_is_uint(QAJ4C_array_get(value, 0)));
    assert(!QAJ4C_is_int(QAJ4C_array_get(value, 0)));
    assert(!QAJ4C_is_int64(QAJ4C_array_get(value, 0)));
    assert(!QAJ4C_is_uint64(QAJ4C_array_get(value, 0)));
    assert(QAJ4C_is_double(QAJ4C_array_get(value, 0)));
    free((void*)value);
}

TEST(SimpleParsingTests, ParseStringUnicodeOverlapsInlineStringLimit) {
    char json[QAJ4C_INLINE_STRING_SIZE + 32]; // oversize the json a little
    int index = 0;
    json[index++] = '[';
    json[index++] = '"';
    for(int i = 0;i < QAJ4C_INLINE_STRING_SIZE; i++) {
        json[index++] = 'x';
    }
    json[index++] = '\\';
    json[index++] = 'u';
    json[index++] = 'F';
    json[index++] = 'f';
    json[index++] = 'f';
    json[index++] = 'F';
    json[index++] = '"';
    json[index++] = ']';
    json[index++] = '\0';

    size_t buff_size = QAJ4C_calculate_max_buffer_size(json);
    uint8_t buff[buff_size];
    const QAJ4C_Value* value;
    size_t write_size = QAJ4C_parse(json, buff, buff_size, &value);
    assert(write_size == buff_size);

    assert(QAJ4C_is_array(value));
    const QAJ4C_Value* entry = QAJ4C_array_get(value, 0);
    assert(QAJ4C_is_string(entry));
    assert(QAJ4C_STRING == QAJ4C_get_internal_type(entry));
}

TEST(SimpleParsingTests, ParseStringWithAllEscapeCharacters) {
    const char json[]  = R"(["\" \\ \/ \b \f \n \r \t"])";
    const QAJ4C_Value* value = QAJ4C_parse_opt_dynamic(json, ARRAY_COUNT(json), 0, realloc);

    assert(QAJ4C_is_array(value));

    const QAJ4C_Value* entry = QAJ4C_array_get(value, 0);
    assert(QAJ4C_is_string(entry));

    const char* string_value = QAJ4C_get_string(entry);
    assert(strstr(string_value, "\"") != NULL);
    assert(strstr(string_value, "\\") != NULL);
    assert(strstr(string_value, "\\/") == NULL); // escaped for json but not for c
    assert(strstr(string_value, "/") != NULL);
    assert(strstr(string_value, "\b") != NULL);
    assert(strstr(string_value, "\f") != NULL);
    assert(strstr(string_value, "\n") != NULL);
    assert(strstr(string_value, "\r") != NULL);
    assert(strstr(string_value, "\t") != NULL);

    free((void*)value);
}

TEST(ErrorHandlingTests, ParseIncompleteObject) {
    const char json[] = R"({)";
    const QAJ4C_Value* value = QAJ4C_parse_opt_dynamic(json, ARRAY_COUNT(json), 0, realloc);
    assert(QAJ4C_is_error(value));
    assert(QAJ4C_error_get_errno(value) == QAJ4C_ERROR_JSON_MESSAGE_TRUNCATED);
    assert(QAJ4C_error_get_json(value) == json);
    assert(QAJ4C_error_get_json_pos(value) == 1);
    free((void*)value);
}

TEST(ErrorHandlingTests, ParseInvalidComment) {
    const char json[] = R"([/# #/])";
    const QAJ4C_Value* value = QAJ4C_parse_opt_dynamic(json, ARRAY_COUNT(json), 0, realloc);
    assert(QAJ4C_is_error(value));
    assert(QAJ4C_error_get_errno(value) == QAJ4C_ERROR_UNEXPECTED_CHAR);
    free((void*)value);
}

TEST(ErrorHandlingTests, ParseNeverEndingLineComment) {
    const char json[] = R"([// ])";
    const QAJ4C_Value* value = QAJ4C_parse_opt_dynamic(json, ARRAY_COUNT(json), 0, realloc);
    assert(QAJ4C_is_error(value));
    assert(QAJ4C_error_get_errno(value) == QAJ4C_ERROR_JSON_MESSAGE_TRUNCATED);
    free((void*)value);
}

TEST(ErrorHandlingTests, ParseNeverEndingLineBlockComment) {
    const char json[] = R"([/* ])";
    const QAJ4C_Value* value = QAJ4C_parse_opt_dynamic(json, ARRAY_COUNT(json), 0, realloc);
    assert(QAJ4C_is_error(value));
    assert(QAJ4C_error_get_errno(value) == QAJ4C_ERROR_JSON_MESSAGE_TRUNCATED);
    free((void*)value);
}

TEST(ErrorHandlingTests, ParseIncompleteArray) {
    const char json[] = R"([)";
    const QAJ4C_Value* value = QAJ4C_parse_opt_dynamic(json, ARRAY_COUNT(json), 0, realloc);
    assert(QAJ4C_is_error(value));
    assert(QAJ4C_error_get_errno(value) == QAJ4C_ERROR_JSON_MESSAGE_TRUNCATED);
    assert(QAJ4C_error_get_json(value) == json);
    assert(QAJ4C_error_get_json_pos(value) == 1);
    free((void*)value);
}

TEST(ErrorHandlingTests, ParseIncompleteString) {
    const char json[] = R"(")";
    const QAJ4C_Value* value = QAJ4C_parse_opt_dynamic(json, ARRAY_COUNT(json), 0, realloc);
    assert(QAJ4C_is_error(value));
    assert(QAJ4C_error_get_errno(value) == QAJ4C_ERROR_JSON_MESSAGE_TRUNCATED);
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

TEST(ErrorHandlingTests, IncompleteNumberAfterComma) {
    char json[] = R"([1.])";
    const QAJ4C_Value* value = QAJ4C_parse_opt_dynamic(json, ARRAY_COUNT(json), 0, realloc);
    assert(QAJ4C_is_error(value));
    assert(QAJ4C_error_get_errno(value) == QAJ4C_ERROR_INVALID_NUMBER_FORMAT);
    free((void*)value);
}

TEST(ErrorHandlingTests, InvalidUnicodeSequence) {
    char json[] = R"(["\u99XA"])";
    const QAJ4C_Value* value = QAJ4C_parse_opt_dynamic(json, ARRAY_COUNT(json), 0, realloc);
    assert(QAJ4C_is_error(value));
    assert(QAJ4C_error_get_errno(value) == QAJ4C_ERROR_INVALID_UNICODE_SEQUENCE);
    free((void*)value);
}

/**
 * In this test it is verified that setting an array in case no memory is available anymore
 * will invoke the fatal error handler function and not cause segmentation faults or whatever.
 */
TEST(ErrorHandlingTests, BuilderOverflowArray) {
    QAJ4C_Builder builder;
    QAJ4C_builder_init(&builder, NULL, 0);

    static bool called = false;
    auto lambda = [](){
        called = true;
    };
    QAJ4C_register_fatal_error_function(lambda);

    QAJ4C_Value value;
    QAJ4C_set_array(&value, 5, &builder);

    assert(called == true);
    assert(QAJ4C_is_array(&value));
    assert(0 == QAJ4C_array_size(&value));
}

/**
 * In this test it is verified that setting an object in case no memory is available anymore
 * will invoke the fatal error handler function and not cause segmentation faults or whatever.
 */
TEST(ErrorHandlingTests, BuilderOverflowObject) {
    QAJ4C_Builder builder;
    QAJ4C_builder_init(&builder, NULL, 0);

    static bool called = false;
    auto lambda = [](){
        called = true;
    };
    QAJ4C_register_fatal_error_function(lambda);

    QAJ4C_Value value;
    QAJ4C_set_object(&value, 5, &builder);
    assert(called == true);
    assert(QAJ4C_is_object(&value));
    assert(0 == QAJ4C_object_size(&value));
}

/**
 * In this test it is verified that setting a string (by copy) in case no memory is available anymore
 * will invoke the fatal error handler function and not cause segmentation faults or whatever.
 */
TEST(ErrorHandlingTests, BuilderOverflowString) {
    QAJ4C_Builder builder;
    QAJ4C_builder_init(&builder, NULL, 0);

    static bool called = false;
    auto lambda = [](){
        called = true;
    };
    QAJ4C_register_fatal_error_function(lambda);

    QAJ4C_Value value;
    QAJ4C_set_string_copy(&value, "abcdefghijklmnopqrstuvwxyz", &builder);
    assert(called == true);
    assert(QAJ4C_is_string(&value));
    assert(0 == QAJ4C_get_string_length(&value));
    assert(strcmp("", QAJ4C_get_string(&value)) == 0);
}

/**
 * In this test the string will not directly overflow the buffer but will cause an overlap
 * with the object memory space.
 */
TEST(ErrorHandlingTests, BuilderOverflowString2) {
    QAJ4C_Builder builder;
    QAJ4C_builder_init(&builder, NULL, 256);
    builder.cur_obj_pos = builder.cur_str_pos;

    static bool called = false;
    auto lambda = [](){
        called = true;
    };
    QAJ4C_register_fatal_error_function(lambda);

    QAJ4C_Value value;
    QAJ4C_set_string_copy(&value, "abcdefghijklmnopqrstuvwxyz", &builder);
    assert(called == true);
    assert(QAJ4C_is_string(&value));
    assert(0 == QAJ4C_get_string_length(&value));
    assert(strcmp("", QAJ4C_get_string(&value)) == 0);
}


TEST(ErrorHandlingTests, GetTypeWithNullpointer) {
    assert(QAJ4C_TYPE_NULL == QAJ4C_get_type( NULL ));
}

TEST(ErrorHandlingTests, InvalidConstantJsons) {
    uint8_t buff[256];
    const char* invalid_strings[] =
    {
            "n", "nu", "nul",
            "t", "tr", "tru",
            "f", "fa", "fal", "fals"
            "nill", "nukl", "nulk",
            "tuue", "troe", "truu",
            "filse","fakse","falze","falsu"
    };
    // Non of these string should result in something else then an error!
    for (const char* invalid_string : invalid_strings) {
        const QAJ4C_Value* val = nullptr;
        QAJ4C_parse(invalid_string, buff, ARRAY_COUNT(buff), &val);
        assert(QAJ4C_is_error(val));
    }
}

/**
 * This verifies that the handling for a truncated buffer will also work in case
 * the start of an element is truncated!
 */
TEST(ErrorHandlingTests, ParseTruncatedString) {
    const char json[] = R"({"id":123, "name": )";
    uint8_t buff[256];
    const QAJ4C_Value* val = nullptr;
    QAJ4C_parse(json, buff, ARRAY_COUNT(buff), &val);

    assert(QAJ4C_is_error(val));
    assert(QAJ4C_error_get_errno(val) == QAJ4C_ERROR_JSON_MESSAGE_TRUNCATED);
}

TEST(ErrorHandlingTests, TabInJsonString) {
    const char json[] = R"({"id":123, "name": ")" "\t" "\"";
    uint8_t buff[256];
    const QAJ4C_Value* val = nullptr;
    QAJ4C_parse(json, buff, ARRAY_COUNT(buff), &val);

    assert(QAJ4C_is_error(val));
    assert(QAJ4C_error_get_errno(val) == QAJ4C_ERROR_UNEXPECTED_CHAR);
}

TEST(ErrorHandlingTests, InvalidEscapeCharacterInString) {
    const char json[] = R"({"id":123, "name": "\x")";
    uint8_t buff[256];
    const QAJ4C_Value* val = nullptr;
    QAJ4C_parse(json, buff, ARRAY_COUNT(buff), &val);

    assert(QAJ4C_is_error(val));
    assert(QAJ4C_error_get_errno(val) == QAJ4C_ERROR_INVALID_ESCAPE_SEQUENCE);
}

/**
 * This test verifies that a unicode character is not accepted in case it is incomplete.
 */
TEST(ErrorHandlingTests, InvalidLongUnicodeSequenceNoAppendingSurrogate) {
    const char json[] = R"({"id":123, "name": "\uD800")";
    uint8_t buff[256];
    const QAJ4C_Value* val = nullptr;
    QAJ4C_parse(json, buff, ARRAY_COUNT(buff), &val);

    assert(QAJ4C_is_error(val));
    assert(QAJ4C_error_get_errno(val) == QAJ4C_ERROR_INVALID_UNICODE_SEQUENCE);
}

/**
 * The same as the other but now a \ has been added to potentially confuse the parser.
 */
TEST(ErrorHandlingTests, InvalidLongUnicodeSequenceIncompleteAppendingSurrogate) {
    const char json[] = R"({"id":123, "name": "\uD800\")";
    uint8_t buff[256];
    const QAJ4C_Value* val = nullptr;
    QAJ4C_parse(json, buff, ARRAY_COUNT(buff), &val);

    assert(QAJ4C_is_error(val));
    assert(QAJ4C_error_get_errno(val) == QAJ4C_ERROR_INVALID_UNICODE_SEQUENCE);
}

/**
 * The same as the other but now only three digits are present.
 */
TEST(ErrorHandlingTests, InvalidLongUnicodeSequenceIncompleteAppendingSurrogate2) {
    const char json[] = R"({"id":123, "name": "\uD800\udC0")";
    uint8_t buff[256];
    const QAJ4C_Value* val = nullptr;
    QAJ4C_parse(json, buff, ARRAY_COUNT(buff), &val);

    assert(QAJ4C_is_error(val));
    assert(QAJ4C_error_get_errno(val) == QAJ4C_ERROR_INVALID_UNICODE_SEQUENCE);
}

/**
 * The same as the other but now only three digits are present.
 */
TEST(ErrorHandlingTests, InvalidLongUnicodeSequenceInvalidLowSurrogate) {
    const char json[] = R"({"id":123, "name": "\uD800\u0123")";
    uint8_t buff[256];
    const QAJ4C_Value* val = nullptr;
    QAJ4C_parse(json, buff, ARRAY_COUNT(buff), &val);

    assert(QAJ4C_is_error(val));
    assert(QAJ4C_error_get_errno(val) == QAJ4C_ERROR_INVALID_UNICODE_SEQUENCE);
}

/**
 * This test verifies that a unicode character is not accepted in case it is incomplete.
 */
TEST(ErrorHandlingTests, InvalidLongUnicodeSequenceNoHighSurrogate) {
    const char json[] = R"({"id":123, "name": "\uDc00")";
    uint8_t buff[256];
    const QAJ4C_Value* val = nullptr;
    QAJ4C_parse(json, buff, ARRAY_COUNT(buff), &val);

    assert(QAJ4C_is_error(val));
    assert(QAJ4C_error_get_errno(val) == QAJ4C_ERROR_INVALID_UNICODE_SEQUENCE);
}

/**
 * This verifies that the handling for a truncated buffer will also work in case
 * the start of an element is truncated!
 */
TEST(ErrorHandlingTests, ParseObjectWithoutQuotesOnValue) {
    const char json[] = R"({"id":123, "name": hossa})";
    uint8_t buff[256];
    const QAJ4C_Value* val = nullptr;
    QAJ4C_parse(json, buff, ARRAY_COUNT(buff), &val);

    assert(QAJ4C_is_error(val));
    assert(QAJ4C_error_get_errno(val) == QAJ4C_ERROR_UNEXPECTED_CHAR);
}

TEST(ErrorHandlingTests, ParseObjectMissingComma) {
    const char json[] = R"({"id":123 "name": "hossa"})";
    uint8_t buff[256];
    const QAJ4C_Value* val = nullptr;
    QAJ4C_parse(json, buff, ARRAY_COUNT(buff), &val);

    assert(QAJ4C_is_error(val));
    assert(QAJ4C_error_get_errno(val) == QAJ4C_ERROR_MISSING_COMMA);
}

TEST(ErrorHandlingTests, ParseObjectMissingColon) {
    const char json[] = R"({"id":123, "name" "hossa"})";
    uint8_t buff[256];
    const QAJ4C_Value* val = nullptr;
    QAJ4C_parse(json, buff, ARRAY_COUNT(buff), &val);

    assert(QAJ4C_is_error(val));
    assert(QAJ4C_error_get_errno(val) == QAJ4C_ERROR_MISSING_COLON);
}

TEST(ErrorHandlingTests, ParseArrayMissingComma) {
    const char json[] = R"([1, 2 3])";
    uint8_t buff[256];
    const QAJ4C_Value* val = nullptr;
    QAJ4C_parse(json, buff, ARRAY_COUNT(buff), &val);

    assert(QAJ4C_is_error(val));
    assert(QAJ4C_error_get_errno(val) == QAJ4C_ERROR_MISSING_COMMA);
}

TEST(ErrorHandlingTests, ParseDoubleWithInvalidExponentFormat) {
    const char json[] = R"(1.2e)";
    uint8_t buff[256];
    const QAJ4C_Value* val = nullptr;
    QAJ4C_parse(json, buff, ARRAY_COUNT(buff), &val);

    assert(QAJ4C_is_error(val));
    assert(QAJ4C_error_get_errno(val) == QAJ4C_ERROR_INVALID_NUMBER_FORMAT);
}

TEST(ErrorHandlingTests, InvalidBufferSizeToReportError) {
    uint8_t buff[sizeof(QAJ4C_Value) + sizeof(QAJ4C_Error_information) - 1];
    const char json[] = "";
    const QAJ4C_Value* val = nullptr;
    QAJ4C_parse(json, buff, ARRAY_COUNT(buff), &val);
    assert(QAJ4C_is_not_set(val)); // buffer is event too small to store the error
}

/**
 * In this test the first pass will trigger the error that the buffer is too small as
 * it cannot store more statistics and as realloc cannot be called the parsing fails.
 */
TEST(ErrorHandlingTests, BufferTooSmallToStoreStatstics) {
    uint8_t buff[sizeof(QAJ4C_Value) + sizeof(QAJ4C_Error_information)];
    const char json[] = "[[],[],[],[],[],[],[],[],[]]";
    const QAJ4C_Value* val = nullptr;
    QAJ4C_parse(json, buff, ARRAY_COUNT(buff), &val);
    assert(QAJ4C_is_error(val));
    assert(QAJ4C_error_get_errno(val) == QAJ4C_ERROR_STORAGE_BUFFER_TO_SMALL);
}

/**
 * In this test the first pass will trigger the error that the buffer is too small as
 * it cannot store more statistics and as realloc fails parsing fails as well.
 */
TEST(ErrorHandlingTests, BufferTooSmallToStoreStatsticsReallocFails) {
    const char json[] = "[[],[],[],[],[],[],[],[],[]]";
    static bool first = true;
    auto lambda = []( void *ptr, size_t size ) {
        if ( first ) {
            first = false;
            return realloc(ptr, size);
        }
        return (void*)NULL;
    };
    // just reduce the buffer size by one single byte
    const QAJ4C_Value* value = QAJ4C_parse_dynamic(json, lambda);
    assert(QAJ4C_is_error(value));

    assert(QAJ4C_error_get_errno(value) == QAJ4C_ERROR_ALLOCATION_ERROR);
}

/**
 * This test validates that the lookup on an object will not fail in case the object
 * is still untouched.
 */
TEST(ErrorHandlingTests, LookupMemberUninitializedObject) {
    uint8_t buff[256];
    QAJ4C_Builder builder;
    QAJ4C_builder_init(&builder, buff, ARRAY_COUNT(buff));

    QAJ4C_Value* value_ptr = QAJ4C_builder_get_document(&builder);
    QAJ4C_set_object(value_ptr, 4, &builder);

    assert(NULL == QAJ4C_object_get(value_ptr, "id"));
}

/**
 * This test will verify that in case an object is accessed incorrectly (with methods of a string or uint)
 * the default value will be returned in case a custom fatal function is set.
 */
TEST(DomObjectAccessTests, ObjectAccess) {
    QAJ4C_Builder builder;
    QAJ4C_Value value;
    QAJ4C_set_object(&value, 0, &builder);

    static int called = 0;
    auto lambda = [](){
        ++called;
    };
    QAJ4C_register_fatal_error_function(lambda);

    assert(0.0 == QAJ4C_get_double(&value));
    assert(1 == called);
    assert(0 == QAJ4C_get_uint(&value));
    assert(2 == called);
    assert(0 == QAJ4C_get_int(&value));
    assert(3 == called);
    assert(false == QAJ4C_get_bool(&value));
    assert(4 == called);
    assert(strcmp("",QAJ4C_get_string(&value))==0);
    assert(5 == called);
    assert(0 == QAJ4C_array_size (&value));
    assert(6 == called);
    assert(NULL == QAJ4C_array_get(&value, 0));
    assert(7 == called);

    // as the object is empty this should also fail!
    assert(NULL == QAJ4C_object_get_member(&value, 0));
    assert(8 == called);
    assert(NULL == QAJ4C_object_create_member_by_ref(&value, "blubb"));
    assert(9 == called);
}

/**
 * This test verifies that an incorrect object access will be triggering the error function
 * and will also not cause the program to crash if the error function will "return"
 */
TEST(DomObjectAccessTests, IncorrectObjectAccess) {
    static int called = 0;
    auto lambda = [](){
        ++called;
    };
    QAJ4C_register_fatal_error_function(lambda);

    int expected = 0;

    assert(!QAJ4C_is_object(NULL));

    assert(0 == QAJ4C_object_size(NULL));
    assert(++expected == called);
    assert(NULL == QAJ4C_object_get_member(NULL, 0));
    assert(++expected == called);
    assert(NULL == QAJ4C_object_get(NULL, "id"));
    assert(++expected == called);
    assert(NULL == QAJ4C_object_create_member_by_copy(NULL, "id", NULL));
    assert(++expected == called);
    assert(NULL == QAJ4C_object_create_member_by_ref(NULL, "id"));
    assert(++expected == called);
    QAJ4C_object_optimize(NULL);
    assert(++expected == called);
    assert(NULL == QAJ4C_member_get_key(NULL));
    assert(++expected == called);
    assert(NULL == QAJ4C_member_get_value(NULL));
    assert(++expected == called);
}

/**
 * This test will verify that in case an array is accessed incorrectly (with methods of a string or uint)
 * the default value will be returned in case a custom fatal function is set.
 */
TEST(DomObjectAccessTests, ArrayAccess) {
    QAJ4C_Builder builder;
    QAJ4C_Value value;
    QAJ4C_set_array(&value, 0, &builder);

    static int called = 0;
    auto lambda = [](){
        ++called;
    };
    QAJ4C_register_fatal_error_function(lambda);

    assert(0.0 == QAJ4C_get_double(&value));
    assert(1 == called);
    assert(0 == QAJ4C_get_uint(&value));
    assert(2 == called);
    assert(0 == QAJ4C_get_int(&value));
    assert(3 == called);
    assert(false == QAJ4C_get_bool(&value));
    assert(4 == called);
    assert(strcmp("",QAJ4C_get_string(&value))==0);
    assert(5 == called);
    assert(0 == QAJ4C_object_size (&value));
    assert(6 == called);
    assert(NULL == QAJ4C_object_get_member(&value, 0));
    assert(7 == called);
    assert(NULL == QAJ4C_object_create_member_by_ref(&value, "blubb"));
    assert(8 == called);

    // as the array has size 0 this should also fail!
    assert(NULL == QAJ4C_array_get(&value, 0));
    assert(9 == called);
    assert(NULL == QAJ4C_array_get_rw(&value, 0));
    assert(10 == called);
}

/**
 * This test verifies that an incorrect array access will be triggering the error function
 * and will also not cause the program to crash if the error function will "return"
 */
TEST(DomObjectAccessTests, IncorrectArrayAccess) {
    static int called = 0;
    auto lambda = [](){
        ++called;
    };
    QAJ4C_register_fatal_error_function(lambda);

    int expected = 0;

    assert(!QAJ4C_is_array(NULL));

    assert(0 == QAJ4C_array_size(NULL));
    assert(++expected == called);
    assert(NULL == QAJ4C_array_get(NULL, 0));
    assert(++expected == called);
    assert(NULL == QAJ4C_array_get_rw(NULL, 0));
    assert(++expected == called);
}

TEST(DomObjectAccessTests, Uint64Access) {
    QAJ4C_Value value;
    QAJ4C_set_uint64(&value, UINT64_MAX);
    static int called = 0;
    auto lambda = [](){
        ++called;
    };
    QAJ4C_register_fatal_error_function(lambda);
    QAJ4C_get_double(&value); // don't check the value as only the invocation of the error function is interesting.
    assert(0 == called);
    assert(UINT64_MAX == QAJ4C_get_uint64(&value));
    assert(0 == called);
    assert(0 == QAJ4C_get_int64(&value));
    assert(1 == called);
    assert(0 == QAJ4C_get_uint(&value));
    assert(2 == called);
    assert(0 == QAJ4C_get_int(&value));
    assert(3 == called);
    assert(false == QAJ4C_get_bool(&value));
    assert(4 == called);
    assert(strcmp("",QAJ4C_get_string(&value))==0);
    assert(5 == called);
    assert(0 == QAJ4C_object_size (&value));
    assert(6 == called);
    assert(NULL == QAJ4C_object_get_member(&value, 0));
    assert(7 == called);
    assert(NULL == QAJ4C_object_create_member_by_ref(&value, "blubb"));
    assert(8 == called);
    assert(NULL == QAJ4C_array_get(&value, 0));
    assert(9 == called);
}

TEST(DomObjectAccessTests, Uint32Access) {
    QAJ4C_Value value;
    QAJ4C_set_uint(&value, UINT32_MAX);
    static int called = 0;
    auto lambda = [](){
        ++called;
    };
    QAJ4C_register_fatal_error_function(lambda);
    QAJ4C_get_double(&value); // don't check the value as only the invocation of the error function is interesting.
    assert(0 == called);
    assert(UINT32_MAX == QAJ4C_get_int64(&value));
    assert(0 == called);
    assert(UINT32_MAX == QAJ4C_get_uint64(&value));
    assert(0 == called);
    assert(UINT32_MAX == QAJ4C_get_uint(&value));
    assert(0 == called);
    assert(0 == QAJ4C_get_int(&value));
    assert(1 == called);
    assert(false == QAJ4C_get_bool(&value));
    assert(2 == called);
    assert(strcmp("",QAJ4C_get_string(&value))==0);
    assert(3 == called);
    assert(0 == QAJ4C_object_size (&value));
    assert(4 == called);
    assert(NULL == QAJ4C_object_get_member(&value, 0));
    assert(5 == called);
    assert(NULL == QAJ4C_object_create_member_by_ref(&value, "blubb"));
    assert(6 == called);
    assert(NULL == QAJ4C_array_get(&value, 0));
    assert(7 == called);
}

TEST(DomObjectAccessTests, Int64Access) {
    QAJ4C_Value value;
    QAJ4C_set_int64(&value, INT64_MIN);
    static int called = 0;
    auto lambda = [](){
        ++called;
    };
    QAJ4C_register_fatal_error_function(lambda);
    QAJ4C_get_double(&value); // don't check the value as only the invocation of the error function is interesting.
    assert(0 == called);
    assert(INT64_MIN == QAJ4C_get_int64(&value));
    assert(0 == called);
    assert(0 == QAJ4C_get_uint64(&value));
    assert(1 == called);
    assert(0 == QAJ4C_get_uint(&value));
    assert(2 == called);
    assert(0 == QAJ4C_get_int(&value));
    assert(3 == called);
    assert(false == QAJ4C_get_bool(&value));
    assert(4 == called);
    assert(strcmp("",QAJ4C_get_string(&value))==0);
    assert(5 == called);
    assert(0 == QAJ4C_object_size (&value));
    assert(6 == called);
    assert(NULL == QAJ4C_object_get_member(&value, 0));
    assert(7 == called);
    assert(NULL == QAJ4C_object_create_member_by_ref(&value, "blubb"));
    assert(8 == called);
    assert(NULL == QAJ4C_array_get(&value, 0));
    assert(9 == called);
}


TEST(DomObjectAccessTests, Int64AccessMax) {
    QAJ4C_Value value;
    QAJ4C_set_int64(&value, INT64_MAX);
    static int called = 0;
    auto lambda = [](){
        ++called;
    };
    QAJ4C_register_fatal_error_function(lambda);
    QAJ4C_get_double(&value); // don't check the value as only the invocation of the error function is interesting.
    assert(0 == called);
    assert(INT64_MAX == QAJ4C_get_int64(&value));
    assert(0 == called);
    assert(INT64_MAX == QAJ4C_get_uint64(&value));
    assert(0 == called);
    assert(0 == QAJ4C_get_uint(&value));
    assert(1 == called);
    assert(0 == QAJ4C_get_int(&value));
    assert(2 == called);
    assert(false == QAJ4C_get_bool(&value));
    assert(3 == called);
    assert(strcmp("",QAJ4C_get_string(&value))==0);
    assert(4 == called);
    assert(0 == QAJ4C_object_size (&value));
    assert(5 == called);
    assert(NULL == QAJ4C_object_get_member(&value, 0));
    assert(6 == called);
    assert(NULL == QAJ4C_object_create_member_by_ref(&value, "blubb"));
    assert(7 == called);
    assert(NULL == QAJ4C_array_get(&value, 0));
    assert(8 == called);
}

TEST(DomObjectAccessTests, Int32Access) {
    QAJ4C_Value value;
    QAJ4C_set_int(&value, INT32_MIN);
    static int called = 0;
    auto lambda = [](){
        ++called;
    };
    QAJ4C_register_fatal_error_function(lambda);
    QAJ4C_get_double(&value); // don't check the value as only the invocation of the error function is interesting.
    assert(0 == called);
    assert(INT32_MIN == QAJ4C_get_int64(&value));
    assert(0 == called);
    assert(INT32_MIN == QAJ4C_get_int(&value));
    assert(0 == called);
    assert(0 == QAJ4C_get_uint64(&value));
    assert(1 == called);
    assert(0 == QAJ4C_get_uint(&value));
    assert(2 == called);
    assert(false == QAJ4C_get_bool(&value));
    assert(3 == called);
    assert(strcmp("",QAJ4C_get_string(&value))==0);
    assert(4 == called);
    assert(0 == QAJ4C_object_size (&value));
    assert(5 == called);
    assert(NULL == QAJ4C_object_get_member(&value, 0));
    assert(6 == called);
    assert(NULL == QAJ4C_object_create_member_by_ref(&value, "blubb"));
    assert(7 == called);
    assert(NULL == QAJ4C_array_get(&value, 0));
    assert(8 == called);
}

TEST(DomObjectAccessTests, Int32AccessMax) {
    QAJ4C_Value value;
    QAJ4C_set_int(&value, INT32_MAX);
    static int called = 0;
    auto lambda = [](){
        ++called;
    };
    QAJ4C_register_fatal_error_function(lambda);
    QAJ4C_get_double(&value); // don't check the value as only the invocation of the error function is interesting.
    assert(0 == called);
    assert(INT32_MAX == QAJ4C_get_int64(&value));
    assert(0 == called);
    assert(INT32_MAX == QAJ4C_get_int(&value));
    assert(0 == called);
    assert(INT32_MAX == QAJ4C_get_uint64(&value));
    assert(0 == called);
    assert(INT32_MAX == QAJ4C_get_uint(&value));
    assert(0 == called);
    assert(false == QAJ4C_get_bool(&value));
    assert(1 == called);
    assert(strcmp("",QAJ4C_get_string(&value))==0);
    assert(2 == called);
    assert(0 == QAJ4C_object_size (&value));
    assert(3 == called);
    assert(NULL == QAJ4C_object_get_member(&value, 0));
    assert(4 == called);
    assert(NULL == QAJ4C_object_create_member_by_ref(&value, "blubb"));
    assert(5 == called);
    assert(NULL == QAJ4C_array_get(&value, 0));
    assert(6 == called);
}

TEST(DomObjectAccessTests, DoubleAccess) {
    QAJ4C_Value value;
    QAJ4C_set_double(&value, 1.2345);
    static int called = 0;
    auto lambda = [](){
        ++called;
    };
    QAJ4C_register_fatal_error_function(lambda);
    QAJ4C_get_double(&value); // don't check the value as only the invocation of the error function is interesting.
    assert(0 == called);
    assert(0 == QAJ4C_get_int64(&value));
    assert(1 == called);
    assert(0 == QAJ4C_get_int(&value));
    assert(2 == called);
    assert(0 == QAJ4C_get_uint64(&value));
    assert(3 == called);
    assert(0 == QAJ4C_get_uint(&value));
    assert(4 == called);
    assert(false == QAJ4C_get_bool(&value));
    assert(5 == called);
    assert(QAJ4C_string_equals(&value, ""));
    assert(6 == called);
    assert(0 == QAJ4C_object_size (&value));
    assert(7 == called);
    assert(NULL == QAJ4C_object_get_member(&value, 0));
    assert(8 == called);
    assert(NULL == QAJ4C_object_create_member_by_ref(&value, "blubb"));
    assert(9 == called);
    assert(NULL == QAJ4C_array_get(&value, 0));
    assert(10 == called);
}

TEST(DomObjectAccessTests, BoolAccess) {
    QAJ4C_Value value;
    QAJ4C_set_bool(&value, true);
    static int called = 0;
    auto lambda = [](){
        ++called;
    };
    QAJ4C_register_fatal_error_function(lambda);
    assert(true == QAJ4C_get_bool(&value));
    assert(0 == called);
    assert(0.0 == QAJ4C_get_double(&value)); // don't check the value as only the invocation of the error function is interesting.
    assert(1 == called);
    assert(0 == QAJ4C_get_int64(&value));
    assert(2 == called);
    assert(0 == QAJ4C_get_int(&value));
    assert(3 == called);
    assert(0 == QAJ4C_get_uint64(&value));
    assert(4 == called);
    assert(0 == QAJ4C_get_uint(&value));
    assert(5 == called);
    assert(strcmp("",QAJ4C_get_string(&value))==0);
    assert(6 == called);
    assert(0 == QAJ4C_object_size (&value));
    assert(7 == called);
    assert(NULL == QAJ4C_object_get_member(&value, 0));
    assert(8 == called);
    assert(NULL == QAJ4C_object_create_member_by_ref(&value, "blubb"));
    assert(9 == called);
    assert(NULL == QAJ4C_array_get(&value, 0));
    assert(10 == called);
}

/**
 * This test verifies that a null value (not null pointer but null data type)
 * will not cause any trouble.
 */
TEST(DomObjectAccessTests, NullAccess) {
    QAJ4C_Value value;
    QAJ4C_set_null(&value);

    assert(!QAJ4C_is_not_set(&value));
    assert(QAJ4C_is_null(&value));

    static int called = 0;
    auto lambda = [](){
        ++called;
    };
    QAJ4C_register_fatal_error_function(lambda);
    assert(false == QAJ4C_get_bool(&value));
    assert(1 == called);
    assert(0.0 == QAJ4C_get_double(&value)); // don't check the value as only the invocation of the error function is interesting.
    assert(2 == called);
    assert(0 == QAJ4C_get_int64(&value));
    assert(3 == called);
    assert(0 == QAJ4C_get_int(&value));
    assert(4 == called);
    assert(0 == QAJ4C_get_uint64(&value));
    assert(5 == called);
    assert(0 == QAJ4C_get_uint(&value));
    assert(6 == called);
    assert(strcmp("",QAJ4C_get_string(&value))==0);
    assert(7 == called);
    assert(0 == QAJ4C_object_size (&value));
    assert(8 == called);
    assert(NULL == QAJ4C_object_get_member(&value, 0));
    assert(9 == called);
    assert(NULL == QAJ4C_object_create_member_by_ref(&value, "blubb"));
    assert(10 == called);
    assert(NULL == QAJ4C_array_get(&value, 0));
    assert(11 == called);
}

/**
 * This test verifies that a null pointer thus NOT the null data type but ((void*)0).
 * will not cause any trouble.
 */
TEST(DomObjectAccessTests, NullPointerAccess) {
    assert(QAJ4C_is_not_set(NULL));
    assert(QAJ4C_is_null(NULL));

    static int called = 0;
    auto lambda = [](){
        ++called;
    };
    QAJ4C_register_fatal_error_function(lambda);
    assert(false == QAJ4C_get_bool(NULL));
    assert(1 == called);
    assert(0.0 == QAJ4C_get_double(NULL)); // don't check the value as only the invocation of the error function is interesting.
    assert(2 == called);
    assert(0 == QAJ4C_get_int64(NULL));
    assert(3 == called);
    assert(0 == QAJ4C_get_int(NULL));
    assert(4 == called);
    assert(0 == QAJ4C_get_uint64(NULL));
    assert(5 == called);
    assert(0 == QAJ4C_get_uint(NULL));
    assert(6 == called);
    assert(strcmp("",QAJ4C_get_string(NULL))==0);
    assert(7 == called);
    assert(0 == QAJ4C_object_size (NULL));
    assert(8 == called);
    assert(NULL == QAJ4C_object_get_member(NULL, 0));
    assert(9 == called);
    assert(NULL == QAJ4C_object_create_member_by_ref(NULL, "blubb"));
    assert(10 == called);
    assert(NULL == QAJ4C_array_get(NULL, 0));
    assert(11 == called);
}

/**
 * This test verifies that an incorrect string access will be triggering the error function
 * and will also not cause the program to crash if the error function will "return"
 */
TEST(DomObjectAccessTests, IncorrectStringAccess) {
    static int called = 0;
    auto lambda = [](){
        ++called;
    };
    QAJ4C_register_fatal_error_function(lambda);

    int expected = 0;

    assert(!QAJ4C_is_string(NULL));

    assert(0 == QAJ4C_get_string_length(NULL));
    assert(++expected == called);
    assert(strcmp(QAJ4C_get_string(NULL), "") == 0);
    assert(++expected == called);
    assert(QAJ4C_string_cmp(NULL, "") == 0);
    assert(++expected == called);
    assert(QAJ4C_string_equals(NULL, ""));
    assert(++expected == called);
}

/**
 * This test verifies that an incorrect error access will be triggering the error function
 * and will also not cause the program to crash if the error function will "return"
 */
TEST(DomObjectAccessTests, IncorrectErrorAccess) {
    static int called = 0;
    auto lambda = [](){
        ++called;
    };
    QAJ4C_register_fatal_error_function(lambda);

    int expected = 0;

    assert(!QAJ4C_is_string(NULL));

    assert(0 == QAJ4C_error_get_errno(NULL));
    assert(++expected == called);
    assert(strcmp("", QAJ4C_error_get_json(NULL)) == 0);
    assert(++expected == called);
    assert(0 == QAJ4C_error_get_json_pos(NULL));
    assert(++expected == called);
}

/**
 * In this test it is verified that even in case one sets a uint with the int64 function
 * it will still be uint compatible!
 */
TEST(DomObjectAccessTests, AccessUint64OnInt64Valid) {
    QAJ4C_Value value;
    QAJ4C_set_int64(&value, 99);

    assert(QAJ4C_is_uint(&value));
}


TEST(DomObjectAccessTests, StringReference) {
    QAJ4C_Value value;
    const char* str = "Hello World!";
    QAJ4C_set_string_ref(&value, str);
    assert(str == QAJ4C_get_string(&value));
    assert(QAJ4C_string_equals(&value, str));
    assert(QAJ4C_string_cmp(&value, str) == 0);
}

TEST(DomObjectAccessTests, ShortString) {
    QAJ4C_Value value;
    const char* str = "a";
    QAJ4C_set_string_copy(&value, str, NULL);
    assert(QAJ4C_string_equals(&value, str));
    assert(QAJ4C_string_cmp(&value, str) == 0);
}

TEST(DomObjectAccessTests, StringCopy) {
    QAJ4C_Value value;
    const char* str = "abcdefghijklmnopqrstuvwxy";
    uint8_t buff[strlen(str) * 2];

    QAJ4C_Builder builder;
    QAJ4C_builder_init(&builder, buff, ARRAY_COUNT(buff));

    QAJ4C_set_string_copy(&value, str, &builder);
    assert(QAJ4C_string_equals(&value, str));
    assert(QAJ4C_string_cmp(&value, str) == 0);
}


TEST(ErrorHandlingTests, TooSmallDomBuffer) {
    char json[] = "[0.123456,9,12,3,5,7,2,3]";
    // just reduce the buffer size by one single byte
    size_t buff_size = QAJ4C_calculate_max_buffer_size(json) - 1;
    char buff[buff_size];
    const QAJ4C_Value* value = NULL;

    size_t actual_size = QAJ4C_parse(json, buff, buff_size, &value);
    assert(QAJ4C_is_error(value));

    assert(QAJ4C_error_get_errno(value) == QAJ4C_ERROR_STORAGE_BUFFER_TO_SMALL);
}

TEST(ErrorHandlingTests, ReallocFailsBegin) {
    char json[] = "[0.123456,9,12,3,5,7,2,3]";
    auto lambda = []( void *ptr, size_t size ) {return (void*)NULL;};
    // just reduce the buffer size by one single byte
    const QAJ4C_Value* value = QAJ4C_parse_dynamic(json, lambda);
    assert(QAJ4C_is_not_set(value));
}

TEST(ErrorHandlingTests, ReallocFailsLater) {
    char json[] = "[0.123456,9,12,3,5,7,2,3]";
    static bool first = true;
    auto lambda = []( void *ptr, size_t size ) {
        if ( first ) {
            first = false;
            return realloc(ptr, size);
        }
        return (void*)NULL;
    };
    // just reduce the buffer size by one single byte
    const QAJ4C_Value* value = QAJ4C_parse_dynamic(json, lambda);
    assert(QAJ4C_is_error(value));

    assert(QAJ4C_error_get_errno(value) == QAJ4C_ERROR_ALLOCATION_ERROR);
}

TEST(ErrorHandlingTests, DoubleSmallPrintBuffer) {
    char json[] = "[0.123456]";
    char out[ARRAY_COUNT(json) + 1];
    memset(out, '\n', ARRAY_COUNT(out));

    size_t buff_size = QAJ4C_calculate_max_buffer_size(json);
    char buff[buff_size];
    const QAJ4C_Value* value = NULL;

    size_t actual_size = QAJ4C_parse(json, buff, buff_size, &value);
    assert(!QAJ4C_is_error(value));

    assert(actual_size == buff_size);

    for (size_t i = 0, n = ARRAY_COUNT(json); i <= n; i++) {
        assert(i == QAJ4C_sprint(value, out, i));
        assert('\n' == out[i]); // check that nothing has been written over the bounds!
        if (i > 0) {
            assert(i - 1 == strlen(out));
        }
    }
}

TEST(ErrorHandlingTests, MultipleDoublesSmallPrintBuffer) {
    char json[] = "[0.123456,6.0]";
    char out[ARRAY_COUNT(json) + 1];
    memset(out, '\n', ARRAY_COUNT(out));

    size_t buff_size = QAJ4C_calculate_max_buffer_size(json);
    char buff[buff_size];
    const QAJ4C_Value* value = NULL;

    size_t actual_size = QAJ4C_parse(json, buff, buff_size, &value);
    assert(!QAJ4C_is_error(value));

    assert(actual_size == buff_size);

    for (size_t i = 0, n = ARRAY_COUNT(json); i <= n; i++) {
        assert(i == QAJ4C_sprint(value, out, i));
        assert('\n' == out[i]); // check that nothing has been written over the bounds!
        if (i > 0) {
            assert(i - 1 == strlen(out));
        }
    }
}

TEST(ErrorHandlingTests, NullSmallPrintBuffer) {
    char json[] = "[null]";
    char out[ARRAY_COUNT(json) + 1];
    memset(out, '\n', ARRAY_COUNT(out));

    size_t buff_size = QAJ4C_calculate_max_buffer_size(json);
    char buff[buff_size];
    const QAJ4C_Value* value = NULL;

    size_t actual_size = QAJ4C_parse(json, buff, buff_size, &value);
    assert(!QAJ4C_is_error(value));

    assert(actual_size == buff_size);

    for (size_t i = 0, n = ARRAY_COUNT(json); i <= n; i++) {
        assert(i == QAJ4C_sprint(value, out, i));
        assert('\n' == out[i]); // check that nothing has been written over the bounds!
        if (i > 0) {
            assert(i - 1 == strlen(out));
        }
    }
}

TEST(PrintTests, PrintErrorObject) {
    char json[] = "{";
    char output[128] = {'\0'};

    static int count = 0;
    auto lambda = []{ count++; }; // set the error method (else the compare will send a signal)
    QAJ4C_register_fatal_error_function(lambda);

    const QAJ4C_Value* value = QAJ4C_parse_opt_dynamic(json, ARRAY_COUNT(json), 0, realloc);

    assert(QAJ4C_is_error(value));
    size_t out = QAJ4C_sprint(value, output, ARRAY_COUNT(output));
    assert(0 == count); // expect no error
    assert(strcmp("", output) != 0); // expect no empty message!
}

TEST(PrintTests, PrintCorruptValue) {
    QAJ4C_Value value;
    char buffer[64] = {'\0'};

    static int count = 0;
    auto lambda = []{ count++; }; // set the error method (else the compare will send a signal)
    QAJ4C_register_fatal_error_function(lambda);

    value.type = QAJ4C_UNSPECIFIED << 8;
    size_t out = QAJ4C_sprint(&value, buffer, ARRAY_COUNT(buffer));
    assert(1 == count); // expect no error

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
    const char json[] = R"([1,2.10101,3,4.123456e+100,5.0,-6])";
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

TEST(PrintTests, PrintDoubleArray) {
    const char json[] = R"([0.0])";
    const QAJ4C_Value* value = QAJ4C_parse_opt_dynamic(json, ARRAY_COUNT(json), 0, realloc);

    assert(!QAJ4C_is_error(value));

    char output[ARRAY_COUNT(json)];
    size_t out = QAJ4C_sprint(value, output, ARRAY_COUNT(output));
    assert(ARRAY_COUNT(output) == out);
    assert(strcmp(json, output) == 0);

    free((void*)value);
}

TEST(PrintTests, PrintUint64Max) {
    const char json[] = R"([18446744073709551615])";
    const QAJ4C_Value* value = QAJ4C_parse_opt_dynamic(json, ARRAY_COUNT(json), 0, realloc);

    assert(!QAJ4C_is_error(value));

    char output[ARRAY_COUNT(json)];
    size_t out = QAJ4C_sprint(value, output, ARRAY_COUNT(output));
    assert(ARRAY_COUNT(output) == out);
    assert(strcmp(json, output) == 0);

    free((void*)value);
}

TEST(PrintTests, PrintStringWithNewLine) {
    char json[] = R"(["Hello\nWorld"])";
    const QAJ4C_Value* value = QAJ4C_parse_opt_dynamic(json, ARRAY_COUNT(json), 0, realloc);

    assert(!QAJ4C_is_error(value));

    char output[ARRAY_COUNT(json)];
    size_t out = QAJ4C_sprint(value, output, ARRAY_COUNT(output));
    assert(ARRAY_COUNT(output) == out);
    assert(strcmp(json, output) == 0);

    free((void*)value);
}

/**
 * Expect the control characters will be printed the same way.
 */
TEST(PrintTests, PrintControlCharacters) {
    char json[] = R"(["\" \\ \/ \b \f \n \r \t"])";
    const QAJ4C_Value* value = QAJ4C_parse_opt_dynamic(json, ARRAY_COUNT(json), 0, realloc);

    assert(!QAJ4C_is_error(value));

    char output[ARRAY_COUNT(json)];
    size_t out = QAJ4C_sprint(value, output, ARRAY_COUNT(output));
    assert(ARRAY_COUNT(output) == out);
    assert(strcmp(json, output) == 0);

    free((void*)value);
}

/**
 * In this test the user input for "NaN" will be translated to 'null' (to at least have
 * some kind of defined behavior)
 */
TEST(PrintTests, PrintDoubleDomNaN) {
    char output[64];

    QAJ4C_Value value;
    QAJ4C_set_double(&value, 1.0/0.0);
    QAJ4C_sprint(&value, output, ARRAY_COUNT(output));
    assert(strcmp("null", output) == 0);
}

/**
 * Parse the same message twice and compare the results with each other.
 * It is expected that the two results are equal!
 */
TEST(VariousTests, ComparisonTest) {
    const char json[] = R"({"id":1,"data":{"name":"foo","param":12,"data":[1,0.3e10,-99,null,false,"abcdefghijklmnopqrstuvwxyz"]}})";
    size_t buff_size = QAJ4C_calculate_max_buffer_size(json);
    uint8_t buffer[buff_size];
    uint8_t buffer2[buff_size];
    const QAJ4C_Value* value_1 = nullptr;
    const QAJ4C_Value* value_2 = nullptr;

    assert(QAJ4C_parse(json, buffer, buff_size, &value_1) == buff_size);
    assert(QAJ4C_parse(json, buffer2, buff_size, &value_2) == buff_size);

    assert(QAJ4C_value_sizeof(value_1) == buff_size);
    assert(QAJ4C_value_sizeof(value_2) == buff_size);

    assert(QAJ4C_equals(value_1, value_2));
}

TEST(VariousTests, ComparisonTestDifferentBooleanInArray) {
    const char json1[] = R"([1,0.3e10,-99,false,"abcdefghijklmnopqrstuvwxyz"])";
    const char json2[] = R"([1,0.3e10,-99,true,"abcdefghijklmnopqrstuvwxyz"])";
    size_t buff_size1 = QAJ4C_calculate_max_buffer_size(json1);
    size_t buff_size2 = QAJ4C_calculate_max_buffer_size(json2);
    uint8_t buffer1[buff_size1];
    uint8_t buffer2[buff_size2];

    const QAJ4C_Value* value_1 = nullptr;
    const QAJ4C_Value* value_2 = nullptr;

    assert(QAJ4C_parse(json1, buffer1, buff_size1, &value_1) == buff_size1);
    assert(QAJ4C_parse(json2, buffer2, buff_size2, &value_2) == buff_size2);

    assert(QAJ4C_is_array(value_1));
    assert(QAJ4C_is_array(value_2));

    assert(QAJ4C_value_sizeof(value_1) == buff_size1);
    assert(QAJ4C_value_sizeof(value_2) == buff_size2);

    assert(!QAJ4C_equals(value_1, value_2));
}

TEST(VariousTests, ComparisonTestDifferentArraySize) {
    const char json1[] = R"([1,0.3e10,-99,false,"abcdefghijklmnopqrstuvwxyz"])";
    const char json2[] = R"([])";
    size_t buff_size1 = QAJ4C_calculate_max_buffer_size(json1);
    size_t buff_size2 = QAJ4C_calculate_max_buffer_size(json2);
    uint8_t buffer1[buff_size1];
    uint8_t buffer2[buff_size2];

    const QAJ4C_Value* value_1 = nullptr;
    const QAJ4C_Value* value_2 = nullptr;

    assert(QAJ4C_parse(json1, buffer1, buff_size1, &value_1) == buff_size1);
    assert(QAJ4C_parse(json2, buffer2, buff_size2, &value_2) == buff_size2);

    assert(QAJ4C_is_array(value_1));
    assert(QAJ4C_is_array(value_2));

    assert(QAJ4C_value_sizeof(value_1) == buff_size1);
    assert(QAJ4C_value_sizeof(value_2) == buff_size2);

    assert(!QAJ4C_equals(value_1, value_2));
}

TEST(VariousTests, ComparisonTestDifferentKeysInObject) {
    const char json1[] = R"({"id":1,"name":"Yo"})";
    const char json2[] = R"({"id":1,"description":"Yo"})";
    size_t buff_size1 = QAJ4C_calculate_max_buffer_size(json1);
    size_t buff_size2 = QAJ4C_calculate_max_buffer_size(json2);
    uint8_t buffer1[buff_size1];
    uint8_t buffer2[buff_size2];

    const QAJ4C_Value* value_1 = nullptr;
    const QAJ4C_Value* value_2 = nullptr;

    assert(QAJ4C_parse(json1, buffer1, buff_size1, &value_1) == buff_size1);
    assert(QAJ4C_parse(json2, buffer2, buff_size2, &value_2) == buff_size2);

    assert(QAJ4C_is_object(value_1));
    assert(QAJ4C_is_object(value_2));

    assert(QAJ4C_value_sizeof(value_1) == buff_size1);
    assert(QAJ4C_value_sizeof(value_2) == buff_size2);

    assert(!QAJ4C_equals(value_1, value_2));
}

TEST(VariousTests, ComparisonTestDifferentObjectSize) {
    const char json1[] = R"({"id":1,"name":"Yo"})";
    const char json2[] = R"({})";
    size_t buff_size1 = QAJ4C_calculate_max_buffer_size(json1);
    size_t buff_size2 = QAJ4C_calculate_max_buffer_size(json2);
    uint8_t buffer1[buff_size1];
    uint8_t buffer2[buff_size2];

    const QAJ4C_Value* value_1 = nullptr;
    const QAJ4C_Value* value_2 = nullptr;

    assert(QAJ4C_parse(json1, buffer1, buff_size1, &value_1) == buff_size1);
    assert(QAJ4C_parse(json2, buffer2, buff_size2, &value_2) == buff_size2);

    assert(QAJ4C_is_object(value_1));
    assert(QAJ4C_is_object(value_2));

    assert(QAJ4C_value_sizeof(value_1) == buff_size1);
    assert(QAJ4C_value_sizeof(value_2) == buff_size2);

    assert(!QAJ4C_equals(value_1, value_2));
}

/**
 * This test will verify that the order of keys does not influence the result!
 */
TEST(VariousTests, ComparisonTestDifferentKeyOrder) {
    const char json1[] = R"({"id":1,"name":"Yo"})";
    const char json2[] = R"({"name":"Yo","id":1})";
    size_t buff_size1 = QAJ4C_calculate_max_buffer_size(json1);
    size_t buff_size2 = QAJ4C_calculate_max_buffer_size(json2);
    uint8_t buffer1[buff_size1];
    uint8_t buffer2[buff_size2];

    const QAJ4C_Value* value_1 = nullptr;
    const QAJ4C_Value* value_2 = nullptr;

    assert(QAJ4C_parse(json1, buffer1, buff_size1, &value_1) == buff_size1);
    assert(QAJ4C_parse(json2, buffer2, buff_size2, &value_2) == buff_size2);

    assert(QAJ4C_is_object(value_1));
    assert(QAJ4C_is_object(value_2));

    assert(QAJ4C_value_sizeof(value_1) == buff_size1);
    assert(QAJ4C_value_sizeof(value_2) == buff_size2);

    assert(QAJ4C_equals(value_1, value_2));
}

/**
 * Parse the message once and then copy the result.
 * It is expected that the copy and the original are equal!
 */
TEST(VariousTests, ComparisonAndCopyTest) {
    const char json[] = R"({"id":1,"data":{"name":"foo","param":12,"data":[1,0.3e10,-99,false,"abcdefghijklmnopqrstuvwxyz"]}})";
    size_t buff_size = QAJ4C_calculate_max_buffer_size(json);
    uint8_t buffer[buff_size];
    uint8_t buffer2[buff_size];
    const QAJ4C_Value* value_1;

    QAJ4C_Builder builder;
    QAJ4C_builder_init(&builder, buffer2, buff_size);

    QAJ4C_parse(json, buffer, buff_size, &value_1);
    QAJ4C_Value* value_2 = QAJ4C_builder_get_document(&builder);
    QAJ4C_copy(value_1, value_2, &builder);

    assert(QAJ4C_value_sizeof(value_1) ==  buff_size);
    assert(QAJ4C_value_sizeof(value_2) ==  buff_size);

    assert(QAJ4C_equals(value_1, value_2));
}

TEST(VariousTests, CopyNotFullyFilledObject) {
    uint8_t buff1[256];
    uint8_t buff2[ARRAY_COUNT(buff1)];
    QAJ4C_Builder builder1;
    QAJ4C_builder_init(&builder1, buff1, ARRAY_COUNT(buff1));
    QAJ4C_Builder builder2;
    QAJ4C_builder_init(&builder2, buff2, ARRAY_COUNT(buff2));

    QAJ4C_Value* root = QAJ4C_builder_get_document(&builder1);
    QAJ4C_set_object(root, 3, &builder1);
    QAJ4C_object_create_member_by_ref(root, "Hossa");

    QAJ4C_Value* copy = QAJ4C_builder_get_document(&builder2);

    QAJ4C_copy(root, copy, &builder2);
    assert(QAJ4C_equals(root, copy));
}

TEST(VariousTests, ErrorsAreNeverEqual) {
    const char json[] = R"([)";

    size_t buff_size = QAJ4C_calculate_max_buffer_size(json);
    uint8_t buffer1[buff_size];
    uint8_t buffer2[buff_size];

    const QAJ4C_Value* value_1 = nullptr;
    const QAJ4C_Value* value_2 = nullptr;

    assert(QAJ4C_parse(json, buffer1, buff_size, &value_1) == buff_size);
    assert(QAJ4C_parse(json, buffer2, buff_size, &value_2) == buff_size);

    assert(QAJ4C_value_sizeof(value_1) ==  buff_size);
    assert(QAJ4C_value_sizeof(value_2) ==  buff_size);

    auto lambda = []{}; // set the error method (else the compare will send a signal)
    QAJ4C_register_fatal_error_function(lambda);

    assert(!QAJ4C_equals(value_1, value_2));
}

TEST(VariousTests, ErrorsCannotBeCopied) {
    const char json[] = R"([)";

    size_t buff_size = QAJ4C_calculate_max_buffer_size(json);
    uint8_t buffer1[buff_size];
    uint8_t buffer2[buff_size];
    QAJ4C_Builder builder;

    QAJ4C_builder_init(&builder, buffer2, buff_size);

    static int count = 0;
    auto lambda = []{ count++; }; // set the error method (else the compare will send a signal)
    QAJ4C_register_fatal_error_function(lambda);

    const QAJ4C_Value* value_1 = nullptr;
    QAJ4C_Value* value_2 = QAJ4C_builder_get_document(&builder);

    assert(QAJ4C_parse(json, buffer1, buff_size, &value_1) == buff_size);
    QAJ4C_copy(value_1, value_2, &builder);
    assert(1 == count);
}

/**
 * The parser is build on some "assumptions" and this test should verify this
 * for the different platform.
 */
TEST(VariousTests, CheckSizes) {
    assert(sizeof(double) == 8);
    assert(sizeof(int64_t) == 8);
    assert(sizeof(uint64_t) == 8);

    printf("Sizeof QAJ4C_Value is: %zu\n", sizeof(QAJ4C_Value));

    assert(sizeof(QAJ4C_Value) == (QAJ4C_MAX(sizeof(uint32_t), sizeof(uintptr_t)) + sizeof(size_type) * 2));
    assert(sizeof(QAJ4C_Value) == sizeof(QAJ4C_Object));
    assert(sizeof(QAJ4C_Value) == sizeof(QAJ4C_Array));
    assert(sizeof(QAJ4C_Value) == sizeof(QAJ4C_String));
    assert(sizeof(QAJ4C_Value) == sizeof(QAJ4C_Short_string));
    assert(sizeof(QAJ4C_Value) == sizeof(QAJ4C_Primitive));
    assert(sizeof(QAJ4C_Value) == sizeof(QAJ4C_Error));

    assert(sizeof(QAJ4C_Member) == 2 * sizeof(QAJ4C_Value));
}

TEST(StrictParsingTests, ParseValidNumericValues) {
    const char json[] = R"([0, 1.0, 0.015, -0.5, -0.005, -256])";
    const QAJ4C_Value* value = QAJ4C_parse_opt_dynamic(json, ARRAY_COUNT(json), QAJ4C_PARSE_OPTS_STRICT, realloc);

    assert(QAJ4C_is_array(value));
    assert(QAJ4C_array_size(value) == 6);

    assert(0 == QAJ4C_get_uint(QAJ4C_array_get(value, 0)));
    assert(1.0 == QAJ4C_get_double(QAJ4C_array_get(value, 1)));
    assert(0.015 == QAJ4C_get_double(QAJ4C_array_get(value, 2)));
    assert(-0.5 == QAJ4C_get_double(QAJ4C_array_get(value, 3)));
    assert(-0.005 == QAJ4C_get_double(QAJ4C_array_get(value, 4)));
    assert(-256 == QAJ4C_get_int(QAJ4C_array_get(value, 5)));

    free((void*)value);
}

TEST(StrictParsingTests, ParseNumericValueWithLeadingZero) {
    const char json[] = R"([007])";
    const QAJ4C_Value* value = QAJ4C_parse_opt_dynamic(json, ARRAY_COUNT(json), QAJ4C_PARSE_OPTS_STRICT, realloc);

    assert(QAJ4C_is_error(value));
    assert(QAJ4C_error_get_errno(value) == QAJ4C_ERROR_INVALID_NUMBER_FORMAT);

    free((void*)value);
}

TEST(StrictParsingTests, ParseNumericValueWithLeadingPlus) {
    const char json[] = R"([+7])";
    const QAJ4C_Value* value = QAJ4C_parse_opt_dynamic(json, ARRAY_COUNT(json), QAJ4C_PARSE_OPTS_STRICT, realloc);

    assert(QAJ4C_is_error(value));
    assert(QAJ4C_error_get_errno(value) == QAJ4C_ERROR_INVALID_NUMBER_FORMAT);

    free((void*)value);
}

TEST(StrictParsingTests, ParseMessageTrailingComma) {
    const char json[] = R"([7],)";
    const QAJ4C_Value* value = QAJ4C_parse_opt_dynamic(json, ARRAY_COUNT(json), QAJ4C_PARSE_OPTS_STRICT, realloc);

    assert(QAJ4C_is_error(value));
    assert(QAJ4C_error_get_errno(value) == QAJ4C_ERROR_UNEXPECTED_JSON_APPENDIX);

    free((void*)value);
}

TEST(StrictParsingTests, ParseObjectTrailingComma) {
    const char json[] = R"({"id":123, "name": "hossa",})";
    uint8_t buff[256];
    const QAJ4C_Value* val = nullptr;
    QAJ4C_parse_opt(json, SIZE_MAX, QAJ4C_PARSE_OPTS_STRICT, buff, ARRAY_COUNT(buff), &val);

    assert(QAJ4C_is_error(val));
    assert(QAJ4C_error_get_errno(val) == QAJ4C_ERROR_TRAILING_COMMA);
}

TEST(StrictParsingTests, ParseArrayTrailingComma) {
    const char json[] = R"([7,])";
    const QAJ4C_Value* value = QAJ4C_parse_opt_dynamic(json, ARRAY_COUNT(json), QAJ4C_PARSE_OPTS_STRICT, realloc);

    assert(QAJ4C_is_error(value));
    assert(QAJ4C_error_get_errno(value) == QAJ4C_ERROR_TRAILING_COMMA);

    free((void*)value);
}


TEST(DomCreation, CreateArray) {
    uint8_t buff[256];
    QAJ4C_Builder builder;
    QAJ4C_builder_init(&builder, buff, ARRAY_COUNT(buff));

    auto lambda = []{};

    QAJ4C_register_fatal_error_function(lambda);

    QAJ4C_Value* value_ptr = QAJ4C_builder_get_document(&builder);

    QAJ4C_set_array(value_ptr, 4, &builder);
    assert(QAJ4C_array_get_rw(value_ptr, 0) != nullptr);
    assert(QAJ4C_array_get_rw(value_ptr, 1) != nullptr);
    assert(QAJ4C_array_get_rw(value_ptr, 2) != nullptr);
    assert(QAJ4C_array_get_rw(value_ptr, 3) != nullptr);
    assert(QAJ4C_array_get_rw(value_ptr, 4) == nullptr);

    // create a mixed array
    QAJ4C_set_uint(QAJ4C_array_get_rw(value_ptr, 0), 1234);
    QAJ4C_set_bool(QAJ4C_array_get_rw(value_ptr, 1), true);
    QAJ4C_set_string_ref(QAJ4C_array_get_rw(value_ptr, 2), "foobar");
    QAJ4C_set_null(QAJ4C_array_get_rw(value_ptr, 3));

    assert(QAJ4C_is_uint(QAJ4C_array_get(value_ptr, 0)));
    assert(1234 == QAJ4C_get_uint(QAJ4C_array_get(value_ptr, 0)));
    assert(QAJ4C_is_bool(QAJ4C_array_get(value_ptr, 1)));
    assert(true == QAJ4C_get_bool(QAJ4C_array_get(value_ptr, 1)));
    assert(QAJ4C_is_string(QAJ4C_array_get(value_ptr, 2)));
    assert(QAJ4C_string_equals(QAJ4C_array_get(value_ptr, 2), "foobar"));
    assert(QAJ4C_is_null(QAJ4C_array_get(value_ptr, 3)));
}

TEST(DomCreation, CreateObject) {
    static const char* key1 = "id";
    static const char* key2 = "very_big_name_just_to_let_you_know";
    static const char* key3 = "name";
    static const char* key4 = "Id"; // similar key!
    static const char* key5 = "xyz";

    uint8_t buff[256];
    QAJ4C_Builder builder;
    QAJ4C_builder_init(&builder, buff, ARRAY_COUNT(buff));

    auto lambda = []{};

    QAJ4C_register_fatal_error_function(lambda);

    QAJ4C_Value* value_ptr = QAJ4C_builder_get_document(&builder);
    QAJ4C_set_object(value_ptr, 4, &builder);
    QAJ4C_Value* member1 = QAJ4C_object_create_member_by_ref(value_ptr, key1);
    QAJ4C_Value* member1_a = QAJ4C_object_create_member_by_copy(value_ptr, key1, &builder);
    QAJ4C_Value* member1_b = QAJ4C_object_create_member_by_ref(value_ptr, key1);
    QAJ4C_Value* member2 = QAJ4C_object_create_member_by_copy(value_ptr, key2, &builder);
    QAJ4C_Value* member3 = QAJ4C_object_create_member_by_copy(value_ptr, key3, &builder);
    QAJ4C_Value* member4 = QAJ4C_object_create_member_by_ref(value_ptr, key4);
    QAJ4C_Value* member5 = QAJ4C_object_create_member_by_ref(value_ptr, key5);

    assert( member1 != nullptr );
    assert( member1_a == nullptr );
    assert( member1_b == nullptr );
    assert( member2 != nullptr );
    assert( member3 != nullptr );
    assert( member4 != nullptr);
    assert( member5 == nullptr );  // there is no room left for more members

    // Check that we still reach all elements!
    assert(QAJ4C_object_get(value_ptr, key1) != nullptr);
    assert(QAJ4C_object_get(value_ptr, key2) != nullptr);
    assert(QAJ4C_object_get(value_ptr, key3) != nullptr);
    assert(QAJ4C_object_get(value_ptr, key4) != nullptr);
    assert(QAJ4C_object_get(value_ptr, key5) == nullptr);

    QAJ4C_object_optimize(value_ptr);

    assert(QAJ4C_get_internal_type(value_ptr) == QAJ4C_OBJECT_SORTED);

    for (int i = 1; i < QAJ4C_object_size(value_ptr); i++) {
        const QAJ4C_Member* lower_member = QAJ4C_object_get_member(value_ptr, i - 1);
        const QAJ4C_Member* current_member = QAJ4C_object_get_member(value_ptr, i);
        assert(QAJ4C_compare_members(lower_member, current_member) < 0);
        assert(QAJ4C_strcmp(QAJ4C_member_get_key(lower_member),
                            QAJ4C_member_get_key(current_member)) < 0);
    }

    // Check that we still can reach all elements!
    assert(QAJ4C_object_get(value_ptr, key1) != nullptr);
    assert(QAJ4C_object_get(value_ptr, key2) != nullptr);
    assert(QAJ4C_object_get(value_ptr, key3) != nullptr);
    assert(QAJ4C_object_get(value_ptr, key4) != nullptr);
    assert(QAJ4C_object_get(value_ptr, key5) == nullptr);
}

TEST(DomCreation, OptimizeEmptyObject) {
    uint8_t buff[256];
    QAJ4C_Builder builder;
    QAJ4C_builder_init(&builder, buff, ARRAY_COUNT(buff));

    QAJ4C_Value* value_ptr = QAJ4C_builder_get_document(&builder);
    QAJ4C_set_object(value_ptr, 0, &builder);
    QAJ4C_object_optimize(value_ptr);
    assert(QAJ4C_get_internal_type(value_ptr) == QAJ4C_OBJECT_SORTED);
}

TEST(DomCreation, OptimizeUninitializedObject) {
    uint8_t buff[256];
    QAJ4C_Builder builder;
    QAJ4C_builder_init(&builder, buff, ARRAY_COUNT(buff));

    QAJ4C_Value* value_ptr = QAJ4C_builder_get_document(&builder);
    QAJ4C_set_object(value_ptr, 4, &builder);
    QAJ4C_object_optimize(value_ptr);
    assert(QAJ4C_get_internal_type(value_ptr) == QAJ4C_OBJECT_SORTED);
}

TEST(DomCreation, OptimizeLowFilledObject) {
    uint8_t buff[256];
    QAJ4C_Builder builder;
    QAJ4C_builder_init(&builder, buff, ARRAY_COUNT(buff));

    QAJ4C_Value* value_ptr = QAJ4C_builder_get_document(&builder);
    QAJ4C_set_object(value_ptr, 4, &builder);
    QAJ4C_object_create_member_by_ref(value_ptr, "id");

    QAJ4C_object_optimize(value_ptr);
    assert(QAJ4C_get_internal_type(value_ptr) == QAJ4C_OBJECT_SORTED);
}


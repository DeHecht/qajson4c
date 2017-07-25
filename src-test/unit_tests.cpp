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

#ifdef __STRICT_ANSI__
#undef __STRICT_ANSI__
#endif

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
#define FMT_SIZE "%zu"
#else
#define FMT_SIZE "%Iu"
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

typedef struct QSJ4C_TEST_DEF {
    const char* subject_name;
    const char* test_name;
    void (*test_func)( void );
    struct QSJ4C_TEST_DEF* next;
    char* failure_message;
    char* error_message;
    time_t execution;
} QSJ4C_TEST_DEF;

QSJ4C_TEST_DEF* TOP_POINTER = NULL;
QSJ4C_TEST_DEF* LAST_POINTER = NULL;

static int tests_total = 0;
static int tests_failed = 0;
static int tests_passed = 0;

static QSJ4C_TEST_DEF* create_test( const char* subject, const char* test, void (*test_func)() ) {
    QSJ4C_TEST_DEF* tmp = (QSJ4C_TEST_DEF*)malloc(sizeof(QSJ4C_TEST_DEF));
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
            QSJ4C_TEST_DEF* current = TOP_POINTER;
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

int fork_and_run( QSJ4C_TEST_DEF* test ) {
    char buff[256];
    buff[0] = '\0';

    QSJ4C_register_fatal_error_function(NULL);

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
    int w = snprintf(testsuite_buffer, ARRAY_COUNT(testsuite_buffer), "<testsuite errors=\"" FMT_SIZE "\" failures=\"" FMT_SIZE "\" name=\"%s\" skips=\"" FMT_SIZE "\" tests=\"" FMT_SIZE "\" time=\"" FMT_SIZE "\">%c", stats->errors, stats->fails, suite_name, (size_t)0, (size_t)(stats->errors + stats->fails + stats->pass), stats->total_time, '\n');
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

    QSJ4C_register_fatal_error_function(custom_error_function);

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

    QSJ4C_TEST_DEF* curr_test = TOP_POINTER;
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
        pos += snprintf(testsuite_content_buffer + pos, testsuite_content_buffer_size, "<testcase classname=\"%s\" name=\"%s\" time=\"" FMT_SIZE "\">", curr_test->subject_name, curr_test->test_name, curr_test->execution);
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
   UNITTESTS_ATTRIBUTE_UNUSED_ QSJ4C_TEST_DEF* a## _## b## _test_ptr = create_test(#a, #b, a## _## b## _test); \
   void a## _## b## _test()


TEST(BufferSizeTests, ParseObjectWithOneNumericMember) {
    const char json[] = R"({"id":1})";

    size_t normal_required_buffer_size = QSJ4C_calculate_max_buffer_size_n(json, ARRAY_COUNT(json));

    // one object + one member
    assert(normal_required_buffer_size == (sizeof(QSJ4C_Value) + sizeof(QSJ4C_Member) + 3));
}


TEST(SimpleParsingTests, ParseObjectWithOneNumericMember) {
    const char json[] = R"({"id":1})";

    QSJ4C_Value* value = QSJ4C_parse_opt_dynamic(json, ARRAY_COUNT(json), 0, realloc);
    assert(QSJ4C_is_object(value));
    assert(QSJ4C_object_size(value) == 1);

    QSJ4C_Value* object_entry = QSJ4C_object_get(value, "id");
    assert(object_entry != NULL);
    assert(QSJ4C_is_uint(object_entry));
    assert(QSJ4C_get_uint(object_entry) == 1);
    free((void*)value);
}

TEST(SimpleParsingTests, ParseObjectWithOneNumericMemberNoOptions) {
    const char json[] = R"({"id":1})";

    QSJ4C_Value* value = QSJ4C_parse_dynamic(json, realloc);
    assert(QSJ4C_is_object(value));
    assert(QSJ4C_object_size(value) == 1);

    QSJ4C_Value* object_entry = QSJ4C_object_get(value, "id");
    assert(object_entry != NULL);
    assert(QSJ4C_is_uint(object_entry));
    assert(QSJ4C_get_uint(object_entry) == 1);
    free((void*)value);
}

TEST(SimpleParsingTests, ParseObjectWithOneNumericMemberWhitespaces) {
    const char json[] = R"({ "id" : 1 })";

    QSJ4C_Value* value = QSJ4C_parse_opt_dynamic(json, ARRAY_COUNT(json), 0, realloc);
    assert(QSJ4C_is_object(value));
    assert(QSJ4C_object_size(value) == 1);

    QSJ4C_Value* object_entry = QSJ4C_object_get(value, "id");
    assert(object_entry != NULL);
    assert(QSJ4C_is_uint(object_entry));
    assert(QSJ4C_get_uint(object_entry) == 1);
    free((void*)value);
}


TEST(SimpleParsingTests, ParseObjectWithOneStringMember) {
    const char json[] = R"({"name":"blah"})";
    QSJ4C_Value* value = QSJ4C_parse_opt_dynamic(json, ARRAY_COUNT(json), 0, realloc);
    assert(QSJ4C_is_object(value));
    assert(QSJ4C_object_size(value) == 1);

    QSJ4C_Value* object_entry = QSJ4C_object_get(value, "name");
    assert(QSJ4C_is_string(object_entry));
    assert(QSJ4C_string_equals(object_entry, "blah"));
    free((void*)value);
}

TEST(SimpleParsingTests, ParseObjectWithOneStringVeryLongMember) {
    const char json[] = R"({"name":"blahblubbhubbeldipup"})";
    QSJ4C_Value* value = QSJ4C_parse_opt_dynamic(json, ARRAY_COUNT(json), 0, realloc);
    assert(QSJ4C_is_object(value));
    assert(QSJ4C_object_size(value) == 1);

    QSJ4C_Value* object_entry = QSJ4C_object_get(value, "name");
    assert(QSJ4C_is_string(object_entry));
    assert(QSJ4C_string_equals(object_entry, "blahblubbhubbeldipup"));
    free((void*)value);
}

TEST(SimpleParsingTests, ParseStringWithNewLine) {
    char json[] = R"(["Hello\nWorld"])";
    QSJ4C_Value* value = QSJ4C_parse_opt_dynamic(json, ARRAY_COUNT(json), 0, realloc);
    assert(QSJ4C_is_array(value));
    assert(QSJ4C_array_size(value) == 1);
    QSJ4C_Value* array_entry = QSJ4C_array_get(value, 0);
    assert(QSJ4C_is_string(array_entry));

    // Now compare with the non-raw string (as the \n should be interpeted)
    assert(QSJ4C_string_equals(array_entry, "Hello\nWorld"));
    free((void*)value);
}

TEST(SimpleParsingTests, ParseStringWithNewLineUnicode) {
    char json[] = R"(["Hello\u000AWorld"])";
    char expected[] = "Hello\nWorld";
    QSJ4C_Value* value = QSJ4C_parse_opt_dynamic(json, ARRAY_COUNT(json), 0, realloc);
    assert(QSJ4C_is_array(value));
    assert(QSJ4C_array_size(value) == 1);
    QSJ4C_Value* array_entry = QSJ4C_array_get(value, 0);
    assert(QSJ4C_is_string(array_entry));

    // Now compare with the non-raw string (as the \n should be interpeted)
    assert(QSJ4C_get_string_length(array_entry) == strlen(expected));
    assert(QSJ4C_string_equals(array_entry, expected));

    free((void*)value);
}

TEST(SimpleParsingTests, ParseStringWithEndOfStringUnicode) {
    char json[] = R"(["Hello\u0000World"])";
    QSJ4C_Value* value = QSJ4C_parse_opt_dynamic(json, ARRAY_COUNT(json), 0, realloc);
    assert(QSJ4C_is_array(value));
    assert(QSJ4C_array_size(value) == 1);
    QSJ4C_Value* array_entry = QSJ4C_array_get(value, 0);
    assert(QSJ4C_is_string(array_entry));

    // Now compare with the non-raw string (as the \n should be interpeted)

    const char* result_str = QSJ4C_get_string(array_entry);

    const char expected[] = "Hello\0World";
    size_t expected_len = ARRAY_COUNT(expected) - 1; // strlen will not work
    assert(expected_len == QSJ4C_get_string_length(array_entry));
    assert(QSJ4C_string_equals_n(array_entry, expected, expected_len));

    free((void*)value);
}

TEST(SimpleParsingTests, ParseDollarSign) {
    const char json[] = R"(["\u0024"])";
    const char expected[] = "$";
    QSJ4C_Value* value = QSJ4C_parse_opt_dynamic(json, ARRAY_COUNT(json), 0, realloc);
    assert(QSJ4C_is_array(value));
    assert(QSJ4C_array_size(value) == 1);
    QSJ4C_Value* array_entry = QSJ4C_array_get(value, 0);
    assert(QSJ4C_is_string(array_entry));

    assert(QSJ4C_string_equals(array_entry, expected));
    assert(QSJ4C_get_string_length(array_entry) == strlen(expected));

    free((void*)value);
}

TEST(SimpleParsingTests, ParseJapaneseTeaSign) {

    const char json[] = R"(["\u8336"])";
    const char expected[] = "茶";
    QSJ4C_Value* value = QSJ4C_parse_opt_dynamic(json, ARRAY_COUNT(json), 0, realloc);
    assert(QSJ4C_is_array(value));
    assert(QSJ4C_array_size(value) == 1);
    QSJ4C_Value* array_entry = QSJ4C_array_get(value, 0);
    assert(QSJ4C_is_string(array_entry));

    assert(QSJ4C_string_equals(array_entry, expected));
    assert(QSJ4C_get_string_length(array_entry) == strlen(expected));

    free((void*)value);
}

TEST(SimpleParsingTests, ParseYenSign) {

    const char json[] = R"(["\u00A5"])";
    const char expected[] = "¥";
    QSJ4C_Value* value = QSJ4C_parse_opt_dynamic(json, ARRAY_COUNT(json), 0, realloc);
    assert(QSJ4C_is_array(value));
    assert(QSJ4C_array_size(value) == 1);
    QSJ4C_Value* array_entry = QSJ4C_array_get(value, 0);
    assert(QSJ4C_is_string(array_entry));

    assert(QSJ4C_string_equals(array_entry, expected));
    assert(QSJ4C_get_string_length(array_entry) == strlen(expected));

    free((void*)value);
}

TEST(SimpleParsingTests, ParseBigUtf16) {
    const char json[] = R"(["\uD834\uDD1E"])";
    const char expected[] = "𝄞";
    QSJ4C_Value* value = QSJ4C_parse_opt_dynamic(json, ARRAY_COUNT(json), 0, realloc);
    assert(QSJ4C_is_array(value));
    assert(QSJ4C_array_size(value) == 1);
    QSJ4C_Value* array_entry = QSJ4C_array_get(value, 0);
    assert(QSJ4C_is_string(array_entry));

    assert(QSJ4C_string_equals(array_entry, expected));
    assert(QSJ4C_get_string_length(array_entry) == strlen(expected));

    free((void*)value);
}

TEST(SimpleParsingTests, ParseStringWithEscapedQuotes) {
    char json[] = R"(["Hello\"World"])";
    QSJ4C_Value* value = QSJ4C_parse_opt_dynamic(json, ARRAY_COUNT(json), 0, realloc);
    assert(QSJ4C_is_array(value));
    assert(QSJ4C_array_size(value) == 1);
    QSJ4C_Value* array_entry = QSJ4C_array_get(value, 0);
    assert(QSJ4C_is_string(array_entry));

    // Now compare with the non-raw string (as the \n should be interpeted)
    assert(QSJ4C_string_equals(array_entry, "Hello\"World"));
    free((void*)value);
}

TEST(SimpleParsingTests, ParseEmptyObject) {
    const char json[] = R"({})";
    QSJ4C_Value* value = QSJ4C_parse_opt_dynamic(json, ARRAY_COUNT(json), 0, realloc);
    assert(QSJ4C_is_object(value));
    assert(QSJ4C_object_size(value) == 0);
    free((void*)value);
}

TEST(SimpleParsingTests, ParseEmptyObjectWhitespaces) {
    const char json[] = R"({ })";
    QSJ4C_Value* value = QSJ4C_parse_opt_dynamic(json, ARRAY_COUNT(json), 0, realloc);
    assert(QSJ4C_is_object(value));
    assert(QSJ4C_object_size(value) == 0);
    free((void*)value);
}

TEST(SimpleParsingTests, ParseEmptyArray) {
    const char json[] = R"([])";
    QSJ4C_Value* value = QSJ4C_parse_opt_dynamic(json, ARRAY_COUNT(json), 0, realloc);
    assert(QSJ4C_is_array(value));
    assert(QSJ4C_array_size(value) == 0);
    free((void*)value);
}

TEST(SimpleParsingTests, ParseEmptyArrayWhitespaces) {
    const char json[] = R"([ ])";
    QSJ4C_Value* value = QSJ4C_parse_opt_dynamic(json, ARRAY_COUNT(json), 0, realloc);
    assert(QSJ4C_is_array(value));
    assert(QSJ4C_array_size(value) == 0);
    free((void*)value);
}


TEST(SimpleParsingTests, ParseNumberArray) {
    const char json[] = R"([1,2,3,-4,5,+6])";
    QSJ4C_Value* value = QSJ4C_parse_opt_dynamic(json, ARRAY_COUNT(json), 0, realloc);
    assert(QSJ4C_is_array(value));
    assert(QSJ4C_array_size(value) == 6);

    assert(1 == QSJ4C_get_uint(QSJ4C_array_get(value, 0)));
    assert(2 == QSJ4C_get_uint(QSJ4C_array_get(value, 1)));
    assert(3 == QSJ4C_get_uint(QSJ4C_array_get(value, 2)));
    assert(-4 == QSJ4C_get_int(QSJ4C_array_get(value, 3)));
    assert(5 == QSJ4C_get_uint(QSJ4C_array_get(value, 4)));
    assert(6 == QSJ4C_get_uint(QSJ4C_array_get(value, 5)));

    free((void*)value);
}

TEST(SimpleParsingTests, ParseNumberArrayWithComments) {
    const char json[] = R"([/* HO */1, /* HO **/ 2, /**/ 3, /***/4,5,6])";
    QSJ4C_Value* value = QSJ4C_parse_opt_dynamic(json, ARRAY_COUNT(json), 0, realloc);
    assert(QSJ4C_is_array(value));
    assert(QSJ4C_array_size(value) == 6);

    assert(1 == QSJ4C_get_uint(QSJ4C_array_get(value, 0)));
    assert(2 == QSJ4C_get_uint(QSJ4C_array_get(value, 1)));
    assert(3 == QSJ4C_get_uint(QSJ4C_array_get(value, 2)));
    assert(4 == QSJ4C_get_uint(QSJ4C_array_get(value, 3)));
    assert(5 == QSJ4C_get_uint(QSJ4C_array_get(value, 4)));
    assert(6 == QSJ4C_get_uint(QSJ4C_array_get(value, 5)));

    free((void*)value);
}

TEST(SimpleParsingTests, ParseArraysWithinArray) {
    const char json[] = "[[],[],[],[],[],[],[],[],[]]";
    QSJ4C_Value* value = QSJ4C_parse_opt_dynamic(json, ARRAY_COUNT(json), 0, realloc);
    assert(QSJ4C_is_array(value));
    assert(QSJ4C_array_size(value) == 9);

    assert(0 == QSJ4C_array_size(QSJ4C_array_get(value, 0)));
    assert(0 == QSJ4C_array_size(QSJ4C_array_get(value, 1)));
    assert(0 == QSJ4C_array_size(QSJ4C_array_get(value, 2)));
    assert(0 == QSJ4C_array_size(QSJ4C_array_get(value, 3)));
    assert(0 == QSJ4C_array_size(QSJ4C_array_get(value, 4)));
    assert(0 == QSJ4C_array_size(QSJ4C_array_get(value, 5)));
    assert(0 == QSJ4C_array_size(QSJ4C_array_get(value, 6)));
    assert(0 == QSJ4C_array_size(QSJ4C_array_get(value, 7)));
    assert(0 == QSJ4C_array_size(QSJ4C_array_get(value, 8)));

    free((void*)value);

}

TEST(SimpleParsingTests, ParseNumberArrayWithLineComments) {
    const char json[] = R"([// HO 
1, // HO 
 2, //
 3, //
4,5,6])";

    QSJ4C_Value* value = QSJ4C_parse_opt_dynamic(json, ARRAY_COUNT(json), 0, realloc);
    assert(QSJ4C_is_array(value));
    assert(QSJ4C_array_size(value) == 6);

    assert(1 == QSJ4C_get_uint(QSJ4C_array_get(value, 0)));
    assert(2 == QSJ4C_get_uint(QSJ4C_array_get(value, 1)));
    assert(3 == QSJ4C_get_uint(QSJ4C_array_get(value, 2)));
    assert(4 == QSJ4C_get_uint(QSJ4C_array_get(value, 3)));
    assert(5 == QSJ4C_get_uint(QSJ4C_array_get(value, 4)));
    assert(6 == QSJ4C_get_uint(QSJ4C_array_get(value, 5)));

    free((void*)value);
}

TEST(SimpleParsingTests, ParseMultilayeredObject) {
    const char json[] = R"({"id":1,"data":{"name":"foo","param":12}})";
    QSJ4C_Value* value = QSJ4C_parse_opt_dynamic(json, ARRAY_COUNT(json), 0, realloc);

    assert(QSJ4C_is_object(value));
    assert(QSJ4C_object_size(value) == 2);

    QSJ4C_Value* id_node = QSJ4C_object_get(value, "id");
    assert(QSJ4C_is_uint(id_node));
    assert(QSJ4C_get_uint(id_node) == 1);

    QSJ4C_Value* data_node = QSJ4C_object_get(value, "data");
    assert(QSJ4C_is_object(data_node));
    assert(QSJ4C_object_size(data_node) == 2);

    QSJ4C_Value* data_name_node = QSJ4C_object_get(data_node, "name");
    assert(QSJ4C_is_string(data_name_node));
    assert(strcmp(QSJ4C_get_string(data_name_node), "foo") == 0);

    QSJ4C_Value* data_param_node = QSJ4C_object_get(data_node, "param");
    assert(QSJ4C_is_uint(data_param_node));
    assert(QSJ4C_get_uint(data_param_node) == 12);

    free((void*)value);
}

TEST(SimpleParsingTests, ParseNumberArrayTrailingComma) {
    const char json[] = R"([1,2,])";
    QSJ4C_Value* value = QSJ4C_parse_opt_dynamic(json, ARRAY_COUNT(json), 0, realloc);
    assert(QSJ4C_is_array(value));
    assert(QSJ4C_array_size(value) == 2);

    assert(1 == QSJ4C_get_uint(QSJ4C_array_get(value, 0)));
    assert(2 == QSJ4C_get_uint(QSJ4C_array_get(value, 1)));

    free((void*)value);
}

TEST(SimpleParsingTests, ParseNumberArrayWhitespaces) {
    const char json[] = R"([ 1 , 2  , ])";
    QSJ4C_Value* value = QSJ4C_parse_opt_dynamic(json, ARRAY_COUNT(json), 0, realloc);
    assert(QSJ4C_is_array(value));
    assert(QSJ4C_array_size(value) == 2);

    assert(1 == QSJ4C_get_uint(QSJ4C_array_get(value, 0)));
    assert(2 == QSJ4C_get_uint(QSJ4C_array_get(value, 1)));

    free((void*)value);
}

TEST(SimpleParsingTests, ParseObjectArrayObjectCombination) {
    const char json[] = R"({"services":[{"id":1},{"id":2},{"id":3}]})";
    QSJ4C_Value* value = QSJ4C_parse_opt_dynamic(json, ARRAY_COUNT(json), 0, realloc);
    assert(QSJ4C_is_object(value));
    assert(QSJ4C_object_size(value) == 1);

    QSJ4C_Value* services_node = QSJ4C_object_get(value, "services");
    assert(QSJ4C_is_array(services_node));
    assert(QSJ4C_array_size(services_node) == 3);

    free((void*)value);
}

TEST(SimpleParsingTests, ParseObjectTrailingComma) {
    const char json[] = R"({"id":1,"name":"foo",})";
    QSJ4C_Value* value = QSJ4C_parse_opt_dynamic(json, ARRAY_COUNT(json), 0, realloc);
    assert(QSJ4C_is_object(value));
    assert(QSJ4C_object_size(value) == 2);

    free((void*)value);
}

/**
 * In this test case a scenario where the memory was corrupted internally is
 * reconstructed.
 */
TEST(SimpleParsingTests, ParseMemoryCornerCase) {
    const char json[] = R"({"b":[1],"c":"d"})";
    QSJ4C_Value* value = QSJ4C_parse_opt_dynamic(json, ARRAY_COUNT(json), 0, realloc);
    assert(!QSJ4C_is_error(value));
    free((void*)value);
}


TEST(SimpleParsingTests, ParseLineCommentInString) {
    const char json[] = R"(["Hallo//Welt"])";
    QSJ4C_Value* value = QSJ4C_parse_opt_dynamic(json, ARRAY_COUNT(json), 0, realloc);
    assert(!QSJ4C_is_error(value));
    free((void*)value);
}

TEST(SimpleParsingTests, ParseDoubleValues) {
    const char json[] = R"([0.123456789e-12, 1.234567890E+34, 23456789012E66, -9876.543210])";
    QSJ4C_Value* value = QSJ4C_parse_opt_dynamic(json, ARRAY_COUNT(json), 0, realloc);
    assert(QSJ4C_is_array(value));
    assert(QSJ4C_is_float(QSJ4C_array_get(value, 0)));
    assert(QSJ4C_is_float(QSJ4C_array_get(value, 1)));
    assert(QSJ4C_is_float(QSJ4C_array_get(value, 2)));
    assert(QSJ4C_is_float(QSJ4C_array_get(value, 3)));
    free((void*)value);
}

TEST(SimpleParsingTests, ParseNumericEValues) {
    const char json[] = R"([1e1,0.1e1])";
    QSJ4C_Value* value = QSJ4C_parse_opt_dynamic(json, ARRAY_COUNT(json), 0, realloc);
    assert(QSJ4C_is_array(value));
    assert(QSJ4C_is_float(QSJ4C_array_get(value, 0)));
    assert(QSJ4C_is_float(QSJ4C_array_get(value, 1)));
    free((void*)value);
}

TEST(SimpleParsingTests, ParseUint64Max) {
    const char json[] = R"([18446744073709551615])";
    QSJ4C_Value* value = QSJ4C_parse_opt_dynamic(json, ARRAY_COUNT(json), 0, realloc);
    assert(QSJ4C_is_array(value));
    QSJ4C_Value* array_value = QSJ4C_array_get(value, 0);

    assert(!QSJ4C_is_uint(array_value));
    assert(!QSJ4C_is_int(array_value));
    assert(QSJ4C_is_float(array_value));

    free((void*)value);
}

TEST(SimpleParsingTests, ParseInt64Max) {
    const char json[] = R"([9223372036854775807])";
    QSJ4C_Value* value = QSJ4C_parse_opt_dynamic(json, ARRAY_COUNT(json), 0, realloc);
    assert(QSJ4C_is_array(value));
    QSJ4C_Value* array_value = QSJ4C_array_get(value, 0);

    assert(!QSJ4C_is_uint(array_value));
    assert(!QSJ4C_is_int(array_value));
    assert(QSJ4C_is_float(array_value));

    free((void*)value);
}

TEST(SimpleParsingTests, ParseInt64MaxPlus1) {
    const char json[] = R"([9223372036854775808])";
    QSJ4C_Value* value = QSJ4C_parse_opt_dynamic(json, ARRAY_COUNT(json), 0, realloc);
    assert(QSJ4C_is_array(value));
    QSJ4C_Value* array_value = QSJ4C_array_get(value, 0);

    assert(!QSJ4C_is_uint(array_value));
    assert(!QSJ4C_is_int(array_value));
    assert(QSJ4C_is_float(array_value));

    free((void*)value);
}

TEST(SimpleParsingTests, ParseInt64Min) {
    const char json[] = R"([-9223372036854775808])";
    QSJ4C_Value* value = QSJ4C_parse_opt_dynamic(json, ARRAY_COUNT(json), 0, realloc);
    assert(QSJ4C_is_array(value));
    QSJ4C_Value* array_value = QSJ4C_array_get(value, 0);

    assert(!QSJ4C_is_uint(array_value));
    assert(!QSJ4C_is_int(array_value));
    assert(QSJ4C_is_float(array_value));

    free((void*)value);
}

TEST(SimpleParsingTests, ParseInt64MaxMinus1) {
    const char json[] = R"([-9223372036854775809])";
    QSJ4C_Value* value = QSJ4C_parse_opt_dynamic(json, ARRAY_COUNT(json), 0, realloc);
    assert(QSJ4C_is_array(value));
    QSJ4C_Value* array_value = QSJ4C_array_get(value, 0);

    assert(!QSJ4C_is_uint(array_value));
    assert(!QSJ4C_is_int(array_value));
    assert(QSJ4C_is_float(array_value));

    free((void*)value);
}


TEST(SimpleParsingTests, ParseUint64MaxPlus1) {
    const char json[] = R"([18446744073709551616])";
    QSJ4C_Value* value = QSJ4C_parse_opt_dynamic(json, ARRAY_COUNT(json), 0, realloc);
    assert(QSJ4C_is_array(value));
    assert(!QSJ4C_is_uint(QSJ4C_array_get(value, 0)));
    assert(!QSJ4C_is_int(QSJ4C_array_get(value, 0)));
    assert(QSJ4C_is_float(QSJ4C_array_get(value, 0)));
    free((void*)value);
}

TEST(SimpleParsingTests, ParseStringWithAllEscapeCharacters) {
    const char json[]  = R"(["\" \\ \/ \b \f \n \r \t"])";
    QSJ4C_Value* value = QSJ4C_parse_opt_dynamic(json, ARRAY_COUNT(json), 0, realloc);

    assert(QSJ4C_is_array(value));

    QSJ4C_Value* entry = QSJ4C_array_get(value, 0);
    assert(QSJ4C_is_string(entry));

    const char* string_value = QSJ4C_get_string(entry);
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
    QSJ4C_Value* value = QSJ4C_parse_opt_dynamic(json, ARRAY_COUNT(json), 0, realloc);
    assert(QSJ4C_is_error(value));
    assert(QSJ4C_error_get_errno(value) == QSJ4C_ERROR_JSON_MESSAGE_TRUNCATED);
    assert(QSJ4C_error_get_json_pos(value) == 1);
    free((void*)value);
}

TEST(ErrorHandlingTests, ParseInvalidComment) {
    const char json[] = R"([/# #/])";
    QSJ4C_Value* value = QSJ4C_parse_opt_dynamic(json, ARRAY_COUNT(json), 0, realloc);
    assert(QSJ4C_is_error(value));
    assert(QSJ4C_error_get_errno(value) == QSJ4C_ERROR_UNEXPECTED_CHAR);
    free((void*)value);
}

TEST(ErrorHandlingTests, ParseNeverEndingLineComment) {
    const char json[] = R"([// ])";
    QSJ4C_Value* value = QSJ4C_parse_opt_dynamic(json, ARRAY_COUNT(json), 0, realloc);
    assert(QSJ4C_is_error(value));
    assert(QSJ4C_error_get_errno(value) == QSJ4C_ERROR_JSON_MESSAGE_TRUNCATED);
    free((void*)value);
}

TEST(ErrorHandlingTests, ParseNeverEndingLineBlockComment) {
    const char json[] = R"([/* ])";
    QSJ4C_Value* value = QSJ4C_parse_opt_dynamic(json, ARRAY_COUNT(json), 0, realloc);
    assert(QSJ4C_is_error(value));
    assert(QSJ4C_error_get_errno(value) == QSJ4C_ERROR_JSON_MESSAGE_TRUNCATED);
    free((void*)value);
}

TEST(ErrorHandlingTests, ParseIncompleteArray) {
    const char json[] = R"([)";
    QSJ4C_Value* value = QSJ4C_parse_opt_dynamic(json, ARRAY_COUNT(json), 0, realloc);
    assert(QSJ4C_is_error(value));
    assert(QSJ4C_error_get_errno(value) == QSJ4C_ERROR_JSON_MESSAGE_TRUNCATED);
    assert(QSJ4C_error_get_json_pos(value) == 1);
    free((void*)value);
}

TEST(ErrorHandlingTests, ParseIncompleteString) {
    const char json[] = R"(")";
    QSJ4C_Value* value = QSJ4C_parse_opt_dynamic(json, ARRAY_COUNT(json), 0, realloc);
    assert(QSJ4C_is_error(value));
    assert(QSJ4C_error_get_errno(value) == QSJ4C_ERROR_JSON_MESSAGE_TRUNCATED);
    free((void*)value);
}

TEST(ErrorHandlingTests, ParseObjectKeyWithoutStartingQuotes) {
    const char json[] = R"({id":1})";
    QSJ4C_Value* value = QSJ4C_parse_opt_dynamic(json, ARRAY_COUNT(json), 0, realloc);
    assert(QSJ4C_is_error(value));
    assert(QSJ4C_error_get_errno(value) == QSJ4C_ERROR_UNEXPECTED_CHAR);
    free((void*)value);
}


TEST(ErrorHandlingTests, ParseBombasticArray) {
    char json[] = "[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]";
    QSJ4C_Value* value = QSJ4C_parse_opt_dynamic(json, ARRAY_COUNT(json), 0, realloc);
    assert(QSJ4C_is_error(value));
    assert(QSJ4C_error_get_errno(value) == QSJ4C_ERROR_DEPTH_OVERFLOW);
    free((void*)value);
}

TEST(ErrorHandlingTests, ParseBombasticObject) {
    char json[] = R"({"a":{"a":{"a":{"a":{"a":{"a":{"a":{"a":{"a":{"a":{"a":{"a":{"a":{"a":{"a":{"a":{"a":{"a":{"a":{"a":{"a":{"a":{"a":{"a":{"a":{"a":{"a":{"a":{"a":{"a":{"a":{"a":{"a":{"a":{}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}})";
    QSJ4C_Value* value = QSJ4C_parse_opt_dynamic(json, ARRAY_COUNT(json), 0, realloc);
    assert(QSJ4C_is_error(value));
    assert(QSJ4C_error_get_errno(value) == QSJ4C_ERROR_DEPTH_OVERFLOW);
    free((void*)value);
}

TEST(ErrorHandlingTests, IncompleteNumberAfterComma) {
    char json[] = R"([1.])";
    QSJ4C_Value* value = QSJ4C_parse_opt_dynamic(json, ARRAY_COUNT(json), 0, realloc);
    assert(QSJ4C_is_error(value));
    assert(QSJ4C_error_get_errno(value) == QSJ4C_ERROR_INVALID_NUMBER_FORMAT);
    free((void*)value);
}

TEST(ErrorHandlingTests, InvalidUnicodeSequence) {
    char json[] = R"(["\u99XA"])";
    QSJ4C_Value* value = QSJ4C_parse_opt_dynamic(json, ARRAY_COUNT(json), 0, realloc);
    assert(QSJ4C_is_error(value));
    assert(QSJ4C_error_get_errno(value) == QSJ4C_ERROR_INVALID_UNICODE_SEQUENCE);
    free((void*)value);
}

TEST(ErrorHandlingTests, GetTypeWithNullpointer) {
    assert(QSJ4C_TYPE_NULL == QSJ4C_get_type( NULL ));
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
        QSJ4C_Value* val = nullptr;
        QSJ4C_parse(invalid_string, buff, ARRAY_COUNT(buff), &val);
        assert(QSJ4C_is_error(val));
    }
}

/**
 * This verifies that the handling for a truncated buffer will also work in case
 * the start of an element is truncated!
 */
TEST(ErrorHandlingTests, ParseTruncatedString) {
    const char json[] = R"({"id":123, "name": )";
    uint8_t buff[256];
    QSJ4C_Value* val = nullptr;
    QSJ4C_parse(json, buff, ARRAY_COUNT(buff), &val);

    assert(QSJ4C_is_error(val));
    assert(QSJ4C_error_get_errno(val) == QSJ4C_ERROR_JSON_MESSAGE_TRUNCATED);
}

TEST(ErrorHandlingTests, TabInJsonString) {
    const char json[] = R"({"id":123, "name": ")" "\t" "\"";
    uint8_t buff[256];
    QSJ4C_Value* val = nullptr;
    QSJ4C_parse(json, buff, ARRAY_COUNT(buff), &val);

    assert(QSJ4C_is_error(val));
    assert(QSJ4C_error_get_errno(val) == QSJ4C_ERROR_UNEXPECTED_CHAR);
}

TEST(ErrorHandlingTests, InvalidEscapeCharacterInString) {
    const char json[] = R"({"id":123, "name": "\x")";
    uint8_t buff[256];
    QSJ4C_Value* val = nullptr;
    QSJ4C_parse(json, buff, ARRAY_COUNT(buff), &val);

    assert(QSJ4C_is_error(val));
    assert(QSJ4C_error_get_errno(val) == QSJ4C_ERROR_INVALID_ESCAPE_SEQUENCE);
}

/**
 * This test verifies that a unicode character is not accepted in case it is incomplete.
 */
TEST(ErrorHandlingTests, InvalidLongUnicodeSequenceNoAppendingSurrogate) {
    const char json[] = R"({"id":123, "name": "\uD800")";
    uint8_t buff[256];
    QSJ4C_Value* val = nullptr;
    QSJ4C_parse(json, buff, ARRAY_COUNT(buff), &val);

    assert(QSJ4C_is_error(val));
    assert(QSJ4C_error_get_errno(val) == QSJ4C_ERROR_INVALID_UNICODE_SEQUENCE);
}

/**
 * The same as the other but now a \ has been added to potentially confuse the parser.
 */
TEST(ErrorHandlingTests, InvalidLongUnicodeSequenceIncompleteAppendingSurrogate) {
    const char json[] = R"({"id":123, "name": "\uD800\")";
    uint8_t buff[256];
    QSJ4C_Value* val = nullptr;
    QSJ4C_parse(json, buff, ARRAY_COUNT(buff), &val);

    assert(QSJ4C_is_error(val));
    assert(QSJ4C_error_get_errno(val) == QSJ4C_ERROR_INVALID_UNICODE_SEQUENCE);
}

/**
 * The same as the other but now only three digits are present.
 */
TEST(ErrorHandlingTests, InvalidLongUnicodeSequenceIncompleteAppendingSurrogate2) {
    const char json[] = R"({"id":123, "name": "\uD800\udC0")";
    uint8_t buff[256];
    QSJ4C_Value* val = nullptr;
    QSJ4C_parse(json, buff, ARRAY_COUNT(buff), &val);

    assert(QSJ4C_is_error(val));
    assert(QSJ4C_error_get_errno(val) == QSJ4C_ERROR_INVALID_UNICODE_SEQUENCE);
}

/**
 * The same as the other but now only three digits are present.
 */
TEST(ErrorHandlingTests, InvalidLongUnicodeSequenceInvalidLowSurrogateTooLowValue) {
    const char json[] = R"({"id":123, "name": "\uD800\udbff")";
    uint8_t buff[256];
    QSJ4C_Value* val = nullptr;
    QSJ4C_parse(json, buff, ARRAY_COUNT(buff), &val);

    assert(QSJ4C_is_error(val));
    assert(QSJ4C_error_get_errno(val) == QSJ4C_ERROR_INVALID_UNICODE_SEQUENCE);
}

/**
 * The same as the other but now only three digits are present.
 */
TEST(ErrorHandlingTests, InvalidLongUnicodeSequenceInvalidLowSurrogateTooHighValue) {
    const char json[] = R"({"id":123, "name": "\uD800\ue000")";
    uint8_t buff[256];
    QSJ4C_Value* val = nullptr;
    QSJ4C_parse(json, buff, ARRAY_COUNT(buff), &val);

    assert(QSJ4C_is_error(val));
    assert(QSJ4C_error_get_errno(val) == QSJ4C_ERROR_INVALID_UNICODE_SEQUENCE);
}

/**
 * This test verifies that a unicode character is not accepted in case it is incomplete.
 */
TEST(ErrorHandlingTests, InvalidLongUnicodeSequenceNoHighSurrogate) {
    const char json[] = R"({"id":123, "name": "\uDc00")";
    uint8_t buff[256];
    QSJ4C_Value* val = nullptr;
    QSJ4C_parse(json, buff, ARRAY_COUNT(buff), &val);

    assert(QSJ4C_is_error(val));
    assert(QSJ4C_error_get_errno(val) == QSJ4C_ERROR_INVALID_UNICODE_SEQUENCE);
}

/**
 * This verifies that the handling for a truncated buffer will also work in case
 * the start of an element is truncated!
 */
TEST(ErrorHandlingTests, ParseObjectWithoutQuotesOnValue) {
    const char json[] = R"({"id":123, "name": hossa})";
    uint8_t buff[256];
    QSJ4C_Value* val = nullptr;
    QSJ4C_parse(json, buff, ARRAY_COUNT(buff), &val);

    assert(QSJ4C_is_error(val));
    assert(QSJ4C_error_get_errno(val) == QSJ4C_ERROR_UNEXPECTED_CHAR);
}

TEST(ErrorHandlingTests, ParseObjectMissingComma) {
    const char json[] = R"({"id":123 "name": "hossa"})";
    uint8_t buff[256];
    QSJ4C_Value* val = nullptr;
    QSJ4C_parse(json, buff, ARRAY_COUNT(buff), &val);

    assert(QSJ4C_is_error(val));
    assert(QSJ4C_error_get_errno(val) == QSJ4C_ERROR_MISSING_COMMA);
}

TEST(ErrorHandlingTests, ParseObjectMissingColon) {
    const char json[] = R"({"id":123, "name" "hossa"})";
    uint8_t buff[256];
    QSJ4C_Value* val = nullptr;
    QSJ4C_parse(json, buff, ARRAY_COUNT(buff), &val);

    assert(QSJ4C_is_error(val));
    assert(QSJ4C_error_get_errno(val) == QSJ4C_ERROR_MISSING_COLON);
}

TEST(ErrorHandlingTests, ParseArrayMissingComma) {
    const char json[] = R"([1, 2 3])";
    uint8_t buff[256];
    QSJ4C_Value* val = nullptr;
    QSJ4C_parse(json, buff, ARRAY_COUNT(buff), &val);

    assert(QSJ4C_is_error(val));
    assert(QSJ4C_error_get_errno(val) == QSJ4C_ERROR_MISSING_COMMA);
}

TEST(ErrorHandlingTests, ParseDoubleWithInvalidExponentFormat) {
    const char json[] = R"(1.2e)";
    uint8_t buff[256];
    QSJ4C_Value* val = nullptr;
    QSJ4C_parse(json, buff, ARRAY_COUNT(buff), &val);

    assert(QSJ4C_is_error(val));
    assert(QSJ4C_error_get_errno(val) == QSJ4C_ERROR_INVALID_NUMBER_FORMAT);
}

TEST(ErrorHandlingTests, InvalidBufferSizeToReportError) {
    uint8_t buff[sizeof(QSJ4C_Value) - 1];
    const char json[] = "";
    QSJ4C_Value* val = nullptr;
    QSJ4C_parse(json, buff, ARRAY_COUNT(buff), &val);
    assert(QSJ4C_is_not_set(val)); // buffer is event too small to store the error
}

/**
 * In this test the first pass will trigger the error that the buffer is too small as
 * it cannot store more statistics and as realloc cannot be called the parsing fails.
 */
TEST(ErrorHandlingTests, BufferTooSmallToStoreStatstics) {
    uint8_t buff[sizeof(QSJ4C_Value) + sizeof(QSJ4C_Value)];
    const char json[] = "[[],[],[],[],[],[],[],[],[]]";
    QSJ4C_Value* val = nullptr;
    QSJ4C_parse(json, buff, ARRAY_COUNT(buff), &val);
    assert(QSJ4C_is_error(val));
    assert(QSJ4C_error_get_errno(val) == QSJ4C_ERROR_STORAGE_BUFFER_TO_SMALL);
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
    QSJ4C_Value* value = QSJ4C_parse_dynamic(json, lambda);
    assert(QSJ4C_is_error(value));

    assert(QSJ4C_error_get_errno(value) == QSJ4C_ERROR_ALLOCATION_ERROR);
}

/**
 * In this test the first pass will trigger the error that the buffer is too small as
 * it cannot store more statistics and as realloc fails parsing fails as well.
 * This time using objects (as the implementation might not be the same as with arrays)
 */
TEST(ErrorHandlingTests, BufferTooSmallToStoreStatsticsReallocFailsWithObject) {
    const char json[] = "[{},{},{},{},{},{},{},{},{}]";
    static bool first = true;
    auto lambda = []( void *ptr, size_t size ) {
        if ( first ) {
            first = false;
            return realloc(ptr, size);
        }
        return (void*)NULL;
    };
    // just reduce the buffer size by one single byte
    QSJ4C_Value* value = QSJ4C_parse_dynamic(json, lambda);
    assert(QSJ4C_is_error(value));

    assert(QSJ4C_error_get_errno(value) == QSJ4C_ERROR_ALLOCATION_ERROR);
}

TEST(ErrorHandlingTests, ParseMultipleLongStrings) {
    static const char* TEST_JSON_1 = R"json(
    {
       "startup": [
          {
             "exec_start": "/path/to/my/binary"
          },
          {
             "exec_start": "/path/to/my/binary"
          },
          {
             "exec_start": "/path/to/my/binary"
          },
          {
             "exec_start": "/path/to/my/binary"
          },
          {
             "exec_start": "/path/to/my/binary"
          },
          {
             "exec_start": "/path/to/my/binary"
          },
          {
             "exec_start": "/path/to/my/binary"
          },
          {
             "exec_start": "/path/to/my/binary"
          },
          {
             "exec_start": "/path/to/my/binary"
          }
       ]
    }
    )json";

    QSJ4C_Value* value = QSJ4C_parse_dynamic(TEST_JSON_1, realloc);
    assert(QSJ4C_is_object(value));

    QSJ4C_Value* startup_array = QSJ4C_object_get(value, "startup");
    assert(QSJ4C_is_array(startup_array));

    for (size_t i = 0; i < QSJ4C_array_size(startup_array); ++i) {
        QSJ4C_Value* val = QSJ4C_array_get(startup_array, i);
        assert(QSJ4C_is_object(val));

        QSJ4C_Value* path_val = QSJ4C_object_get(val, "exec_start");
        assert(QSJ4C_is_string(path_val));

        assert(strcmp("/path/to/my/binary", QSJ4C_get_string(path_val)) == 0);
    }
}

/**
 * This test will verify that in case an object is accessed incorrectly (with methods of a string or uint)
 * the default value will be returned in case a custom fatal function is set.
 */
TEST(DomObjectAccessTests, ObjectAccess) {
    QSJ4C_Builder builder;
    QSJ4C_Value value;
    QSJ4C_set_object_data(&value, nullptr, 0);

    static int called = 0;
    auto lambda = [](){
        ++called;
    };
    QSJ4C_register_fatal_error_function(lambda);

    assert(0.0 == QSJ4C_get_float(&value));
    assert(1 == called);
    assert(0 == QSJ4C_get_uint(&value));
    assert(2 == called);
    assert(0 == QSJ4C_get_int(&value));
    assert(3 == called);
    assert(false == QSJ4C_get_bool(&value));
    assert(4 == called);
    assert(strcmp("",QSJ4C_get_string(&value))==0);
    assert(5 == called);
    assert(0 == QSJ4C_array_size (&value));
    assert(6 == called);
    assert(NULL == QSJ4C_array_get(&value, 0));
    assert(7 == called);

    // as the object is empty this should also fail!
    assert(NULL == QSJ4C_object_get_member(&value, 0));
    assert(8 == called);
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
    QSJ4C_register_fatal_error_function(lambda);

    int expected = 0;

    assert(!QSJ4C_is_object(NULL));

    assert(0 == QSJ4C_object_size(NULL));
    assert(++expected == called);
    assert(NULL == QSJ4C_object_get_member(NULL, 0));
    assert(++expected == called);
    assert(NULL == QSJ4C_object_get(NULL, "id"));
    assert(++expected == called);
    assert(NULL == QSJ4C_member_get_key(NULL));
    assert(++expected == called);
    assert(NULL == QSJ4C_member_get_value(NULL));
    assert(++expected == called);
}

/**
 * This test will verify that in case an array is accessed incorrectly (with methods of a string or uint)
 * the default value will be returned in case a custom fatal function is set.
 */
TEST(DomObjectAccessTests, ArrayAccess) {
    QSJ4C_Builder builder;
    QSJ4C_Value value;
    QSJ4C_set_array_data(&value, nullptr, 0);

    static int called = 0;
    auto lambda = [](){
        ++called;
    };
    QSJ4C_register_fatal_error_function(lambda);

    int expected = 0;

    assert(0.0 == QSJ4C_get_float(&value));
    assert(++expected == called);
    assert(0 == QSJ4C_get_uint(&value));
    assert(++expected == called);
    assert(0 == QSJ4C_get_int(&value));
    assert(++expected == called);
    assert(false == QSJ4C_get_bool(&value));
    assert(++expected == called);
    assert(strcmp("",QSJ4C_get_string(&value))==0);
    assert(++expected == called);
    assert(0 == QSJ4C_object_size (&value));
    assert(++expected == called);
    assert(NULL == QSJ4C_object_get_member(&value, 0));
    assert(++expected == called);

    // as the array has size 0 this should also fail!
    assert(NULL == QSJ4C_array_get(&value, 0));
    assert(++expected == called);
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
    QSJ4C_register_fatal_error_function(lambda);

    int expected = 0;

    assert(!QSJ4C_is_array(NULL));

    assert(0 == QSJ4C_array_size(NULL));
    assert(++expected == called);
    assert(NULL == QSJ4C_array_get(NULL, 0));
    assert(++expected == called);
}

TEST(DomObjectAccessTests, Uint32Access) {
    QSJ4C_Value value;
    QSJ4C_set_uint(&value, UINT32_MAX);
    static int called = 0;
    auto lambda = [](){
        ++called;
    };
    QSJ4C_register_fatal_error_function(lambda);
    QSJ4C_get_float(&value); // don't check the value as only the invocation of the error function is interesting.
    assert(0 == called);
    assert(UINT32_MAX == QSJ4C_get_uint(&value));
    assert(0 == called);
    assert(0 == QSJ4C_get_int(&value));
    assert(1 == called);
    assert(false == QSJ4C_get_bool(&value));
    assert(2 == called);
    assert(strcmp("",QSJ4C_get_string(&value))==0);
    assert(3 == called);
    assert(0 == QSJ4C_object_size (&value));
    assert(4 == called);
    assert(NULL == QSJ4C_object_get_member(&value, 0));
    assert(5 == called);
    assert(NULL == QSJ4C_array_get(&value, 0));
    assert(6 == called);
}

TEST(DomObjectAccessTests, Int32Access) {
    QSJ4C_Value value;
    QSJ4C_set_int(&value, INT32_MIN);
    static int called = 0;
    auto lambda = [](){
        ++called;
    };
    QSJ4C_register_fatal_error_function(lambda);
    QSJ4C_get_float(&value); // don't check the value as only the invocation of the error function is interesting.
    assert(0 == called);
    assert(INT32_MIN == QSJ4C_get_int(&value));
    assert(0 == called);
    assert(0 == QSJ4C_get_uint(&value));
    assert(1 == called);
    assert(false == QSJ4C_get_bool(&value));
    assert(2 == called);
    assert(strcmp("",QSJ4C_get_string(&value))==0);
    assert(3 == called);
    assert(0 == QSJ4C_object_size (&value));
    assert(4 == called);
    assert(NULL == QSJ4C_object_get_member(&value, 0));
    assert(5 == called);
    assert(NULL == QSJ4C_array_get(&value, 0));
    assert(6 == called);
}

TEST(DomObjectAccessTests, Int32AccessMax) {
    QSJ4C_Value value;
    QSJ4C_set_int(&value, INT32_MAX);
    static int called = 0;
    auto lambda = [](){
        ++called;
    };
    QSJ4C_register_fatal_error_function(lambda);
    QSJ4C_get_float(&value); // don't check the value as only the invocation of the error function is interesting.
    assert(0 == called);
    assert(INT32_MAX == QSJ4C_get_int(&value));
    assert(0 == called);
    assert(INT32_MAX == QSJ4C_get_uint(&value));
    assert(0 == called);
    assert(false == QSJ4C_get_bool(&value));
    assert(1 == called);
    assert(strcmp("",QSJ4C_get_string(&value))==0);
    assert(2 == called);
    assert(0 == QSJ4C_object_size (&value));
    assert(3 == called);
    assert(NULL == QSJ4C_object_get_member(&value, 0));
    assert(4 == called);
    assert(NULL == QSJ4C_array_get(&value, 0));
    assert(5 == called);
}

TEST(DomObjectAccessTests, FloatAccess) {
    QSJ4C_Value value;
    QSJ4C_set_float(&value, 1.2345);
    static int called = 0;
    auto lambda = [](){
        ++called;
    };
    QSJ4C_register_fatal_error_function(lambda);
    QSJ4C_get_float(&value); // don't check the value as only the invocation of the error function is interesting.
    assert(0 == called);
    assert(0 == QSJ4C_get_int(&value));
    assert(1 == called);
    assert(0 == QSJ4C_get_uint(&value));
    assert(2 == called);
    assert(false == QSJ4C_get_bool(&value));
    assert(3 == called);
    assert(QSJ4C_string_equals(&value, ""));
    assert(4 == called);
    assert(0 == QSJ4C_object_size (&value));
    assert(5 == called);
    assert(NULL == QSJ4C_object_get_member(&value, 0));
    assert(6 == called);
    assert(NULL == QSJ4C_array_get(&value, 0));
    assert(7 == called);
}

TEST(DomObjectAccessTests, BoolAccess) {
    QSJ4C_Value value;
    QSJ4C_set_bool(&value, true);
    static int called = 0;
    auto lambda = [](){
        ++called;
    };
    QSJ4C_register_fatal_error_function(lambda);
    assert(true == QSJ4C_get_bool(&value));
    assert(0 == called);
    assert(0.0 == QSJ4C_get_float(&value)); // don't check the value as only the invocation of the error function is interesting.
    assert(1 == called);
    assert(0 == QSJ4C_get_int(&value));
    assert(2 == called);
    assert(0 == QSJ4C_get_uint(&value));
    assert(3 == called);
    assert(strcmp("",QSJ4C_get_string(&value))==0);
    assert(4 == called);
    assert(0 == QSJ4C_object_size (&value));
    assert(5 == called);
    assert(NULL == QSJ4C_object_get_member(&value, 0));
    assert(6 == called);
    assert(NULL == QSJ4C_array_get(&value, 0));
    assert(7 == called);
}

/**
 * This test verifies that a null value (not null pointer but null data type)
 * will not cause any trouble.
 */
TEST(DomObjectAccessTests, NullAccess) {
    QSJ4C_Value value;
    QSJ4C_set_null(&value);

    assert(!QSJ4C_is_not_set(&value));
    assert(QSJ4C_is_null(&value));

    static int called = 0;
    auto lambda = [](){
        ++called;
    };
    QSJ4C_register_fatal_error_function(lambda);
    assert(false == QSJ4C_get_bool(&value));
    assert(1 == called);
    assert(0.0 == QSJ4C_get_float(&value)); // don't check the value as only the invocation of the error function is interesting.
    assert(2 == called);
    assert(0 == QSJ4C_get_int(&value));
    assert(3 == called);
    assert(0 == QSJ4C_get_uint(&value));
    assert(4 == called);
    assert(strcmp("",QSJ4C_get_string(&value))==0);
    assert(5 == called);
    assert(0 == QSJ4C_object_size (&value));
    assert(6 == called);
    assert(NULL == QSJ4C_object_get_member(&value, 0));
    assert(7 == called);
    assert(NULL == QSJ4C_array_get(&value, 0));
    assert(8 == called);
}

/**
 * This test verifies that a null pointer thus NOT the null data type but ((void*)0).
 * will not cause any trouble.
 */
TEST(DomObjectAccessTests, NullPointerAccess) {
    assert(QSJ4C_is_not_set(NULL));
    assert(QSJ4C_is_null(NULL));

    static int called = 0;
    auto lambda = [](){
        ++called;
    };
    QSJ4C_register_fatal_error_function(lambda);
    assert(false == QSJ4C_get_bool(NULL));
    assert(1 == called);
    assert(0.0 == QSJ4C_get_float(NULL)); // don't check the value as only the invocation of the error function is interesting.
    assert(2 == called);
    assert(0 == QSJ4C_get_int(NULL));
    assert(3 == called);
    assert(0 == QSJ4C_get_uint(NULL));
    assert(4 == called);
    assert(strcmp("",QSJ4C_get_string(NULL))==0);
    assert(5 == called);
    assert(0 == QSJ4C_object_size (NULL));
    assert(6 == called);
    assert(NULL == QSJ4C_object_get_member(NULL, 0));
    assert(7 == called);
    assert(NULL == QSJ4C_array_get(NULL, 0));
    assert(8 == called);
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
    QSJ4C_register_fatal_error_function(lambda);

    int expected = 0;

    assert(!QSJ4C_is_string(NULL));

    assert(0 == QSJ4C_get_string_length(NULL));
    assert(++expected == called);
    assert(strcmp(QSJ4C_get_string(NULL), "") == 0);
    assert(++expected == called);
    assert(QSJ4C_string_cmp(NULL, "") == 0);
    assert(++expected == called);
    assert(QSJ4C_string_equals(NULL, ""));
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
    QSJ4C_register_fatal_error_function(lambda);

    int expected = 0;

    assert(!QSJ4C_is_string(NULL));

    assert(0 == QSJ4C_error_get_errno(NULL));
    assert(++expected == called);
    assert(0 == QSJ4C_error_get_json_pos(NULL));
    assert(++expected == called);
}

TEST(ErrorHandlingTests, TooSmallDomBuffer) {
    char json[] = "[0.123456,9,12,3,5,7,2,3]";
    // just reduce the buffer size by one single byte
    size_t buff_size = QSJ4C_calculate_max_buffer_size(json) - 1;
    char buff[buff_size];
    QSJ4C_Value* value = NULL;

    size_t actual_size = QSJ4C_parse(json, buff, buff_size, &value);
    assert(QSJ4C_is_error(value));

    assert(QSJ4C_error_get_errno(value) == QSJ4C_ERROR_STORAGE_BUFFER_TO_SMALL);
}

TEST(ErrorHandlingTests, ReallocFailsBegin) {
    char json[] = "[0.123456,9,12,3,5,7,2,3]";
    auto lambda = []( void *ptr, size_t size ) {return (void*)NULL;};
    // just reduce the buffer size by one single byte
    QSJ4C_Value* value = QSJ4C_parse_dynamic(json, lambda);
    assert(QSJ4C_is_not_set(value));
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
    QSJ4C_Value* value = QSJ4C_parse_dynamic(json, lambda);
    assert(QSJ4C_is_error(value));

    assert(QSJ4C_error_get_errno(value) == QSJ4C_ERROR_ALLOCATION_ERROR);
}

TEST(ErrorHandlingTests, DoubleSmallPrintBuffer) {
    char json[] = "[0.123456]";
    char out[ARRAY_COUNT(json) + 1];
    memset(out, '\n', ARRAY_COUNT(out));

    size_t buff_size = QSJ4C_calculate_max_buffer_size(json);
    char buff[buff_size];
    QSJ4C_Value* value = NULL;

    size_t actual_size = QSJ4C_parse(json, buff, buff_size, &value);
    assert(!QSJ4C_is_error(value));

    assert(actual_size == buff_size);

    for (size_t i = 0, n = ARRAY_COUNT(json); i <= n; i++) {
        assert(i == QSJ4C_sprint(value, out, i));
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

    size_t buff_size = QSJ4C_calculate_max_buffer_size(json);
    char buff[buff_size];
    QSJ4C_Value* value = NULL;

    size_t actual_size = QSJ4C_parse(json, buff, buff_size, &value);
    assert(!QSJ4C_is_error(value));

    assert(actual_size == buff_size);

    for (size_t i = 0, n = ARRAY_COUNT(json); i <= n; i++) {
        assert(i == QSJ4C_sprint(value, out, i));
        assert('\n' == out[i]); // check that nothing has been written over the bounds!
        if (i > 0) {
            assert(i - 1 == strlen(out));
        }
    }
}

TEST(ErrorHandlingTests, PrintStringPartially) {
    char json[] = R"(["so ka?","ey chummer!"])";
    char out[ARRAY_COUNT(json) + 1];
    memset(out, '\n', ARRAY_COUNT(out));

    size_t buff_size = QSJ4C_calculate_max_buffer_size(json);
    char buff[buff_size];
    QSJ4C_Value* value = NULL;

    size_t actual_size = QSJ4C_parse(json, buff, buff_size, &value);
    assert(!QSJ4C_is_error(value));

    assert(actual_size == buff_size);

    for (size_t i = 0, n = ARRAY_COUNT(json); i <= n; i++) {
    	size_t len = QSJ4C_sprint(value, out, i);
        assert(i == len);
        assert('\n' == out[i]); // check that nothing has been written over the bounds!
        if (i > 0) {
            assert(i - 1 == strlen(out));
        }
    }
}

TEST(ErrorHandlingTests, PrintCompositionOfObjectsAndArraysPartially) {
    char json[] = R"({"id":5,"values":[{},[],{"key":"val","key2":"val2"},[12,34],5.0]})";
    char out[ARRAY_COUNT(json) + 1];
    memset(out, '\n', ARRAY_COUNT(out));

    size_t buff_size = QSJ4C_calculate_max_buffer_size(json);
    char buff[buff_size];
    QSJ4C_Value* value = NULL;

    size_t actual_size = QSJ4C_parse(json, buff, buff_size, &value);
    assert(!QSJ4C_is_error(value));

    assert(actual_size == buff_size);

    for (size_t i = 0, n = ARRAY_COUNT(json); i <= n; i++) {
    	size_t len = QSJ4C_sprint(value, out, i);
        assert(i == len);
        assert('\n' == out[i]); // check that nothing has been written over the bounds!
        if (i > 0) {
            assert(i - 1 == strlen(out));
        }
    }
}

TEST(ErrorHandlingTests, PrintConstantsPartially) {
    char json[] = R"([null,true,false])";
    char out[ARRAY_COUNT(json) + 1];
    memset(out, '\n', ARRAY_COUNT(out));

    size_t buff_size = QSJ4C_calculate_max_buffer_size(json);
    char buff[buff_size];
    QSJ4C_Value* value = NULL;

    size_t actual_size = QSJ4C_parse(json, buff, buff_size, &value);
    assert(!QSJ4C_is_error(value));

    assert(actual_size == buff_size);

    for (size_t i = 0, n = ARRAY_COUNT(json); i <= n; i++) {
        assert(i == QSJ4C_sprint(value, out, i));
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

    size_t buff_size = QSJ4C_calculate_max_buffer_size(json);
    char buff[buff_size];
    QSJ4C_Value* value = NULL;

    size_t actual_size = QSJ4C_parse(json, buff, buff_size, &value);
    assert(!QSJ4C_is_error(value));

    assert(actual_size == buff_size);

    for (size_t i = 0, n = ARRAY_COUNT(json); i <= n; i++) {
        assert(i == QSJ4C_sprint(value, out, i));
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
    QSJ4C_register_fatal_error_function(lambda);

    QSJ4C_Value* value = QSJ4C_parse_opt_dynamic(json, ARRAY_COUNT(json), 0, realloc);

    assert(QSJ4C_is_error(value));
    size_t out = QSJ4C_sprint(value, output, ARRAY_COUNT(output));
    assert(0 == count); // expect no error
    assert(strcmp("", output) != 0); // expect no empty message!
}

/**
 * The following float value triggered that the number was printed as "64." on x86_64-linux.
 * Therefore a special rule for printing was added.
 */
TEST(PrintTests, PrintDoubleCornercase) {
    QSJ4C_Value value;
	float d = -63.999999999999943;
    char buffer[64] = {'\0'};

	QSJ4C_set_float(&value, d);
    size_t out = QSJ4C_sprint(&value, buffer, ARRAY_COUNT(buffer));
    assert( '\0' == buffer[out - 1]);
    assert( '.' != buffer[out - 2]);
}

/**
 * The following test triggers printing a very small numeric value and
 * verifies that this number is printed scientificly.
 */
TEST(PrintTests, PrintDoubleCornercase2) {
    QSJ4C_Value value;
	float d = -1.0e-10;
    char buffer[64] = {'\0'};

	QSJ4C_set_float(&value, d);
    size_t out = QSJ4C_sprint(&value, buffer, ARRAY_COUNT(buffer));
    assert( strchr( buffer, 'e') != nullptr );
}


TEST(PrintTests, PrintEmtpyObject) {
    char json[] = "{}";
    QSJ4C_Value* value = QSJ4C_parse_opt_dynamic(json, ARRAY_COUNT(json), 0, realloc);

    assert(!QSJ4C_is_error(value));

    char output[ARRAY_COUNT(json)];
    size_t out = QSJ4C_sprint(value, output, ARRAY_COUNT(output));
    assert(ARRAY_COUNT(output) == out);
    assert(strcmp(json, output) == 0);

    free((void*)value);
}

TEST(PrintTests, PrintNumericArray) {
    const char json[] = R"([1,2.10101,3,4.123456e+20,5.0,-6])";
    QSJ4C_Value* value = QSJ4C_parse_opt_dynamic(json, ARRAY_COUNT(json), 0, realloc);

    assert(!QSJ4C_is_error(value));

    char output[ARRAY_COUNT(json)];
    size_t out = QSJ4C_sprint(value, output, ARRAY_COUNT(output));
    // puts(output);
    assert(ARRAY_COUNT(output) == out);

    assert(strcmp(json, output) == 0);

    free((void*)value);
}

TEST(PrintTests, PrintStringArray) {
    const char json[] = R"(["1","22","333","4444","55555","666666"])";
    QSJ4C_Value* value = QSJ4C_parse_opt_dynamic(json, ARRAY_COUNT(json), 0, realloc);

    assert(!QSJ4C_is_error(value));

    char output[ARRAY_COUNT(json)];
    size_t out = QSJ4C_sprint(value, output, ARRAY_COUNT(output));
    assert(ARRAY_COUNT(output) == out);
    assert(strcmp(json, output) == 0);

    free((void*)value);
}

TEST(PrintTests, PrintMultiLayerObject) {
    const char json[] = R"({"id":1,"data":{"name":"foo","param":12}})";
    QSJ4C_Value* value = QSJ4C_parse_opt_dynamic(json, ARRAY_COUNT(json), 0, realloc);

    assert(!QSJ4C_is_error(value));

    char output[ARRAY_COUNT(json)];
    size_t out = QSJ4C_sprint(value, output, ARRAY_COUNT(output));

    assert(ARRAY_COUNT(output) == out);
    assert(strcmp(json, output) == 0);

    free((void*)value);
}

TEST(PrintTests, PrintDoubleArray) {
    const char json[] = R"([0.0])";
    QSJ4C_Value* value = QSJ4C_parse_opt_dynamic(json, ARRAY_COUNT(json), 0, realloc);

    assert(!QSJ4C_is_error(value));

    char output[ARRAY_COUNT(json)];
    size_t out = QSJ4C_sprint(value, output, ARRAY_COUNT(output));
    assert(ARRAY_COUNT(output) == out);
    assert(strcmp(json, output) == 0);

    free((void*)value);
}

TEST(PrintTests, PrintStringWithNewLine) {
    char json[] = R"(["Hello\nWorld"])";
    QSJ4C_Value* value = QSJ4C_parse_opt_dynamic(json, ARRAY_COUNT(json), 0, realloc);

    assert(!QSJ4C_is_error(value));

    char output[ARRAY_COUNT(json)];
    size_t out = QSJ4C_sprint(value, output, ARRAY_COUNT(output));
    assert(ARRAY_COUNT(output) == out);
    assert(strcmp(json, output) == 0);

    free((void*)value);
}

/**
 * Expect the control characters will be printed the same way.
 */
TEST(PrintTests, PrintControlCharacters) {
    char json[] = R"(["\" \\ \/ \b \f \n \r \t"])";
    QSJ4C_Value* value = QSJ4C_parse_opt_dynamic(json, ARRAY_COUNT(json), 0, realloc);

    assert(!QSJ4C_is_error(value));

    char output[ARRAY_COUNT(json)];
    size_t out = QSJ4C_sprint(value, output, ARRAY_COUNT(output));
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
    float nan_value = 0.0/0.0;

    QSJ4C_Value value;
    QSJ4C_set_float(&value, nan_value);
    QSJ4C_sprint(&value, output, ARRAY_COUNT(output));
    assert(strcmp("null", output) == 0);
}

/**
 * In this test the user input for "NaN" will be translated to 'null' (to at least have
 * some kind of defined behavior)
 */
TEST(PrintTests, PrintDoubleDomInfinity) {
    char output[64];
    float infinity_value = 1.0/0.0;

    QSJ4C_Value value;
    QSJ4C_set_float(&value, infinity_value);
    QSJ4C_sprint(&value, output, ARRAY_COUNT(output));
    assert(strcmp("null", output) == 0);
}

/**
 * The parser is build on some "assumptions" and this test should verify this
 * for the different platform.
 */
TEST(VariousTests, CheckSizes) {
    assert(sizeof(float) == 4);
    assert(sizeof(int32_t) == 4);
    assert(sizeof(uint32_t) == 4);

    printf("Sizeof QSJ4C_Value is: " FMT_SIZE "\n", sizeof(QSJ4C_Value));

    assert(sizeof(QSJ4C_Value) == 6);

    assert(sizeof(QSJ4C_Member) == 2 * sizeof(QSJ4C_Value));
}

TEST(StrictParsingTests, ParseValidNumericValues) {
    const char json[] = R"([0, 1.0, 0.015, -0.5, -0.005, -256])";
    QSJ4C_Value* value = QSJ4C_parse_opt_dynamic(json, ARRAY_COUNT(json), QSJ4C_PARSE_OPTS_STRICT, realloc);

    assert(QSJ4C_is_array(value));
    assert(QSJ4C_array_size(value) == 6);

    assert(0 == QSJ4C_get_uint(QSJ4C_array_get(value, 0)));
    assert(1.0 == QSJ4C_get_float(QSJ4C_array_get(value, 1)));
    assert(0.016 >= QSJ4C_get_float(QSJ4C_array_get(value, 2)) && 0.014 <= QSJ4C_get_float(QSJ4C_array_get(value, 2)));
    assert(-0.501 <= QSJ4C_get_float(QSJ4C_array_get(value, 3)) && -0.499 >= QSJ4C_get_float(QSJ4C_array_get(value, 3)));
    assert(-0.006 <= QSJ4C_get_float(QSJ4C_array_get(value, 4)) && -0.004 >= QSJ4C_get_float(QSJ4C_array_get(value, 4)));
    assert(-256 == QSJ4C_get_int(QSJ4C_array_get(value, 5)));

    free((void*)value);
}

TEST(StrictParsingTests, ParseNumericValueWithLeadingZero) {
    const char json[] = R"([007])";
    QSJ4C_Value* value = QSJ4C_parse_opt_dynamic(json, ARRAY_COUNT(json), QSJ4C_PARSE_OPTS_STRICT, realloc);

    assert(QSJ4C_is_error(value));
    assert(QSJ4C_error_get_errno(value) == QSJ4C_ERROR_INVALID_NUMBER_FORMAT);

    free((void*)value);
}

TEST(StrictParsingTests, ParseNumericValueWithLeadingPlus) {
    const char json[] = R"([+7])";
    QSJ4C_Value* value = QSJ4C_parse_opt_dynamic(json, ARRAY_COUNT(json), QSJ4C_PARSE_OPTS_STRICT, realloc);

    assert(QSJ4C_is_error(value));
    assert(QSJ4C_error_get_errno(value) == QSJ4C_ERROR_INVALID_NUMBER_FORMAT);

    free((void*)value);
}

TEST(StrictParsingTests, ParseMessageTrailingComma) {
    const char json[] = R"([7],)";
    QSJ4C_Value* value = QSJ4C_parse_opt_dynamic(json, ARRAY_COUNT(json), QSJ4C_PARSE_OPTS_STRICT, realloc);

    assert(QSJ4C_is_error(value));
    assert(QSJ4C_error_get_errno(value) == QSJ4C_ERROR_UNEXPECTED_JSON_APPENDIX);

    free((void*)value);
}

TEST(StrictParsingTests, ParseObjectTrailingComma) {
    const char json[] = R"({"id":123, "name": "hossa",})";
    uint8_t buff[256];
    QSJ4C_Value* val = nullptr;
    QSJ4C_parse_opt(json, SIZE_MAX, QSJ4C_PARSE_OPTS_STRICT, buff, ARRAY_COUNT(buff), &val);

    assert(QSJ4C_is_error(val));
    assert(QSJ4C_error_get_errno(val) == QSJ4C_ERROR_TRAILING_COMMA);
}

TEST(StrictParsingTests, ParseArrayTrailingComma) {
    const char json[] = R"([7,])";
    QSJ4C_Value* value = QSJ4C_parse_opt_dynamic(json, ARRAY_COUNT(json), QSJ4C_PARSE_OPTS_STRICT, realloc);

    assert(QSJ4C_is_error(value));
    assert(QSJ4C_error_get_errno(value) == QSJ4C_ERROR_TRAILING_COMMA);

    free((void*)value);
}

/**
 * In this test a cornercase of qajson4c is triggered. The last empty array within the
 * object will get its first pass statistics corrumpted by the array member.
 * The reason therefore is that no memory has to be stored for the elements of the array ...
 * but the memory of the array will already be "consumed" by the object creation ... thus
 * the last element that will "consume" memory will be the value of "a". The integer
 * value will be initialized before b, thus will cause b to be overwritten.
 */
TEST(CornerCaseTests, AttachedEmptyArray) {
    const char* json = R"({"a":[1],"b":[]})";
    QSJ4C_Value* value = QSJ4C_parse_dynamic(json, realloc);

    assert(QSJ4C_is_object(value));
    assert(QSJ4C_array_size(QSJ4C_object_get(value, "a")) == 1);
    assert(QSJ4C_array_size(QSJ4C_object_get(value, "b")) == 0);
}

/**
 * Same test as AttachedEmptyArray but now with an trailing empty object.
 */
TEST(CornerCaseTests, AttachedEmptyObject) {
    const char* json = R"({"a":[1],"b":{}})";
    QSJ4C_Value* value = QSJ4C_parse_dynamic(json, realloc);

    assert(QSJ4C_is_object(value));
}

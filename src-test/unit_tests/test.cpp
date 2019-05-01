/*
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

#ifdef __STRICT_ANSI__
#undef __STRICT_ANSI__
#endif

#include <iostream>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <signal.h>
#include <string.h>
#include <time.h>
#include <tuple>

#ifndef _WIN32
#include <wait.h>
#define FMT_SIZE "%zu"
#else
#define FMT_SIZE "%Iu"
#endif

#include "test.h"
#include "qajson4c/qajson4c.h"

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

QAJ4C_TEST_DEF* create_test( const char* subject, const char* test, void (*test_func)() ) {
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
            if (current->next == NULL) {
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
    std::cerr << test->subject_name << "." << test->test_name << std::endl;
    char buff[256];
    buff[0] = '\0';

    QAJ4C_register_fatal_error_function(NULL);

#ifndef _WIN32
    if (DEBUGGING) {
#endif
        test->test_func();
        tests_passed++;
#ifndef _WIN32
    } else {
        test->execution = -time(NULL);
        pid_t pid = fork();
        if (pid == 0) {
            test->test_func();
            exit(0);
        } else {
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

void flush_test_suite( FILE* fp, const char* suite_name, char* buff, size_t buff_size, test_suite_stats* stats ) {
    char testsuite_buffer[1024];
    int w =
            snprintf(
                    testsuite_buffer,
                    ARRAY_COUNT(testsuite_buffer),
                    "<testsuite errors=\"" FMT_SIZE "\" failures=\"" FMT_SIZE "\" name=\"%s\" skips=\"" FMT_SIZE "\" tests=\"" FMT_SIZE "\" time=\"" FMT_SIZE "\">%c",
                    stats->errors, stats->fails, suite_name, (size_t)0,
                    (size_t)(stats->errors + stats->fails + stats->pass), stats->total_time, '\n');
    fwrite(testsuite_buffer, w, sizeof(char), fp);
    fwrite(buff, sizeof(char), buff_size, fp);
    fwrite("</testsuite>\n", sizeof(char), ARRAY_COUNT("</testsuite>\n") - 1, fp);
    fflush(fp);
}

int test_main( int argc, char **argv ) {
    // output=xml:gtestresults.xml
    static const char* gtest_xml_notation = "xml:";

    const char* filename = "test.xml";
    for (int i = 0; i < argc; ++i) {
        char* candidate = NULL;
        if (strstr(argv[i], "--debug") == argv[i]) {
            DEBUGGING = true;
        } else if (strstr(argv[i], "output=") != NULL) {
            candidate = (strstr(argv[i], "output=") + strlen("output="));
        } else if (strstr(argv[i], "output") != NULL) {
            if (i + 1 < argc) {
                candidate = argv[i + 1];
            }
        }
        if (candidate != NULL) {
            if (strncmp(candidate, gtest_xml_notation, strlen(gtest_xml_notation)) == 0) {
                filename = candidate + strlen(gtest_xml_notation);
            } else {
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
        pos += snprintf(testsuite_content_buffer + pos, testsuite_content_buffer_size,
                        "<testcase classname=\"%s\" name=\"%s\" time=\"" FMT_SIZE "\">", curr_test->subject_name,
                        curr_test->test_name, curr_test->execution);
        if (curr_test->failure_message != NULL) {
            pos += snprintf(testsuite_content_buffer + pos, testsuite_content_buffer_size,
                            R"(<failure message="test failure">%s</failure>)", curr_test->failure_message);
            test_suite_stats.fails += 1;
        } else if (curr_test->error_message != NULL) {
            pos += snprintf(testsuite_content_buffer + pos, testsuite_content_buffer_size,
                            R"(<error message="test failure">%s</error>)", curr_test->error_message);
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
        free(curr_test->error_message);
        free(curr_test->failure_message);

        auto tmp = curr_test;
        curr_test = curr_test->next;
        free(tmp);
    }

    flush_test_suite(out, current_testsuite_name, testsuite_content_buffer, pos, &test_suite_stats);

    printf("%d tests of %d total tests passed (%d failed)\n", tests_passed, tests_total, tests_failed);

    fwrite("</testsuites>", sizeof(char), ARRAY_COUNT("</testsuites>") - 1, out);

    fclose(out);
    free(testsuite_content_buffer);
    exit(tests_failed > 0);
}




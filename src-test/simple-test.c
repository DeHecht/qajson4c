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
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>

#include <qajson4c/qajson4c.h>

/* Used by main to communicate with parse_opt. */
struct arguments
{
    char* input_file;
    char* output_file;
    bool dynamic_parsing;
    bool verbose;
};

void parse_args(int argc, char **argv, struct arguments* args);

#ifndef _WIN32

#define FMT_SIZE "%zu"

#include <argp.h>
const char *argp_program_version = "simple-test 1.0";
const char *argp_program_bug_address = "<noreply@doe.com>";

/* Program documentation. */
static char doc[] = "Simple test application that parses input files.";

/* A description of the arguments we accept. */
static char args_doc[] = "";

/* The options we understand. */
static struct argp_option options[] = {
  {"file",     'f', "FILE", 0, "Read input file.", 0 },
  {"output",   'o', "FILE", 0, "Filename to write the output json to.", 0 },
  {"dynamic",  'd', "bool", 0, "0 => off, 1 => on. (overrules insitu)", 0},
  {"verbose",  'v', 0,      0, "Print more information about allocated buffer sizes etc.", 0},
  { 0 }
};

/* Parse a single option. */
static error_t parse_opt (int key, char *arg, struct argp_state *state)
{
  /* Get the input argument from argp_parse, which we
     know is a pointer to our arguments structure. */
  struct arguments *arguments = state->input;

  switch (key)
    {
    case 'o':
      arguments->output_file = arg;
      break;
    case 'f':
      arguments->input_file = arg;
      break;
    case 'v':
      arguments->verbose = true;
      break;
    case 'd':
      arguments->dynamic_parsing = true;
      break;
    case ARGP_KEY_ARG:
        argp_usage(state);
        break;
    case ARGP_KEY_END:
        if( strcmp(arguments->input_file, "-") == 0 && strcmp(arguments->output_file, "-") == 0) {
            fprintf(stderr, "Error: Missing input or output file!\n");
            argp_usage(state);
        }

      break;

    default:
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
}

/* Our argp parser. */

static struct argp argp = { options, parse_opt, args_doc, doc, 0, 0, 0 };

void parse_args(int argc, char **argv, struct arguments *args) {
    argp_parse(&argp, argc, argv, 0, 0, args);
}
#else

#define FMT_SIZE "%Iu"

void parse_args(int argc, char **argv, struct arguments *args) {
    for ( int i = 0; i < argc; ++i ) {
        if ( strstr(argv[i], "--file") == argv[i] || strstr(argv[i], "-f") == argv[i]) {
            args->input_file = argv[i+1];
        }
        if ( strstr(argv[i], "--output") == argv[i] || strstr(argv[i], "-o") == argv[i]) {
            args->output_file = argv[i+1];
        }
        if ( strstr(argv[i], "--dynamic") == argv[i] || strstr(argv[i], "-d") == argv[i]) {
            args->dynamic_parsing = (argv[i+1][0] == '1');
        }
        if ( strstr(argv[i], "--verbose") == argv[i] || strstr(argv[i], "-v") == argv[i]) {
            args->verbose = true;
        }
    }
}
#endif

char* read_file_content(const char* filename) {
    FILE* fp = fopen(filename, "r");
    if (fp == NULL) {
        return NULL;
    }
    fseek(fp, 0L, SEEK_END);
    size_t sz = ftell(fp);
    fseek(fp, 0L, SEEK_SET);
    char* buff = malloc(sz * sizeof(char) + 1);
    fread(buff, sizeof(char), sz + 1, fp);
    fclose(fp);

    buff[sz] = '\0';
    return buff;
}

typedef struct QSJ4C_Statistics {
	size_t objectCount;
	size_t memberCount;
	size_t stringCount;
	size_t nullCount;
	size_t arrayCount;
	size_t elementCount;
	size_t stringLength;
	size_t numberCount;
	size_t trueCount;
	size_t falseCount;
} QSJ4C_Statistics;

void init(QSJ4C_Statistics* s) {
	s->objectCount = 0;
	s->memberCount = 0;
	s->stringCount = 0;
	s->nullCount = 0;
	s->arrayCount = 0;
	s->elementCount = 0;
	s->stringLength = 0;
	s->numberCount = 0;
	s->trueCount = 0;
	s->falseCount = 0;
}

void generate_statistics( QSJ4C_Statistics* s, QSJ4C_Value* v ) {
    QSJ4C_Member* member;
    size_t count;

	switch (QSJ4C_get_type(v)) {
	case QSJ4C_TYPE_OBJECT:
		count = QSJ4C_object_size(v);
		for (size_t index = 0; index < count; ++index) {
			member = QSJ4C_object_get_member(v, index);
			s->stringLength += QSJ4C_get_string_length(QSJ4C_member_get_key(member));
			generate_statistics(s, QSJ4C_member_get_value(member));
		}
		s->objectCount++;
		s->memberCount += count;
		s->stringCount += count;
		break;
	case QSJ4C_TYPE_NULL:
		s->nullCount++;
		break;
	case QSJ4C_TYPE_ARRAY:
		count = QSJ4C_array_size(v);
		for (size_t index = 0; index < count; ++index) {
			generate_statistics(s, QSJ4C_array_get(v, index));
		}
		s->arrayCount++;
		s->elementCount += count;
		break;
	case QSJ4C_TYPE_STRING:
		s->stringCount++;
		s->stringLength += QSJ4C_get_string_length(v);
		break;
	case QSJ4C_TYPE_NUMBER:
		s->numberCount++;
		break;
	case QSJ4C_TYPE_BOOL:
		if (QSJ4C_get_bool(v)) {
			s->trueCount++;
		} else {
			s->falseCount++;
		}
		break;
	default:
		break;
	}

}

int main(int argc, char **argv) {
    struct arguments arguments;

    /* Default values. */
    arguments.dynamic_parsing = false;
    arguments.input_file = "-";
    arguments.output_file = NULL;
    arguments.verbose = false;

    parse_args(argc, argv, &arguments);

    char* input_string = read_file_content(arguments.input_file);
    if (input_string == NULL) {
        fprintf(stderr, "Unable to open file '%s'\n", arguments.input_file);
        exit(1);
    }
    size_t input_string_size = strlen(input_string);

    FILE* output_file = NULL;

	if (arguments.output_file) {
		output_file = fopen(arguments.output_file, "w");
		if (output_file == NULL) {
			fprintf(stderr, "Unable to open file '%s'\n", arguments.output_file);
			free(input_string);
			exit(1);
		}
	}

    char* buffer = NULL;
    QSJ4C_Value* document = NULL;
    if ( arguments.dynamic_parsing ) {
        document = QSJ4C_parse_opt_dynamic(input_string, SIZE_MAX, QSJ4C_PARSE_OPTS_STRICT, realloc);
    } else {
        size_t buffer_size = QSJ4C_calculate_max_buffer_size(input_string);

        buffer = malloc(buffer_size);

		size_t buffer_used = QSJ4C_parse_opt(input_string, input_string_size, QSJ4C_PARSE_OPTS_STRICT, buffer, buffer_size, &document);
		if (arguments.verbose) {
			printf("Size of value " FMT_SIZE " (inclusive doc)\n", buffer_used);
		}

        if ( arguments.verbose ) {
            printf("Required buffer size %lu\n", buffer_size);
        }
    }

    QSJ4C_Statistics stats;
    init(&stats);
    generate_statistics(&stats, document);

    puts("Statistics");
    printf("objectCount:  %10zu\n", stats.objectCount);
    printf("arrayCount:   %10zu\n", stats.arrayCount);
    printf("numberCount:  %10zu\n", stats.numberCount);
    printf("stringCount:  %10zu\n", stats.stringCount);
    printf("trueCount:    %10zu\n", stats.trueCount);
    printf("falseCount:   %10zu\n", stats.falseCount);
    printf("nullCount:    %10zu\n", stats.nullCount);
    printf("memberCount:  %10zu\n", stats.memberCount);
    printf("elementCount: %10zu\n", stats.elementCount);
    printf("stringLength: %10zu\n", stats.stringLength);

    size_t output_string_size = sizeof(char) * input_string_size + 64;
    char* output_string = malloc(output_string_size);

    QSJ4C_sprint(document, output_string, output_string_size);

    if (output_file != NULL) {
        fwrite(output_string, sizeof(char), strlen(output_string), output_file);
        fclose(output_file);
    } else if (output_string_size > 0 && output_string[0] != '\0') {
        puts(output_string);
    }

	if (buffer == NULL) {
		free((void*)document);
	} else {
		free(buffer);
	}
    free(input_string);
    free(output_string);
    return 0;
}

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
#include <argp.h>

#include <qajson4c/qajson4c.h>

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
  {"buff-size",'b', "size", 0, "Buffer size in bytes. 0 => auto-detection.", 0 },
  {"insitu",   'i', "bool", 0, "0 => off, 1 => on.", 0 },
  {"verbose",  'v', 0,      0, "Print more information about allocated buffer sizes etc.", 0},
  { 0 }
};

/* Used by main to communicate with parse_opt. */
struct arguments
{
	char* input_file;
	char* output_file;
	size_t buffer_size;
	bool insitu_parsing;
	bool verbose;
};

/* Parse a single option. */
static error_t parse_opt (int key, char *arg, struct argp_state *state)
{
  /* Get the input argument from argp_parse, which we
     know is a pointer to our arguments structure. */
  struct arguments *arguments = state->input;

  switch (key)
    {
    case 'b':
      arguments->buffer_size = strtoul(arg, NULL, 0);
      break;
    case 'i':
      arguments->insitu_parsing = strtol(arg, NULL, 0);
      break;
    case 'o':
      arguments->output_file = arg;
      break;
    case 'f':
      arguments->input_file = arg;
      break;
    case 'v':
      arguments->verbose = true;
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

int main(int argc, char **argv) {
	struct arguments arguments;

	/* Default values. */
	arguments.insitu_parsing = 0;
	arguments.buffer_size = 0;
	arguments.input_file = "-";
	arguments.output_file = "-";
	arguments.verbose = false;

	argp_parse(&argp, argc, argv, 0, 0, &arguments);
	char* input_string = read_file_content(arguments.input_file);
	if (input_string == NULL) {
		fprintf(stderr, "Unable to open file '%s'\n", arguments.input_file);
		exit(1);
	}
	size_t input_string_size = strlen(input_string);

	FILE* output_file = fopen(arguments.output_file, "w");
	if ( output_file == NULL ) {
		fprintf(stderr, "Unable to open file '%s'\n", arguments.output_file);
		free(input_string);
		exit(1);
	}

	if ( arguments.buffer_size == 0 ) {
		if( arguments.insitu_parsing ) {
			arguments.buffer_size = QAJSON4C_calculate_max_buffer_size_insitu(input_string);
		} else {
			arguments.buffer_size = QAJSON4C_calculate_max_buffer_size(input_string);
		}
	}

	char* buffer = malloc(arguments.buffer_size);

	const QAJSON4C_Document* document = NULL;
	if( arguments.insitu_parsing ) {
		document = QAJSON4C_parse_insitu(input_string, buffer, arguments.buffer_size);
	} else {
		document = QAJSON4C_parse(input_string, buffer, arguments.buffer_size);
	}

	if ( arguments.verbose ) {
		printf("Required buffer size %lu\n", arguments.buffer_size);
	}

	size_t output_string_size = sizeof(char) * input_string_size;
	char* output_string = malloc(output_string_size);

	QAJSON4C_sprint(document, output_string, output_string_size);
	fwrite(output_string, sizeof(char), strlen(output_string), output_file);

	fclose(output_file);
	free(output_string);
	free(buffer);
	free(input_string);
	return 0;
}

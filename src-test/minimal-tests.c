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
#include <stdlib.h>
#include <assert.h>
#include <qajson4c/qajson4c.h>

void do_parse(char* json) {
	char outbuff[2048];
	printf("Test: %s\n", json);

	for( int i = 0; i < 2; i++) {
		char* buff = NULL;
		const QAJSON4C_Document* document = NULL;
		if (i == 0) {
			unsigned buffer_size = QAJSON4C_calculate_max_buffer_size(json);
			buff = malloc(sizeof(char) * buffer_size);
			printf("Required Buffer size: %u\n", buffer_size);
			document = QAJSON4C_parse(json, buff, buffer_size);
		} else if (i == 1) {
			unsigned buffer_size = QAJSON4C_calculate_max_buffer_size_insitu(json);
			buff = malloc(sizeof(char) * buffer_size);
			printf("Required Buffer size (insitu): %u\n", buffer_size);
			document = QAJSON4C_parse_insitu(json, buff, buffer_size);
		} else {
			assert(false);
		}
		if ( document != NULL ) {
			const QAJSON4C_Value* root_value = QAJSON4C_get_root_value(document);

			if ( QAJSON4C_is_error(root_value) ) {
				printf("ERROR: JSON message could not be parsed (stopped @ position %u)!\n", QAJSON4C_get_json_pos(root_value));
			} else {
				QAJSON4C_sprint(document, outbuff, 2048);
				printf("Printed: %s\n", outbuff);
			}
		} else {
			puts("Document is NULL");
		}
		free(buff);
	}
}

void test_dom_creation(void) {
	char buff[2048];
	char outbuff[2048];
	QAJSON4C_Builder builder;
	QAJSON4C_builder_init(&builder, buff, 2048);

	QAJSON4C_Document* document = QAJSON4C_builder_get_document(&builder);
	QAJSON4C_Value* root_value = QAJSON4C_get_root_value_rw(document);
	QAJSON4C_set_object(root_value, 2, &builder);

	QAJSON4C_Value* id_value = QAJSON4C_object_create_member(root_value, "id", .ref=true);
	QAJSON4C_set_uint(id_value, 123);

    QAJSON4C_Value* name_value = QAJSON4C_object_create_member(root_value, "name", .ref=true);
    QAJSON4C_set_string(name_value, "USE System Technology BV", .ref=true);

	QAJSON4C_sprint(document, outbuff, 2048);
	printf("Printed: %s\n", outbuff);
}

void test_dom_access(void) {
	puts("test_dom_access");
	char test[] = "{\"id\":1, \"name\": \"dude\", \"very_long_key_value\": 10}";
	char buff[2048];
	const QAJSON4C_Document* document = QAJSON4C_parse(test, buff, 2048);
	const QAJSON4C_Value* root_value = QAJSON4C_get_root_value(document);
	const QAJSON4C_Value* id_value = QAJSON4C_object_get(root_value, "id", true);
	assert(id_value != NULL);
	const QAJSON4C_Value* vlkv_value = QAJSON4C_object_get(root_value, "very_long_key_value", true);
	assert(vlkv_value != NULL);
}

int main() {
	char test1[] = "{}";
	char test2[] = "{\"id\":1, \"name\": \"dude\",}";
	char test3[] = "{\"id\":1, \"name\": \"Mr. John Doe the Third\"}";
	char subObjectTest[] = "{\"type\":\"object\", \"data\": {\"id\": 1}}";
	char subArrayTest[] = "{\"type\":\"object\", \"data\": [1,-2,3.0,4,5.1234E10,6,]}";
	char mainArray[] = "[1,2,3,4,5,6]";
	char bombasticNesting[] = "[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]";
	char numberJson[] = "123";
	char nullJson[] = "null";
	char trueJson[] = "true";
	char falseJson[] = "false";
	char someLongStrings[] = "{\"idddddddddddddddddi\":1, \"nameeeeeeeeeeeeeeeeeeeees\": /*** TEST COMMENT **/ \"Mr. John Doe the Third\"}";
	char empty[] = "";

	QAJSON4C_print_stats();

	test_dom_access();

	do_parse(numberJson);
	do_parse(nullJson);
	do_parse(trueJson);
	do_parse(falseJson);
	do_parse(test1);
	do_parse(test2);
	do_parse(test3);
	do_parse(mainArray);
	do_parse(subObjectTest);
	do_parse(subArrayTest);
	do_parse(someLongStrings);
	do_parse(bombasticNesting);
	do_parse(empty);

	test_dom_creation();
	return 0;
}

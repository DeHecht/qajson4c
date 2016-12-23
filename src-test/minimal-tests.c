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
    int json_len = strlen(json);
    char json_copy[json_len];
	char outbuff[2048];
	printf("Test: %s\n", json);

    for (int t = 0; t <= json_len; t++) {
        if (t > 0) {
            memcpy(json_copy, json, t);
        }
        json_copy[t] = '\0';

        for( int i = 0; i < 2; i++) {
            char* buff = NULL;
            const QAJ4C_Document* document = NULL;
            if (i == 0) {
                unsigned buffer_size = QAJ4C_calculate_max_buffer_size(json);
                buff = malloc(sizeof(char) * buffer_size);
                printf("Required Buffer size: %u\n", buffer_size);
                document = QAJ4C_parse(json_copy, buff, buffer_size);
            } else if (i == 1) {
                unsigned buffer_size = QAJ4C_calculate_max_buffer_size_insitu(json);
                buff = malloc(sizeof(char) * buffer_size);
                printf("Required Buffer size (insitu): %u\n", buffer_size);
                document = QAJ4C_parse_insitu(json_copy, buff, buffer_size);
            } else {
                assert(false);
            }
            if ( document != NULL ) {
                const QAJ4C_Value* root_value = QAJ4C_get_root_value(document);

                if ( QAJ4C_is_error(root_value) ) {
                    printf("ERROR: JSON message could not be parsed (stopped @ position %u)!\n", QAJ4C_error_get_json_pos(root_value));
                } else {
                    QAJ4C_sprint(document, outbuff, 2048);
                    printf("Printed: %s\n", outbuff);
                }
            } else {
                puts("Document is NULL");
            }
            free(buff);
        }

	}

    char buff[64];
    memset(buff, 0, 64);
    const QAJ4C_Document* document = QAJ4C_parse(json, buff, 64);
    const QAJ4C_Value* root_value = QAJ4C_get_root_value(document);

    if ( QAJ4C_is_error(root_value) ) {
        printf("ERROR: JSON message could not be parsed (stopped @ position %u), errno %d!\n", QAJ4C_error_get_json_pos(root_value), QAJ4C_error_get_errno(root_value));
    }

    document = QAJ4C_parse_dynamic(json, realloc);
    root_value = QAJ4C_get_root_value(document);

    if ( QAJ4C_is_error(root_value) ) {
        printf("ERROR: JSON message could not be parsed dynamically (stopped @ position %u), errno %d!\n", QAJ4C_error_get_json_pos(root_value), QAJ4C_error_get_errno(root_value));
    }

    free((QAJ4C_Document*)document);

}

void test_dom_creation(void) {
	char buff[2048];
	char outbuff[2048];
	QAJ4C_Builder builder;
	QAJ4C_builder_init(&builder, buff, 2048);

	QAJ4C_Document* document = QAJ4C_builder_get_document(&builder);
	QAJ4C_Value* root_value = QAJ4C_get_root_value_rw(document);
	QAJ4C_set_object(root_value, 2, &builder);

	QAJ4C_Value* id_value = QAJ4C_object_create_member_by_ref(root_value, "id");
	QAJ4C_set_uint(id_value, 123);

    QAJ4C_Value* name_value = QAJ4C_object_create_member_by_ref(root_value, "name");
    QAJ4C_set_string_ref(name_value, "USE System Technology BV");

	QAJ4C_sprint(document, outbuff, 2048);
	printf("Printed: %s\n", outbuff);
}

void test_dom_access(void) {
	puts("test_dom_access");
	char test[] = "{\"id\":1, \"name\": \"dude\", \"very_long_key_value\": 10}";
	char buff[2048];
	const QAJ4C_Document* document = QAJ4C_parse(test, buff, 2048);
	const QAJ4C_Value* root_value = QAJ4C_get_root_value(document);
	const QAJ4C_Value* id_value = QAJ4C_object_get(root_value, "id");
	assert(id_value != NULL);
	const QAJ4C_Value* vlkv_value = QAJ4C_object_get(root_value, "very_long_key_value");
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
	char arrayWithObjects[] = "[{\"id\":1, },{\"id\":2},{\"id\":3}]";

	QAJ4C_print_stats();

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
	do_parse(arrayWithObjects);

	test_dom_creation();
	return 0;
}

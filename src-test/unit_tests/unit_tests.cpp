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

#include <cassert>
#include <tuple>

#include "unit_tests/test.h"
#include "qajson4c/qajson4c.h"
#include "qajson4c/internal/types.h"

TEST(BufferSizeTests, ParseObjectWithOneNumericMember) {
    const char json[] = R"({"id":1})";

    size_t normal_required_buffer_size = QAJ4C_calculate_max_buffer_size_n(json, ARRAY_COUNT(json));
    size_t insitu_required_buffer_size = QAJ4C_calculate_max_buffer_size_insitu_n(json, ARRAY_COUNT(json));
    assert(normal_required_buffer_size == insitu_required_buffer_size);

    // one object + one member
    assert(normal_required_buffer_size == (sizeof(QAJ4C_Value) + sizeof(QAJ4C_Member)));
}

TEST(BufferSizeTests, ParseObjectWithTwoNumericMembers) {
    const char json[] = R"({"id":1,"foo":2})";

    size_t normal_required_buffer_size = QAJ4C_calculate_max_buffer_size_n(json, ARRAY_COUNT(json));
    size_t insitu_required_buffer_size = QAJ4C_calculate_max_buffer_size_insitu_n(json, ARRAY_COUNT(json));
    assert(normal_required_buffer_size == insitu_required_buffer_size);

    // one object + two members
    assert(normal_required_buffer_size == (sizeof(QAJ4C_Value) + sizeof(QAJ4C_Member) * 2));
}

TEST(BufferSizeTests, ParseArrayWithOneNumericMember) {
    const char json[] = R"([1])";
    size_t normal_required_buffer_size = QAJ4C_calculate_max_buffer_size_n(json, ARRAY_COUNT(json));
    // one object + one member (also of type value)
    assert(normal_required_buffer_size == (sizeof(QAJ4C_Value) + sizeof(QAJ4C_Value)));
}

TEST(BufferSizeTests, ParseArrayWithTwoNumericMembers) {
    const char json[] = R"([1,2])";
    size_t normal_required_buffer_size = QAJ4C_calculate_max_buffer_size_n(json, ARRAY_COUNT(json));
    // one object + two members  (also of type value)
    assert(normal_required_buffer_size == (sizeof(QAJ4C_Value) + sizeof(QAJ4C_Value) * 2));
}

TEST(BufferSizeTests, ParseArrayWithBlockComments) {
    const char json[] = R"([1/*,2*/])";
    size_t normal_required_buffer_size = QAJ4C_calculate_max_buffer_size_n(json, ARRAY_COUNT(json));
    // one object + one member as the other is commented out
    assert(normal_required_buffer_size == (sizeof(QAJ4C_Value) + sizeof(QAJ4C_Value)));
}

TEST(BufferSizeTests, ParseArrayWithLineComment) {
    const char json[] = "([1//,2\n]";
    size_t normal_required_buffer_size = QAJ4C_calculate_max_buffer_size_n(json, ARRAY_COUNT(json));
    // one object + one member as the other is commented out
    assert(normal_required_buffer_size == (sizeof(QAJ4C_Value) + sizeof(QAJ4C_Value)));
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

	assert(normal_required_buffer_size == insitu_required_buffer_size + strlen("blahblubbhubbeldipup") + 1 );

    // one object + one member
    assert(insitu_required_buffer_size == (sizeof(QAJ4C_Value) + sizeof(QAJ4C_Member)));
}

TEST(BufferSizeTests, ParseObjectWithEscapedString) {
    const char json[] = R"({"name":"blah\nblubbh\tubbel\u0021dipup"})";

    size_t normal_required_buffer_size = QAJ4C_calculate_max_buffer_size_n(json, ARRAY_COUNT(json));
    size_t insitu_required_buffer_size = QAJ4C_calculate_max_buffer_size_insitu_n(json, ARRAY_COUNT(json));

    // one object + one member
    assert(insitu_required_buffer_size == (sizeof(QAJ4C_Value) + sizeof(QAJ4C_Member)));

    size_t expected_string_size = strlen("blah blubbh ubbel!dipup") + 1;
	assert(normal_required_buffer_size == insitu_required_buffer_size + expected_string_size);
}

TEST(BufferSizeTests, ParseObjectWithUnicode2BytesString) {
    const char json[] = R"({"name":"\u07FF\u07FF\u07FF\u07FF\u07FF"})";

    size_t normal_required_buffer_size = QAJ4C_calculate_max_buffer_size_n(json, ARRAY_COUNT(json));
    size_t insitu_required_buffer_size = QAJ4C_calculate_max_buffer_size_insitu_n(json, ARRAY_COUNT(json));

    // one object + one member
    assert(insitu_required_buffer_size == (sizeof(QAJ4C_Value) + sizeof(QAJ4C_Member)));

    size_t expected_string_size = 11;
	assert(normal_required_buffer_size == insitu_required_buffer_size + expected_string_size);
}

TEST(BufferSizeTests, ParseObjectWithUnicode3BytesString) {
    const char json[] = R"({"name":"\uFFFF\uFFFF\uFFFF\uFFFF\uFFFF"})";

    size_t normal_required_buffer_size = QAJ4C_calculate_max_buffer_size_n(json, ARRAY_COUNT(json));
    size_t insitu_required_buffer_size = QAJ4C_calculate_max_buffer_size_insitu_n(json, ARRAY_COUNT(json));

    size_t expected_string_size = 16;
	assert(normal_required_buffer_size == insitu_required_buffer_size + expected_string_size);

    // one object + one member
    assert(insitu_required_buffer_size == (sizeof(QAJ4C_Value) + sizeof(QAJ4C_Member)));
}

TEST(BufferSizeTests, ParseObjectWithUnicodeAndLowSurrogateString) {
    const char json[] = R"({"name":"\uD800\uDFFF\uD800\uDFFF\uD800\uDFFF"})";

    size_t normal_required_buffer_size = QAJ4C_calculate_max_buffer_size_n(json, ARRAY_COUNT(json));
    size_t insitu_required_buffer_size = QAJ4C_calculate_max_buffer_size_insitu_n(json, ARRAY_COUNT(json));

    size_t expected_string_size = 13;
	assert(normal_required_buffer_size == insitu_required_buffer_size + expected_string_size);

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

    required = QAJ4C_parse_insitu(json, (void*)buffer, required, &value);
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

/**
 * This test verifies that the optimize on an already optimized object will not
 * alter the binary.
 */
TEST(SimpleParsingTests, ParseObjectCheckOptimizedNoChangeLater) {
    const char json[] = R"({"id":1,"name":"foo","age":39,"job":null,"role":"admin"})";

    static const size_t SIZE = 256;
    uint8_t buff1[SIZE];
    uint8_t buff2[SIZE];

    const QAJ4C_Value* value = nullptr;

    size_t size = QAJ4C_parse(json, buff1, SIZE, &value);

    // backup the content of the old buffer
    memcpy(buff2, buff1, size * sizeof(uint8_t));

    for( size_t i = 0; i < size; ++i )
    {
        assert(buff1[i] == buff2[i]);
    }

    assert(QAJ4C_is_object(value));
    assert(QAJ4C_object_size(value) == 5);
    assert(QAJ4C_get_internal_type(value) == QAJ4C_OBJECT_SORTED);

    QAJ4C_object_optimize((QAJ4C_Value*) value);

    // compare the content with the previously backuped content.
    for( size_t i = 0; i < size; ++i )
    {
        assert(buff1[i] == buff2[i]);
    }

}


TEST(SimpleParsingTests, ParseObjectCheckNonOptimized) {
    const char json[] = R"({"id":1,"name":"foo","age":39,"job":null,"role":"admin"})";
    const QAJ4C_Value* value = QAJ4C_parse_opt_dynamic(json, ARRAY_COUNT(json), QAJ4C_PARSE_OPTS_DONT_SORT_OBJECT_MEMBERS, realloc);
    assert(QAJ4C_is_object(value));
    assert(QAJ4C_object_size(value) == 5);
    assert(QAJ4C_get_internal_type(value) == QAJ4C_OBJECT);

    free((void*)value);
}

TEST(SimpleParsingTests, ParseObjectCheckNonOptimizedAndOptimizeLater) {
    const char json[] = R"({"id":1,"name":"foo","age":39,"job":null,"role":"admin"})";
    const QAJ4C_Value* value = QAJ4C_parse_opt_dynamic(json, ARRAY_COUNT(json), QAJ4C_PARSE_OPTS_DONT_SORT_OBJECT_MEMBERS, realloc);
    assert(QAJ4C_is_object(value));
    assert(QAJ4C_object_size(value) == 5);
    assert(QAJ4C_get_internal_type(value) == QAJ4C_OBJECT);

    QAJ4C_object_optimize((QAJ4C_Value*) value);
    assert(QAJ4C_get_internal_type(value) == QAJ4C_OBJECT_SORTED);

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

TEST(SimpleParsingTests, ParseInt64Max) {
    const char json[] = R"([9223372036854775807])";
    const QAJ4C_Value* value = QAJ4C_parse_opt_dynamic(json, ARRAY_COUNT(json), 0, realloc);
    assert(QAJ4C_is_array(value));
    const QAJ4C_Value* array_value = QAJ4C_array_get(value, 0);

    assert(!QAJ4C_is_uint(array_value));
    assert(!QAJ4C_is_int(array_value));
    assert(QAJ4C_is_int64(array_value));
    assert(QAJ4C_is_uint64(array_value));
    assert(QAJ4C_is_double(array_value));

    assert(INT64_MAX == QAJ4C_get_int64(array_value));
    free((void*)value);
}

TEST(SimpleParsingTests, ParseInt64MaxPlus1) {
    const char json[] = R"([9223372036854775808])";
    const QAJ4C_Value* value = QAJ4C_parse_opt_dynamic(json, ARRAY_COUNT(json), 0, realloc);
    assert(QAJ4C_is_array(value));
    const QAJ4C_Value* array_value = QAJ4C_array_get(value, 0);

    assert(!QAJ4C_is_uint(array_value));
    assert(!QAJ4C_is_int(array_value));
    assert(!QAJ4C_is_int64(array_value));
    assert(QAJ4C_is_uint64(array_value));
    assert(QAJ4C_is_double(array_value));

    free((void*)value);
}

TEST(SimpleParsingTests, ParseInt64Min) {
    const char json[] = R"([-9223372036854775808])";
    const QAJ4C_Value* value = QAJ4C_parse_opt_dynamic(json, ARRAY_COUNT(json), 0, realloc);
    assert(QAJ4C_is_array(value));
    const QAJ4C_Value* array_value = QAJ4C_array_get(value, 0);

    assert(!QAJ4C_is_uint(array_value));
    assert(!QAJ4C_is_int(array_value));
    assert(QAJ4C_is_int64(array_value));
    assert(!QAJ4C_is_uint64(array_value));
    assert(QAJ4C_is_double(array_value));

    assert(INT64_MIN == QAJ4C_get_int64(array_value));
    free((void*)value);
}

TEST(SimpleParsingTests, ParseInt64MaxMinus1) {
    const char json[] = R"([-9223372036854775809])";
    const QAJ4C_Value* value = QAJ4C_parse_opt_dynamic(json, ARRAY_COUNT(json), 0, realloc);
    assert(QAJ4C_is_array(value));
    const QAJ4C_Value* array_value = QAJ4C_array_get(value, 0);

    assert(!QAJ4C_is_uint(array_value));
    assert(!QAJ4C_is_int(array_value));
    assert(!QAJ4C_is_int64(array_value));
    assert(!QAJ4C_is_uint64(array_value));
    assert(QAJ4C_is_double(array_value));

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
    QAJ4C_Builder builder = QAJ4C_builder_create(NULL, 0);

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
    QAJ4C_Builder builder = QAJ4C_builder_create(NULL, 0);

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
    QAJ4C_Builder builder = QAJ4C_builder_create(NULL, 0);

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
    QAJ4C_Builder builder = QAJ4C_builder_create(NULL, 256);
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
TEST(ErrorHandlingTests, InvalidLongUnicodeSequenceInvalidLowSurrogateTooLowValue) {
    const char json[] = R"({"id":123, "name": "\uD800\udbff")";
    uint8_t buff[256];
    const QAJ4C_Value* val = nullptr;
    QAJ4C_parse(json, buff, ARRAY_COUNT(buff), &val);

    assert(QAJ4C_is_error(val));
    assert(QAJ4C_error_get_errno(val) == QAJ4C_ERROR_INVALID_UNICODE_SEQUENCE);
}

/**
 * The same as the other but now only three digits are present.
 */
TEST(ErrorHandlingTests, InvalidLongUnicodeSequenceInvalidLowSurrogateTooHighValue) {
    const char json[] = R"({"id":123, "name": "\uD800\ue000")";
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
    free((void*)value);
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
    const QAJ4C_Value* value = QAJ4C_parse_dynamic(json, lambda);
    assert(QAJ4C_is_error(value));

    assert(QAJ4C_error_get_errno(value) == QAJ4C_ERROR_ALLOCATION_ERROR);
    free((void*)value);
}

/**
 * This test validates that the lookup on an object will not fail in case the object
 * is still untouched.
 */
TEST(ErrorHandlingTests, LookupMemberUninitializedObject) {
    uint8_t buff[256];
    QAJ4C_Builder builder = QAJ4C_builder_create(buff, ARRAY_COUNT(buff));

    QAJ4C_Value* value_ptr = QAJ4C_builder_get_document(&builder);
    QAJ4C_set_object(value_ptr, 4, &builder);

    assert(NULL == QAJ4C_object_get(value_ptr, "id"));
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

    const QAJ4C_Value* value = QAJ4C_parse_dynamic(TEST_JSON_1, realloc);
    assert(QAJ4C_is_object(value));

    const QAJ4C_Value* startup_array = QAJ4C_object_get(value, "startup");
    assert(QAJ4C_is_array(startup_array));

    for (size_t i = 0; i < QAJ4C_array_size(startup_array); ++i) {
        const QAJ4C_Value* val = QAJ4C_array_get(startup_array, i);
        assert(QAJ4C_is_object(val));

        const QAJ4C_Value* path_val = QAJ4C_object_get(val, "exec_start");
        assert(QAJ4C_is_string(path_val));

        assert(strcmp("/path/to/my/binary", QAJ4C_get_string(path_val)) == 0);
    }
    free((void*)value);
}

/**
 * This test will verify that in case an object is accessed incorrectly (with methods of a string or uint)
 * the default value will be returned in case a custom fatal function is set.
 */
TEST(DomObjectAccessTests, ObjectAccess) {
    QAJ4C_Builder builder = QAJ4C_builder_create(NULL, 0);
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
    QAJ4C_Builder builder = QAJ4C_builder_create(NULL, 0);
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

    QAJ4C_Builder builder = QAJ4C_builder_create(buff, ARRAY_COUNT(buff));

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
    free((void*)value);
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
    free((void*)value);
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
    char json[] = "[0.123456,6.9]";
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

TEST(ErrorHandlingTests, PrintStringPartially) {
    char json[] = R"(["so ka?","ey chummer!"])";
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

TEST(ErrorHandlingTests, PrintCompositionOfObjectsAndArraysPartially) {
    char json[] = R"({"id":5,"values":[{},[],{"key":"val","key2":"val2"},[12,34],5.4]})";
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

TEST(ErrorHandlingTests, PrintConstantsPartially) {
    char json[] = R"([null,true,false])";
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
    free((void*)value);
}

/**
 * The following double value triggered that the number was printed as "64." on x86_64-linux.
 * Therefore a special rule for printing was added.
 */
TEST(PrintTests, PrintDoubleCornercase) {
    QAJ4C_Value value;
    double d = -63.999999999999943;
    char buffer[64] = {'\0'};

    QAJ4C_set_double(&value, d);
    size_t out = QAJ4C_sprint(&value, buffer, ARRAY_COUNT(buffer));
    assert( '\0' == buffer[out - 1]);
    assert( '.' != buffer[out - 2]);
}

/**
 * The following test triggers printing a very small numeric value and
 * verifies that this number is printed scientificly.
 */
TEST(PrintTests, PrintDoubleCornercase2) {
    QAJ4C_Value value;
    double d = -1.0e-10;
    char buffer[64] = {'\0'};

    QAJ4C_set_double(&value, d);
    size_t out = QAJ4C_sprint(&value, buffer, ARRAY_COUNT(buffer));
    assert( strchr( buffer, 'e') != nullptr );
}

TEST(PrintTests, PrintSecondCharIsE) {
   QAJ4C_Value value;
   double d = 2.0e-308;
   char buffer[64] = {'\0'};

   QAJ4C_set_double(&value, d);
   size_t out = QAJ4C_sprint(&value, buffer, ARRAY_COUNT(buffer));
   assert( strchr( buffer, 'e') != nullptr );
}

TEST(PrintTests, PrintDoubleZero) {
   QAJ4C_Value value;
   double d = 0;
   char buffer[64] = {'\0'};

   QAJ4C_set_double(&value, d);
   size_t out = QAJ4C_sprint(&value, buffer, ARRAY_COUNT(buffer));
   assert( strcmp("0", buffer) == 0 );
}

TEST(PrintTests, PrintCorruptValue) {
    QAJ4C_Value value;
    char buffer[64] = {'\0'};

    static int count = 0;
    auto lambda = []{ count++; }; // set the error method (else the compare will send a signal)
    QAJ4C_register_fatal_error_function(lambda);

    value.type = QAJ4C_UNSPECIFIED << 8;
    size_t out = QAJ4C_sprint(&value, buffer, ARRAY_COUNT(buffer));
    assert(1 == count); // expect an error
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
    const char json[] = R"([1,2.10101,3,4.123456e+100,5.1,-6])";
    const QAJ4C_Value* value = QAJ4C_parse_opt_dynamic(json, ARRAY_COUNT(json), 0, realloc);

    assert(!QAJ4C_is_error(value));

    char output[ARRAY_COUNT(json)];
    size_t out = QAJ4C_sprint(value, output, ARRAY_COUNT(output));
    assert(ARRAY_COUNT(output) == out);

    assert(strcmp(json, output) == 0);

    free((void*)value);
}

/**
 * Parses and prints a row of decimals of base 10 (regression test)
 */
TEST(PrintTests, PrintAllTwoDigitDecimalsArray) {
    char json[512];

    char* p = json;
    *p = '[';
    p += 1;
    for (unsigned i = 0; i< 100; ++i) {
        p += sprintf(p, "%u,", i);
    }
    *(p-1) = ']';
    const QAJ4C_Value* value = QAJ4C_parse_opt_dynamic(json, ARRAY_COUNT(json), 0, realloc);

    assert(!QAJ4C_is_error(value));

    char output[strlen(json)+1];
    size_t out = QAJ4C_sprint(value, output, ARRAY_COUNT(output));
    assert(ARRAY_COUNT(output) == out);

    assert(strcmp(json, output) == 0);

    free((void*)value);
}

/**
 * Parses and prints a row of decimals of base 10 (regression test)
 */
TEST(PrintTests, PrintDecimalsArray) {
    const char json[] = R"([1,10,100,1000,10000,100000])";
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
    const char json[] = R"([0.1])";
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

TEST(PrintTests, PrintUintZero) {
    const char json[] = R"([0])";
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
    double nan_value = 0.0/0.0;

    QAJ4C_Value value;
    QAJ4C_set_double(&value, nan_value);
    QAJ4C_sprint(&value, output, ARRAY_COUNT(output));
    assert(strcmp("null", output) == 0);
}

/**
 * In this test the user input for "NaN" will be translated to 'null' (to at least have
 * some kind of defined behavior)
 */
TEST(PrintTests, PrintDoubleDomInfinity) {
    char output[64];
    double infinity_value = 1.0/0.0;

    QAJ4C_Value value;
    QAJ4C_set_double(&value, infinity_value);
    QAJ4C_sprint(&value, output, ARRAY_COUNT(output));
    assert(strcmp("null", output) == 0);
}

/**
 * This callback method simply compares each individual character and returns true on a match
 * and false on a miss.
 */
static bool simple_print_callback_impl( void *ptr, char c ) {
    std::pair<size_t, const char*>& data = *reinterpret_cast<std::pair<size_t, const char*>*>(ptr);
    bool result = data.second[data.first] == c;
    data.first += 1;
    return result;
}

TEST(PrintTests, PrintWithCallback) {
    const char* msg = "Hello";
    std::pair<size_t, const char*> expected_data{0, "\"Hello\""};
    QAJ4C_Value value;
    QAJ4C_set_string_ref(&value, msg);

    bool result = QAJ4C_print_callback(&value, simple_print_callback_impl, &expected_data );
    assert(result);
}

TEST(PrintTests, PrintWithCallbackNullsafe) {
    const char* msg = "Hello";
    QAJ4C_Value value;
    QAJ4C_set_string_ref(&value, msg);

    assert(!QAJ4C_print_callback(&value, NULL, NULL ));
}

/**
 * For simplicity the simple_print_callback_impl implementation will be used to check the
 * message char by char.
 */
static bool buffer_print_callback_impl( void* ptr, const char* buffer, size_t buffer_length ) {
    bool result = true;
    for (size_t i = 0; i < buffer_length; ++i) {
        result = result && simple_print_callback_impl(ptr, buffer[i]);
    }
    return result;
}


TEST(PrintTests, PrintBufferWithCallback) {
    const char* msg = "Hello";
    std::pair<size_t, const char*> expected_data{0, "\"Hello\""};
    QAJ4C_Value value;
    QAJ4C_set_string_ref(&value, msg);

    bool result = QAJ4C_print_buffer_callback(&value, buffer_print_callback_impl, &expected_data );
    assert(result);
}

TEST(PrintTests, PrintBufferWithCallbackNullsafe) {
    const char* msg = "Hello";
    QAJ4C_Value value;
    QAJ4C_set_string_ref(&value, msg);

    assert(!QAJ4C_print_buffer_callback(&value, NULL, NULL ));
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

    QAJ4C_Builder builder = QAJ4C_builder_create(buffer2, buff_size);
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
    QAJ4C_Builder builder1 = QAJ4C_builder_create(buff1, ARRAY_COUNT(buff1));
    QAJ4C_Builder builder2 = QAJ4C_builder_create(buff2, ARRAY_COUNT(buff2));

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
    QAJ4C_Builder builder = QAJ4C_builder_create(buffer2, buff_size);

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

    printf("Sizeof QAJ4C_Value is: " FMT_SIZE "\n", sizeof(QAJ4C_Value));

    assert(sizeof(QAJ4C_Value) == (QAJ4C_MAX(sizeof(uint32_t), sizeof(uintptr_t)) + sizeof(size_type) * 2));
    assert(sizeof(QAJ4C_Value) == sizeof(QAJ4C_Object));
    assert(sizeof(QAJ4C_Value) == sizeof(QAJ4C_Array));
    assert(sizeof(QAJ4C_Value) == sizeof(QAJ4C_String));
    assert(sizeof(QAJ4C_Value) == sizeof(QAJ4C_Short_string));
    assert(sizeof(QAJ4C_Value) == sizeof(QAJ4C_Primitive));
    assert(sizeof(QAJ4C_Value) == sizeof(QAJ4C_Error));

    assert(sizeof(QAJ4C_Member) == 2 * sizeof(QAJ4C_Value));
}

TEST(VariousTests, ResetBuilder) {
    char buff[256];

    QAJ4C_Builder builder = QAJ4C_builder_create(buff, ARRAY_COUNT(buff));
    QAJ4C_Builder builder2 = QAJ4C_builder_create(buff, ARRAY_COUNT(buff));

    assert(QAJ4C_MEMCMP(&builder, &builder2, sizeof(builder)) == 0);

    // alter the builder
    QAJ4C_set_array(QAJ4C_builder_get_document(&builder), 10, &builder);

    // compare should not result in the builders are equal
    assert(QAJ4C_MEMCMP(&builder, &builder2, sizeof(builder)) != 0);

    // now reset the builder
    QAJ4C_builder_reset(&builder);

    // builders should be equal again
    assert(QAJ4C_MEMCMP(&builder, &builder2, sizeof(builder)) == 0);
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
    QAJ4C_Builder builder = QAJ4C_builder_create(buff, ARRAY_COUNT(buff));

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
    QAJ4C_Builder builder = QAJ4C_builder_create(buff, ARRAY_COUNT(buff));

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
    QAJ4C_Value* member6 = QAJ4C_object_create_member_by_copy(value_ptr, key5, &builder); // also verify this for copy

    assert( member1 != nullptr );
    assert( member1_a == nullptr );
    assert( member1_b == nullptr );
    assert( member2 != nullptr );
    assert( member3 != nullptr );
    assert( member4 != nullptr);
    assert( member5 == nullptr );  // there is no room left for more members
    assert( member6 == nullptr );  // also no room left for members

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
    QAJ4C_Builder builder = QAJ4C_builder_create(buff, ARRAY_COUNT(buff));

    QAJ4C_Value* value_ptr = QAJ4C_builder_get_document(&builder);
    QAJ4C_set_object(value_ptr, 0, &builder);
    QAJ4C_object_optimize(value_ptr);
    assert(QAJ4C_get_internal_type(value_ptr) == QAJ4C_OBJECT_SORTED);
}

TEST(DomCreation, OptimizeUninitializedObject) {
    uint8_t buff[256];
    QAJ4C_Builder builder = QAJ4C_builder_create(buff, ARRAY_COUNT(buff));

    QAJ4C_Value* value_ptr = QAJ4C_builder_get_document(&builder);
    QAJ4C_set_object(value_ptr, 4, &builder);
    QAJ4C_object_optimize(value_ptr);
    assert(QAJ4C_get_internal_type(value_ptr) == QAJ4C_OBJECT_SORTED);
}

TEST(DomCreation, OptimizeLowFilledObject) {
    uint8_t buff[256];
    QAJ4C_Builder builder = QAJ4C_builder_create(buff, ARRAY_COUNT(buff));

    QAJ4C_Value* value_ptr = QAJ4C_builder_get_document(&builder);
    QAJ4C_set_object(value_ptr, 4, &builder);
    QAJ4C_object_create_member_by_ref(value_ptr, "id");

    QAJ4C_object_optimize(value_ptr);
    assert(QAJ4C_get_internal_type(value_ptr) == QAJ4C_OBJECT_SORTED);
}

/**
 * This test will verify that even though a object is not filled completely
 * the output will stay valid. In this case a object with 5 members is created
 * but no member is added. It is expected that the printed string will be {}
 */
TEST(DomCreation, PrintIncompleteObject) {
    uint8_t buff[256];
    QAJ4C_Builder builder = QAJ4C_builder_create(buff, ARRAY_COUNT(buff));

    QAJ4C_Value* value_ptr = QAJ4C_builder_get_document(&builder);
    QAJ4C_set_object(value_ptr, 4, &builder);

    char out[5];
    QAJ4C_sprint(value_ptr, out, ARRAY_COUNT(out));
    assert(strcmp("{}", out) == 0);

}

/**
 * This test will verify the basic functionality of the QAJ4C_Object_builder
 */
TEST(ObjectBuilderTests, SimpleObjectWithIntegers) {
    uint8_t buff[256];
    QAJ4C_Builder builder = QAJ4C_builder_create(buff, ARRAY_COUNT(buff));
    QAJ4C_Value* value_ptr = QAJ4C_builder_get_document(&builder);

    QAJ4C_Object_builder obj_builder = QAJ4C_object_builder_init(value_ptr, 2, false, &builder);

    QAJ4C_set_uint(QAJ4C_object_builder_create_member_by_ref(&obj_builder, "id"), 32);
    QAJ4C_set_uint(QAJ4C_object_builder_create_member_by_ref(&obj_builder, "value"), 99);

    char out[64];
    QAJ4C_sprint(value_ptr, out, ARRAY_COUNT(out));
    assert(strcmp(R"({"id":32,"value":99})", out) == 0);
}

/**
 * In this test the object builder is created for more fields than actually used.
 * It is expected that the unused fields will not be printed.
 */
TEST(ObjectBuilderTests, SimpleObjectWithIntegersAndUnusedFields) {
    uint8_t buff[256];
    QAJ4C_Builder builder = QAJ4C_builder_create(buff, ARRAY_COUNT(buff));
    QAJ4C_Value* value_ptr = QAJ4C_builder_get_document(&builder);

    QAJ4C_Object_builder obj_builder = QAJ4C_object_builder_init(value_ptr, 4, false, &builder);

    QAJ4C_set_uint(QAJ4C_object_builder_create_member_by_ref(&obj_builder, "id"), 32);
    QAJ4C_set_uint(QAJ4C_object_builder_create_member_by_ref(&obj_builder, "value"), 99);

    char out[64];
    QAJ4C_sprint(value_ptr, out, ARRAY_COUNT(out));
    assert(QAJ4C_object_size(value_ptr) == 2);
    assert(strcmp(R"({"id":32,"value":99})", out) == 0);
}

/**
 * In this test the behavior is verified using deduplication. In this case the object
 * will be ensured to not contain duplicate keys.
 */
TEST(ObjectBuilderTests, SimpleObjectWithIntegersDeduplication) {
    uint8_t buff[256];
    QAJ4C_Builder builder = QAJ4C_builder_create(buff, ARRAY_COUNT(buff));
    QAJ4C_Value* value_ptr = QAJ4C_builder_get_document(&builder);

    QAJ4C_Object_builder obj_builder = QAJ4C_object_builder_init(value_ptr, 4, true, &builder);

    QAJ4C_set_uint(QAJ4C_object_builder_create_member_by_ref(&obj_builder, "id"), 32);
    QAJ4C_set_uint(QAJ4C_object_builder_create_member_by_ref(&obj_builder, "value"), 99);
    QAJ4C_set_uint(QAJ4C_object_builder_create_member_by_ref(&obj_builder, "id"), 1);

    char out[64];
    QAJ4C_sprint(value_ptr, out, ARRAY_COUNT(out));
    assert(QAJ4C_object_size(value_ptr) == 2);
    assert(strcmp(R"({"id":1,"value":99})", out) == 0);
}

/**
 * In this test the behavior is verified that switching deduplication off will result
 * in duplicate keys of the printed message.
 */
TEST(ObjectBuilderTests, SimpleObjectWithIntegersNoDeduplication) {
    uint8_t buff[256];
    QAJ4C_Builder builder = QAJ4C_builder_create(buff, ARRAY_COUNT(buff));
    QAJ4C_Value* value_ptr = QAJ4C_builder_get_document(&builder);

    QAJ4C_Object_builder obj_builder = QAJ4C_object_builder_init(value_ptr, 4, false, &builder);

    QAJ4C_set_uint(QAJ4C_object_builder_create_member_by_ref(&obj_builder, "id"), 32);
    QAJ4C_set_uint(QAJ4C_object_builder_create_member_by_ref(&obj_builder, "value"), 99);
    QAJ4C_set_uint(QAJ4C_object_builder_create_member_by_ref(&obj_builder, "id"), 1);

    char out[64];
    QAJ4C_sprint(value_ptr, out, ARRAY_COUNT(out));
    assert(QAJ4C_object_size(value_ptr) == 3);
    assert(strcmp(R"({"id":32,"value":99,"id":1})", out) == 0);
}

/**
 * Same test as SimpleObjectWithIntegers ... but now by creating members by copy.
 */
TEST(ObjectBuilderTests, SimpleObjectWithIntegersKeysAsCopy) {
    uint8_t buff[256];
    QAJ4C_Builder builder = QAJ4C_builder_create(buff, ARRAY_COUNT(buff));
    QAJ4C_Value* value_ptr = QAJ4C_builder_get_document(&builder);

    QAJ4C_Object_builder obj_builder = QAJ4C_object_builder_init(value_ptr, 2, false, &builder);

    QAJ4C_set_uint(QAJ4C_object_builder_create_member_by_copy(&obj_builder, "id", &builder), 32);
    QAJ4C_set_uint(QAJ4C_object_builder_create_member_by_copy(&obj_builder, "value", &builder), 99);

    char out[64];
    QAJ4C_sprint(value_ptr, out, ARRAY_COUNT(out));
    assert(strcmp(R"({"id":32,"value":99})", out) == 0);
}

/**
 * Same test as SimpleObjectWithIntegersAndUnusedFields ... but now by creating members by copy.
 */
TEST(ObjectBuilderTests, SimpleObjectWithIntegersAndUnusedFieldsKeysAsCopy) {
    uint8_t buff[256];
    QAJ4C_Builder builder = QAJ4C_builder_create(buff, ARRAY_COUNT(buff));
    QAJ4C_Value* value_ptr = QAJ4C_builder_get_document(&builder);

    QAJ4C_Object_builder obj_builder = QAJ4C_object_builder_init(value_ptr, 4, false, &builder);

    QAJ4C_set_uint(QAJ4C_object_builder_create_member_by_copy(&obj_builder, "id", &builder), 32);
    QAJ4C_set_uint(QAJ4C_object_builder_create_member_by_copy(&obj_builder, "value", &builder), 99);

    char out[64];
    QAJ4C_sprint(value_ptr, out, ARRAY_COUNT(out));
    assert(QAJ4C_object_size(value_ptr) == 2);
    assert(strcmp(R"({"id":32,"value":99})", out) == 0);
}


/**
 * Same test as SimpleObjectWithIntegersDeduplication ... but now by creating members by copy.
 */
TEST(ObjectBuilderTests, SimpleObjectWithIntegersDeduplicationKeysAsCopy) {
    uint8_t buff[256];
    QAJ4C_Builder builder = QAJ4C_builder_create(buff, ARRAY_COUNT(buff));
    QAJ4C_Value* value_ptr = QAJ4C_builder_get_document(&builder);

    QAJ4C_Object_builder obj_builder = QAJ4C_object_builder_init(value_ptr, 4, true, &builder);

    QAJ4C_set_uint(QAJ4C_object_builder_create_member_by_copy(&obj_builder, "id", &builder), 32);
    QAJ4C_set_uint(QAJ4C_object_builder_create_member_by_copy(&obj_builder, "value", &builder), 99);
    QAJ4C_set_uint(QAJ4C_object_builder_create_member_by_copy(&obj_builder, "id", &builder), 1);

    char out[64];
    QAJ4C_sprint(value_ptr, out, ARRAY_COUNT(out));
    assert(QAJ4C_object_size(value_ptr) == 2);
    assert(strcmp(R"({"id":1,"value":99})", out) == 0);
}

/**
 * Same test as SimpleObjectWithIntegersNoDeduplication ... but now by creating members by copy.
 */
TEST(ObjectBuilderTests, SimpleObjectWithIntegersNoDeduplicationKeysAsCopy) {
    uint8_t buff[256];
    QAJ4C_Builder builder = QAJ4C_builder_create(buff, ARRAY_COUNT(buff));
    QAJ4C_Value* value_ptr = QAJ4C_builder_get_document(&builder);

    QAJ4C_Object_builder obj_builder = QAJ4C_object_builder_init(value_ptr, 4, false, &builder);

    QAJ4C_set_uint(QAJ4C_object_builder_create_member_by_copy(&obj_builder, "id", &builder), 32);
    QAJ4C_set_uint(QAJ4C_object_builder_create_member_by_copy(&obj_builder, "value", &builder), 99);
    QAJ4C_set_uint(QAJ4C_object_builder_create_member_by_copy(&obj_builder, "id", &builder), 1);

    char out[64];
    QAJ4C_sprint(value_ptr, out, ARRAY_COUNT(out));
    assert(QAJ4C_object_size(value_ptr) == 3);
    assert(strcmp(R"({"id":32,"value":99,"id":1})", out) == 0);
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
    const QAJ4C_Value* value = QAJ4C_parse_dynamic(json, realloc);

    assert(QAJ4C_is_object(value));
    assert(QAJ4C_array_size(QAJ4C_object_get(value, "a")) == 1);
    assert(QAJ4C_array_size(QAJ4C_object_get(value, "b")) == 0);
    free((void*)value);
}

/**
 * Same test as AttachedEmptyArray but now with an trailing empty object.
 */
TEST(CornerCaseTests, AttachedEmptyObject) {
    const char* json = R"({"a":[1],"b":{}})";
    const QAJ4C_Value* value = QAJ4C_parse_dynamic(json, realloc);

    assert(QAJ4C_is_object(value));
    free((void*)value);
}

/**
 * This test will print not fully initialized objects and arrays.
 * It is expected that objects will only print the "set" values as the
 * member is only valid in case the key is not "null". Arrays that are
 * not filled completely will print null values as they do not have a "fill" level.
 *
 * The buffer is set to a "non-null" value to trigger initialization failures
 * that have occurred in the past.
 */
TEST(CornerCaseTests, PrintArrayWithNullValues) {
    uint8_t buff[256];
    char json[64];

    for (int i = 0; i < ARRAY_COUNT(buff); ++i) {
        buff[i] = 0xFF;
    }

    QAJ4C_Builder builder = QAJ4C_builder_create(buff, ARRAY_COUNT(buff));
    QAJ4C_Value* root_node = QAJ4C_builder_get_document(&builder);
    QAJ4C_set_object(root_node, 5, &builder);

    QAJ4C_Value* groups_node = QAJ4C_object_create_member_by_ref(root_node, "a");
    QAJ4C_set_array(groups_node, 2, &builder);

    QAJ4C_sprint(root_node, json, ARRAY_COUNT(json));
    assert( strcmp(R"({"a":[null,null]})", json) == 0);
}

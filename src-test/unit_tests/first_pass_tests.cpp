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

#include "unit_tests/test.h"
#include "qajson4c/internal/first_pass.h"

static uint8_t BUFFER[1024];

TEST(FirstPassParserTests, SimpleExample) {
    const char json[] = R"({"id":1})";

	QAJ4C_Json_message msg { json, json + ARRAY_COUNT(json) - 1, json };

	auto builder = QAJ4C_builder_create(&BUFFER, ARRAY_COUNT(BUFFER));
    auto parser = QAJ4C_first_pass_parser_create(&builder, 0, NULL);

    QAJ4C_first_pass_parse(&parser, &msg);

    assert(QAJ4C_ERROR_NO_ERROR == parser.err_code);
    assert(3 == parser.amount_nodes);
}

TEST(FirstPassParserTests, SimpleArrayExample) {
    const char json[] = R"(["id",1,true,null,false])";

	QAJ4C_Json_message msg { json, json + ARRAY_COUNT(json) - 1, json };

	auto builder = QAJ4C_builder_create(&BUFFER, ARRAY_COUNT(BUFFER));
    auto parser = QAJ4C_first_pass_parser_create(&builder, 0, NULL);

    QAJ4C_first_pass_parse(&parser, &msg);

    assert(QAJ4C_ERROR_NO_ERROR == parser.err_code);
    assert(6 == parser.amount_nodes);
}

TEST(FirstPassParserTests, SimpleArrayExampleWithWhitespaces) {
    const char json[] = R"( [ "id" , 1 , true , null , false ] )";

	QAJ4C_Json_message msg { json, json + ARRAY_COUNT(json) - 1, json };

	auto builder = QAJ4C_builder_create(&BUFFER, ARRAY_COUNT(BUFFER));
    auto parser = QAJ4C_first_pass_parser_create(&builder, 0, NULL);

    QAJ4C_first_pass_parse(&parser, &msg);

    assert(QAJ4C_ERROR_NO_ERROR == parser.err_code);
    assert(6 == parser.amount_nodes);
}

/**
 * Besides the storage size the first pass parser also ensures the message has valid length.
 * So it should detect a truncated json buffer.
 */
TEST(FirstPassParserTests, TruncatedExample) {
    const char json[] = R"({"id":1)";

	QAJ4C_Json_message msg { json, json + ARRAY_COUNT(json) - 1, json };

	auto builder = QAJ4C_builder_create(&BUFFER, ARRAY_COUNT(BUFFER));
    auto parser = QAJ4C_first_pass_parser_create(&builder, 0, NULL);

    QAJ4C_first_pass_parse(&parser, &msg);

    assert(QAJ4C_ERROR_JSON_MESSAGE_TRUNCATED == parser.err_code);
}

/**
 * In this test case the json message length is extended. The first pass parser should not
 * declare this json to be truncated (only because the json_end is not correctly specified)
 */
TEST(FirstPassParserTests, SimpleExampleTrailingNullChars) {
    const char json[] = R"({"id":1})" "\0\0\0";

	QAJ4C_Json_message msg { json, json + ARRAY_COUNT(json) - 1, json };

	auto builder = QAJ4C_builder_create(&BUFFER, ARRAY_COUNT(BUFFER));
    auto parser = QAJ4C_first_pass_parser_create(&builder, 0, NULL);

    QAJ4C_first_pass_parse(&parser, &msg);

    assert(QAJ4C_ERROR_NO_ERROR == parser.err_code);
    assert(3 == parser.amount_nodes);
}

TEST(FirstPassParserTests, EmptyObject) {
    const char json[] = R"({})";

	QAJ4C_Json_message msg { json, json + ARRAY_COUNT(json) - 1, json };

	auto builder = QAJ4C_builder_create(&BUFFER, ARRAY_COUNT(BUFFER));
    auto parser = QAJ4C_first_pass_parser_create(&builder, 0, NULL);

    QAJ4C_first_pass_parse(&parser, &msg);

    assert(QAJ4C_ERROR_NO_ERROR == parser.err_code);
    assert(1 == parser.amount_nodes);
}

TEST(FirstPassParserTests, EmptyArray) {
    const char json[] = R"([])";

	QAJ4C_Json_message msg { json, json + ARRAY_COUNT(json) - 1, json };

	auto builder = QAJ4C_builder_create(&BUFFER, ARRAY_COUNT(BUFFER));
    auto parser = QAJ4C_first_pass_parser_create(&builder, 0, NULL);

    QAJ4C_first_pass_parse(&parser, &msg);

    assert(QAJ4C_ERROR_NO_ERROR == parser.err_code);
    assert(1 == parser.amount_nodes);
}

TEST(FirstPassParserTests, EmptyObjectWhitespaces) {
    const char json[] = R"({           })";

	QAJ4C_Json_message msg { json, json + ARRAY_COUNT(json) - 1, json };

	auto builder = QAJ4C_builder_create(&BUFFER, ARRAY_COUNT(BUFFER));
    auto parser = QAJ4C_first_pass_parser_create(&builder, 0, NULL);

    QAJ4C_first_pass_parse(&parser, &msg);

    assert(QAJ4C_ERROR_NO_ERROR == parser.err_code);
    assert(1 == parser.amount_nodes);
}

TEST(FirstPassParserTests, EmptyArrayWhitespaces) {
    const char json[] = R"([          ])";

	QAJ4C_Json_message msg { json, json + ARRAY_COUNT(json) - 1, json };

	auto builder = QAJ4C_builder_create(&BUFFER, ARRAY_COUNT(BUFFER));
    auto parser = QAJ4C_first_pass_parser_create(&builder, 0, NULL);

    QAJ4C_first_pass_parse(&parser, &msg);

    assert(QAJ4C_ERROR_NO_ERROR == parser.err_code);
    assert(1 == parser.amount_nodes);
}

TEST(FirstPassParserTests, PlainString) {
    const char json[] = R"("Hello World")";

	QAJ4C_Json_message msg { json, json + ARRAY_COUNT(json) - 1, json };

	auto builder = QAJ4C_builder_create(&BUFFER, ARRAY_COUNT(BUFFER));
    auto parser = QAJ4C_first_pass_parser_create(&builder, 0, NULL);

    QAJ4C_first_pass_parse(&parser, &msg);

    assert(QAJ4C_ERROR_NO_ERROR == parser.err_code);
    assert(1 == parser.amount_nodes);
    assert(12 == parser.complete_string_length);
}

TEST(FirstPassParserTests, Integer) {
    const char json[] = R"(42)";

	QAJ4C_Json_message msg { json, json + ARRAY_COUNT(json) - 1, json };

	auto builder = QAJ4C_builder_create(&BUFFER, ARRAY_COUNT(BUFFER));
    auto parser = QAJ4C_first_pass_parser_create(&builder, 0, NULL);

    QAJ4C_first_pass_parse(&parser, &msg);

    assert(QAJ4C_ERROR_NO_ERROR == parser.err_code);
    assert(1 == parser.amount_nodes);
}

TEST(FirstPassParserTests, Double) {
    const char json[] = R"(4.2)";

	QAJ4C_Json_message msg { json, json + ARRAY_COUNT(json) - 1, json };

	auto builder = QAJ4C_builder_create(&BUFFER, ARRAY_COUNT(BUFFER));
    auto parser = QAJ4C_first_pass_parser_create(&builder, 0, NULL);

    QAJ4C_first_pass_parse(&parser, &msg);

    assert(QAJ4C_ERROR_NO_ERROR == parser.err_code);
    assert(1 == parser.amount_nodes);
}

TEST(FirstPassParserTests, UTF16_1_Byte) {
    const char json[] = R"("Hello World\u0021")";

	QAJ4C_Json_message msg { json, json + ARRAY_COUNT(json) - 1, json };

	auto builder = QAJ4C_builder_create(&BUFFER, ARRAY_COUNT(BUFFER));
    auto parser = QAJ4C_first_pass_parser_create(&builder, 0, NULL);

    QAJ4C_first_pass_parse(&parser, &msg);

    assert(QAJ4C_ERROR_NO_ERROR == parser.err_code);
    assert(1 == parser.amount_nodes);
    assert(13 == parser.complete_string_length);
}

TEST(FirstPassParserTests, UTF16_2_Bytes) {
    const char json[] = R"("Hello World\u0721")";

	QAJ4C_Json_message msg { json, json + ARRAY_COUNT(json) - 1, json };

	auto builder = QAJ4C_builder_create(&BUFFER, ARRAY_COUNT(BUFFER));
    auto parser = QAJ4C_first_pass_parser_create(&builder, 0, NULL);

    QAJ4C_first_pass_parse(&parser, &msg);

    assert(QAJ4C_ERROR_NO_ERROR == parser.err_code);
    assert(1 == parser.amount_nodes);
    assert(14 == parser.complete_string_length);
}

TEST(FirstPassParserTests, UTF16_3_Bytes_1rst_range) {
    const char json[] = R"("Hello World\u0821")";

	QAJ4C_Json_message msg { json, json + ARRAY_COUNT(json) - 1, json };

	auto builder = QAJ4C_builder_create(&BUFFER, ARRAY_COUNT(BUFFER));
    auto parser = QAJ4C_first_pass_parser_create(&builder, 0, NULL);

    QAJ4C_first_pass_parse(&parser, &msg);

    assert(QAJ4C_ERROR_NO_ERROR == parser.err_code);
    assert(1 == parser.amount_nodes);
    assert(15 == parser.complete_string_length);
}

TEST(FirstPassParserTests, UTF16_3_Bytes_2nd_range) {
    const char json[] = R"("Hello World\uF021")";

	QAJ4C_Json_message msg { json, json + ARRAY_COUNT(json) - 1, json };

	auto builder = QAJ4C_builder_create(&BUFFER, ARRAY_COUNT(BUFFER));
    auto parser = QAJ4C_first_pass_parser_create(&builder, 0, NULL);

    QAJ4C_first_pass_parse(&parser, &msg);

    assert(QAJ4C_ERROR_NO_ERROR == parser.err_code);
    assert(1 == parser.amount_nodes);
    assert(15 == parser.complete_string_length);
}

TEST(FirstPassParserTests, UTF16_4_Bytes) {
    const char json[] = R"("Hello World\ud821\u0000")";

	QAJ4C_Json_message msg { json, json + ARRAY_COUNT(json) - 1, json };

	auto builder = QAJ4C_builder_create(&BUFFER, ARRAY_COUNT(BUFFER));
    auto parser = QAJ4C_first_pass_parser_create(&builder, 0, NULL);

    QAJ4C_first_pass_parse(&parser, &msg);

    assert(QAJ4C_ERROR_NO_ERROR == parser.err_code);
    assert(1 == parser.amount_nodes);
    assert(16 == parser.complete_string_length);
}

TEST(FirstPassParserTests, Invalid_json_comma) {
    const char json[] = R"(,)";

	QAJ4C_Json_message msg { json, json + ARRAY_COUNT(json) - 1, json };

	auto builder = QAJ4C_builder_create(&BUFFER, ARRAY_COUNT(BUFFER));
    auto parser = QAJ4C_first_pass_parser_create(&builder, 0, NULL);

    QAJ4C_first_pass_parse(&parser, &msg);

    assert(QAJ4C_ERROR_UNEXPECTED_CHAR == parser.err_code);
}


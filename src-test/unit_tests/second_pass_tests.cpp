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
#include "qajson4c/internal/second_pass.h"

static uint8_t BUFFER[1024];

TEST(SecondPassParserTests, SimpleExample) {
    const char json[] = R"({"id":1})";

	QAJ4C_Json_message msg { json, json + ARRAY_COUNT(json) - 1, json };

	auto builder = QAJ4C_builder_create(&BUFFER, ARRAY_COUNT(BUFFER));
    auto parser = QAJ4C_first_pass_parser_create(&builder, 0, NULL);
    auto second_parser = QAJ4C_second_pass_parser_create(&parser);

    auto result = QAJ4C_second_pass_process(&second_parser, &msg);
    assert( nullptr != result );
}

/**
  @file

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

#ifndef QAJSON4C_INTERNAL_PARSE_H_
#define QAJSON4C_INTERNAL_PARSE_H_

typedef enum QAJ4C_Char_type {
    QAJ4C_CHAR_NULL = 0,
    QAJ4C_CHAR_WHITESPACE,
    QAJ4C_CHAR_NUMERIC_START,
    QAJ4C_CHAR_OBJECT_START,
    QAJ4C_CHAR_OBJECT_END,
    QAJ4C_CHAR_ARRAY_START,
    QAJ4C_CHAR_ARRAY_END,
    QAJ4C_CHAR_COLON,
    QAJ4C_CHAR_COMMA,
    QAJ4C_CHAR_COMMENT_START,
    QAJ4C_CHAR_STRING_START,
    QAJ4C_CHAR_LITERAL_START,
    QAJ4C_CHAR_UNSUPPORTED
} QAJ4C_Char_type;

static QAJ4C_Char_type QAJ4C_parse_char( char c ) {
    switch (c) {
    case '\0':
        return QAJ4C_CHAR_NULL;
    case ' ':
    case '\t':
    case '\n':
    case '\r':
        return QAJ4C_CHAR_WHITESPACE;
    case 'f':
    case 't':
    case 'n':
        return QAJ4C_CHAR_LITERAL_START;
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
    case '+':
    case '-':
        return QAJ4C_CHAR_NUMERIC_START;
    case '{':
        return QAJ4C_CHAR_OBJECT_START;
    case '[':
        return QAJ4C_CHAR_ARRAY_START;
    case '"':
        return QAJ4C_CHAR_STRING_START;
    case '/':
        return QAJ4C_CHAR_COMMENT_START;
    case '}':
        return QAJ4C_CHAR_OBJECT_END;
    case ']':
        return QAJ4C_CHAR_ARRAY_END;
    case ':':
        return QAJ4C_CHAR_COLON;
    case ',':
        return QAJ4C_CHAR_COMMA;
    default:
        return QAJ4C_CHAR_UNSUPPORTED;
    }
}

static void QAJ4C_json_message_skip_whitespaces( QAJ4C_Json_message* msg ) {
    while (msg->pos < msg->end) {
        if (QAJ4C_parse_char(*msg->pos) != QAJ4C_CHAR_WHITESPACE) {
            break;
        }
        msg->pos += 1;
    }
}



#endif /* QAJSON4C_INTERNAL_PARSE_H_ */

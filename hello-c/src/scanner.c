#include "scanner.h"
#include <string.h>
#include <stdio.h>

#include "common.h"

typedef struct
{
    const char *start;
    const char *current;
    int line;
} Scanner;

static Scanner scanner;

void initScanner(const char *source)
{
    scanner.start = source;
    scanner.current = source;
    scanner.line = 1;
}

static bool isAtEnd()
{
    return *scanner.current == '\0';
}

static token makeToken(TokenType type)
{
    token result;
    result.type = type;
    result.start = scanner.start;
    result.length = (int)(scanner.current - scanner.start);
    result.line = scanner.line;
    return result;
}

static token errorToken(const char *message)
{
    token result;
    result.type = TOKEN_ERROR;
    result.start = message;
    result.length = (int)strlen(message);
    result.line = scanner.line;
    return result;
}

static char advance()
{
    scanner.current++;
    return scanner.current[-1]; // 等同于 *[scanner.current-1]
}
static bool match(char c)
{
    if (isAtEnd())
        return false;
    if (*scanner.current != c)
        return false;

    scanner.current++;

    return true;
}
static char peek()
{
    return *scanner.current;
}
static char peekNext()
{
    if (isAtEnd())
        return '\0';
    return scanner.current[1];
}

static void skipWhitespace()
{
    while (1)
    {
        char c = peek();
        switch (c)
        {
        case ' ':
        case '\r':
        case '\t':
            advance();
            break;
        case '\n':
            scanner.line++;
            advance();
            break;
        case '/':
            if (peekNext() == '/')
            {
                while (peek() != '\n' && !isAtEnd())
                {
                    advance();
                }
            }
            else
            {
                return;
            }
        default:
            break;
        }
    }
}

static token string()
{
    while (peek() != '"' && !isAtEnd())
    {
        if (peek() == '\n')
            scanner.line++;
        advance();
    }
    if (isAtEnd())
        return errorToken("Unterminated string");
    return makeToken(TOKEN_STRING);
}
token scanToken(void)
{
    skipWhitespace();
    scanner.start = scanner.current;
    if (isAtEnd())
    {
        return makeToken(TOKEN_EOF);
    }
    char c = advance();
    switch (c)
    {
    case '(':
        return makeToken(TOKEN_LEFT_PAREN);
    case ')':
        return makeToken(TOKEN_RIGHT_PAREN);
    case '{':
        return makeToken(TOKEN_LEFT_BRACE);
    case '}':
        return makeToken(TOKEN_RIGHT_BRACE);
    case ';':
        return makeToken(TOKEN_SEMICOLON);
    case ',':
        return makeToken(TOKEN_COMMA);
    case '.':
        return makeToken(TOKEN_DOT);
    case '-':
        return makeToken(TOKEN_MINUS);
    case '+':
        return makeToken(TOKEN_PLUS);
    case '/':
        return makeToken(TOKEN_SLASH);
    case '*':
        return makeToken(TOKEN_STAR);

        // 二元字符
    case '!':
        return makeToken(match('=') ? TOKEN_BANG_EQUAL : TOKEN_BANG);
    case '=':
        return makeToken(match('=') ? TOKEN_EQUAL_EQUAL : TOKEN_EQUAL);
    case '<':
        return makeToken(match('=') ? TOKEN_LESS_EQUAL : TOKEN_LESS);
    case '>':
        return makeToken(match('=') ? TOKEN_GREATER_EQUAL : TOKEN_GREATER);

    case '"':
        return string();
    }
    return errorToken("Unexpected character");
}

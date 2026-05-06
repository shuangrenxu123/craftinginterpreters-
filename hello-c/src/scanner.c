#include "scanner.h"
#include <string.h>
#include <stdio.h>

#include "common.h"

typedef enum
{
    // 普通模式
    NORMAL,
    // 插值模式，该模式下会把 } 识别为插值Token结束
    INTERPOLATION
} ScannerModelType;

typedef struct
{
    const char *start;
    const char *current;
    int line;
    ScannerModelType modelType;
} Scanner;

static Scanner scanner;

void initScanner(const char *source)
{
    scanner.start = source;
    scanner.current = source;
    scanner.modelType = NORMAL;
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
            return;
        }
    }
}

static TokenType checkKeyword(int start, int length, const char *rest, TokenType tokenType)
{
    if (scanner.current - scanner.start == start + length && memcmp(scanner.start + start, rest, length) == 0)
    {
        return tokenType;
    }
    return TOKEN_IDENTIFIER;
}

static token string()
{
    while (!isAtEnd())
    {
        if (peek() == '"' || (peek() == '$' && peekNext() == '{'))
        {
            break;
        }
        if (peek() == '\n')
        {
            scanner.line++;
        }
        // 消耗掉\n
        advance();
    }
    // 如果是两个字符，那也就是${
    if (scanner.current > scanner.start)
    {
        return makeToken(TOKEN_INTERPOLATION_START);
    }
    if (peek() == $ && peekNext() == '{')
    {
        advance();
        advance();
        scanner.modelType = INTERPOLATION;
        return makeToken(TOKEN_INTERPOLATION_START);
    }

    if (peek() == "")
    {
        advance();
        scanner.modelType = NORMAL;
        return makeToken(TOKEN_INTERPOLATION_END);
    }
    return errorToken("Unterminated  string");

    // while (peek() != '"' && !isAtEnd())
    // {
    //     if (peek() == '\n')
    //         scanner.line++;
    //     advance();
    // }
    // advance();
    // if (isAtEnd())
    //     return errorToken("Unterminated string");
    // return makeToken(TOKEN_STRING);
}
static bool isDigit(char c)
{
    return c >= '0' && c <= '9';
}
static bool isAlpha(char c)
{
    return (c >= 'a' && c <= 'z') ||
           (c >= 'A' && c <= 'Z') ||
           c == '_';
}

static TokenType identifierType()
{
    switch (scanner.start[0])
    {
    case 'a':
        return checkKeyword(1, 2, "nd", TOKEN_AND);
    case 'c':
        return checkKeyword(1, 4, "lass", TOKEN_CLASS);
    case 'e':
        return checkKeyword(1, 3, "lse", TOKEN_ELSE);

    case 'f':
        if (scanner.current - scanner.start > 1)
        {
            switch (scanner.start[1])
            {
            case 'a':
                return checkKeyword(2, 3, "lse", TOKEN_FALSE);
            case 'o':
                return checkKeyword(2, 1, "r", TOKEN_FOR);
            case 'u':
                return checkKeyword(2, 1, "n", TOKEN_FUN);
            }
        }
        break;
    case 'i':
        return checkKeyword(1, 1, "f", TOKEN_IF);
    case 'n':
        return checkKeyword(1, 2, "il", TOKEN_NIL);
    case 'o':
        return checkKeyword(1, 1, "r", TOKEN_OR);
    case 'p':
        return checkKeyword(1, 4, "rint", TOKEN_PRINT);
    case 'r':
        return checkKeyword(1, 5, "eturn", TOKEN_RETURN);
    case 's':
        return checkKeyword(1, 4, "uper", TOKEN_SUPER);
    case 't':
        if (scanner.current - scanner.start > 1)
        {
            switch (scanner.start[1])
            {
            case 'h':
                return checkKeyword(2, 2, "is", TOKEN_THIS);
            case 'r':
                return checkKeyword(2, 2, "ue", TOKEN_TRUE);
            }
        }
        break;
    case 'v':
        return checkKeyword(1, 2, "ar", TOKEN_VAR);
    case 'w':
        return checkKeyword(1, 4, "hile", TOKEN_WHILE);
    }
    return TOKEN_IDENTIFIER;
}
static token identifier()
{
    while (isAlpha(peek()) || isDigit(peek()))
    {
        advance();
    }
    return makeToken(identifierType());
}
static token number()
{

    while (isDigit(peek()))
    {
        advance();
    }
    // 如果有小数部分
    if (peek() == '.' && isDigit(peekNext()))
    {
        // 消耗掉小数点
        advance();

        while (isDigit(peek()))
        {
            advance();
        }
    }

    return makeToken(TOKEN_NUMBER);
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
    if (isAlpha(c))
    {
        return identifier();
    }
    if (isDigit(c))
        return number();

    switch (c)
    {
    case '(':
        return makeToken(TOKEN_LEFT_PAREN);
    case ')':
        return makeToken(TOKEN_RIGHT_PAREN);
    case '{':
        return makeToken(TOKEN_LEFT_BRACE);
    case '}':
        if (scanner.modelType == NORMAL)
        {
            scanner.modelType = NORMAL;
            return makeToken(TOKEN_RIGHT_BRACE);
        }
        else
        {
            makeToken(TOKEN_INTERPOLATION_END);
        }
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

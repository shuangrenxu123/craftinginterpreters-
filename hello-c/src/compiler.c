#include "compiler.h"
#include "chunk.h"
#include "scanner.h"

#include <stdio.h>
#include <stdlib.h>

typedef struct
{
    token previous;
    token current;
    bool hadError;
    bool panicMode;
} Parser;
typedef enum
{
    PREC_NONE,
    PREC_ASSIGNMENT, // =
    PREC_OR,         // or
    PREC_AND,        // and
    PREC_EQUALITY,   // == !=
    PREC_COMPARISON, // < > <= >=
    PREC_TERM,       // + -
    PREC_FACTOR,     // * /
    PREC_UNARY,      // ! -
    PREC_CALL,       // . ()
    PREC_PRIMARY
} Precedence;

typedef void (*ParserFn)();

typedef struct
{
    ParserFn prefix;
    ParserFn infix;
    Precedence precedence;
} ParserRule;

Parser parser;
chunk *compilingChunk;

static void expression();
static ParserRule *getRule(TokenType type);
static void parsePrecedence(Precedence precedence);

static void errorAt(token *token, const char *message)
{
    if (parser.panicMode)
        return;
    parser.panicMode = true;
    fprintf(stderr, "[line %d] Error", token->line);
    if (token->type == TOKEN_EOF)
    {
        fprintf(stderr, " at end");
    }
    else if (token->type == TOKEN_ERROR)
    {
        // Nothing.
    }
    else
    {
        fprintf(stderr, " at '%.*s'", token->length, token->start);
    }

    fprintf(stderr, ": %s\n", message);
    parser.hadError = true;
}
static void errorAtCurrent(const char *message)
{
    errorAt(&parser.current, message);
}

static void error(const char *message)
{
    errorAt(&parser.previous, message);
}
/// @brief 推进一个Token
static void advance()
{
    parser.previous = parser.current;
    for (;;)
    {
        parser.current = scanToken();
        if (parser.current.type != TOKEN_ERROR)
        {
            break;
        }
        errorAtCurrent(parser.current.start);
    }
}
static void consume(TokenType type, const char *message)
{
    if (parser.current.type == type)
    {
        advance();
        return;
    }
    errorAtCurrent(message);
}

static chunk *currentChunk()
{
    return compilingChunk;
}

static void parsePrecedence(Precedence precedence)
{
    advance();
    ParserFn prefixRule = getRule(parser.previous.type)->prefix;
    if (prefixRule == NULL)
    {
        error("Expect expression");
        return;
    }
    prefixRule();
    while (precedence <= getRule(parser.current.type)->precedence)
    {
        advance();
        ParserFn infixRule = getRule(parser.previous.type)->infix;
        infixRule();
    }
}

static void emitByte(uint8_t byte)
{
    writeChunk(currentChunk(), byte, parser.previous.line);
}

static void emitReturn()
{
    emitByte(OP_RETURN);
}
static void endCompiler()
{
    emitReturn();
}
static void binary()
{
    TokenType operatorType = parser.previous.type;
    ParserRule *rule = getRule(operatorType);
    parsePrecedence((Precedence)(rule->precedence + 1));
    switch (operatorType)
    {
    case TOKEN_PLUS:
        emitByte(OP_ADD);
        break;
    case TOKEN_MINUS:
        emitByte(OP_SUBTRACT);
        break;
    case TOKEN_STAR:
        emitByte(OP_MULTIPLY);
        break;
    case TOKEN_SLASH:
        emitByte(OP_DIVIDE);
        break;
    default:
        break;
    }
}
static void emitConstant(double value)
{
    writeConstant(currentChunk(), value, parser.previous.line);
}

static void number()
{
    double value = strtod(parser.previous.start, NULL);
    emitConstant(value);
}

static void unary()
{
    TokenType operatorType = parser.previous.type;
    // expression();
    parsePrecedence(PREC_UNARY);
    if (operatorType == TOKEN_MINUS)
    {
        emitByte(OP_NEGATE);
    };
}

static void grouping()
{
    expression();
    // 判断下一个token是不是我们想要的Token。
    consume(TOKEN_RIGHT_PAREN, "Expect ')' , after expression");
}

ParserRule rules[] = {
    [TOKEN_LEFT_PAREN] = {grouping, NULL, PREC_NONE},
    [TOKEN_RIGHT_PAREN] = {NULL, NULL, PREC_NONE},
    [TOKEN_LEFT_BRACE] = {NULL, NULL, PREC_NONE},
    [TOKEN_RIGHT_BRACE] = {NULL, NULL, PREC_NONE},
    [TOKEN_COMMA] = {NULL, NULL, PREC_NONE},
    [TOKEN_DOT] = {NULL, NULL, PREC_NONE},
    [TOKEN_MINUS] = {unary, binary, PREC_TERM},
    [TOKEN_PLUS] = {NULL, binary, PREC_TERM},
    [TOKEN_SEMICOLON] = {NULL, NULL, PREC_NONE},
    [TOKEN_SLASH] = {NULL, binary, PREC_FACTOR},
    [TOKEN_STAR] = {NULL, binary, PREC_FACTOR},
    [TOKEN_BANG] = {NULL, NULL, PREC_NONE},
    [TOKEN_BANG_EQUAL] = {NULL, NULL, PREC_NONE},
    [TOKEN_EQUAL] = {NULL, NULL, PREC_NONE},
    [TOKEN_EQUAL_EQUAL] = {NULL, NULL, PREC_NONE},
    [TOKEN_GREATER] = {NULL, NULL, PREC_NONE},
    [TOKEN_GREATER_EQUAL] = {NULL, NULL, PREC_NONE},
    [TOKEN_LESS] = {NULL, NULL, PREC_NONE},
    [TOKEN_LESS_EQUAL] = {NULL, NULL, PREC_NONE},
    [TOKEN_IDENTIFIER] = {NULL, NULL, PREC_NONE},
    [TOKEN_STRING] = {NULL, NULL, PREC_NONE},
    [TOKEN_NUMBER] = {number, NULL, PREC_NONE},
    [TOKEN_AND] = {NULL, NULL, PREC_NONE},
    [TOKEN_CLASS] = {NULL, NULL, PREC_NONE},
    [TOKEN_ELSE] = {NULL, NULL, PREC_NONE},
    [TOKEN_FALSE] = {NULL, NULL, PREC_NONE},
    [TOKEN_FOR] = {NULL, NULL, PREC_NONE},
    [TOKEN_FUN] = {NULL, NULL, PREC_NONE},
    [TOKEN_IF] = {NULL, NULL, PREC_NONE},
    [TOKEN_NIL] = {NULL, NULL, PREC_NONE},
    [TOKEN_OR] = {NULL, NULL, PREC_NONE},
    [TOKEN_PRINT] = {NULL, NULL, PREC_NONE},
    [TOKEN_RETURN] = {NULL, NULL, PREC_NONE},
    [TOKEN_SUPER] = {NULL, NULL, PREC_NONE},
    [TOKEN_THIS] = {NULL, NULL, PREC_NONE},
    [TOKEN_TRUE] = {NULL, NULL, PREC_NONE},
    [TOKEN_VAR] = {NULL, NULL, PREC_NONE},
    [TOKEN_WHILE] = {NULL, NULL, PREC_NONE},
    [TOKEN_ERROR] = {NULL, NULL, PREC_NONE},
    [TOKEN_EOF] = {NULL, NULL, PREC_NONE},
};
static ParserRule *getRule(TokenType type)
{
    return &rules[type];
}
/// @brief 解析表达式
static void expression()
{
    parsePrecedence(PREC_ASSIGNMENT);
}

bool compile(const char *source, chunk *chunk)
{
    initScanner(source);
    parser.hadError = false;
    parser.panicMode = false;
    compilingChunk = chunk;

    advance();
    expression();
    consume(TOKEN_EOF, "Expect end of expression.");
    endCompiler();
    return !parser.hadError;

    // int line = -1;
    // for (;;)
    // {
    //     token token = scanToken();
    //     if (token.line != line)
    //     {
    //         printf("%4d ", token.line);
    //         line = token.line;
    //     }
    //     else
    //     {
    //         printf("   | ");
    //     }

    //     printf("%2d '%.*s'\n", token.type, token.length, token.start);

    //     if (token.type == TOKEN_EOF)
    //     {
    //         break;
    //     }
    // }
}

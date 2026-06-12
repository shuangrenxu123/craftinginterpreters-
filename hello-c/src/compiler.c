#include "compiler.h"
#include "chunk.h"
#include "scanner.h"
#include "object.h"

#ifdef DEBUG_PRINT_CODE
#include "debug.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

typedef void (*ParserFn)(bool canAssign);

typedef struct
{
    ParserFn prefix;
    ParserFn infix;
    Precedence precedence;
} ParserRule;
typedef struct
{
    token name;
    int depth;
} Local;

typedef struct
{
    Local locals[UINT8_COUNT];
    int localCount;
    int scopeDepth;
} Compiler;

Parser parser;
Compiler *current = NULL;
chunk *compilingChunk;

static void expression();
static void statement();
static void declaration();

static ParserRule *getRule(tokenType type);
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
            printf("%s\n", tokenTypeName(parser.current.type));
            break;
        }
        errorAtCurrent(parser.current.start);
    }
}
static void consume(tokenType type, const char *message)
{
    if (parser.current.type == type)
    {
        advance();
        return;
    }
    errorAtCurrent(message);
}

static bool check(tokenType token)
{
    return parser.current.type == token;
}
static bool match(tokenType token)
{
    if (!check(token))
        return false;
    advance();
    return true;
}

static chunk *currentChunk()
{
    return compilingChunk;
}

static uint8_t makeConstant(value value)
{
    int constant = writeValueArray(&currentChunk()->constants, value);
    if (constant > UINT8_MAX)
    {
        error("Too many constants in one chunk.");
        return 0;
    }
    return (uint8_t)constant;
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

    bool canAssign = precedence <= PREC_ASSIGNMENT;
    prefixRule(canAssign);

    while (precedence <= getRule(parser.current.type)->precedence)
    {
        advance();
        ParserFn infixRule = getRule(parser.previous.type)->infix;
        infixRule();
    }
    if (canAssign && match(TOKEN_EQUAL))
    {
        error("Invalid assignment target");
    }
}

static uint8_t identifierConstant(token *name)
{
    return makeConstant(OBJ_VAL(copyString(name->start, name->length)));
}
static void addLocal(token name)
{

    Local *local = &current->locals[current->localCount++];
    local->name = name;

    local->depth = -1;
}

static bool identifiersEqual(token *a, token *b)
{
    if (a->length != b->length)
        return false;
    return memcmp(a->start, b->start, a->length) == 0;
}

static void declareVariable()
{
    if (current->scopeDepth == 0)
        return;
    token *name = &parser.previous;
    for (int i = current->localCount - 1; i >= 0; i--)
    {
        Local *local = &current->locals[i];
        if (local->depth != -1 && local->depth < current->scopeDepth)
        {
            break;
        }
        if (identifiersEqual(name, &local->name))
        {
            error("Already a variable with this name in this scope");
        }
    }

    addLocal(*name);
}

static uint8_t parserVariable(const char *errorMessage)
{
    consume(TOKEN_IDENTIFIER, errorMessage);
    declareVariable();
    if (current->scopeDepth > 0)
        return 0;

    return identifierConstant(&parser.previous);
}

static void emitByte(uint8_t byte)
{
    writeChunk(currentChunk(), byte, parser.previous.line);
}

static void emitBytes(uint8_t b1, uint8_t b2)
{
    writeChunk(currentChunk(), b1, parser.previous.line);
    writeChunk(currentChunk(), b2, parser.previous.line);
}
static int emitJump(uint8_t instruction)
{
    emitByte(instruction);
    emitByte(0xff);
    emitByte(0xff);
    return currentChunk()->count - 2;
}
static void emitReturn()
{
    emitByte(OP_RETURN);
}
static void endCompiler()
{
    emitReturn();

#ifdef DEBUG_PRINT_CODE
    if (!parser.hadError)
    {
        disassembleChunk(currentChunk(), "code");
    }
#endif
}

static void binary(bool canAssign)
{
    tokenType operatorType = parser.previous.type;
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

    case TOKEN_BANG_EQUAL: // !=
        emitBytes(OP_EQUAL, OP_NOT);
        break;
    case TOKEN_EQUAL_EQUAL:
        emitByte(OP_EQUAL);
        break;
    case TOKEN_GREATER:
        emitByte(OP_GREATER);
        break;
    case TOKEN_GREATER_EQUAL:
        emitBytes(OP_LESS, OP_NOT);
    case TOKEN_LESS:
        emitByte(OP_LESS);
        break;
    case TOKEN_LESS_EQUAL:
        emitBytes(OP_GREATER, OP_NOT);
    default:
        break;
    }
}
// 写入一个字面量
static void literal(bool canAssign)
{
    switch (parser.previous.type)
    {
    case TOKEN_FALSE:
        emitByte(OP_FALSE);
        break;
    case TOKEN_TRUE:
        emitByte(OP_TRUE);
        break;
    case TOKEN_NIL:
        emitByte(OP_NIL);
        break;
    default:
        break;
    }
}

static void emitConstant(value value)
{
    writeConstant(currentChunk(), value, parser.previous.line);
}
static void initCompiler(Compiler *compiler)
{
    compiler->localCount = 0;
    compiler->scopeDepth = 0;
    current = compiler;
}

static void number(bool canAssign)
{
    double number = strtod(parser.previous.start, NULL);
    emitConstant(NUMBER_VAL(number));
}
static void string(bool canAssign)
{
    emitConstant(OBJ_VAL(copyString(parser.previous.start - 1, parser.previous.length - 2)));
}

static int resolveLocal(Compiler *compiler, token *name)
{
    for (int i = compiler->localCount - 1; i >= 0; i--)
    {
        Local *local = &compiler->locals[i];
        if (identifiersEqual(name, &local->name))
        {
            if (local->depth == -1)
            {
                error("Can't read local variable in its own initializer.");
            }
            return i;
        }
    }

    return -1;
}
static void nameVariable(token name, bool canAsign)
{
    uint8_t getop, setop;
    int arg = resolveLocal(current, &name);
    if (arg != -1)
    {
        getop = OP_GET_LOCAL;
        setop = OP_SET_LOCAL;
    }
    else
    {
        arg = identifierConstant(&name);
        getop = OP_GET_GLOBAL;
        setop = OP_SET_GLOBAL;
    }
    if (canAsign && match(TOKEN_EQUAL))
    {
        expression();
        emitBytes(setop, (uint8_t)arg);
    }
    else
    {
        emitBytes(getop, arg);
    }
}
static void variable(bool canAssign)
{
    nameVariable(parser.previous, canAssign);
}
static void unary(bool canAssign)
{
    tokenType operatorType = parser.previous.type;
    parsePrecedence(PREC_UNARY);
    if (operatorType == TOKEN_MINUS)
    {
        emitByte(OP_NEGATE);
    }
    else if (operatorType == TOKEN_BANG)
    {
        emitByte(OP_NOT);
    }
}

static void grouping(bool canAssign)
{
    expression();
    // 判断下一个token是不是我们想要的Token。
    consume(TOKEN_RIGHT_PAREN, "Expect ')' , after expression");
}

static void Interpolation(bool canAssign)
{
    expression();
    consume(TOKEN_INTERPOLATION_END, "Expect '}' , after expression");
    emitByte(OP_TOSTRING);
}
static void and(bool canAssign)
{
    int endJump = emitJump(OP_JUMP_IF_FALSE);
    emitByte(OP_POP);
    parsePrecedence(PREC_AND);
    patchJump(endJump);
}
static void or(bool canAssign)
{
    int elseJump = emitJump(OP_JUMP_IF_FALSE);
    int endJump = emitJump(OP_JUMP);
    patchJump(elseJump);
    emitByte(OP_POP);
    parsePrecedence(PREC_OR);
    patchJump(endJump);
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
    [TOKEN_BANG] = {unary, NULL, PREC_NONE},
    [TOKEN_BANG_EQUAL] = {NULL, binary, PREC_NONE},
    [TOKEN_EQUAL] = {NULL, NULL, PREC_NONE},
    [TOKEN_EQUAL_EQUAL] = {NULL, binary, PREC_NONE},
    [TOKEN_GREATER] = {NULL, binary, PREC_NONE},
    [TOKEN_GREATER_EQUAL] = {NULL, binary, PREC_NONE},
    [TOKEN_LESS] = {NULL, binary, PREC_NONE},
    [TOKEN_LESS_EQUAL] = {NULL, binary, PREC_NONE},
    [TOKEN_STRING] = {string, NULL, PREC_NONE},
    [TOKEN_IDENTIFIER] = {NULL, NULL, PREC_NONE},
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
    [TOKEN_FALSE] = {literal, NULL, PREC_NONE},
    [TOKEN_TRUE] = {literal, NULL, PREC_NONE},
    [TOKEN_AND] = {NULL, and, PREC_AND},
    [TOKEN_NIL] = {literal, NULL, PREC_NONE},
    [TOKEN_IDENTIFIER] = {variable, NULL, PREC_NONE},
    [TOKEN_INTERPOLATION_START] = {Interpolation, NULL, PREC_NONE},
    [TOKEN_INTERPOLATION_END] = {NULL, NULL, PREC_NONE},
    [TOKEN_OR] = {NULL, or, PREC_OR},

};
static ParserRule *getRule(tokenType type)
{
    return &rules[type];
}
/// @brief 解析表达式
static void expression()
{
    parsePrecedence(PREC_ASSIGNMENT);
}

/// @brief 一条完整的表达式语句。会移除一次栈值
static void expressionStatement()
{
    expression();
    consume(TOKEN_SEMICOLON, "Expect ; after value");
    emitByte(OP_POP);
}
static void printStatement()
{
    expression();
    consume(TOKEN_SEMICOLON, "Expect ; after value");
    emitByte(OP_PRINT);
}
static void emitLoop(int loopStart)
{
    emitByte(OP_LOOP);
    int offset = currentChunk()->count - loopStart + 2;
    if (offset > UINT16_MAX)
    {
        error("loop body too longer");
    }
    emitByte((offset >> 8) & 0xff);
    emitByte((offset) & 0xff);
}

static void synchronize()
{
    parser.panicMode = false;

    while (parser.current.type != TOKEN_EOF)
    {
        if (parser.previous.type == TOKEN_SEMICOLON)
            return;
        switch (parser.current.type)
        {
        case TOKEN_CLASS:
        case TOKEN_FUN:
        case TOKEN_VAR:
        case TOKEN_FOR:
        case TOKEN_IF:
        case TOKEN_WHILE:
        case TOKEN_PRINT:
        case TOKEN_RETURN:
            return;

        default:; // Do nothing.
        }

        advance();
    }
}

static void markInitialized()
{
    current->locals[current->localCount - 1].depth =
        current->scopeDepth;
}

static void defineVariable(uint8_t global)
{
    if (current->scopeDepth > 0)
    {
        markInitialized();
        return;
    }

    emitBytes(OP_DEFINE_GLOBAL, global);
}

static void varDeclaration()
{
    uint8_t global = parserVariable("Expect variable name");
    if (match(TOKEN_EQUAL))
    {
        expression();
    }
    else
    {
        emitByte(OP_NIL);
    }
    // 确保是; 结尾
    consume(TOKEN_SEMICOLON,
            "Expect ';' after variable declaration.");
    defineVariable(global);
}
static void declaration()
{
    if (match(TOKEN_VAR))
    {
        varDeclaration();
    }
    else
    {
        statement();
    }
    if (parser.panicMode)
    {
        synchronize();
    }
}
static void beginScope()
{
    current->scopeDepth++;
}
static void endScope()
{
    current->scopeDepth--;
    while (current->localCount > 0 &&
           current->locals[current->localCount - 1].depth > current->scopeDepth)
    {
        emitByte(OP_POP);
        current->localCount--;
    }
}

static void block()
{
    while (!check(TOKEN_RIGHT_BRACE) && !check(TOKEN_EOF))
    {
        declaration();
    }

    consume(TOKEN_RIGHT_BRACE, "Exprect } after");
}
static void patchJump(int offset)
{
    int jump = currentChunk()->count - offset - 2;
    if (jump > UINT16_MAX)
    {
        error("Too much code to jump over");
    }
    currentChunk()->code[offset] = (jump >> 8) & 0xff;
    currentChunk()->code[offset + 1] = jump & 0xff;
}
static void whileStatement()
{
    int loopStart = currentChunk()->count;
    consume(TOKEN_LEFT_BRACE, "not is )");
    expression();
    consume(TOKEN_RIGHT_BRACE, "not is )");

    int exitJump = emitJump(OP_JUMP_IF_FALSE);
    emitByte(OP_POP);
    statement();

    emitLoop(loopStart);

    patchJump(exitJump);
    emitByte(OP_POP);
}
static void forStatement()
{
    beginScope();
    consume(TOKEN_LEFT_PAREN, "not is (");
    if (match(TOKEN_SEMICOLON))
    {
    }

    else if (match(TOKEN_VAR))
    {
        varDeclaration();
    }
    else
    {
        expressionStatement();
    }

    int loopStart = currentChunk()->count;
    int exitJump = -1;
    if (!match(TOKEN_SEMICOLON))
    {
        expression();
        consume(TOKEN_SEMICOLON, "Expect ;");
        exitJump = emitJump(OP_JUMP_IF_FALSE);
        emitByte(OP_POP);
    }
    if (!match(TOKEN_LEFT_PAREN))
    {
        int bodyJump = emitJump(OP_JUMP);
        int incrementStart = currentChunk()->count;
        expression();
        emitByte(OP_POP);
        consume(TOKEN_LEFT_PAREN, "");
        emitLoop(loopStart);
        loopStart = incrementStart;
        patchJump(bodyJump);
    }
    statement();
    emitLoop(loopStart);

    if (exitJump != -1)
    {
        patchJump(exitJump);
        emitByte(OP_POP);
    }

    endScope();
}

static void statement()
{
    if (match(TOKEN_PRINT))
    {
        printStatement();
    }
    else if (match(TOKEN_IF))
    {
        ifStatement();
    }
    else if (match(TOKEN_WHILE))
    {
        whileStatement();
    }
    else if (match(TOKEN_FOR))
    {
        forStatement();
    }
    else if (match(TOKEN_LEFT_BRACE))
    {
        beginScope();
        block();
        endScope();
    }
    else
    {
        expressionStatement();
    }
}

static void ifStatement()
{
    consume(TOKEN_LEFT_BRACE, "Exprect ( after");
    expression();
    consume(TOKEN_RIGHT_BRACE, "not is )");

    int thenJump = emitJump(OP_JUMP_IF_FALSE);
    emitByte(OP_POP);
    statement();

    int elseJump = emitJump(OP_JUMP);

    patchJump(thenJump);
    emitByte(OP_POP);
    if (match(TOKEN_ELSE))
    {
        statement();
    }
}
bool compile(const char *source, chunk *chunk)
{
    initScanner(source);
    Compiler compiler;
    initCompiler(&compiler);
    parser.hadError = false;
    parser.panicMode = false;
    compilingChunk = chunk;

    advance();

    while (!match(TOKEN_EOF))
    {
        declaration();
    }

    // expression();
    // consume(TOKEN_EOF, "Expect end of expression.");
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

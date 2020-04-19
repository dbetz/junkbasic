/* compile.h - definitions for a simple basic compiler
 *
 * Copyright (c) 2020 by David Michael Betz.  All rights reserved.
 *
 */

#ifndef __COMPILE_H__
#define __COMPILE_H__

#include <stdio.h>
#include <setjmp.h>
#include "types.h"
#include "image.h"
#include "system.h"

/* program limits */
#define MAXTOKEN        32

/* forward type declarations */
typedef struct SymbolTable SymbolTable;
typedef struct Symbol Symbol;
typedef struct ParseTreeNode ParseTreeNode;
typedef struct NodeListEntry NodeListEntry;

/* lexical tokens */
enum {
    T_NONE,
    T_REM = 0x100,  /* keywords start here */
    T_DIM,
    T_FUNCTION,
    T_SUB,
    T_AS,
    T_LET,
    T_IF,
    T_THEN,
    T_ELSE,
    T_END,
    T_FOR,
    T_TO,
    T_STEP,
    T_NEXT,
    T_DO,
    T_WHILE,
    T_UNTIL,
    T_LOOP,
    T_GOTO,
    T_MOD,
    T_AND,
    T_OR,
    T_NOT,
    T_STOP,
    T_RETURN,
    T_PRINT,
    T_END_FUNCTION, /* compound keywords */
    T_END_SUB,
    T_ELSE_IF,
    T_END_IF,
    T_DO_WHILE,
    T_DO_UNTIL,
    T_LOOP_WHILE,
    T_LOOP_UNTIL,
    T_LE,           /* non-keyword tokens */
    T_NE,
    T_GE,
    T_SHL,
    T_SHR,
    T_IDENTIFIER,
    T_NUMBER,
    T_STRING,
    T_EOL,
    T_EOF
};

/* block type */
typedef enum {
    BLOCK_NONE,
    BLOCK_FUNCTION,
    BLOCK_IF,
    BLOCK_ELSE,
    BLOCK_FOR,
    BLOCK_DO
} BlockType;

/* block structure */
typedef struct Block Block;
struct Block {
    BlockType type;
    ParseTreeNode *node;
    NodeListEntry **pNextStatement;
};

/* string structure */
typedef struct String String;
struct String {
    String *next;
    int placed;
    VMVALUE value;
    char data[1];
};

/* storage class ids */
typedef enum {
    SC_CONSTANT,
    SC_VARIABLE
} StorageClass;

/* symbol types */
typedef enum {
    TYPE_INTEGER,
    TYPE_BYTE,
    TYPE_STRING,
    TYPE_ARRAY,
    TYPE_POINTER,
    TYPE_FUNCTION
} Type;

/* symbol table */
struct SymbolTable {
    Symbol *head;
    Symbol **pTail;
    int count;
};

/* symbol structure */
struct Symbol {
    Symbol *next;
    StorageClass storageClass;
    Type type;
    VMVALUE offset;
    int placed;
    union {
        VMVALUE value;
        String *string;
    } v;
    char name[1];
};

/* parse context */
typedef struct {
    System *sys;                    /* system context */
    uint8_t *nextGlobal;            /* next global heap space location */
    uint8_t *nextLocal;             /* next local heap space location */
    size_t heapSize;                /* size of heap space in bytes */
    size_t maxHeapUsed;             /* maximum amount of heap space allocated so far */
    int lineNumber;                 /* scan - current line number */
    int savedToken;                 /* scan - lookahead token */
    int tokenOffset;                /* scan - offset to the start of the current token */
    char token[MAXTOKEN];           /* scan - current token string */
    VMVALUE tokenValue;             /* scan - current token integer value */
    int inComment;                  /* scan - inside of a slash/star comment */
    SymbolTable globals;            /* parse - global variables and constants */
    String *strings;                /* parse - string constants */
    ParseTreeNode *mainFunction;    /* parse - the main function */
    ParseTreeNode *currentFunction; /* parse - the function currently being parsed */
    Block blockBuf[10];             /* parse - stack of nested blocks */
    Block *bptr;                    /* parse - current block */
    Block *btop;                    /* parse - top of block stack */
} ParseContext;

/* parse tree node types */
enum {
    NodeTypeFunctionDefinition,
    NodeTypeLetStatement,
    NodeTypeIfStatement,
    NodeTypeForStatement,
    NodeTypeDoWhileStatement,
    NodeTypeDoUntilStatement,
    NodeTypeLoopStatement,
    NodeTypeLoopWhileStatement,
    NodeTypeLoopUntilStatement,
    NodeTypeReturnStatement,
    NodeTypeEndStatement,
    NodeTypeCallStatement,
    NodeTypeGlobalRef,
    NodeTypeArgumentRef,
    NodeTypeLocalRef,
    NodeTypeStringLit,
    NodeTypeIntegerLit,
    NodeTypeUnaryOp,
    NodeTypeBinaryOp,
    NodeTypeArrayRef,
    NodeTypeFunctionCall,
    NodeTypeDisjunction,
    NodeTypeConjunction
};

/* parse tree node structure */
struct ParseTreeNode {
    int nodeType;
    int type;
    union {
        struct {
            Symbol *symbol;
            SymbolTable arguments;
            SymbolTable locals;
            int argumentOffset;
            int localOffset;
            NodeListEntry *bodyStatements;
        } functionDefinition;
        struct {
            ParseTreeNode *lvalue;
            ParseTreeNode *rvalue;
        } letStatement;
        struct {
            ParseTreeNode *test;
            NodeListEntry *thenStatements;
            NodeListEntry *elseStatements;
        } ifStatement;
        struct {
            ParseTreeNode *var;
            ParseTreeNode *startExpr;
            ParseTreeNode *endExpr;
            ParseTreeNode *stepExpr;
            NodeListEntry *bodyStatements;
        } forStatement;
        struct {
            ParseTreeNode *test;
            NodeListEntry *bodyStatements;
        } loopStatement;
        struct {
            ParseTreeNode *expr;
        } returnStatement;
        struct {
            ParseTreeNode *expr;
        } callStatement;
        struct {
            Symbol *symbol;
        } symbolRef;
        struct {
            String *string;
        } stringLit;
        struct {
            VMVALUE value;
        } integerLit;
        struct {
            int op;
            ParseTreeNode *expr;
        } unaryOp;
        struct {
            int op;
            ParseTreeNode *left;
            ParseTreeNode *right;
        } binaryOp;
        struct {
            ParseTreeNode *array;
            ParseTreeNode *index;
        } arrayRef;
        struct {
            ParseTreeNode *fcn;
            NodeListEntry *args;
            int argc;
        } functionCall;
        struct {
            NodeListEntry *exprs;
        } exprList;
    } u;
};

/* node list entry structure */
struct NodeListEntry {
    ParseTreeNode *node;
    NodeListEntry *next;
};

/* code generator context */
typedef struct GenerateContext GenerateContext;

/* parse.c */
ParseContext *InitParseContext(System *sys);
void Compile(ParseContext *c);
String *AddString(ParseContext *c, const char *value);
void DumpStrings(ParseContext *c);
void *GlobalAlloc(ParseContext *c, size_t size);
void *LocalAlloc(ParseContext *c, size_t size);

/* statement.c */
void ParseStatement(ParseContext *c, int tkn);

/* expr.c */
void ParseRValue(ParseContext *c);
ParseTreeNode *ParseExpr(ParseContext *c);
ParseTreeNode *ParsePrimary(ParseContext *c);
ParseTreeNode *GetSymbolRef(ParseContext *c, const char *name);
int IsIntegerLit(ParseTreeNode *node);
ParseTreeNode *GetSymbolRef(ParseContext *c, const char *name);
int IsIntegerLit(ParseTreeNode *node);
void PrintNode(ParseTreeNode *node, int indent);

/* scan.c */
void InitScan(ParseContext *c);
void FRequire(ParseContext *c, int requiredToken);
void Require(ParseContext *c, int token, int requiredToken);
int GetToken(ParseContext *c);
void SaveToken(ParseContext *c, int token);
char *TokenName(int token);
int SkipSpaces(ParseContext *c);
int GetChar(ParseContext *c);
void UngetC(ParseContext *c);
void ParseError(ParseContext *c, const char *fmt, ...);

/* symbols.c */
void InitSymbolTable(SymbolTable *table);
Symbol *AddGlobal(ParseContext *c, const char *name, StorageClass storageClass, VMVALUE value);
Symbol *AddArgument(ParseContext *c, const char *name, StorageClass storageClass, int value);
Symbol *AddLocal(ParseContext *c, const char *name, StorageClass storageClass, int value);
Symbol *FindGlobal(ParseContext *c, const char *name);
Symbol *FindArgument(ParseContext *c, const char *name);
Symbol *FindLocal(ParseContext *c, const char *name);
int IsConstant(Symbol *symbol);
void DumpSymbols(SymbolTable *table, const char *tag);

/* generate.c */
GenerateContext *InitGenerateContext(uint8_t *freeSpace, size_t freeSize);
void Generate(GenerateContext *c, ParseTreeNode *node);

#endif


/* compile.h - definitions for a simple basic compiler
 *
 * Copyright (c) 2019 by David Michael Betz.  All rights reserved.
 *
 */

#ifndef __COMPILE_H__
#define __COMPILE_H__

#include <stdio.h>
#include <setjmp.h>
#include "types.h"
#include "image.h"
#include "system.h"

#ifdef WIN32
#define strcasecmp  _stricmp
#endif

/* program limits */
#define MAXTOKEN        32

/* forward type declarations */
typedef struct SymbolTable SymbolTable;
typedef struct Symbol Symbol;
typedef struct ParseTreeNode ParseTreeNode;
typedef struct NodeListEntry NodeListEntry;
typedef struct NodeListEntry NodeListEntry;

typedef int Type;

/* lexical tokens */
enum {
    T_NONE,
    T_REM = 0x100,  /* keywords start here */
    T_DEF,
    T_DIM,
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
#ifdef USE_ASM
    T_ASM,
#endif
    T_ELSE_IF,  /* compound keywords */
    T_END_DEF,
    T_END_IF,
#ifdef USE_ASM
    T_END_ASM,
#endif
    T_DO_WHILE,
    T_DO_UNTIL,
    T_LOOP_WHILE,
    T_LOOP_UNTIL,
    T_LE,       /* non-keyword tokens */
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
    BLOCK_MAIN,
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

typedef enum {
    GEN_BLOCK_SELECT
} GenBlockType;

typedef struct {
    GenBlockType type;
    union {
        struct {
            int first;
            VMUVALUE nxt;
            VMUVALUE end;
        } selectBlock;
    } u;
} GenBlock;

/* string structure */
typedef struct String String;
struct String {
    String *next;
    int placed;
    int fixups;
    VMVALUE value;
    char data[1];
};

/* label structure */
typedef struct Label Label;
struct Label {
    Label *next;
    int placed;
    int fixups;
    int offset;
    char name[1];
};

/* storage class ids */
typedef enum {
    SC_CONSTANT,
    SC_VARIABLE
} StorageClass;

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
    int type;
    union {
        struct {
            VMUVALUE offset;
            VMUVALUE fixups;
        } variable;
        VMVALUE value;
        String *string;
    } v;
    char name[1];
};

/* code types */
typedef enum {
    CODE_TYPE_MAIN,
    CODE_TYPE_FUNCTION
} CodeType;

/* parse context */
typedef struct {
    System *sys;                    /* system context */
    uint8_t *freeMark;              /* saved position for reclaiming compiler memory */
    uint8_t *nextGlobal;            /* next global heap space location */
    uint8_t *nextLocal;             /* next local heap space location */
    size_t heapSize;                /* size of heap space in bytes */
    size_t maxHeapUsed;             /* maximum amount of heap space allocated so far */
    int (*getLine)(void *cookie, char *buf, int len, VMVALUE *pLineNumber);
                                    /* scan - function to get a line of input */
    void *getLineCookie;            /* scan - cookie for the getLine function */
    int lineNumber;                 /* scan - current line number */
    int savedToken;                 /* scan - lookahead token */
    int tokenOffset;                /* scan - offset to the start of the current token */
    char token[MAXTOKEN];           /* scan - current token string */
    VMVALUE value;                  /* scan - current token integer value */
    int inComment;                  /* scan - inside of a slash/star comment */
    SymbolTable globals;            /* parse - global variables and constants */
    String *strings;                /* parse - string constants */
    Label *labels;                  /* parse - local labels */
    CodeType codeType;              /* parse - type of code under construction */
    Symbol *codeSymbol;             /* parse - symbol table entry of code under construction */
    SymbolTable arguments;          /* parse - arguments of current function definition */
    SymbolTable locals;             /* parse - local variables of current function definition */
    int localOffset;                /* parse - offset to next available local variable */
    NodeListEntry *mainStatements;  /* parse - list of statements in the main program */
    Block blockBuf[10];             /* parse - stack of nested blocks */
    Block *bptr;                    /* parse - current block */
    Block *btop;                    /* parse - top of block stack */
    uint8_t *cptr;                  /* generate - next available code staging buffer position */
    uint8_t *ctop;                  /* generate - top of code staging buffer */
    uint8_t *codeBuf;               /* generate - code staging buffer */
    ImageHdr *image;                /* header of image being constructed */
} ParseContext;

/* partial value */
typedef struct PVAL PVAL;

/* partial value function codes */
typedef enum {
    PV_LOAD,
    PV_STORE,
    PV_REFERENCE
} PValOp;

typedef void GenFcn(ParseContext *c, PValOp op, PVAL *pv);
#define GEN_NULL    ((GenFcn *)0)

/* partial value structure */
struct PVAL {
    Type *type;
    GenFcn *fcn;
    union {
        Symbol *sym;
        String *str;
        VMVALUE val;
    } u;
};

/* types */
typedef enum {
    TYPE_INTEGER,
    TYPE_BYTE,
    TYPE_STRING,
    TYPE_ARRAY,
    TYPE_POINTER,
    TYPE_FUNCTION
} TypeID;

/* parse tree node types */
enum {
    NodeTypeLetStatement,
    NodeTypeIfStatement,
    NodeTypeForStatement,
    NodeTypeDoWhileStatement,
    NodeTypeDoUntilStatement,
    NodeTypeLoopStatement,
    NodeTypeLoopWhileStatement,
    NodeTypeLoopUntilStatement,
    NodeTypeReturnStatement,
    NodeTypeCallStatement,
    NodeTypeGlobalRef,
    NodeTypeArgumentRef,
    NodeTypeLocalRef,
    NodeTypeStringLit,
    NodeTypeIntegerLit,
    NodeTypeFunctionLit,
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
            int offset;
        } symbolRef;
        struct {
            String *string;
        } stringLit;
        struct {
            VMVALUE value;
        } integerLit;
        struct {
            int offset;
        } functionLit;
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

/* parse.c */
ParseContext *InitParseContext(System *sys);
void Compile(ParseContext *c);
void InitCodeBuffer(ParseContext *c);
void StartCode(ParseContext *c, CodeType type);
VMVALUE StoreCode(ParseContext *c);
void AddIntrinsic(ParseContext *c, const char *name, int index);
String *AddString(ParseContext *c, const char *value);
void DumpStrings(ParseContext *c);
VMVALUE AddStringRef(String *str, int offset);
VMVALUE AddSymbolRef(Symbol *sym, int offset);
void *GlobalAlloc(ParseContext *c, size_t size);
void *LocalAlloc(ParseContext *c, size_t size);
void Fatal(ParseContext *c, const char *fmt, ...);

/* statement.c */
void ParseStatement(ParseContext *c, int tkn);
BlockType CurrentBlockType(ParseContext *c);
void CheckLabels(ParseContext *c);

/* expr.c */
void ParseRValue(ParseContext *c);
ParseTreeNode *ParseExpr(ParseContext *c);
ParseTreeNode *ParsePrimary(ParseContext *c);
ParseTreeNode *GetSymbolRef(ParseContext *c, const char *name);
int IsIntegerLit(ParseTreeNode *node);
ParseTreeNode *GetSymbolRef(ParseContext *c, const char *name);
int IsIntegerLit(ParseTreeNode *node);

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
Symbol *FindSymbol(SymbolTable *table, const char *name);
int IsConstant(Symbol *symbol);
void DumpSymbols(SymbolTable *table, const char *tag);

/* generate.c */
void code_lvalue(ParseContext *c, ParseTreeNode *expr, PVAL *pv);
void code_rvalue(ParseContext *c, ParseTreeNode *expr);
void rvalue(ParseContext *c, PVAL *pv);
void chklvalue(ParseContext *c, PVAL *pv);
void code_global(ParseContext *c, PValOp fcn, PVAL *pv);
void code_local(ParseContext *c, PValOp fcn, PVAL *pv);
int codeaddr(ParseContext *c);
int putcbyte(ParseContext *c, int b);
int putcword(ParseContext *c, VMVALUE w);
void fixup(ParseContext *c, VMUVALUE chn, VMUVALUE val);
void fixupbranch(ParseContext *c, VMUVALUE chn, VMUVALUE val);

#endif


/* generate.c - code generation functions
 *
 * Copyright (c) 2020 by David Michael Betz.  All rights reserved.
 *
 */

#include <string.h>
#include "compile.h"

typedef struct GenerateContext GenerateContext;

/* partial value */
typedef struct PVAL PVAL;

/* partial value function codes */
typedef enum {
    PV_LOAD,
    PV_STORE
} PValOp;

typedef void GenFcn(GenerateContext *c, PValOp op, PVAL *pv);
#define GEN_NULL    ((GenFcn *)0)

/* partial value structure */
struct PVAL {
    GenFcn *fcn;
    union {
        Symbol *sym;
        String *str;
        VMVALUE val;
    } u;
};

struct GenerateContext {
    uint8_t *cptr;                  /* generate - next available code staging buffer position */
    uint8_t *ctop;                  /* generate - top of code staging buffer */
    uint8_t *codeBuf;               /* generate - code staging buffer */
};

/* generate.c */
void Generate(GenerateContext *c, ParseTreeNode *expr);
void code_expr(GenerateContext *c, ParseTreeNode *expr, PVAL *pv);
void code_global(GenerateContext *c, PValOp fcn, PVAL *pv);
void code_local(GenerateContext *c, PValOp fcn, PVAL *pv);
VMUVALUE codeaddr(GenerateContext *c);
VMUVALUE putcbyte(GenerateContext *c, int b);
VMUVALUE putcword(GenerateContext *c, VMVALUE w);
VMVALUE rd_cword(GenerateContext *c, VMUVALUE off);
void wr_cword(GenerateContext *c, VMUVALUE off, VMVALUE w);
void fixupbranch(GenerateContext *c, VMUVALUE chn, VMUVALUE val);

/* local function prototypes */
static void code_lvalue(GenerateContext *c, ParseTreeNode *expr, PVAL *pv);
static void code_rvalue(GenerateContext *c, ParseTreeNode *expr);
static void code_function_definition(GenerateContext *c, ParseTreeNode *node);
static void code_if_statement(GenerateContext *c, ParseTreeNode *node);
static void code_for_statement(GenerateContext *c, ParseTreeNode *node);
static void code_do_while_statement(GenerateContext *c, ParseTreeNode *node);
static void code_do_until_statement(GenerateContext *c, ParseTreeNode *node);
static void code_loop_statement(GenerateContext *c, ParseTreeNode *node);
static void code_loop_while_statement(GenerateContext *c, ParseTreeNode *node);
static void code_loop_until_statement(GenerateContext *c, ParseTreeNode *node);
static void code_return_statement(GenerateContext *c, ParseTreeNode *node);
static void code_statement_list(GenerateContext *c, NodeListEntry *entry);
static void code_shortcircuit(GenerateContext *c, int op, ParseTreeNode *expr);
static void code_addressof(GenerateContext *c, ParseTreeNode *expr);
static void code_call(GenerateContext *c, ParseTreeNode *expr);
static void code_symbolRef(GenerateContext *c, Symbol *sym);
static void code_arrayref(GenerateContext *c, ParseTreeNode *expr, PVAL *pv);
static void code_index(GenerateContext *c, PValOp fcn, PVAL *pv);
static VMUVALUE AddStringRef(GenerateContext *c, String *str);
static void GenerateError(GenerateContext *c, const char *fmt, ...);
static void GenerateFatal(GenerateContext *c, const char *fmt, ...);

/* Generate - generate code for a function */
void Generate(GenerateContext *c, ParseTreeNode *node)
{
    PVAL pv;
    code_expr(c, node, &pv);
}

/* code_lvalue - generate code for an l-value expression */
static void code_lvalue(GenerateContext *c, ParseTreeNode *expr, PVAL *pv)
{
    code_expr(c, expr, pv);
    if (pv->fcn == GEN_NULL)
        GenerateError(c,"Expecting an lvalue");
}

/* code_rvalue - generate code for an r-value expression */
static void code_rvalue(GenerateContext *c, ParseTreeNode *expr)
{
    PVAL pv;
    code_expr(c, expr, &pv);
    if (pv.fcn)
        (*pv.fcn)(c, PV_LOAD, &pv);
}

/* code_expr - generate code for an expression parse tree */
void code_expr(GenerateContext *c, ParseTreeNode *expr, PVAL *pv)
{
    VMVALUE ival;
    switch (expr->nodeType) {
    case NodeTypeFunctionDefinition:
        code_function_definition(c, expr);
        break;
    case NodeTypeLetStatement:
        code_rvalue(c, expr->u.letStatement.rvalue);
        code_lvalue(c, expr->u.letStatement.lvalue, pv);
        (pv->fcn)(c, PV_STORE, pv);
        break;
    case NodeTypeIfStatement:
        code_if_statement(c, expr);
        break;
    case NodeTypeForStatement:
        code_for_statement(c, expr);
        break;
    case NodeTypeDoWhileStatement:
        code_do_while_statement(c, expr);
        break;
    case NodeTypeDoUntilStatement:
        code_do_until_statement(c, expr);
        break;
    case NodeTypeLoopStatement:
        code_loop_statement(c, expr);
        break;
    case NodeTypeLoopWhileStatement:
        code_loop_while_statement(c, expr);
        break;
    case NodeTypeLoopUntilStatement:
        code_loop_until_statement(c, expr);
        break;
    case NodeTypeReturnStatement:
        code_return_statement(c, expr);
        break;
    case NodeTypeCallStatement:
        code_rvalue(c, expr->u.callStatement.expr);
        putcbyte(c, OP_DROP);
        break;
    case NodeTypeGlobalRef:
        pv->fcn = code_global;
        pv->u.sym = expr->u.symbolRef.symbol;
        break;
    case NodeTypeLocalRef:
        pv->fcn = code_local;
        //pv->u.val = expr->u.symbolRef.offset;
        break;
    case NodeTypeStringLit:
        putcbyte(c, OP_LIT);
        putcword(c, AddStringRef(c, expr->u.stringLit.string));
        pv->fcn = GEN_NULL;
        break;
    case NodeTypeIntegerLit:
        ival = expr->u.integerLit.value;
        if (ival >= -128 && ival <= 127) {
            putcbyte(c, OP_SLIT);
            putcbyte(c, ival);
        }
        else {
            putcbyte(c, OP_LIT);
            putcword(c, ival);
        }
        pv->fcn = GEN_NULL;
        break;
    case NodeTypeUnaryOp:
        code_rvalue(c, expr->u.unaryOp.expr);
        putcbyte(c, expr->u.unaryOp.op);
        pv->fcn = GEN_NULL;
        break;
    case NodeTypeBinaryOp:
        code_rvalue(c, expr->u.binaryOp.left);
        code_rvalue(c, expr->u.binaryOp.right);
        putcbyte(c, expr->u.binaryOp.op);
        pv->fcn = GEN_NULL;
        break;
    case NodeTypeFunctionCall:
        code_call(c, expr);
        pv->fcn = GEN_NULL;
        break;
    case NodeTypeArrayRef:
        code_arrayref(c, expr, pv);
        break;
    case NodeTypeDisjunction:
        code_shortcircuit(c, OP_BRTSC, expr);
        pv->fcn = GEN_NULL;
        break;
    case NodeTypeConjunction:
        code_shortcircuit(c, OP_BRFSC, expr);
        pv->fcn = GEN_NULL;
        break;
    }
}

/* code_function_definition - generate code for a function definition */
static void code_function_definition(GenerateContext *c, ParseTreeNode *node)
{
    putcbyte(c, OP_FRAME);
    putcbyte(c, F_SIZE + node->u.functionDefinition.localOffset);
    code_statement_list(c, node->u.functionDefinition.bodyStatements);
    if (node->type)
        putcbyte(c, OP_RETURNZ);
    else
        putcbyte(c, OP_HALT);
}

/* code_if_statement - generate code for an IF statement */
static void code_if_statement(GenerateContext *c, ParseTreeNode *node)
{
    VMUVALUE nxt, end;
    code_rvalue(c, node->u.ifStatement.test);
    putcbyte(c, OP_BRF);
    nxt = putcword(c, 0);
    code_statement_list(c, node->u.ifStatement.thenStatements);
    putcbyte(c, OP_BR);
    end = putcword(c, 0);
    fixupbranch(c, nxt, codeaddr(c));
    code_statement_list(c, node->u.ifStatement.elseStatements);
    fixupbranch(c, end, codeaddr(c));
}

/* code_for_statement - generate code for a FOR statement */
static void code_for_statement(GenerateContext *c, ParseTreeNode *node)
{
    VMUVALUE nxt, upd, inst;
    PVAL pv;
    code_rvalue(c, node->u.forStatement.startExpr);
    code_lvalue(c, node->u.forStatement.var, &pv);
    putcbyte(c, OP_BR);
    upd = putcword(c, 0);
    nxt = codeaddr(c);
    code_statement_list(c, node->u.forStatement.bodyStatements);
    (*pv.fcn)(c, PV_LOAD, &pv);
    if (node->u.forStatement.stepExpr)
        code_rvalue(c, node->u.forStatement.stepExpr);
    else {
        putcbyte(c, OP_SLIT);
        putcbyte(c, 1);
    }
    putcbyte(c, OP_ADD);
    fixupbranch(c, upd, codeaddr(c));
    putcbyte(c, OP_DUP);
    (*pv.fcn)(c, PV_STORE, &pv);
    code_rvalue(c, node->u.forStatement.endExpr);
    putcbyte(c, OP_LE);
    inst = putcbyte(c, OP_BRT);
    putcword(c, nxt - inst - 1 - sizeof(VMVALUE));
}

/* code_do_while_statement - generate code for a DO WHILE statement */
static void code_do_while_statement(GenerateContext *c, ParseTreeNode *node)
{
    VMUVALUE nxt, test, inst;
    putcbyte(c, OP_BR);
    test = putcword(c, 0);
    nxt = codeaddr(c);
    code_statement_list(c, node->u.loopStatement.bodyStatements);
    fixupbranch(c, test, codeaddr(c));
    code_rvalue(c, node->u.loopStatement.test);
    inst = putcbyte(c, OP_BRT);
    putcword(c, nxt - inst - 1 - sizeof(VMVALUE));
}

/* code_do_until_statement - generate code for a DO UNTIL statement */
static void code_do_until_statement(GenerateContext *c, ParseTreeNode *node)
{
    VMUVALUE nxt, test, inst;
    putcbyte(c, OP_BR);
    test = putcword(c, 0);
    nxt = codeaddr(c);
    code_statement_list(c, node->u.loopStatement.bodyStatements);
    fixupbranch(c, test, codeaddr(c));
    code_rvalue(c, node->u.loopStatement.test);
    inst = putcbyte(c, OP_BRF);
    putcword(c, nxt - inst - 1 - sizeof(VMVALUE));
}

/* code_loop_statement - generate code for a LOOP statement */
static void code_loop_statement(GenerateContext *c, ParseTreeNode *node)
{
    VMUVALUE nxt, inst;
    nxt = codeaddr(c);
    code_statement_list(c, node->u.loopStatement.bodyStatements);
    inst = putcbyte(c, OP_BR);
    putcword(c, nxt - inst - 1 - sizeof(VMVALUE));
}

/* code_loop_while_statement - generate code for a LOOP WHILE statement */
static void code_loop_while_statement(GenerateContext *c, ParseTreeNode *node)
{
    VMUVALUE nxt, inst;
    nxt = codeaddr(c);
    code_statement_list(c, node->u.loopStatement.bodyStatements);
    code_rvalue(c, node->u.loopStatement.test);
    inst = putcbyte(c, OP_BRT);
    putcword(c, nxt - inst - 1 - sizeof(VMVALUE));
}

/* code_loop_until_statement - generate code for a LOOP UNTIL statement */
static void code_loop_until_statement(GenerateContext *c, ParseTreeNode *node)
{
    VMUVALUE nxt, inst;
    nxt = codeaddr(c);
    code_statement_list(c, node->u.loopStatement.bodyStatements);
    code_rvalue(c, node->u.loopStatement.test);
    inst = putcbyte(c, OP_BRF);
    putcword(c, nxt - inst - 1 - sizeof(VMVALUE));
}

/* code_return_statement - generate code for a RETURN statement */
static void code_return_statement(GenerateContext *c, ParseTreeNode *node)
{
    if (node->u.returnStatement.expr) {
        code_rvalue(c, node->u.returnStatement.expr);
        putcbyte(c, OP_RETURN);
    }
    else
        putcbyte(c, OP_RETURNZ);
}

/* code_statement_list - code a list of statements */
static void code_statement_list(GenerateContext *c, NodeListEntry *entry)
{
    while (entry) {
        PVAL pv;
        code_expr(c, entry->node, &pv);
        entry = entry->next;
    }
}

/* code_shortcircuit - generate code for a conjunction or disjunction of boolean expressions */
static void code_shortcircuit(GenerateContext *c, int op, ParseTreeNode *expr)
{
    NodeListEntry *entry = expr->u.exprList.exprs;
    int end = 0;

    code_rvalue(c, entry->node);
    entry = entry->next;

    do {
        putcbyte(c, op);
        end = putcword(c, end);
        code_rvalue(c, entry->node);
    } while ((entry = entry->next) != NULL);

    fixupbranch(c, end, codeaddr(c));
}

/* code_call - code a function call */
static void code_call(GenerateContext *c, ParseTreeNode *expr)
{
    NodeListEntry *arg;
    
    /* code each argument expression */
    for (arg = expr->u.functionCall.args; arg != NULL; arg = arg->next)
        code_rvalue(c, arg->node);

    /* get the value of the function */
    code_rvalue(c, expr->u.functionCall.fcn);

    /* call the function */
    putcbyte(c, OP_PUSHJ);
    if (expr->u.functionCall.argc > 0) {
        putcbyte(c, OP_CLEAN);
        putcbyte(c, expr->u.functionCall.argc);
    }
}

/* code_symbolRef - code a global reference */
static void code_symbolRef(GenerateContext *c, Symbol *sym)
{
#if 0
    VMUVALUE offset = sym->v.variable.offset;
    putcbyte(c, OP_LIT);
    if (offset == UNDEF_VALUE)
        putcword(c, AddLocalSymbolFixup(c, sym, codeaddr(c)));
    else {
        switch (sym->storageClass) {
        case SC_CONSTANT: // function text offset
        case SC_GLOBAL:
            putcword(c, sym->section ? sym->section->base + offset : offset);
            break;
        case SC_COG:
        case SC_HUB:
            putcword(c, offset);
            break;
        default:
            GenerateError(c, "unexpected storage class");
            break;
        }
    }
#endif
}

/* code_arrayref - code an array reference */
static void code_arrayref(GenerateContext *c, ParseTreeNode *expr, PVAL *pv)
{
    code_rvalue(c, expr->u.arrayRef.array);
    code_rvalue(c, expr->u.arrayRef.index);
    putcbyte(c, OP_INDEX);
    pv->fcn = code_index;
}

/* code_global - compile a global variable reference */
void code_global(GenerateContext *c, PValOp fcn, PVAL *pv)
{
    code_symbolRef(c, pv->u.sym);
    switch (fcn) {
    case PV_LOAD:
        putcbyte(c, OP_LOAD);
        break;
    case PV_STORE:
        putcbyte(c, OP_STORE);
        break;
    }
}

/* code_local - compile an local reference */
void code_local(GenerateContext *c, PValOp fcn, PVAL *pv)
{
    switch (fcn) {
    case PV_LOAD:
        putcbyte(c, OP_LREF);
        putcbyte(c, pv->u.val);
        break;
    case PV_STORE:
        putcbyte(c, OP_LSET);
        putcbyte(c, pv->u.val);
        break;
    }
}

/* code_index - compile a vector reference */
static void code_index(GenerateContext *c, PValOp fcn, PVAL *pv)
{
    switch (fcn) {
    case PV_LOAD:
        putcbyte(c, OP_LOAD);
        break;
    case PV_STORE:
        putcbyte(c, OP_STORE);
        break;
    }
}

/* codeaddr - get the current code address (actually, offset) */
VMUVALUE codeaddr(GenerateContext *c)
{
    return (VMUVALUE)(c->cptr - c->codeBuf);
}

/* putcbyte - put a code byte into the code buffer */
VMUVALUE putcbyte(GenerateContext *c, int b)
{
    VMUVALUE addr = codeaddr(c);
    if (c->cptr >= c->ctop)
        GenerateFatal(c, "Bytecode buffer overflow");
    *c->cptr++ = b;
    return addr;
}

/* putcword - put a code word into the code buffer */
VMUVALUE putcword(GenerateContext *c, VMVALUE w)
{
    VMUVALUE addr = codeaddr(c);
    uint8_t *p;
    int cnt = sizeof(VMVALUE);
    if (c->cptr + sizeof(VMVALUE) > c->ctop)
        GenerateFatal(c, "Bytecode buffer overflow");
     c->cptr += sizeof(VMVALUE);
     p = c->cptr;
     while (--cnt >= 0) {
        *--p = w;
        w >>= 8;
    }
    return addr;
}

/* rd_cword - get a code word from the code buffer */
VMVALUE rd_cword(GenerateContext *c, VMUVALUE off)
{
    int cnt = sizeof(VMVALUE);
    VMVALUE w = 0;
    while (--cnt >= 0)
        w = (w << 8) | c->codeBuf[off++];
    return w;
}

/* wr_cword - put a code word into the code buffer */
void wr_cword(GenerateContext *c, VMUVALUE off, VMVALUE w)
{
    uint8_t *p = &c->codeBuf[off] + sizeof(VMVALUE);
    int cnt = sizeof(VMVALUE);
    while (--cnt >= 0) {
        *--p = w;
        w >>= 8;
    }
}

/* fixupbranch - fixup a reference chain */
void fixupbranch(GenerateContext *c, VMUVALUE chn, VMUVALUE val)
{
    while (chn != 0) {
        VMUVALUE nxt = rd_cword(c, chn);
        VMUVALUE off = val - (chn + sizeof(VMUVALUE));
        wr_cword(c, chn, off);
        chn = nxt;
    }
}

/* AddStringRef - add a reference to a string in the string table */
static VMUVALUE AddStringRef(GenerateContext *c, String *str)
{
    return 0;
}

/* GenerateError - report a code generation error */
static void GenerateError(GenerateContext *c, const char *fmt, ...)
{
}

/* GenerateError - report a fatal code generation error */
static void GenerateFatal(GenerateContext *c, const char *fmt, ...)
{
}

/* generate.c - code generation functions
 *
 * Copyright (c) 2020 by David Michael Betz.  All rights reserved.
 *
 */

#include <string.h>
#include "compile.h"
#include "vmdebug.h"

/* code generator context */
struct GenerateContext {
    System *sys;
    uint8_t *codeBuf;
};

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
static void code_expr(GenerateContext *c, ParseTreeNode *expr, PVAL *pv);
static void code_global(GenerateContext *c, PValOp fcn, PVAL *pv);
static void code_local(GenerateContext *c, PValOp fcn, PVAL *pv);
static VMUVALUE codeaddr(GenerateContext *c);
static VMUVALUE putcbyte(GenerateContext *c, int b);
static VMUVALUE putcword(GenerateContext *c, VMVALUE w);
static VMVALUE rd_cword(GenerateContext *c, VMUVALUE off);
static void wr_cword(GenerateContext *c, VMUVALUE off, VMVALUE w);
static void fixup(GenerateContext *c, VMUVALUE chn, VMUVALUE val);
static void fixupbranch(GenerateContext *c, VMUVALUE chn, VMUVALUE val);
static VMVALUE AddSymbolRef(GenerateContext *c, Symbol *sym);
static VMUVALUE AddStringRef(GenerateContext *c, String *str);
static VMVALUE StoreVector(GenerateContext *c, const VMVALUE *buf, int size);
static VMVALUE StoreByteVector(GenerateContext *c, const uint8_t *buf, int size);
static void GenerateError(GenerateContext *c, const char *fmt, ...);
static void GenerateFatal(GenerateContext *c, const char *fmt, ...);

/* InitGenerateContext - initialize a generate context */
GenerateContext *InitGenerateContext(System *sys)
{
    GenerateContext *g;
    if (!(g = (GenerateContext *)AllocateLowMemory(sys, sizeof(GenerateContext))))
        return NULL;
    g->sys = sys;
    g->codeBuf = sys->nextLow;
    return g;
}

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
static void code_expr(GenerateContext *c, ParseTreeNode *expr, PVAL *pv)
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
        pv->u.val = expr->u.symbolRef.symbol->v.value;
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
    System *sys = c->sys;
    uint8_t *base = sys->nextLow;
    VMVALUE code;
    size_t codeSize;
    putcbyte(c, OP_FRAME);
    putcbyte(c, F_SIZE + node->u.functionDefinition.localOffset);
    code_statement_list(c, node->u.functionDefinition.bodyStatements);
    if (node->type)
        putcbyte(c, OP_RETURNZ);
    else
        putcbyte(c, OP_HALT);
    codeSize = sys->nextLow - base;
    if (!(code = StoreByteVector(c, base, codeSize)))
        GenerateError(c, "insufficient memory");
    if (node->u.functionDefinition.symbol)
        node->u.functionDefinition.symbol->v.value = code;
    DecodeFunction(code, sys->freeSpace + code, codeSize);
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
    putcbyte(c, OP_LIT);
    putcword(c, AddSymbolRef(c, sym));
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
static void code_global(GenerateContext *c, PValOp fcn, PVAL *pv)
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
static void code_local(GenerateContext *c, PValOp fcn, PVAL *pv)
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
static VMUVALUE codeaddr(GenerateContext *c)
{
    return (VMUVALUE)(c->sys->nextLow - c->codeBuf);
}

/* putcbyte - put a code byte into the code buffer */
static VMUVALUE putcbyte(GenerateContext *c, int b)
{
    System *sys = c->sys;
    VMUVALUE addr = codeaddr(c);
    if (sys->nextLow >= sys->nextHigh)
        GenerateFatal(c, "Bytecode buffer overflow");
    *sys->nextLow++ = b;
    return addr;
}

/* putcword - put a code word into the code buffer */
static VMUVALUE putcword(GenerateContext *c, VMVALUE w)
{
    System *sys = c->sys;
    VMUVALUE addr = codeaddr(c);
    uint8_t *p;
    int cnt = sizeof(VMVALUE);
    if (sys->nextLow + sizeof(VMVALUE) > sys->nextHigh)
        GenerateFatal(c, "Bytecode buffer overflow");
     sys->nextLow += sizeof(VMVALUE);
     p = sys->nextLow;
     while (--cnt >= 0) {
        *--p = w;
        w >>= 8;
    }
    return addr;
}

/* rd_cword - get a code word from the code buffer */
static VMVALUE rd_cword(GenerateContext *c, VMUVALUE off)
{
    int cnt = sizeof(VMVALUE);
    VMVALUE w = 0;
    while (--cnt >= 0)
        w = (w << 8) | c->codeBuf[off++];
    return w;
}

/* wr_cword - put a code word into the code buffer */
static void wr_cword(GenerateContext *c, VMUVALUE off, VMVALUE w)
{
    uint8_t *p = &c->codeBuf[off] + sizeof(VMVALUE);
    int cnt = sizeof(VMVALUE);
    while (--cnt >= 0) {
        *--p = w;
        w >>= 8;
    }
}

/* fixup - fixup a reference chain */
static void fixup(GenerateContext *c, VMUVALUE chn, VMUVALUE val)
{
    while (chn != 0) {
        int nxt = rd_cword(c, chn);
        wr_cword(c, chn, val);
        chn = nxt;
    }
}

/* fixupbranch - fixup a reference chain */
static void fixupbranch(GenerateContext *c, VMUVALUE chn, VMUVALUE val)
{
    while (chn != 0) {
        VMUVALUE nxt = rd_cword(c, chn);
        VMUVALUE off = val - (chn + sizeof(VMUVALUE));
        wr_cword(c, chn, off);
        chn = nxt;
    }
}

/* AddSymbolRef - add a reference to a symbol */
static VMVALUE AddSymbolRef(GenerateContext *c, Symbol *sym)
{
    if (!sym->placed) {
        sym->v.value = StoreVector(c, &sym->v.value, 1);
        sym->placed = VMTRUE;
    }
    return sym->v.value;
}

/* AddStringRef - add a reference to a string in the string table */
static VMUVALUE AddStringRef(GenerateContext *c, String *str)
{
    return (uint8_t *)str->data - c->sys->freeSpace;
}

/* StoreVector - store a VMVALUE vector */
static VMVALUE StoreVector(GenerateContext *c, const VMVALUE *buf, int size)
{
    return StoreByteVector(c, (uint8_t *)buf, size * sizeof(VMVALUE));
}

/* StoreByteVector - store a byte vector */
static VMVALUE StoreByteVector(GenerateContext *c, const uint8_t *buf, int size)
{
    System *sys = c->sys;
    uint8_t *p;
    if (!(p = AllocateHighMemory(sys, size)))
        return 0;
    memcpy(p, buf, size);
    return p - sys->freeSpace;
}

/* GenerateError - report a code generation error */
static void GenerateError(GenerateContext *c, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    VM_printf("error: ");
    VM_vprintf(fmt, ap);
    va_end(ap);
}

/* GenerateFatal - report a fatal code generation error */
static void GenerateFatal(GenerateContext *c, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    VM_printf("fatal: ");
    VM_vprintf(fmt, ap);
    va_end(ap);
}

/* parse.c - expression parser
 *
 * Copyright (c) 2020 by David Michael Betz.  All rights reserved.
 *
 */

#include <stdlib.h>
#include "compile.h"

/* local function prototypes */
static void ParseFunction(ParseContext *c);
static void ParseEndFunction(ParseContext *c);
static ParseTreeNode *StartFunction(ParseContext *c, Symbol *symbol);
static void EndFunction(ParseContext *c);
static void ParseDim(ParseContext *c);
static int ParseVariableDecl(ParseContext *c, char *name, VMVALUE *pSize);
static VMVALUE ParseScalarInitializer(ParseContext *c);
static void ParseArrayInitializers(ParseContext *c, VMVALUE size);
static void ClearArrayInitializers(ParseContext *c, VMVALUE size);
static void ParseImpliedLetOrFunctionCall(ParseContext *c);
static void ParseLet(ParseContext *c);
static void ParseIf(ParseContext *c);
static void ParseElse(ParseContext *c);
static void ParseElseIf(ParseContext *c);
static void ParseEndIf(ParseContext *c);
static void ParseFor(ParseContext *c);
static void ParseNext(ParseContext *c);
static void ParseDo(ParseContext *c);
static void ParseDoWhile(ParseContext *c);
static void ParseDoUntil(ParseContext *c);
static void ParseLoop(ParseContext *c);
static void ParseLoopWhile(ParseContext *c);
static void ParseLoopUntil(ParseContext *c);
static void ParseReturn(ParseContext *c);
static void ParsePrint(ParseContext *c);
static void ParseEnd(ParseContext *c);
static ParseTreeNode *ParseExpr2(ParseContext *c);
static ParseTreeNode *ParseExpr3(ParseContext *c);
static ParseTreeNode *ParseExpr4(ParseContext *c);
static ParseTreeNode *ParseExpr5(ParseContext *c);
static ParseTreeNode *ParseExpr6(ParseContext *c);
static ParseTreeNode *ParseExpr7(ParseContext *c);
static ParseTreeNode *ParseExpr8(ParseContext *c);
static ParseTreeNode *ParseExpr9(ParseContext *c);
static ParseTreeNode *ParseExpr10(ParseContext *c);
static ParseTreeNode *ParseExpr11(ParseContext *c);
static ParseTreeNode *ParseSimplePrimary(ParseContext *c);
static ParseTreeNode *ParseArrayReference(ParseContext *c, ParseTreeNode *arrayNode);
static ParseTreeNode *ParseCall(ParseContext *c, ParseTreeNode *functionNode);
static ParseTreeNode *MakeUnaryOpNode(ParseContext *c, int op, ParseTreeNode *expr);
static ParseTreeNode *MakeBinaryOpNode(ParseContext *c, int op, ParseTreeNode *left, ParseTreeNode *right);
static ParseTreeNode *NewParseTreeNode(ParseContext *c, int type);
static void AddNodeToList(ParseContext *c, NodeListEntry ***ppNextEntry, ParseTreeNode *node);
static void PushBlock(ParseContext *c, BlockType type, ParseTreeNode *node);
static void PopBlock(ParseContext *c);
static void PrintNodeList(NodeListEntry *entry, int indent);

static void EnterBuiltInSymbols(ParseContext *c);
static void EnterBuiltInVariable(ParseContext *c, const char *name, VMVALUE addr);
static void EnterBuiltInFunction(ParseContext *c, const char *name, VMVALUE addr);

/* InitParseContext - parse a statement */
ParseContext *InitParseContext(System *sys)
{
    ParseContext *c = (ParseContext *)AllocateFreeSpace(sys, sizeof(ParseContext));
    if (c) {
        memset(c, 0, sizeof(ParseContext));
        c->sys = sys;    
    }
    return c;
}

/* EnterBuiltInSymbols - enter the built-in symbols */
static void EnterBuiltInSymbols(ParseContext *c)
{
    EnterBuiltInFunction(c, "printInt",  (VMVALUE)0);
    EnterBuiltInFunction(c, "printStr",  (VMVALUE)0);
    EnterBuiltInFunction(c, "printNL",  (VMVALUE)0);
}

/* EnterBuiltInVariable - enter a built-in variable */
static void EnterBuiltInVariable(ParseContext *c, const char *name, VMVALUE addr)
{
    Symbol *sym;
    sym = AddGlobal(c, name, SC_VARIABLE,  addr);
}

/* EnterBuiltInFunction - enter a built-in function */
static void EnterBuiltInFunction(ParseContext *c, const char *name, VMVALUE addr)
{
    Symbol *sym;
    sym = AddGlobal(c, name, SC_CONSTANT,  addr);
}

/* Compile - parse a program */
void Compile(ParseContext *c)
{
    System *sys = c->sys;
    
    /* setup an error target */
    if (setjmp(c->sys->errorTarget) != 0)
        return;

    /* use the rest of the system free space for the compiler heap */
    c->nextLocal = sys->freeNext;
    c->nextGlobal = sys->freeTop;
    c->heapSize = sys->freeTop - sys->freeNext;
    c->maxHeapUsed = 0;
    
    /* initialize the string table */
    c->strings = NULL;

    /* initialize block nesting table */
    c->btop = (Block *)((char *)c->blockBuf + sizeof(c->blockBuf));
    c->bptr = &c->blockBuf[0] - 1;

    /* create the main function */
    c->mainFunction = StartFunction(c, NULL);
    
    /* initialize the global symbol table */
    InitSymbolTable(&c->globals);
    
    /* enter the built-in symbols */
    EnterBuiltInSymbols(c);

    /* initialize scanner */
    InitScan(c);
    
    /* parse the program */
    while (GetLine(c->sys)) {
        int tkn;
        if ((tkn = GetToken(c)) != T_EOL)
            ParseStatement(c, tkn);
    }
    
    DumpSymbols(&c->globals, "Globals");
    
    PrintNode(c->mainFunction, 0);
    
    {
        GenerateContext *g = InitGenerateContext(c->nextLocal, c->nextGlobal - c->nextLocal);
        if (g) {
            Generate(g, c->mainFunction);
        }
    }
}

/* ParseStatement - parse a statement */
void ParseStatement(ParseContext *c, int tkn)
{
    /* dispatch on the statement keyword */
    switch (tkn) {
    case T_REM:
        /* just a comment so ignore the rest of the line */
        break;
    case T_FUNCTION:
        ParseFunction(c);
        break;
    case T_END_FUNCTION:
        ParseEndFunction(c);
        break;
    case T_DIM:
        ParseDim(c);
        break;
    case T_LET:
        ParseLet(c);
        break;
    case T_IF:
        ParseIf(c);
        break;
    case T_ELSE:
        ParseElse(c);
        break;
    case T_ELSE_IF:
        ParseElseIf(c);
        break;
    case T_END_IF:
        ParseEndIf(c);
        break;
    case T_FOR:
        ParseFor(c);
        break;
    case T_NEXT:
        ParseNext(c);
        break;
    case T_DO:
        ParseDo(c);
        break;
    case T_DO_WHILE:
        ParseDoWhile(c);
        break;
    case T_DO_UNTIL:
        ParseDoUntil(c);
        break;
    case T_LOOP:
        ParseLoop(c);
        break;
    case T_LOOP_WHILE:
        ParseLoopWhile(c);
        break;
    case T_LOOP_UNTIL:
        ParseLoopUntil(c);
        break;
    case T_RETURN:
        ParseReturn(c);
        break;
    case T_PRINT:
        ParsePrint(c);
        break;
    case T_END:
        ParseEnd(c);
        break;
    default:
        SaveToken(c, tkn);
        ParseImpliedLetOrFunctionCall(c);
        break;
    }
}

/* ParseFunction - parse the 'FUNCTION' statement */
static void ParseFunction(ParseContext *c)
{
    ParseTreeNode *function;
    Symbol *symbol;
    int tkn;

    if (c->currentFunction != c->mainFunction)
        ParseError(c, "nested function definitions not supported");
    else if (c->bptr->type != BLOCK_FUNCTION)
        ParseError(c, "function definition not allowed in another block");
        
    /* get the function name */
    FRequire(c, T_IDENTIFIER);

    /* enter the function name in the global symbol table */
    symbol = AddGlobal(c, c->token, SC_CONSTANT, 0);
    
    /* create the function node */
    function = StartFunction(c, symbol);

    /* get the argument list */
    if ((tkn = GetToken(c)) == '(') {
        if ((tkn = GetToken(c)) != ')') {
            SaveToken(c, tkn);
            do {
                FRequire(c, T_IDENTIFIER);
                AddArgument(c, c->token, SC_VARIABLE, function->u.functionDefinition.argumentOffset);
                ++function->u.functionDefinition.argumentOffset;
            } while ((tkn = GetToken(c)) == ',');
        }
        Require(c, tkn, ')');
    }
    else
        SaveToken(c, tkn);

    FRequire(c, T_EOL);
}

/* ParseEndFunction - parse the 'END FUNCTION' statement */
static void ParseEndFunction(ParseContext *c)
{
    if (c->currentFunction == c->mainFunction)
        ParseError(c, "not in a function definition");
    else if (c->bptr->type != BLOCK_FUNCTION)
        ParseError(c, "function definition not complete");
    PrintNode(c->currentFunction, 0);
    EndFunction(c);
}

/* StartFunction - start a function definition */
static ParseTreeNode *StartFunction(ParseContext *c, Symbol *symbol)
{
    ParseTreeNode *node = NewParseTreeNode(c, NodeTypeFunctionDefinition);
    node->u.functionDefinition.symbol = symbol;
    InitSymbolTable(&node->u.functionDefinition.arguments);
    InitSymbolTable(&node->u.functionDefinition.locals);
    PushBlock(c, BLOCK_FUNCTION, node);
    c->bptr->pNextStatement = &node->u.functionDefinition.bodyStatements;
    c->currentFunction = node;
    return node;
}

/* EndFunction - end a function definition */
static void EndFunction(ParseContext *c)
{
    PopBlock(c);
    c->currentFunction = c->mainFunction;
}

/* ParseDim - parse the 'DIM' statement */
static void ParseDim(ParseContext *c)
{
    char name[MAXTOKEN];
    VMVALUE value, size = 0;
    int isArray;
    int tkn;

    /* parse variable declarations */
    do {

        /* get variable name */
        isArray = ParseVariableDecl(c, name, &size);

        /* add to the global symbol table if outside a function definition */
        if (c->currentFunction == c->mainFunction) {
            Symbol *sym;

#if 0
            /* check for initializers */
            if ((tkn = GetToken(c)) == '=') {
                if (isArray)
                    ParseArrayInitializers(c, size);
                else {
                    VMVALUE *dataPtr = (VMVALUE *)c->codeBuf;
                    if (dataPtr >= (VMVALUE *)c->ctop)
                        ParseError(c, "insufficient image space");
                    *dataPtr = ParseScalarInitializer(c);
                }
            }

            /* no initializers */
            else {
                ClearArrayInitializers(c, isArray ? size : 1);
                SaveToken(c, tkn);
            }
#endif

            /* allocate space for the data */
            //value = (VMVALUE)c->image->free;
            //c->image->free += size;
            
            /* add the symbol to the global symbol table */
            sym = AddGlobal(c, name, SC_VARIABLE, value);
        }

        /* otherwise, add to the local symbol table */
        else {
            if (isArray)
                ParseError(c, "local arrays are not supported");
            AddLocal(c, name, SC_VARIABLE, c->currentFunction->u.functionDefinition.localOffset);
            ++c->currentFunction->u.functionDefinition.localOffset;
        }

    } while ((tkn = GetToken(c)) == ',');

    Require(c, tkn, T_EOL);
}

/* ParseVariableDecl - parse a variable declaration */
static int ParseVariableDecl(ParseContext *c, char *name, VMVALUE *pSize)
{
    int isArray;
    int tkn;

    /* parse the variable name */
    FRequire(c, T_IDENTIFIER);
    strcpy(name, c->token);

    /* handle arrays */
    if ((tkn = GetToken(c)) == '[') {

        /* check for an array with unspecified size */
        if ((tkn = GetToken(c)) == ']')
            *pSize = 0;

        /* otherwise, parse the array size */
        else {
            ParseTreeNode *expr;

            /* put back the token */
            SaveToken(c, tkn);

            /* get the array size */
            expr = ParseExpr(c);

            /* make sure it's a constant */
            if (!IsIntegerLit(expr) || expr->u.integerLit.value <= 0)
                ParseError(c, "expecting a positive constant expression");
            *pSize = expr->u.integerLit.value;

            /* only one dimension allowed for now */
            FRequire(c, ']');
        }

        /* return an array and its size */
        isArray = VMTRUE;
        return VMTRUE;
    }

    /* not an array */
    else {
        SaveToken(c, tkn);
        isArray = VMFALSE;
        *pSize = 1;
    }

    /* return array indicator */
    return isArray;
}

/* ParseScalarInitializer - parse a scalar initializer */
static VMVALUE ParseScalarInitializer(ParseContext *c)
{
    ParseTreeNode *expr = ParseExpr(c);
    if (!IsIntegerLit(expr))
        ParseError(c, "expecting a constant expression");
    return expr->u.integerLit.value;
}

/* ParseArrayInitializers - parse array initializers */
static void ParseArrayInitializers(ParseContext *c, VMVALUE size)
{
#if 0
    VMVALUE *dataPtr = (VMVALUE *)c->codeBuf;
    VMVALUE *dataTop = (VMVALUE *)c->ctop;
    int done = VMFALSE;
    int tkn;

    FRequire(c, '{');

    /* handle each line of initializers */
    while (!done) {
        int lineDone = VMFALSE;

        /* look for the first non-blank line */
        while ((tkn = GetToken(c)) == T_EOL) {
            if (!GetLine(c->sys))
                ParseError(c, "unexpected end of file in initializers");
        }

        /* check for the end of the initializers */
        if (tkn == '}')
            break;
        SaveToken(c, tkn);

        /* handle each initializer in the current line */
        while (!lineDone) {
            VMVALUE value;

            /* get a constant expression */
            value = ParseScalarInitializer(c);

            /* check for too many initializers */
            if (--size < 0)
                ParseError(c, "too many initializers");

            /* store the initial value */
            if (dataPtr >= dataTop)
                ParseError(c, "insufficient image space");
            *dataPtr++ = value;

            switch (tkn = GetToken(c)) {
            case T_EOL:
                lineDone = VMTRUE;
                break;
            case '}':
                lineDone = VMTRUE;
                done = VMTRUE;
                break;
            case ',':
                break;
            default:
                ParseError(c, "expecting a comma, right brace or end of line");
                break;
            }

        }
    }
#endif
}

/* ClearArrayInitializers - clear the array initializers */
static void ClearArrayInitializers(ParseContext *c, VMVALUE size)
{
#if 0
    VMVALUE *dataPtr = (VMVALUE *)c->codeBuf;
    VMVALUE *dataTop = (VMVALUE *)c->ctop;
    if (dataPtr + size > dataTop)
        ParseError(c, "insufficient image space");
    memset(dataPtr, 0, size * sizeof(VMVALUE));
#endif
}

/* ParseImpliedLetOrFunctionCall - parse an implied let statement or a function call */
static void ParseImpliedLetOrFunctionCall(ParseContext *c)
{
    ParseTreeNode *node, *expr;
    int tkn;
    expr = ParsePrimary(c);
    switch (tkn = GetToken(c)) {
    case '=':
        node = NewParseTreeNode(c, NodeTypeLetStatement);
        node->u.letStatement.lvalue = expr;
        node->u.letStatement.rvalue = ParseExpr(c);
        break;
    default:
        SaveToken(c, tkn);
        node = NewParseTreeNode(c, NodeTypeCallStatement);
        node->u.callStatement.expr = expr;
        break;
    }
    AddNodeToList(c, &c->bptr->pNextStatement, node);
    FRequire(c, T_EOL);
}

/* ParseLet - parse the 'LET' statement */
static void ParseLet(ParseContext *c)
{
    ParseTreeNode *node = NewParseTreeNode(c, NodeTypeLetStatement);
    node->u.letStatement.lvalue = ParsePrimary(c);
    FRequire(c, '=');
    node->u.letStatement.rvalue = ParseExpr(c);
    AddNodeToList(c, &c->bptr->pNextStatement, node);
    FRequire(c, T_EOL);
}

/* ParseIf - parse the 'IF' statement */
static void ParseIf(ParseContext *c)
{
    ParseTreeNode *node = NewParseTreeNode(c, NodeTypeIfStatement);
    int tkn;
    node->u.ifStatement.test = ParseExpr(c);
    AddNodeToList(c, &c->bptr->pNextStatement, node);
    FRequire(c, T_THEN);
    PushBlock(c, BLOCK_IF, node);
    c->bptr->pNextStatement = &node->u.ifStatement.thenStatements;
    if ((tkn = GetToken(c)) != T_EOL) {
        ParseStatement(c, tkn);
        PopBlock(c);
    }
    Require(c, tkn, T_EOL);
}

/* ParseElseIf - parse the 'ELSE IF' statement */
static void ParseElseIf(ParseContext *c)
{
    NodeListEntry **pNext;
    ParseTreeNode *node;
    switch (c->bptr->type) {
    case BLOCK_IF:
        node = NewParseTreeNode(c, NodeTypeIfStatement);
        pNext = &c->bptr->node->u.ifStatement.elseStatements;
        AddNodeToList(c, &pNext, node);
        c->bptr->node = node;
        node->u.ifStatement.test = ParseExpr(c);
        c->bptr->pNextStatement = &node->u.ifStatement.thenStatements;
        FRequire(c, T_THEN);
        FRequire(c, T_EOL);
        break;
    default:
        ParseError(c, "ELSE IF without a matching IF");
        break;
    }
}

/* ParseElse - parse the 'ELSE' statement */
static void ParseElse(ParseContext *c)
{
    switch (c->bptr->type) {
    case BLOCK_IF:
        c->bptr->type = BLOCK_ELSE;
        c->bptr->pNextStatement = &c->bptr->node->u.ifStatement.elseStatements;
        FRequire(c, T_EOL);
        break;
    default:
        ParseError(c, "ELSE without a matching IF");
        break;
    }
}

/* ParseEndIf - parse the 'END IF' statement */
static void ParseEndIf(ParseContext *c)
{
    switch (c->bptr->type) {
    case BLOCK_IF:
    case BLOCK_ELSE:
        PopBlock(c);
        break;
    default:
        ParseError(c, "END IF without a matching IF/ELSE IF/ELSE");
        break;
    }
    FRequire(c, T_EOL);
}

/* ParseFor - parse the 'FOR' statement */
static void ParseFor(ParseContext *c)
{
    ParseTreeNode *node = NewParseTreeNode(c, NodeTypeForStatement);
    int tkn;

    AddNodeToList(c, &c->bptr->pNextStatement, node);

    PushBlock(c, BLOCK_FOR, node);
    c->bptr->pNextStatement = &node->u.forStatement.bodyStatements;

    /* get the control variable */
    FRequire(c, T_IDENTIFIER);
    node->u.forStatement.var = GetSymbolRef(c, c->token);

    /* parse the starting value expression */
    FRequire(c, '=');
    node->u.forStatement.startExpr = ParseExpr(c);

    /* parse the TO expression and generate the loop termination test */
    FRequire(c, T_TO);
    node->u.forStatement.endExpr = ParseExpr(c);

    /* get the STEP expression */
    if ((tkn = GetToken(c)) == T_STEP) {
        node->u.forStatement.stepExpr = ParseExpr(c);
        tkn = GetToken(c);
    }
    Require(c, tkn, T_EOL);
}

/* ParseNext - parse the 'NEXT' statement */
static void ParseNext(ParseContext *c)
{
    switch (c->bptr->type) {
    case BLOCK_FOR:
        FRequire(c, T_IDENTIFIER);
        //if (GetSymbolRef(c, c->token) != c->bptr->node->u.forStatement.var)
        //    ParseError(c, "wrong variable in FOR");
        PopBlock(c);
        break;
    default:
        ParseError(c, "NEXT without a matching FOR");
        break;
    }
    FRequire(c, T_EOL);
}

/* ParseDo - parse the 'DO' statement */
static void ParseDo(ParseContext *c)
{
    ParseTreeNode *node = NewParseTreeNode(c, NodeTypeLoopStatement);
    node->u.loopStatement.test = NULL;
    AddNodeToList(c, &c->bptr->pNextStatement, node);
    PushBlock(c, BLOCK_DO, node);
    c->bptr->pNextStatement = &node->u.loopStatement.bodyStatements;
    FRequire(c, T_EOL);
}

/* ParseDoWhile - parse the 'DO WHILE' statement */
static void ParseDoWhile(ParseContext *c)
{
    ParseTreeNode *node = NewParseTreeNode(c, NodeTypeDoWhileStatement);
    node->u.loopStatement.test = ParseExpr(c);
    AddNodeToList(c, &c->bptr->pNextStatement, node);
    PushBlock(c, BLOCK_DO, node);
    c->bptr->pNextStatement = &node->u.loopStatement.bodyStatements;
    FRequire(c, T_EOL);
}

/* ParseDoUntil - parse the 'DO UNTIL' statement */
static void ParseDoUntil(ParseContext *c)
{
    ParseTreeNode *node = NewParseTreeNode(c, NodeTypeDoUntilStatement);
    node->u.loopStatement.test = ParseExpr(c);
    AddNodeToList(c, &c->bptr->pNextStatement, node);
    PushBlock(c, BLOCK_DO, node);
    c->bptr->pNextStatement = &node->u.loopStatement.bodyStatements;
    FRequire(c, T_EOL);
}

/* ParseLoop - parse the 'LOOP' statement */
static void ParseLoop(ParseContext *c)
{
    switch (c->bptr->type) {
    case BLOCK_DO:
        PopBlock(c);
        break;
    default:
        ParseError(c, "LOOP without a matching DO");
        break;
    }
    FRequire(c, T_EOL);
}

/* ParseLoopWhile - parse the 'LOOP WHILE' statement */
static void ParseLoopWhile(ParseContext *c)
{
    switch (c->bptr->type) {
    case BLOCK_DO:
        if (c->bptr->node->nodeType != NodeTypeLoopStatement)
            ParseError(c, "can't have a test at both the top and bottom of a loop");
        c->bptr->node->nodeType = NodeTypeLoopWhileStatement;
        c->bptr->node->u.loopStatement.test = ParseExpr(c);
        PopBlock(c);
        break;
    default:
        ParseError(c, "LOOP without a matching DO");
        break;
    }
    FRequire(c, T_EOL);
}

/* ParseLoopUntil - parse the 'LOOP UNTIL' statement */
static void ParseLoopUntil(ParseContext *c)
{
    switch (c->bptr->type) {
    case BLOCK_DO:
        if (c->bptr->node->nodeType != NodeTypeLoopStatement)
            ParseError(c, "can't have a test at both the top and bottom of a loop");
        c->bptr->node->nodeType = NodeTypeLoopUntilStatement;
        c->bptr->node->u.loopStatement.test = ParseExpr(c);
        PopBlock(c);
        break;
    default:
        ParseError(c, "LOOP without a matching DO");
        break;
    }
    FRequire(c, T_EOL);
}

/* ParseReturn - parse the 'RETURN' statement */
static void ParseReturn(ParseContext *c)
{
    ParseTreeNode *node = NewParseTreeNode(c, NodeTypeReturnStatement);
    int tkn;
            
    /* return with no value returns zero */
    if ((tkn = GetToken(c)) == T_EOL)
        node->u.returnStatement.expr = NULL;
        
    /* handle return with a value */
    else {
        SaveToken(c, tkn);
        node->u.returnStatement.expr = ParseExpr(c);
        FRequire(c, T_EOL);
    }
    
    /* add the statement to the current function */
    AddNodeToList(c, &c->bptr->pNextStatement, node);
}

/* BuildHandlerFunctionCall - compile a call to a runtime print function */
static ParseTreeNode *BuildHandlerCall(ParseContext *c, char *name, ParseTreeNode *devExpr, ParseTreeNode *expr)
{
    ParseTreeNode *functionNode, *callNode, *node;
    NodeListEntry *actual;
    Type *functionType;
    Symbol *symbol;

    if (!(symbol = FindGlobal(c, name)))
        ParseError(c, "print helper not defined: %s", name);
        
    functionNode = NewParseTreeNode(c, NodeTypeGlobalRef);
    functionNode->u.symbolRef.symbol = symbol;
    
    /* intialize the function call node */
    callNode = NewParseTreeNode(c, NodeTypeFunctionCall);
    callNode->u.functionCall.fcn = functionNode;
    callNode->u.functionCall.args = NULL;
    
    actual = (NodeListEntry *)LocalAlloc(c, sizeof(NodeListEntry));
    actual->node = devExpr;
    actual->next = callNode->u.functionCall.args;
    callNode->u.functionCall.args = actual;
    ++callNode->u.functionCall.argc;

    if (expr) {
        actual = (NodeListEntry *)LocalAlloc(c, sizeof(NodeListEntry));
        actual->node = expr;
        actual->next = callNode->u.functionCall.args;
        callNode->u.functionCall.args = actual;
        ++callNode->u.functionCall.argc;
    }

    /* build the function call statement */
    node = NewParseTreeNode(c, NodeTypeCallStatement);
    node->u.callStatement.expr = callNode;
    
    /* return the function call statement */
    return node;
}


/* ParsePrint - handle the 'PRINT' statement */
static void ParsePrint(ParseContext *c)
{
    ParseTreeNode *devExpr, *expr;
    int needNewline = VMTRUE;
    int tkn;

    /* check for file output */
    if ((tkn = GetToken(c)) == '#') {
        devExpr = ParseExpr(c);
        FRequire(c, ',');
    }
    
    /* handle terminal output */
    else {
        SaveToken(c, tkn);
        devExpr = NewParseTreeNode(c, NodeTypeIntegerLit);
        devExpr->u.integerLit.value = 0;
    }
    
    while ((tkn = GetToken(c)) != T_EOL) {
        switch (tkn) {
        case ',':
            needNewline = VMFALSE;
            AddNodeToList(c, &c->bptr->pNextStatement, BuildHandlerCall(c, "printTab", devExpr, NULL));
            break;
        case ';':
            needNewline = VMFALSE;
            break;
        case T_STRING:
            needNewline = VMTRUE;
            expr = NewParseTreeNode(c, NodeTypeStringLit);
            expr->u.stringLit.string = AddString(c, c->token);
            AddNodeToList(c, &c->bptr->pNextStatement, BuildHandlerCall(c, "printStr", devExpr, expr));
            break;
        default:
            needNewline = VMTRUE;
            SaveToken(c, tkn);
            expr = ParseExpr(c);
            AddNodeToList(c, &c->bptr->pNextStatement, BuildHandlerCall(c, "printInt", devExpr, expr));
            break;
        }
    }

    if (needNewline)
        AddNodeToList(c, &c->bptr->pNextStatement, BuildHandlerCall(c, "printNL", devExpr, NULL));
}

/* ParseEnd - parse the 'END' statement */
static void ParseEnd(ParseContext *c)
{
    ParseTreeNode *node = NewParseTreeNode(c, NodeTypeEndStatement);
    AddNodeToList(c, &c->bptr->pNextStatement, node);
    FRequire(c, T_EOL);
}

/* ParseExpr - handle the OR operator */
ParseTreeNode *ParseExpr(ParseContext *c)
{
    ParseTreeNode *node;
    int tkn;
    node = ParseExpr2(c);
    if ((tkn = GetToken(c)) == T_OR) {
        ParseTreeNode *node2 = NewParseTreeNode(c, NodeTypeDisjunction);
        NodeListEntry *entry, **pLast;
        node2->u.exprList.exprs = entry = (NodeListEntry *)LocalAlloc(c, sizeof(NodeListEntry));
        entry->node = node;
        entry->next = NULL;
        pLast = &entry->next;
        do {
            entry = (NodeListEntry *)LocalAlloc(c, sizeof(NodeListEntry));
            entry->node = ParseExpr2(c);
            entry->next = NULL;
            *pLast = entry;
            pLast = &entry->next;
        } while ((tkn = GetToken(c)) == T_OR);
        node = node2;
    }
    SaveToken(c, tkn);
    return node;
}

/* ParseExpr2 - handle the AND operator */
static ParseTreeNode *ParseExpr2(ParseContext *c)
{
    ParseTreeNode *node;
    int tkn;
    node = ParseExpr3(c);
    if ((tkn = GetToken(c)) == T_AND) {
        ParseTreeNode *node2 = NewParseTreeNode(c, NodeTypeConjunction);
        NodeListEntry *entry, **pLast;
        node2->u.exprList.exprs = entry = (NodeListEntry *)LocalAlloc(c, sizeof(NodeListEntry));
        entry->node = node;
        entry->next = NULL;
        pLast = &entry->next;
        do {
            entry = (NodeListEntry *)LocalAlloc(c, sizeof(NodeListEntry));
            entry->node = ParseExpr2(c);
            entry->next = NULL;
            *pLast = entry;
            pLast = &entry->next;
        } while ((tkn = GetToken(c)) == T_AND);
        node = node2;
    }
    SaveToken(c, tkn);
    return node;
}

/* ParseExpr3 - handle the BXOR operator */
static ParseTreeNode *ParseExpr3(ParseContext *c)
{
    ParseTreeNode *expr, *expr2;
    int tkn;
    expr = ParseExpr4(c);
    while ((tkn = GetToken(c)) == '^') {
        expr2 = ParseExpr4(c);
        if (IsIntegerLit(expr) && IsIntegerLit(expr2))
            expr->u.integerLit.value = expr->u.integerLit.value ^ expr2->u.integerLit.value;
        else
            expr = MakeBinaryOpNode(c, OP_BXOR, expr, expr2);
    }
    SaveToken(c,tkn);
    return expr;
}

/* ParseExpr4 - handle the BOR operator */
static ParseTreeNode *ParseExpr4(ParseContext *c)
{
    ParseTreeNode *expr, *expr2;
    int tkn;
    expr = ParseExpr5(c);
    while ((tkn = GetToken(c)) == '|') {
        expr2 = ParseExpr5(c);
        if (IsIntegerLit(expr) && IsIntegerLit(expr2))
            expr->u.integerLit.value = expr->u.integerLit.value | expr2->u.integerLit.value;
        else
            expr = MakeBinaryOpNode(c, OP_BOR, expr, expr2);
    }
    SaveToken(c,tkn);
    return expr;
}

/* ParseExpr5 - handle the BAND operator */
static ParseTreeNode *ParseExpr5(ParseContext *c)
{
    ParseTreeNode *expr, *expr2;
    int tkn;
    expr = ParseExpr6(c);
    while ((tkn = GetToken(c)) == '&') {
        expr2 = ParseExpr6(c);
        if (IsIntegerLit(expr) && IsIntegerLit(expr2))
            expr->u.integerLit.value = expr->u.integerLit.value & expr2->u.integerLit.value;
        else
            expr = MakeBinaryOpNode(c, OP_BAND, expr, expr2);
    }
    SaveToken(c,tkn);
    return expr;
}

/* ParseExpr6 - handle the '=' and '<>' operators */
static ParseTreeNode *ParseExpr6(ParseContext *c)
{
    ParseTreeNode *expr, *expr2;
    int tkn;
    expr = ParseExpr7(c);
    while ((tkn = GetToken(c)) == '=' || tkn == T_NE) {
        int op;
        expr2 = ParseExpr7(c);
        switch (tkn) {
        case '=':
            op = OP_EQ;
            break;
        case T_NE:
            op = OP_NE;
            break;
        default:
            /* not reached */
            op = 0;
            break;
        }
        expr = MakeBinaryOpNode(c, op, expr, expr2);
    }
    SaveToken(c,tkn);
    return expr;
}

/* ParseExpr7 - handle the '<', '<=', '>=' and '>' operators */
static ParseTreeNode *ParseExpr7(ParseContext *c)
{
    ParseTreeNode *expr, *expr2;
    int tkn;
    expr = ParseExpr8(c);
    while ((tkn = GetToken(c)) == '<' || tkn == T_LE || tkn == T_GE || tkn == '>') {
        int op;
        expr2 = ParseExpr8(c);
        switch (tkn) {
        case '<':
            op = OP_LT;
            break;
        case T_LE:
            op = OP_LE;
            break;
        case T_GE:
            op = OP_GE;
            break;
        case '>':
            op = OP_GT;
            break;
        default:
            /* not reached */
            op = 0;
            break;
        }
        expr = MakeBinaryOpNode(c, op, expr, expr2);
    }
    SaveToken(c,tkn);
    return expr;
}

/* ParseExpr8 - handle the '<<' and '>>' operators */
static ParseTreeNode *ParseExpr8(ParseContext *c)
{
    ParseTreeNode *expr, *expr2;
    int tkn;
    expr = ParseExpr9(c);
    while ((tkn = GetToken(c)) == T_SHL || tkn == T_SHR) {
        expr2 = ParseExpr9(c);
        if (IsIntegerLit(expr) && IsIntegerLit(expr2)) {
            switch (tkn) {
            case T_SHL:
                expr->u.integerLit.value = expr->u.integerLit.value << expr2->u.integerLit.value;
                break;
            case T_SHR:
                expr->u.integerLit.value = expr->u.integerLit.value >> expr2->u.integerLit.value;
                break;
            default:
                /* not reached */
                break;
            }
        }
        else {
            int op;
            switch (tkn) {
            case T_SHL:
                op = OP_SHL;
                break;
            case T_SHR:
                op = OP_SHR;
                break;
            default:
                /* not reached */
                op = 0;
                break;
            }
            expr = MakeBinaryOpNode(c, op, expr, expr2);
        }
    }
    SaveToken(c,tkn);
    return expr;
}

/* ParseExpr9 - handle the '+' and '-' operators */
static ParseTreeNode *ParseExpr9(ParseContext *c)
{
    ParseTreeNode *expr, *expr2;
    int tkn;
    expr = ParseExpr10(c);
    while ((tkn = GetToken(c)) == '+' || tkn == '-') {
        expr2 = ParseExpr10(c);
        if (IsIntegerLit(expr) && IsIntegerLit(expr2)) {
            switch (tkn) {
            case '+':
                expr->u.integerLit.value = expr->u.integerLit.value + expr2->u.integerLit.value;
                break;
            case '-':
                expr->u.integerLit.value = expr->u.integerLit.value - expr2->u.integerLit.value;
                break;
            default:
                /* not reached */
                break;
            }
        }
        else {
            int op;
            switch (tkn) {
            case '+':
                op = OP_ADD;
                break;
            case '-':
                op = OP_SUB;
                break;
            default:
                /* not reached */
                op = 0;
                break;
            }
            expr = MakeBinaryOpNode(c, op, expr, expr2);
        }
    }
    SaveToken(c, tkn);
    return expr;
}

/* ParseExpr10 - handle the '*', '/' and MOD operators */
static ParseTreeNode *ParseExpr10(ParseContext *c)
{
    ParseTreeNode *node, *node2;
    int tkn;
    node = ParseExpr11(c);
    while ((tkn = GetToken(c)) == '*' || tkn == '/' || tkn == T_MOD) {
        node2 = ParseExpr11(c);
        if (IsIntegerLit(node) && IsIntegerLit(node2)) {
            switch (tkn) {
            case '*':
                node->u.integerLit.value = node->u.integerLit.value * node2->u.integerLit.value;
                break;
            case '/':
                if (node2->u.integerLit.value == 0)
                    ParseError(c, "division by zero in constant expression");
                node->u.integerLit.value = node->u.integerLit.value / node2->u.integerLit.value;
                break;
            case T_MOD:
                if (node2->u.integerLit.value == 0)
                    ParseError(c, "division by zero in constant expression");
                node->u.integerLit.value = node->u.integerLit.value % node2->u.integerLit.value;
                break;
            default:
                /* not reached */
                break;
            }
        }
        else {
            int op;
            switch (tkn) {
            case '*':
                op = OP_MUL;
                break;
            case '/':
                op = OP_DIV;
                break;
            case T_MOD:
                op = OP_REM;
                break;
            default:
                /* not reached */
                op = 0;
                break;
            }
            node = MakeBinaryOpNode(c, op, node, node2);
        }
    }
    SaveToken(c, tkn);
    return node;
}

/* ParseExpr11 - handle unary operators */
static ParseTreeNode *ParseExpr11(ParseContext *c)
{
    ParseTreeNode *node;
    int tkn;
    switch (tkn = GetToken(c)) {
    case '+':
        node = ParsePrimary(c);
        break;
    case '-':
        node = ParsePrimary(c);
        if (IsIntegerLit(node))
            node->u.integerLit.value = -node->u.integerLit.value;
        else
            node = MakeUnaryOpNode(c, OP_NEG, node);
        break;
    case T_NOT:
        node = ParsePrimary(c);
        if (IsIntegerLit(node))
            node->u.integerLit.value = !node->u.integerLit.value;
        else
            node = MakeUnaryOpNode(c, OP_NOT, node);
        break;
    case '~':
        node = ParsePrimary(c);
        if (IsIntegerLit(node))
            node->u.integerLit.value = ~node->u.integerLit.value;
        else
            node = MakeUnaryOpNode(c, OP_BNOT, node);
        break;
    default:
        SaveToken(c,tkn);
        node = ParsePrimary(c);
        break;
    }
    return node;
}

/* ParsePrimary - parse function calls and array references */
ParseTreeNode *ParsePrimary(ParseContext *c)
{
    ParseTreeNode *node;
    int tkn;
    node = ParseSimplePrimary(c);
    while ((tkn = GetToken(c)) == '(' || tkn == '[') {
        switch (tkn) {
        case '[':
            node = ParseArrayReference(c, node);
            break;
        case '(':
            node = ParseCall(c, node);
            break;
        }
    }
    SaveToken(c, tkn);
    return node;
}

/* ParseArrayReference - parse an array reference */
static ParseTreeNode *ParseArrayReference(ParseContext *c, ParseTreeNode *arrayNode)
{
    ParseTreeNode *node = NewParseTreeNode(c, NodeTypeArrayRef);

    /* setup the array reference */
    node->u.arrayRef.array = arrayNode;

    /* get the index expression */
    node->u.arrayRef.index = ParseExpr(c);

    /* check for the close bracket */
    FRequire(c, ']');
    return node;
}

/* ParseCall - parse a function or subroutine call */
static ParseTreeNode *ParseCall(ParseContext *c, ParseTreeNode *functionNode)
{
    ParseTreeNode *node = NewParseTreeNode(c, NodeTypeFunctionCall);
    NodeListEntry **pLast;
    int tkn;

    /* intialize the function call node */
    node->u.functionCall.fcn = functionNode;
    pLast = &node->u.functionCall.args;

    /* parse the argument list */
    if ((tkn = GetToken(c)) != ')') {
        SaveToken(c, tkn);
        do {
            NodeListEntry *actual;
            actual = (NodeListEntry *)LocalAlloc(c, sizeof(NodeListEntry));
            actual->node = ParseExpr(c);
            actual->next = NULL;
            *pLast = actual;
            pLast = &actual->next;
            ++node->u.functionCall.argc;
        } while ((tkn = GetToken(c)) == ',');
        Require(c, tkn, ')');
    }

    /* return the function call node */
    return node;
}

/* ParseSimplePrimary - parse a primary expression */
static ParseTreeNode *ParseSimplePrimary(ParseContext *c)
{
    ParseTreeNode *node;
    switch (GetToken(c)) {
    case '(':
        node = ParseExpr(c);
        FRequire(c,')');
        break;
    case T_NUMBER:
        node = NewParseTreeNode(c, NodeTypeIntegerLit);
        node->u.integerLit.value = c->value;
        break;
    case T_STRING:
        node = NewParseTreeNode(c, NodeTypeStringLit);
        node->u.stringLit.string = AddString(c, c->token);
        break;
    case T_IDENTIFIER:
        node = GetSymbolRef(c, c->token);
        break;
    default:
        ParseError(c, "Expecting a primary expression");
        node = NULL; /* not reached */
        break;
    }
    return node;
}

/* GetSymbolRef - setup a symbol reference */
ParseTreeNode *GetSymbolRef(ParseContext *c, const char *name)
{
    ParseTreeNode *node;
    Symbol *symbol;

    /* handle local variables within a function or subroutine */
    if (c->currentFunction != c->mainFunction && (symbol = FindLocal(c, name)) != NULL) {
        if (symbol->storageClass == SC_CONSTANT) {
            node = NewParseTreeNode(c, NodeTypeIntegerLit);
            node->u.integerLit.value = symbol->v.value;
        }
        else {
            node = NewParseTreeNode(c, NodeTypeLocalRef);
            node->u.symbolRef.symbol = symbol;
        }
    }

    /* handle function or subroutine arguments or the return value symbol */
    else if (c->currentFunction != c->mainFunction && (symbol = FindArgument(c, name)) != NULL) {
        node = NewParseTreeNode(c, NodeTypeArgumentRef);
        node->u.symbolRef.symbol = symbol;
    }

    /* handle global symbols */
    else if ((symbol = FindGlobal(c, c->token)) != NULL) {
        if (symbol->storageClass == SC_CONSTANT) {
            node = NewParseTreeNode(c, NodeTypeIntegerLit);
            node->u.integerLit.value = symbol->v.value;
        }
        else {
            node = NewParseTreeNode(c, NodeTypeGlobalRef);
            node->u.symbolRef.symbol = symbol;
        }
    }

    /* handle undefined symbols */
    else {
        symbol = AddGlobal(c, name, SC_VARIABLE, 0);
        node = NewParseTreeNode(c, NodeTypeGlobalRef);
        node->u.symbolRef.symbol = symbol;
    }

    /* return the symbol reference node */
    return node;
}

/* MakeUnaryOpNode - allocate a unary operation parse tree node */
static ParseTreeNode *MakeUnaryOpNode(ParseContext *c, int op, ParseTreeNode *expr)
{
    ParseTreeNode *node;
    if (IsIntegerLit(expr)) {
        node = expr;
        switch (op) {
        case OP_NEG:
            node->u.integerLit.value = -expr->u.integerLit.value;
            break;
        case OP_NOT:
            node->u.integerLit.value = !expr->u.integerLit.value;
            break;
        case OP_BNOT:
            node->u.integerLit.value = ~expr->u.integerLit.value;
            break;
        }
    }
    else if (expr->type == TYPE_INTEGER) {
        node = NewParseTreeNode(c, NodeTypeUnaryOp);
        node->type = TYPE_INTEGER;
        node->u.unaryOp.op = op;
        node->u.unaryOp.expr = expr;
    }
    else {
        ParseError(c, "Expecting a numeric expression");
        node = NULL; /* not reached */
    }
    return node;
}

/* MakeBinaryOpNode - allocate a binary operation parse tree node */
static ParseTreeNode *MakeBinaryOpNode(ParseContext *c, int op, ParseTreeNode *left, ParseTreeNode *right)
{
    ParseTreeNode *node;
    if (IsIntegerLit(left) && IsIntegerLit(right)) {
        node = left;
        switch (op) {
        case OP_BXOR:
            node->u.integerLit.value = left->u.integerLit.value ^ right->u.integerLit.value;
            break;
        case OP_BOR:
            node->u.integerLit.value = left->u.integerLit.value | right->u.integerLit.value;
            break;
        case OP_BAND:
            node->u.integerLit.value = left->u.integerLit.value & right->u.integerLit.value;
            break;
        case OP_EQ:
            node->u.integerLit.value = left->u.integerLit.value == right->u.integerLit.value;
            break;
        case OP_NE:
            node->u.integerLit.value = left->u.integerLit.value != right->u.integerLit.value;
            break;
        case OP_LT:
            node->u.integerLit.value = left->u.integerLit.value < right->u.integerLit.value;
            break;
        case OP_LE:
            node->u.integerLit.value = left->u.integerLit.value <= right->u.integerLit.value;
            break;
        case OP_GE:
            node->u.integerLit.value = left->u.integerLit.value >= right->u.integerLit.value;
            break;
        case OP_GT:
            node->u.integerLit.value = left->u.integerLit.value > right->u.integerLit.value;
            break;
        case OP_SHL:
            node->u.integerLit.value = left->u.integerLit.value << right->u.integerLit.value;
            break;
        case OP_SHR:
            node->u.integerLit.value = left->u.integerLit.value >> right->u.integerLit.value;
            break;
        case OP_ADD:
            node->u.integerLit.value = left->u.integerLit.value + right->u.integerLit.value;
            break;
        case OP_SUB:
            node->u.integerLit.value = left->u.integerLit.value - right->u.integerLit.value;
            break;
        case OP_MUL:
            node->u.integerLit.value = left->u.integerLit.value * right->u.integerLit.value;
            break;
        case OP_DIV:
            if (right->u.integerLit.value == 0)
                ParseError(c, "division by zero in constant expression");
            node->u.integerLit.value = left->u.integerLit.value / right->u.integerLit.value;
            break;
        case OP_REM:
            if (right->u.integerLit.value == 0)
                ParseError(c, "division by zero in constant expression");
            node->u.integerLit.value = left->u.integerLit.value % right->u.integerLit.value;
            break;
        default:
            goto integerOp;
        }
    }
    else if (left->type == TYPE_INTEGER && right->type == TYPE_INTEGER) {
integerOp:
        node = NewParseTreeNode(c, NodeTypeBinaryOp);
        node->type = TYPE_INTEGER;
        node->u.binaryOp.op = op;
        node->u.binaryOp.left = left;
        node->u.binaryOp.right = right;
    }
    else {
        ParseError(c, "Expecting a numeric expression");
        node = NULL; /* not reached */
    }
    return node;
}

/* PushBlock - push a block on the stack */
static void PushBlock(ParseContext *c, BlockType type, ParseTreeNode *node)
{
    if (++c->bptr >= c->btop)
        ParseError(c, "statements too deeply nested");
    c->bptr->type = type;
    c->bptr->node = node;
}

/* PopBlock - pop a block off the stack */
static void PopBlock(ParseContext *c)
{
    --c->bptr;
}

/* NewParseTreeNode - allocate a new parse tree node */
static ParseTreeNode *NewParseTreeNode(ParseContext *c, int type)
{
    ParseTreeNode *node = (ParseTreeNode *)LocalAlloc(c, sizeof(ParseTreeNode));
    memset(node, 0, sizeof(ParseTreeNode));
    node->nodeType = type;
    return node;
}

/* AddNodeToList - add a node to a parse tree node list */
static void AddNodeToList(ParseContext *c, NodeListEntry ***ppNextEntry, ParseTreeNode *node)
{
    NodeListEntry *entry = (NodeListEntry *)LocalAlloc(c, sizeof(NodeListEntry));
    entry->node = node;
    entry->next = NULL;
    **ppNextEntry = entry;
    *ppNextEntry = &entry->next;
}

/* IsIntegerLit - check to see if a node is an integer literal */
int IsIntegerLit(ParseTreeNode *node)
{
    return node->nodeType == NodeTypeIntegerLit;
}

/* AddString - add a string to the string table */
String *AddString(ParseContext *c, const char *value)
{
    String *str;
    
    /* check to see if the string is already in the table */
    for (str = c->strings; str != NULL; str = str->next)
        if (strcmp(value, str->data) == 0)
            return str;

    /* allocate the string structure */
    str = (String *)GlobalAlloc(c, sizeof(String) + strlen(value));
    memset(str, 0, sizeof(String));
    strcpy((char *)str->data, value);
    str->next = c->strings;
    c->strings = str;

    /* return the string table entry */
    return str;
}

/* GlobalAlloc - allocate memory from the global heap */
void *GlobalAlloc(ParseContext *c, size_t size)
{
    size = (size + ALIGN_MASK) & ~ALIGN_MASK;
    if (c->nextGlobal - size < c->nextLocal)
        ParseError(c, "insufficient memory");
    c->nextGlobal -= size;
    if (c->heapSize - (c->nextGlobal - c->nextLocal) > c->maxHeapUsed)
        c->maxHeapUsed = c->heapSize - (c->nextGlobal - c->nextLocal);
    return c->nextGlobal;
}

/* LocalAlloc - allocate memory from the local heap */
void *LocalAlloc(ParseContext *c, size_t size)
{
    uint8_t *p = c->nextLocal;
    size = (size + ALIGN_MASK) & ~ALIGN_MASK;
    if (p + size > c->nextGlobal)
        ParseError(c, "insufficient memory");
    c->nextLocal += size;
    if (c->heapSize - (c->nextGlobal - c->nextLocal) > c->maxHeapUsed)
        c->maxHeapUsed = c->heapSize - (c->nextGlobal - c->nextLocal);
    return p;
}

void PrintNode(ParseTreeNode *node, int indent)
{
	printf("%*s", indent, "");
    switch (node->nodeType) {
    case NodeTypeFunctionDefinition:
        printf("FunctionDefinition: %s\n", node->u.functionDefinition.symbol ? node->u.functionDefinition.symbol->name : "<main>");
        DumpSymbols(&node->u.functionDefinition.arguments, "Arguments");
        DumpSymbols(&node->u.functionDefinition.locals, "Locals");
        PrintNodeList(node->u.functionDefinition.bodyStatements, indent + 2);
        break;
    case NodeTypeLetStatement:
        printf("Let\n");
        printf("%*slvalue\n", indent + 2, "");
        PrintNode(node->u.letStatement.lvalue, indent + 4);
        printf("%*srvalue\n", indent + 2, "");
        PrintNode(node->u.letStatement.rvalue, indent + 4);
        break;
    case NodeTypeIfStatement:
        printf("If\n");
        printf("%*stest\n", indent + 2, "");
        PrintNode(node->u.ifStatement.test, indent + 4);
        printf("%*sthen\n", indent + 2, "");
        PrintNodeList(node->u.ifStatement.thenStatements, indent + 4);
        if (node->u.ifStatement.elseStatements) {
            printf("%*selse\n", indent + 2, "");
            PrintNodeList(node->u.ifStatement.elseStatements, indent + 4);
        }
        break;
    case NodeTypeForStatement:
        printf("For\n");
        printf("%*svar\n", indent + 2, "");
        PrintNode(node->u.forStatement.var, indent + 4);
        printf("%*sstart\n", indent + 2, "");
        PrintNode(node->u.forStatement.startExpr, indent + 4);
        printf("%*send\n", indent + 2, "");
        PrintNode(node->u.forStatement.endExpr, indent + 4);
        if (node->u.forStatement.stepExpr) {
            printf("%*sstep\n", indent + 2, "");
            PrintNode(node->u.forStatement.stepExpr, indent + 4);
        }
        PrintNodeList(node->u.forStatement.bodyStatements, indent + 2);
        break;
    case NodeTypeDoWhileStatement:
        printf("DoWhile\n");
        printf("%*stest\n", indent + 2, "");
        PrintNode(node->u.loopStatement.test, indent + 4);
        PrintNodeList(node->u.loopStatement.bodyStatements, indent + 2);
        break;
    case NodeTypeDoUntilStatement:
        printf("DoUntil\n");
        printf("%*stest\n", indent + 2, "");
        PrintNode(node->u.loopStatement.test, indent + 4);
        PrintNodeList(node->u.loopStatement.bodyStatements, indent + 2);
        break;
    case NodeTypeLoopStatement:
        printf("Loop\n");
        PrintNodeList(node->u.loopStatement.bodyStatements, indent + 2);
        break;
    case NodeTypeLoopWhileStatement:
        printf("LoopWhile\n");
        printf("%*stest\n", indent + 2, "");
        PrintNode(node->u.loopStatement.test, indent + 4);
        PrintNodeList(node->u.loopStatement.bodyStatements, indent + 2);
        break;
    case NodeTypeLoopUntilStatement:
        printf("LoopUntil\n");
        printf("%*stest\n", indent + 2, "");
        PrintNode(node->u.loopStatement.test, indent + 4);
        PrintNodeList(node->u.loopStatement.bodyStatements, indent + 2);
        break;
    case NodeTypeReturnStatement:
        printf("Return\n");
        if (node->u.returnStatement.expr) {
            printf("%*sexpr\n", indent + 2, "");
            PrintNode(node->u.returnStatement.expr, indent + 4);
        }
        break;
    case NodeTypeEndStatement:
        printf("End\n");
        break;
    case NodeTypeCallStatement:
        printf("CallStatement\n");
        printf("%*sexpr\n", indent + 2, "");
        PrintNode(node->u.callStatement.expr, indent + 4);
        break;
    case NodeTypeGlobalRef:
        printf("GlobalRef: %s\n", node->u.symbolRef.symbol->name);
        break;
    case NodeTypeArgumentRef:
        printf("ArgumentRef: " INT_FMT "\n", node->u.symbolRef.symbol->v.value);
        break;
    case NodeTypeLocalRef:
        printf("LocalRef: " INT_FMT "\n", node->u.symbolRef.symbol->v.value);
        break;
    case NodeTypeStringLit:
		printf("StringLit: '%s'\n",node->u.stringLit.string->data);
        break;
    case NodeTypeIntegerLit:
		printf("IntegerLit: " INT_FMT "\n",node->u.integerLit.value);
        break;
    case NodeTypeUnaryOp:
        printf("UnaryOp: %d\n", node->u.unaryOp.op);
        printf("%*sexpr\n", indent + 2, "");
        PrintNode(node->u.unaryOp.expr, indent + 4);
        break;
    case NodeTypeBinaryOp:
        printf("BinaryOp: %d\n", node->u.binaryOp.op);
        printf("%*sleft\n", indent + 2, "");
        PrintNode(node->u.binaryOp.left, indent + 4);
        printf("%*sright\n", indent + 2, "");
        PrintNode(node->u.binaryOp.right, indent + 4);
        break;
    case NodeTypeArrayRef:
        printf("ArrayRef\n");
        printf("%*sarray\n", indent + 2, "");
        PrintNode(node->u.arrayRef.array, indent + 4);
        printf("%*sindex\n", indent + 2, "");
        PrintNode(node->u.arrayRef.index, indent + 4);
        break;
    case NodeTypeFunctionCall:
        printf("FunctionCall: %d\n", node->u.functionCall.argc);
        printf("%*sfcn\n", indent + 2, "");
        PrintNode(node->u.functionCall.fcn, indent + 4);
        PrintNodeList(node->u.functionCall.args, indent + 2);
        break;
    case NodeTypeDisjunction:
        printf("Disjunction\n");
        PrintNodeList(node->u.exprList.exprs, indent + 2);
        break;
    case NodeTypeConjunction:
        printf("Conjunction\n");
        PrintNodeList(node->u.exprList.exprs, indent + 2);
        break;
    default:
        printf("<unknown node type: %d>\n", node->nodeType);
        break;
    }
}

static void PrintNodeList(NodeListEntry *entry, int indent)
{
    while (entry != NULL) {
        PrintNode(entry->node, indent);
        entry = entry->next;
    }
}


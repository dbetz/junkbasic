/* parse.c - expression parser
 *
 * Copyright (c) 2020 by David Michael Betz.  All rights reserved.
 *
 */

#include <stdlib.h>
#include "compile.h"

/* local variables */
static char *storageClassNames[] = {
    "unknown",
    "constant",
    "variable",
    "function"
};

static char *typeNames[] = {
    "unknown",
    "integer",
    "byte",
    "string",
    "array",
    "struct",
    "pointer",
    "function",
};

/* local function prototypes */
static void PrintNodeList(NodeListEntry *entry, int indent);
static char *StorageClassName(StorageClass storageClass);
static char *TypeName(TypeID typeID);

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
    case NodeTypeAsmStatement:
        printf("Asm\n");
        break;
    case NodeTypeCallStatement:
        printf("CallStatement\n");
        printf("%*sexpr\n", indent + 2, "");
        PrintNode(node->u.callStatement.expr, indent + 4);
        break;
    case NodeTypeGlobalRef:
        printf("GlobalRef: %s\n", node->u.symbolRef.symbol->name);
        printf("%*sstorageClass %s\n", indent + 2, "", StorageClassName(node->u.symbolRef.symbol->storageClass));
        if (node->u.symbolRef.symbol->type)
            printf("%*stypeID %s\n", indent + 2, "", TypeName(node->u.symbolRef.symbol->type->id));
        else
            printf("%*stypeID <none>\n", indent + 2, "");
        break;
    case NodeTypeArgumentRef:
        printf("ArgumentRef: %s (" INT_FMT ")\n", node->u.symbolRef.symbol->name, node->u.symbolRef.symbol->value);
        break;
    case NodeTypeLocalRef:
        printf("LocalRef: %s (" INT_FMT ")\n", node->u.symbolRef.symbol->name, node->u.symbolRef.symbol->value);
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

static char *StorageClassName(StorageClass storageClass)
{
    return storageClass < _SC_MAX ? storageClassNames[storageClass] : "<invalid>";
}

static char *TypeName(TypeID typeID)
{
    return typeID < _TYPE_MAX ? typeNames[typeID] : "<invalid>";
}



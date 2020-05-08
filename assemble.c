/* compile.c - compiler
 *
 * Copyright (c) 2020 by David Michael Betz.  All rights reserved.
 *
 */

#include <string.h>
#include <ctype.h>
#include "compile.h"
#include "vmdebug.h"

static void Assemble(ParseContext *c, char *name);

/* ParseAsm - parse the 'ASM ... END ASM' statement */
void ParseAsm(ParseContext *c)
{
    ParseTreeNode *node = NewParseTreeNode(c, NodeTypeAsmStatement);
    System *sys = c->sys;
    uint8_t *start = sys->nextLow;
    int length;
    int tkn;
    
    /* check for the end of the 'ASM' statement */
    FRequire(c, T_EOL);
    
    /* parse each assembly instruction */
    for (;;) {
    
        /* get the next line */
        if (!ParseGetLine(c))
            ParseError(c, "unexpected end of file in ASM statement");
        
        /* check for the end of the assembly instructions */
        if ((tkn = GetToken(c)) == T_END_ASM)
            break;
            
        /* check for an empty statement */
        else if (tkn == T_EOL)
            continue;
            
        /* check for an opcode name */
        else if (tkn != T_IDENTIFIER)
            ParseError(c, "expected an assembly instruction opcode");
            
        /* assemble a single instruction */
        Assemble(c, c->token);
    }
    
    /* store the code */
    length = sys->nextLow - start;
    node->u.asmStatement.code = start;
    node->u.asmStatement.length = length;
    AddNodeToList(c, &c->bptr->pNextStatement, node);
    
    /* check for the end of the 'END ASM' statement */
    FRequire(c, T_EOL);
}

/* Assemble - assemble a single line */
static void Assemble(ParseContext *c, char *name)
{
    GenerateContext *g = c->g;
    OTDEF *def;
    
    /* lookup the opcode */
    for (def = OpcodeTable; def->name != NULL; ++def)
        if (strcasecmp(name, def->name) == 0) {
            putcbyte(g, def->code);
            switch (def->fmt) {
            case FMT_NONE:
                break;
            case FMT_BYTE:
            case FMT_SBYTE:
                putcbyte(g, ParseIntegerConstant(c));
                break;
            case FMT_WORD:
                putcword(g, ParseIntegerConstant(c));
                break;
            case FMT_NATIVE:
                putcword(g, ParseIntegerConstant(c));
                break;
            default:
                ParseError(c, "instruction not currently supported");
                break;
            }
            FRequire(c, T_EOL);
            return;
        }
        
    ParseError(c, "undefined opcode");
}

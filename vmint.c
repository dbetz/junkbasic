/* vmint.c - bytecode interpreter for a simple virtual machine
 *
 * Copyright (c) 2020 by David Michael Betz.  All rights reserved.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include "image.h"
#include "vmdebug.h"
#include "vmint.h"
#include "system.h"

/* prototypes for local functions */
static void DoTrap(Interpreter *i, int op);

/* InitInterpreter - initialize the interpreter */
Interpreter *InitInterpreter(System *sys, uint8_t *base, int stackSize)
{
    Interpreter *i;
    
    if (!(i = (Interpreter *)AllocateLowMemory(sys, sizeof(Interpreter))))
        return NULL;
        
    if (!(i->stack = (VMVALUE *)AllocateLowMemory(sys, stackSize * sizeof(VMVALUE))))
        return NULL;
        
    i->base = base;
    i->stackTop = i->stack + stackSize;
    
    return i;
}

/* Execute - execute the main code */
int Execute(Interpreter *i, VMVALUE mainCode)
{
    VMVALUE tmp;
    int8_t tmpb;
    int cnt;

    /* initialize */    
    i->pc = i->base + mainCode;
    i->sp = i->fp = i->stackTop;

    if (setjmp(i->errorTarget))
        return VMFALSE;

    for (;;) {
#if 0
        ShowStack(i);
        DecodeInstruction(i->pc - i->base, i->pc);
#endif
        switch (VMCODEBYTE(i->pc++)) {
        case OP_HALT:
            return VMTRUE;
        case OP_BRT:
            for (tmp = 0, cnt = sizeof(VMUVALUE); --cnt >= 0; )
                tmp = (tmp << 8) | VMCODEBYTE(i->pc++);
            if (i->tos)
                i->pc += tmp;
            i->tos = Pop(i);
            break;
        case OP_BRTSC:
            for (tmp = 0, cnt = sizeof(VMUVALUE); --cnt >= 0; )
                tmp = (tmp << 8) | VMCODEBYTE(i->pc++);
            if (i->tos)
                i->pc += tmp;
            else
                i->tos = Pop(i);
            break;
        case OP_BRF:
            for (tmp = 0, cnt = sizeof(VMUVALUE); --cnt >= 0; )
                tmp = (tmp << 8) | VMCODEBYTE(i->pc++);
            if (!i->tos)
                i->pc += tmp;
            i->tos = Pop(i);
            break;
        case OP_BRFSC:
            for (tmp = 0, cnt = sizeof(VMUVALUE); --cnt >= 0; )
                tmp = (tmp << 8) | VMCODEBYTE(i->pc++);
            if (!i->tos)
                i->pc += tmp;
            else
                i->tos = Pop(i);
            break;
        case OP_BR:
            for (tmp = 0, cnt = sizeof(VMUVALUE); --cnt >= 0; )
                tmp = (tmp << 8) | VMCODEBYTE(i->pc++);
            i->pc += tmp;
            break;
        case OP_NOT:
            i->tos = (i->tos ? VMFALSE : VMTRUE);
            break;
        case OP_NEG:
            i->tos = -i->tos;
            break;
        case OP_ADD:
            tmp = Pop(i);
            i->tos = tmp + i->tos;
            break;
        case OP_SUB:
            tmp = Pop(i);
            i->tos = tmp - i->tos;
            break;
        case OP_MUL:
            tmp = Pop(i);
            i->tos = tmp * i->tos;
            break;
        case OP_DIV:
            tmp = Pop(i);
            i->tos = (i->tos == 0 ? 0 : tmp / i->tos);
            break;
        case OP_REM:
            tmp = Pop(i);
            i->tos = (i->tos == 0 ? 0 : tmp % i->tos);
            break;
        case OP_BNOT:
            i->tos = ~i->tos;
            break;
        case OP_BAND:
            tmp = Pop(i);
            i->tos = tmp & i->tos;
            break;
        case OP_BOR:
            tmp = Pop(i);
            i->tos = tmp | i->tos;
            break;
        case OP_BXOR:
            tmp = Pop(i);
            i->tos = tmp ^ i->tos;
            break;
        case OP_SHL:
            tmp = Pop(i);
            i->tos = tmp << i->tos;
            break;
        case OP_SHR:
            tmp = Pop(i);
            i->tos = tmp >> i->tos;
            break;
        case OP_LT:
            tmp = Pop(i);
            i->tos = (tmp < i->tos ? VMTRUE : VMFALSE);
            break;
        case OP_LE:
            tmp = Pop(i);
            i->tos = (tmp <= i->tos ? VMTRUE : VMFALSE);
            break;
        case OP_EQ:
            tmp = Pop(i);
            i->tos = (tmp == i->tos ? VMTRUE : VMFALSE);
            break;
        case OP_NE:
            tmp = Pop(i);
            i->tos = (tmp != i->tos ? VMTRUE : VMFALSE);
            break;
        case OP_GE:
            tmp = Pop(i);
            i->tos = (tmp >= i->tos ? VMTRUE : VMFALSE);
            break;
        case OP_GT:
            tmp = Pop(i);
            i->tos = (tmp > i->tos ? VMTRUE : VMFALSE);
            break;
        case OP_LIT:
            for (tmp = 0, cnt = sizeof(VMUVALUE); --cnt >= 0; )
                tmp = (tmp << 8) | VMCODEBYTE(i->pc++);
            CPush(i, i->tos);
            i->tos = tmp;
            break;
        case OP_SLIT:
            tmpb = (int8_t)VMCODEBYTE(i->pc++);
            CPush(i, i->tos);
            i->tos = tmpb;
            break;
        case OP_LOAD:
            i->tos = *(VMVALUE *)(i->base + i->tos);
            break;
        case OP_LOADB:
            i->tos = *(i->base + i->tos);
            break;
        case OP_STORE:
            tmp = Pop(i);
            *(VMVALUE *)(i->base + i->tos) = tmp;
            i->tos = Pop(i);
            break;
        case OP_STOREB:
            tmp = Pop(i);
            *(i->base + i->tos) = tmp;
            i->tos = Pop(i);
            break;
        case OP_LREF:
            tmpb = (int8_t)VMCODEBYTE(i->pc++);
            CPush(i, i->tos);
            i->tos = i->fp[(int)tmpb];
            break;
        case OP_LSET:
            tmpb = (int8_t)VMCODEBYTE(i->pc++);
            i->fp[(int)tmpb] = i->tos;
            i->tos = Pop(i);
            break;
        case OP_INDEX:
            tmp = Pop(i);
            i->tos = tmp + i->tos * sizeof (VMVALUE);
            break;
        case OP_CALL:
            tmp = (VMVALUE)(i->pc - (uint8_t *)i->base);
            i->pc = i->base + i->tos;
            i->tos = tmp;
            break;
        case OP_CLEAN:
            cnt = VMCODEBYTE(i->pc++);
            Drop(i, cnt);
            break;
        case OP_FRAME:
            cnt = VMCODEBYTE(i->pc++);
            tmp = (VMVALUE)(i->fp - i->stack);
            i->fp = i->sp;
            Reserve(i, cnt);
            i->fp[F_FP] = tmp;
            break;
        case OP_RETURNZ:
            CPush(i, i->tos);
            i->tos = 0;
            // fall through
        case OP_RETURN:
            i->pc = (uint8_t *)i->base + Top(i);
            i->sp = i->fp;
            i->fp = (VMVALUE *)(i->stack + i->fp[F_FP]);
            break;
        case OP_DROP:
            i->tos = Pop(i);
            break;
        case OP_DUP:
            CPush(i, i->tos);
            break;
        case OP_NATIVE:
            for (tmp = 0, cnt = sizeof(VMUVALUE); --cnt >= 0; )
                tmp = (tmp << 8) | VMCODEBYTE(i->pc++);
            break;
        case OP_TRAP:
            DoTrap(i, VMCODEBYTE(i->pc++));
            break;
        default:
            AbortVM(i, "undefined opcode 0x%02x", VMCODEBYTE(i->pc - 1));
            break;
        }
    }
}

static void DoTrap(Interpreter *i, int op)
{
    switch (op) {
    case TRAP_GetChar:
        Push(i, i->tos);
        i->tos = VM_getchar();
        break;
    case TRAP_PutChar:
        VM_putchar(i->tos);
        i->tos = Pop(i);
        break;
    default:
        AbortVM(i, "undefined print opcode 0x%02x", op);
        break;
    }
}

void ShowStack(Interpreter *i)
{
    VMVALUE *p;
    if (i->sp < i->stackTop) {
        VM_printf(" %d", i->tos);
        for (p = i->sp; p < i->stackTop - 1; ++p) {
            if (p == i->fp)
                VM_printf(" <fp>");
            VM_printf(" %d", *p);
        }
        VM_printf("\n");
    }
}

void StackOverflow(Interpreter *i)
{
    AbortVM(i, "stack overflow");
}

void AbortVM(Interpreter *i, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    VM_printf("abort: ");
    VM_vprintf(fmt, ap);
    VM_printf("\n");
    va_end(ap);
    if (i)
        longjmp(i->errorTarget, 1);
    else
        exit(1);
}

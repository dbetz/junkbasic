/* vmint.h - definitions for a simple virtual machine
 *
 * Copyright (c) 2020 by David Michael Betz.  All rights reserved.
 *
 */

#ifndef __VMINT_H__
#define __VMINT_H__

#include <stdio.h>
#include <stdarg.h>
#include <setjmp.h>
#include "system.h"
#include "image.h"

/* forward type declarations */
typedef struct Interpreter Interpreter;

/* intrinsic function handler type */
typedef void IntrinsicFcn(Interpreter *i);

typedef struct ImageHdr ImageHdr;
struct ImageHdr {
    VMUVALUE mainCode;
    VMUVALUE stackSize;
};

/* interpreter state structure */
struct Interpreter {
    System *sys;
    ImageHdr *image;
    jmp_buf errorTarget;
    VMVALUE *stack;
    VMVALUE *stackTop;
    uint8_t *pc;
    VMVALUE *fp;
    VMVALUE *sp;
    VMVALUE tos;
    int argc;
    int linePos;
};

/* stack frame offsets */
#define F_FP    -1
#define F_SIZE  1

/* stack manipulation macros */
#define Reserve(i, n)   do {                                    \
                            if ((i)->sp - (n) < (i)->stack)     \
                                StackOverflow(i);               \
                            else  {                             \
                                int _cnt = (n);                 \
                                while (--_cnt >= 0)             \
                                    Push(i, 0);                 \
                            }                                   \
                        } while (0)
#define CPush(i, v)     do {                                    \
                            if ((i)->sp - 1 < (i)->stack)       \
                                StackOverflow(i);               \
                            else                                \
                                Push(i, v);                     \
                        } while (0)
#define Push(i, v)      (*--(i)->sp = (v))
#define Pop(i)          (*(i)->sp++)
#define Top(i)          (*(i)->sp)
#define Drop(i, n)      ((i)->sp += (n))

/* prototypes for xbint.c */
void Fatal(System *sys, const char *fmt, ...);

/* prototypes from db_vmimage.c */
ImageHdr *LoadImage(System *sys, const char *name);

/* prototypes from db_vmint.c */
Interpreter *InitInterpreter(System *sys, ImageHdr *image);
int Execute(Interpreter *i, ImageHdr *image);
void AbortVM(Interpreter *i, const char *fmt, ...);
void StackOverflow(Interpreter *i);
void ShowStack(Interpreter *i);

/* prototypes and variables from db_vmfcn.c */
extern IntrinsicFcn *Intrinsics[];
extern int IntrinsicCount;

#endif

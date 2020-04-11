/* system.h - global system context
 *
 * Copyright (c) 2019 by David Michael Betz.  All rights reserved.
 *
 */

#ifndef __SYSTEM_H__
#define __SYSTEM_H__

#include <stdarg.h>
#include <setjmp.h>
#include "types.h"

/* program limits */
#define MAXLINE         128

/* line input handler */
typedef int GetLineHandler(char *buf, int len, void *cookie);

/* system context */
typedef struct {
    jmp_buf errorTarget;        /* error target */
    GetLineHandler *getLine;    /* function to get a line of input */
    void *getLineCookie;        /* cookie for the getLine function */
    uint8_t *freeSpace;         /* base of free space */
    uint8_t *freeMark;          /* top of permanently allocated storage */
    uint8_t *freeNext;          /* next free space available */
    uint8_t *freeTop;           /* top of free space */
    char lineBuf[MAXLINE];      /* current input line */
    char *linePtr;              /* pointer to the current character */
} System;

System *InitSystem(uint8_t *freeSpace, size_t freeSize);
uint8_t *AllocateFreeSpace(System *sys, size_t size);
int GetLine(System *sys);
void Abort(System *sys, const char *fmt, ...);

void VM_sysinit(int argc, char *argv[]);
int VM_getchar(void);
char *VM_getline(char *buf, int size);
void VM_printf(const char *fmt, ...);
void VM_vprintf(const char *fmt, va_list ap);
void VM_putchar(int ch);
void VM_flush(void);

#endif

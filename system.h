/* system.h - global system context
 *
 * Copyright (c) 2020 by David Michael Betz.  All rights reserved.
 *
 */

#ifndef __SYSTEM_H__
#define __SYSTEM_H__

#include <stdarg.h>
#include <setjmp.h>
#include "types.h"

/* program limits */
#define MAXLINE         128

/* forward type definitions */
typedef struct System System;

/* line input handler */
typedef char *GetLineHandler(char *buf, int len, int *pLineNumber, void *cookie);

/* code generator context */
typedef struct GenerateContext GenerateContext;
struct GenerateContext {
    System *sys;
    uint8_t *codeBuf;
};

/* system context */
struct System {
    jmp_buf errorTarget;            /* error target */
    uint8_t *freeSpace;             /* base of free space */
    uint8_t *freeNext;              /* next free space available */
    uint8_t *freeTop;               /* top of free space */
    uint8_t *nextHigh;              /* next high memory heap space location */
    uint8_t *nextLow;               /* next low memory heap space location */
    size_t heapSize;                /* size of heap space in bytes */
    size_t maxHeapUsed;             /* maximum amount of heap space allocated so far */
    GetLineHandler *getLine;        /* function to get a line from the source program */
    void *getLineCookie;            /* cookie for the rewind and getLine functions */
    char lineBuf[MAXLINE];          /* current input line */
    char *linePtr;                  /* pointer to the current character */
};

System *InitSystem(uint8_t *freeSpace, size_t freeSize);
void *AllocateHighMemory(System *sys, size_t size);
void *AllocateLowMemory(System *sys, size_t size);

void GetMainSource(System *sys, GetLineHandler **pGetLine, void **pGetLineCookie);
void SetMainSource(System *sys, GetLineHandler *getLine, void *getLineCookie);
int GetLine(System *sys, int *pLineNumber);
void Abort(System *sys, const char *fmt, ...);

void VM_sysinit(int argc, char *argv[]);
int VM_getchar(void);
void VM_printf(const char *fmt, ...);
void VM_vprintf(const char *fmt, va_list ap);
void VM_putchar(int ch);
void VM_flush(void);

void *VM_open(System *sys, const char *name, const char *mode);
char *VM_getline(char *buf, int size, void *fp);
void VM_close(void *fp);

#ifdef LOAD_SAVE
int VM_opendir(const char *path, VMDIR *dir);
int VM_readdir(VMDIR *dir, VMDIRENT *entry);
void VM_closedir(VMDIR *dir);
#endif

#endif

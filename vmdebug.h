#ifndef __VMDEBUG_H__
#define __VMDEBUG_H__

#include "system.h"

/* instruction output formats */
#define FMT_NONE        0
#define FMT_BYTE        1
#define FMT_SBYTE       2
#define FMT_WORD        3
#define FMT_NATIVE      4
#define FMT_BR          5

typedef struct {
    int code;
    char *name;
    int fmt;
} OTDEF;

extern OTDEF OpcodeTable[];

void DecodeFunction(VMUVALUE base, const uint8_t *code, int len);
int DecodeInstruction(VMUVALUE addr, const uint8_t *lc);

#endif

#ifndef __EDIT_H__
#define __EDIT_H__

#include "system.h"

typedef struct {
    int lineNumber;
    int length;
    char text[1];
} Line;

typedef struct {
    uint8_t *bufferMax;
    uint8_t *bufferTop;
    Line *currentLine;
    uint8_t buffer[1];
} EditBuf;

/* edit.c */
void EditWorkspace(System *sys);

/* editbuf.c */
EditBuf *BufInit(uint8_t *space, size_t size);
void BufNew(EditBuf *buf);
int BufAddLineN(EditBuf *buf, int lineNumber, const char *text);
int BufDeleteLineN(EditBuf *buf, int lineNumber);
int BufSeekN(EditBuf *buf, int lineNumber);
int BufGetLine(EditBuf *buf, int *pLineNumber, char *text);

#endif


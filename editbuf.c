#include <string.h>
#include "edit.h"

static int FindLineN(EditBuf *buf, int lineNumber, Line **pLine);

EditBuf *BufInit(System *sys, uint8_t *space, size_t size)
{
    EditBuf *buf = (EditBuf *)space;
    if (size < sizeof(EditBuf))
        return NULL;
    buf->sys = sys;
    buf->bufferMax = space + size;
    buf->bufferTop = buf->buffer;
    buf->currentLine = (Line *)buf->buffer;
    return buf;
}

void BufNew(EditBuf *buf)
{
    buf->bufferTop = buf->buffer;
    buf->currentLine = (Line *)buf->buffer;
}

int BufAddLineN(EditBuf *buf, int lineNumber, const char *text)
{
    int newLength = sizeof(Line) + strlen(text);
    int spaceNeeded;
    uint8_t *next;
    Line *line;

    /* make sure the length is a multiple of the word size */
    newLength = (newLength + ALIGN_MASK) & ~ALIGN_MASK;

    /* replace an existing line */
    if (FindLineN(buf, lineNumber, &line)) {
        next = (uint8_t *)line + line->length;
        spaceNeeded = newLength - line->length;
    }

    /* insert a new line */
    else {
        next = (uint8_t *)line;
        spaceNeeded = newLength;
    }

    /* make sure there is enough space */
    if (buf->bufferTop + spaceNeeded > buf->bufferMax)
        return VMFALSE;

    /* make space for the new line */
    if (next < buf->bufferTop && spaceNeeded != 0)
        memmove(next + spaceNeeded, next, buf->bufferTop - next);
    buf->bufferTop += spaceNeeded;

    /* insert the new line */
    if (newLength > 0) {
        line->lineNumber = lineNumber;
        line->length = newLength;
        strcpy(line->text, text);
    }

    /* return successfully */
    return VMTRUE;
}

int BufDeleteLineN(EditBuf *buf, int lineNumber)
{
    Line *line, *next;
    int spaceFreed;

    /* find the line to delete */
    if (!FindLineN(buf, lineNumber, &line))
        return VMFALSE;

    /* get a pointer to the next line */
    next = (Line *)((uint8_t *)line + line->length);
    spaceFreed = line->length;

    /* remove the line to be deleted */
    if ((uint8_t *)next < buf->bufferTop)
        memmove(line, next, buf->bufferTop - (uint8_t *)next);
    buf->bufferTop -= spaceFreed;

    /* return successfully */
    return VMTRUE;
}

int BufSeekN(EditBuf *buf, int lineNumber)
{
    /* if the line number is zero start at the first line */
    if (lineNumber == 0)
        buf->currentLine = (Line *)buf->buffer;

    /* otherwise, start at the specified line */
    else if (!FindLineN(buf, lineNumber, &buf->currentLine))
        return VMFALSE;

    /* return successfully */
    return VMTRUE;
}

int BufGetLine(EditBuf *buf, int *pLineNumber, char *text)
{
    /* check for the end of the buffer */
    if ((uint8_t *)buf->currentLine >= buf->bufferTop)
        return VMFALSE;

    /* get the current line */
    *pLineNumber = buf->currentLine->lineNumber;
    strcpy(text, buf->currentLine->text);

    /* move ahead to the next line */
    buf->currentLine = (Line *)((char *)buf->currentLine + buf->currentLine->length);

    /* return successfully */
    return VMTRUE;
}

static int FindLineN(EditBuf *buf, int lineNumber, Line **pLine)
{
    uint8_t *p = buf->buffer;
    while (p < buf->bufferTop) {
        Line *line = (Line *)p;
        if (lineNumber <= line->lineNumber) {
            *pLine = line;
            return lineNumber == line->lineNumber;
        }
        p = p + line->length;
    }
    *pLine = (Line *)p;
    return VMFALSE;
}

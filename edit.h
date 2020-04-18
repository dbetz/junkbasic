#ifndef __EDIT_H__
#define __EDIT_H__

#include "system.h"

typedef struct {
    int lineNumber;
    int length;
    char text[1];
} Line;

typedef struct {
    System *sys;
#ifdef LOAD_SAVE
    char programName[FILENAME_MAX];
#endif
    uint8_t *bufferMax;
    uint8_t *bufferTop;
    Line *currentLine;
    uint8_t buffer[1];
} EditBuf;

/* edit.c */
void EditWorkspace(System *sys);

#endif


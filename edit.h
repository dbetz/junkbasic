#ifndef __DB_EDIT_H__
#define __DB_EDIT_H__

#include "system.h"

/* edit.c */
void EditWorkspace(System *sys);

/* editbuf.c */
void BufInit(void);
int BufAddLineN(int lineNumber, const char *text);
int BufDeleteLineN(int lineNumber);
int BufSeekN(int lineNumber);
int BufGetLine(int *pLineNumber, char *text);

#endif


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "edit.h"
#include "compile.h"

#define MAXTOKEN    32

/* command handlers */
static void DoNew(System *sys);
static void DoList(System *sys);
static void DoRun(System *sys);

/* command table */
static struct {
    char *name;
    void (*handler)(System *sys);
} cmds[] = {
{   "NEW",      DoNew   },
{   "LIST",     DoList  },
{   "RUN",      DoRun   },
{   NULL,       NULL    }
};

static EditBuf *editBuf;

/* prototypes */
static char *NextToken(System *sys);
static int ParseNumber(char *token, int *pValue);
static int IsBlank(char *p);

void EditWorkspace(System *sys)
{
    uint8_t *editBuffer;
    size_t editBufferSize;
    int lineNumber;
    char *token;

    editBuffer = AllocateAllFreeSpace(sys, &editBufferSize);
    
    if (!(editBuf = BufInit(editBuffer, editBufferSize)))
        Abort(sys, "insufficient memory for edit buffer");

    while (GetLine(sys)) {

        if ((token = NextToken(sys)) != NULL) {
            if (ParseNumber(token, &lineNumber)) {
                if (IsBlank(sys->linePtr)) {
                    if (!BufDeleteLineN(editBuf, lineNumber))
                        VM_printf("no line %d\n", lineNumber);
                }
                else if (!BufAddLineN(editBuf, lineNumber, sys->linePtr))
                    VM_printf("out of edit buffer space\n");
            }

            else {
                int i;
                for (i = 0; cmds[i].name != NULL; ++i)
                    if (strcasecmp(token, cmds[i].name) == 0)
                        break;
                if (cmds[i].handler) {
                    (*cmds[i].handler)(sys);
                    VM_printf("OK\n");
                }
                else {
                    VM_printf("Unknown command: %s\n", token);
                }
            }
        }
    }
}

static void DoNew(System *sys)
{
    BufNew(editBuf);
}

static void DoList(System *sys)
{
    int lineNumber;
    BufSeekN(editBuf, 0);
    while (BufGetLine(editBuf, &lineNumber, sys->lineBuf))
        VM_printf("%d%s", lineNumber, sys->lineBuf);
}

static int EditGetLine(char *buf, int len, void *cookie)
{
    int lineNumber;
    return BufGetLine(editBuf, &lineNumber, buf);
}

static void DoRun(System *sys)
{
    ParseContext *c;

    ResetToMark(sys, editBuf->bufferTop);
    
    if (!(c = InitParseContext(sys)))
        VM_printf("insufficient memory");
    else {
        GetLineHandler *getLine;
        void *getLineCookie;

        getLine = sys->getLine;
        getLineCookie = sys->getLineCookie;
    
        sys->getLine = EditGetLine;

        BufSeekN(editBuf, 0);

        Compile(c);

        sys->getLine = getLine;
        sys->getLineCookie = getLineCookie;
    }
    
    ResetToMark(sys, editBuf->bufferMax);
}

static char *NextToken(System *sys)
{
    static char token[MAXTOKEN];
    int ch, i;
    
    /* skip leading spaces */
    while ((ch = *sys->linePtr) != '\0' && isspace(ch))
        ++sys->linePtr;
        
    /* collect a token until the next non-space */
    for (i = 0; (ch = *sys->linePtr) != '\0' && !isspace(ch); ++sys->linePtr)
        if (i < sizeof(token) - 1)
            token[i++] = ch;
    token[i] = '\0';
    
    return token[0] == '\0' ? NULL : token;
}

static int ParseNumber(char *token, int *pValue)
{
    int ch;
    *pValue = 0;
    while ((ch = *token++) != '\0' && isdigit(ch))
        *pValue = *pValue * 10 + ch - '0';
    return ch == '\0';
}

static int IsBlank(char *p)
{
    int ch;
    while ((ch = *p++) != '\0')
        if (!isspace(ch))
            return VMFALSE;
    return VMTRUE;
}

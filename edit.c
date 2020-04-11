#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "edit.h"
#include "compile.h"

#define MAXTOKEN    32

/* command handlers */
static void DoNew(EditBuf *buf);
static void DoList(EditBuf *buf);
static void DoRun(EditBuf *buf);

/* command table */
static struct {
    char *name;
    void (*handler)(EditBuf *buf);
} cmds[] = {
{   "NEW",      DoNew   },
{   "LIST",     DoList  },
{   "RUN",      DoRun   },
{   NULL,       NULL    }
};

/* prototypes */
static char *NextToken(System *sys);
static int ParseNumber(char *token, int *pValue);
static int IsBlank(char *p);

void EditWorkspace(System *sys)
{
    uint8_t *editBuffer;
    size_t editBufferSize;
    EditBuf *editBuf;
    int lineNumber;
    char *token;

    if (!(editBuf = BufInit(sys, editBuffer, editBufferSize)))
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
                    (*cmds[i].handler)(editBuf);
                    VM_printf("OK\n");
                }
                else {
                    VM_printf("Unknown command: %s\n", token);
                }
            }
        }
    }
}

static void DoNew(EditBuf *buf)
{
    BufNew(buf);
}

static void DoList(EditBuf *buf)
{
    int lineNumber;
    BufSeekN(buf, 0);
    while (BufGetLine(buf, &lineNumber, buf->sys->lineBuf))
        VM_printf("%d%s", lineNumber, buf->sys->lineBuf);
}

static int EditGetLine(char *buf, int len, void *cookie)
{
    EditBuf *editBuf = (EditBuf *)cookie;
    int lineNumber;
    return BufGetLine(editBuf, &lineNumber, buf);
}

static void DoRun(EditBuf *buf)
{
    System *sys = buf->sys;
    ParseContext *c;

    /* give the unused edit buffer space back to the system */
    ResetToMark(sys, buf->bufferTop);
    
    if (!(c = InitParseContext(sys)))
        VM_printf("insufficient memory");
    else {
        GetLineHandler *getLine;
        void *getLineCookie;

        getLine = sys->getLine;
        getLineCookie = sys->getLineCookie;
    
        sys->getLine = EditGetLine;
        sys->getLineCookie = buf;

        BufSeekN(buf, 0);

        Compile(c);

        sys->getLine = getLine;
        sys->getLineCookie = getLineCookie;
    }
    
    /* grab the rest of the system memory again for the edit buffer */
    ResetToMark(sys, buf->bufferMax);
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

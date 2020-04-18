#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "edit.h"
#include "compile.h"

#define MAXTOKEN        32

/* command handlers */
static void DoNew(EditBuf *buf);
static void DoList(EditBuf *buf);
static void DoRun(EditBuf *buf);
#ifdef LOAD_SAVE
static void DoLoad(EditBuf *buf);
static void DoSave(EditBuf *buf);
static void DoCat(EditBuf *buf);
#endif

/* command table */
static struct {
    char *name;
    void (*handler)(EditBuf *buf);
} cmds[] = {
{   "NEW",      DoNew   },
{   "LIST",     DoList  },
{   "RUN",      DoRun   },
#ifdef LOAD_SAVE
{   "LOAD",     DoLoad  },
{   "SAVE",     DoSave  },
{   "CAT",      DoCat   },
#endif
{   NULL,       NULL    }
};

/* prototypes */
static char *NextToken(System *sys);
static int ParseNumber(char *token, int *pValue);
static int IsBlank(char *p);
#ifdef LOAD_SAVE
static int SetProgramName(EditBuf *buf);
#endif

/* edit buffer prototypes */
static EditBuf *BufInit(System *sys, uint8_t *space, size_t size);
static void BufNew(EditBuf *buf);
static int BufAddLineN(EditBuf *buf, int lineNumber, const char *text);
static int BufDeleteLineN(EditBuf *buf, int lineNumber);
static int BufSeekN(EditBuf *buf, int lineNumber);
static int BufGetLine(EditBuf *buf, int *pLineNumber, char *text);
static int FindLineN(EditBuf *buf, int lineNumber, Line **pLine);

void EditWorkspace(System *sys)
{
    uint8_t *editBuffer;
    size_t editBufferSize;
    EditBuf *editBuf;
    int lineNumber;
    char *token;
    
    editBuffer = AllocateAllFreeSpace(sys, &editBufferSize);

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
#ifdef LOAD_SAVE
    SetProgramName(buf);
#endif
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

#ifdef LOAD_SAVE

static int SetProgramName(EditBuf *buf)
{
    char *name;
    if ((name = NextToken(buf->sys)) != NULL) {
        strncpy(buf->programName, name, FILENAME_MAX - 1);
        buf->programName[FILENAME_MAX - 1] = '\0';
        if (!strchr(buf->programName, '.')) {
            if (strlen(buf->programName) < FILENAME_MAX - 5)
                strcat(buf->programName, ".bas");
        }
    }
    return buf->programName[0] != '\0';
}

static void DoLoad(EditBuf *buf)
{
    VMFILE *fp;
    
    /* check for a program name on the command line */
    if (!SetProgramName(buf)) {
        VM_printf("expecting a file name\n");
        return;
    }
    
    /* load the program */
    if (!(fp = VM_fopen(buf->programName, "r")))
        VM_printf("error loading '%s'\n", buf->programName);
    else {
        System *sys = buf->sys;
        VM_printf("Loading '%s'\n", buf->programName);
        BufNew(buf);
        while (VM_fgets(sys->lineBuf, sizeof(sys->lineBuf), fp) != NULL) {
            int lineNumber;
            char *token;
            sys->linePtr = sys->lineBuf;
            if ((token = NextToken(sys)) != NULL) {
                if (ParseNumber(token, &lineNumber))
                    BufAddLineN(buf, lineNumber, sys->linePtr);
                else
                    VM_printf("expecting a line number: %s\n", token);
            }
        }
        VM_fclose(fp);
    }
}

static void DoSave(EditBuf *buf)
{
    VMFILE *fp;
    
    /* check for a program name on the command line */
    if (!SetProgramName(buf)) {
        VM_printf("expecting a file name\n");
        return;
    }
    
    /* save the program */
    if (!(fp = VM_fopen(buf->programName, "w")))
        VM_printf("error saving '%s'\n", buf->programName);
    else {
        System *sys = buf->sys;
        int lineNumber;
        VM_printf("Saving '%s'\n", buf->programName);
        BufSeekN(buf, 0);
        while (BufGetLine(buf, &lineNumber, sys->lineBuf)) {
            char buf[32];
            sprintf(buf, "%d", lineNumber);
            VM_fputs(buf, fp);
            VM_fputs(sys->lineBuf, fp);
        }
        VM_fclose(fp);
    }
}

static void DoCat(EditBuf *buf)
{
    VMDIRENT entry;
    VMDIR dir;    
    if (VM_opendir(".", &dir) == 0) {
        while (VM_readdir(&dir, &entry) == 0) {
            int len = strlen(entry.name);
            if (len >= 4 && strcasecmp(&entry.name[len - 4], ".bas") == 0)
                VM_printf("  %s\n", entry.name);
        }
        VM_closedir(&dir);
    }
}

#endif

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

static EditBuf *BufInit(System *sys, uint8_t *space, size_t size)
{
    EditBuf *buf = (EditBuf *)space;
    if (size < sizeof(EditBuf))
        return NULL;
    memset(buf, 0, sizeof(EditBuf));
    buf->sys = sys;
    buf->bufferMax = space + size;
    buf->bufferTop = buf->buffer;
    buf->currentLine = (Line *)buf->buffer;
    return buf;
}

static void BufNew(EditBuf *buf)
{
    buf->bufferTop = buf->buffer;
    buf->currentLine = (Line *)buf->buffer;
}

static int BufAddLineN(EditBuf *buf, int lineNumber, const char *text)
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

static int BufDeleteLineN(EditBuf *buf, int lineNumber)
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

static int BufSeekN(EditBuf *buf, int lineNumber)
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

static int BufGetLine(EditBuf *buf, int *pLineNumber, char *text)
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


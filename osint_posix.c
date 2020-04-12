#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>
#include "edit.h"
#include "compile.h"
#include "system.h"

#define WORKSPACE_SIZE  (64 * 1024)

uint8_t workspace[WORKSPACE_SIZE];

#ifdef LINE_EDIT
static char *EditLine(char *buf, int size, void *cookie);
#endif

int main(int argc, char *argv[])
{
    System *sys = InitSystem(workspace, sizeof(workspace));
    if (sys) {
#ifdef LINE_EDIT
        sys->getLine = EditLine;
#else
        sys->getLine = (GetLineHandler *)fgets;
        sys->getLineCookie = stdin;
#endif
        EditWorkspace(sys);
    }
    return 0;
}

void VM_flush(void)
{
    fflush(stdout);
}

int VM_getchar(void)
{
    int ch = getchar();
    if (ch == '\r')
        ch = '\n';
    return ch;
}

void VM_putchar(int ch)
{
    if (ch == '\n')
        putchar('\r');
    putchar(ch);
}

#ifdef LINE_EDIT
static char *EditLine(char *buf, int size, void *cookie)
{
    int i = 0;
    while (i < size - 1) {
        int ch = VM_getchar();
        if (ch == '\n') {
            buf[i++] = '\n';
            VM_putchar('\n');
            break;
        }
        else if (ch == '\b' || ch == 0x7f) {
            if (i > 0) {
                if (ch == 0x7f)
                    VM_putchar('\b');
                VM_putchar(' ');
                VM_putchar('\b');
                VM_flush();
                --i;
            }
        }
        else {
            buf[i++] = ch;
            VM_flush();
        }
    }
    buf[i] = '\0';
    return buf;
}
#endif

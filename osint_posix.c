#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>
#include "compile.h"
#include "system.h"

#define WORKSPACE_SIZE  (64 * 1024)

uint8_t workspace[WORKSPACE_SIZE];

int main(int argc, char *argv[])
{
    System *sys = InitSystem(workspace, sizeof(workspace));
    if (sys) {
        ParseContext *c;
        sys->getLine = (GetLineHandler *)fgets;
        sys->getLineCookie = stdin;
        c = InitParseContext(sys);
        if (c) {
            Compile(c);
        }
    }
    return 0;
}

void VM_vprintf(const char *fmt, va_list ap)
{
    char buf[80], *p = buf;
    vsprintf(buf, fmt, ap);
    while (*p != '\0')
        VM_putchar(*p++);
}

void VM_putchar(int ch)
{
    putchar(ch);
}

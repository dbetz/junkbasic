#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>
#include "edit.h"
#include "compile.h"
#include "system.h"

#define WORKSPACE_SIZE  (64 * 1024)

uint8_t workspace[WORKSPACE_SIZE];

int main(int argc, char *argv[])
{
    System *sys = InitSystem(workspace, sizeof(workspace));
    if (sys) {
        sys->getLine = (GetLineHandler *)fgets;
        sys->getLineCookie = stdin;
        EditWorkspace(sys);
    }
    return 0;
}

void VM_putchar(int ch)
{
    putchar(ch);
}

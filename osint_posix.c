#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>
#include "edit.h"
#include "compile.h"
#include "system.h"

#ifdef PROPELLER
#include <sys/vfs.h>
#endif

/*
fastspin memory map:
There is a command line switch (-H) to specify the starting address, but the default is 0.
First comes whatever COG code is needed (very little for the P2, just enough to bootstrap).
Then comes the hubexec code. Then comes the data (including all variables). The heap (the 
size of which is given by HEAPSIZE) is part of that data. After that comes the stack, which 
grows upwards towards the end of memory. You can access the stack pointer by the variable 
"sp" in inline assembly, or by calling __getsp() in a high level language. The stuff above 
that is "free" (as long as you don't need a deeper stack, of course).
*/

#ifdef PROPELLER
#define STACK_SIZE      (32 * 1024)
#else
#define WORKSPACE_SIZE  (64 * 1024)
uint8_t workspace[WORKSPACE_SIZE];
#endif

static char *GetConsoleLine(char *buf, int size, int *pLineNumber, void *cookie);

int main(int argc, char *argv[])
{
#ifdef PROPELLER
    uint8_t *workspace = (uint8_t *)__getsp() + STACK_SIZE;
    size_t workspaceSize = (64 * 1024);
    System *sys = InitSystem(workspace, workspaceSize);
#else
    System *sys = InitSystem(workspace, sizeof(workspace));
#endif
    if (sys) {
#ifdef PROPELLER
    _setrootvfs(_vfs_open_host());  // to access host files
    //_setrootvfs(_vfs_open_sdcard()); // to access files on SD card
#endif
        sys->mainFile.u.main.getLine = GetConsoleLine;
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
#ifdef LINE_EDIT
    if (ch == '\r')
        ch = '\n';
#endif
    return ch;
}

void VM_putchar(int ch)
{
#ifdef LINE_EDIT
    if (ch == '\n')
        putchar('\r');
#endif
    putchar(ch);
}

void *VM_open(System *sys, const char *name, const char *mode)
{
    return (void *)fopen(name, mode);
}

char *VM_getline(char *buf, int size, void *fp)
{
	return fgets(buf, size, (FILE *)fp);
}

void VM_close(void *fp)
{
    fclose((FILE *)fp);
}

int VM_opendir(const char *path, VMDIR *dir)
{
    if (!(dir->dirp = opendir(path)))
        return -1;
    return 0;
}

int VM_readdir(VMDIR *dir, VMDIRENT *entry)
{
    struct dirent *ansi_entry;
    
    if (!(ansi_entry = readdir(dir->dirp)))
        return -1;
        
    strcpy(entry->name, ansi_entry->d_name);
    
    return 0;
}

void VM_closedir(VMDIR *dir)
{
    closedir(dir->dirp);
}

#ifdef LINE_EDIT
static char *GetConsoleLine(char *buf, int size, int *pLineNumber, void *cookie)
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
#else
static char *GetConsoleLine(char *buf, int size, int *pLineNumber, void *cookie)
{
    *pLineNumber = 0;
    return fgets(buf, size, stdin);
}
#endif

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

#define WORKSPACE_SIZE  (64 * 1024)

uint8_t workspace[WORKSPACE_SIZE];

#ifdef LINE_EDIT
static char *EditLine(char *buf, int size, void *cookie);
#endif

int main(int argc, char *argv[])
{
    System *sys = InitSystem(workspace, sizeof(workspace));
    if (sys) {
#ifdef PROPELLER
    _setrootvfs(_vfs_open_host());  // to access host files
    //_setrootvfs(_vfs_open_sdcard()); // to access files on SD card
#endif
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
#ifdef LINE_EDIT
    int ch = getchar();
    if (ch == '\r')
        ch = '\n';
    return ch;
#else
    return getchar();
#endif
}

void VM_putchar(int ch)
{
#ifdef LINE_EDIT
    if (ch == '\n')
        putchar('\r');
#endif
    putchar(ch);
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
    char *ptr;
    int i;
    
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

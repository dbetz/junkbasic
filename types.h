/* types.h - type definitions for a simple virtual machine
 *
 * Copyright (c) 2020 by David Michael Betz.  All rights reserved.
 *
 */

#ifndef __TYPES_H__
#define __TYPES_H__

/**********/
/* Common */
/**********/

#define VMTRUE      1
#define VMFALSE     0

/* system heap size (includes compiler heap and image buffer) */
#ifndef HEAPSIZE
#define HEAPSIZE            5000
#endif

/* size of image buffer (allocated from system heap) */
#ifndef IMAGESIZE
#define IMAGESIZE           2500
#endif

/* edit buffer size (separate from the system heap) */
#ifndef EDITBUFSIZE
#define EDITBUFSIZE         1500
#endif

/*******/
/* MAC */
/*******/

#if defined(MAC)

#include <stdio.h>
#include <stdint.h>
#include <string.h>

#if 0
typedef int64_t VMVALUE;
typedef uint64_t VMUVALUE;

#define INT_FMT     "%lld"
#define UINT_FMT    "%llu"
#else
typedef int32_t VMVALUE;
typedef uint32_t VMUVALUE;

#define INT_FMT     "%d"
#define UINT_FMT    "%u"
#endif

#define VMCODEBYTE(p)           *(uint8_t *)(p)
#define VMINTRINSIC(i)          Intrinsics[i]

#define ANSI_FILE_IO

/*********/
/* LINUX */
/*********/

#elif defined(LINUX)

#include <stdio.h>
#include <stdint.h>
#include <string.h>

typedef int64_t VMVALUE;
typedef uint64_t VMUVALUE;

#define INT_FMT     "%lld"
#define UINT_FMT    "%llu"

#define VMCODEBYTE(p)           *(uint8_t *)(p)
#define VMINTRINSIC(i)          Intrinsics[i]

#define ANSI_FILE_IO

/*************/
/* PROPELLER */
/*************/

#elif defined(PROPELLER)

#include <string.h>
#include <stdint.h>

int strcasecmp(const char *s1, const char *s2);

#define VMCODEBYTE(p)           *(uint8_t *)(p)
#define VMINTRINSIC(i)          Intrinsics[i]

typedef int32_t VMVALUE;
typedef uint32_t VMUVALUE;

#define INT_FMT     "%d"
#define UINT_FMT    "%u"

#define LINE_EDIT

#define ANSI_FILE_IO

#else

#error Must define MAC, LINUX, or PROPELLER

#endif

#define ALIGN_MASK  (sizeof(VMVALUE) - 1)

/****************/
/* ANSI_FILE_IO */
/****************/

#ifdef ANSI_FILE_IO

#include <stdio.h>
#include <dirent.h>

typedef FILE VMFILE;

#define VM_fopen	fopen
#define VM_fclose	fclose
#define VM_fgets	fgets
#define VM_fputs	fputs

typedef struct {
    DIR *dirp;
} VMDIR;

typedef struct {
    char name[FILENAME_MAX];
} VMDIRENT;

#endif // ANSI_FILE_IO

#endif

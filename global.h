#pragma once

#define BUFF_SIZE_LARGE 4096
#define BUFF_SIZE_SMALL 256
#define BUFF_SIZE_TINY 64
#define BUFF_INCREASE BUFF_SIZE_TINY

//#define DEBUG

#ifndef bool
#define bool int
#endif

#ifndef true
#define true 1
#endif

#ifndef false
#define false 0
#endif


#ifdef WIN32
#define str_cmpi _strcmpi
#else
#define str_cmpi strcasecmp
#endif


#ifdef WIN32
#define alloca _malloca
#else
#include <unistd.h>

#define Sleep usleep
#endif
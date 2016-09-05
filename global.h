#pragma once

#define BUFF_SIZE_LARGE 4096
#define BUFF_SIZE_MEDIUM 1024
#define BUFF_SIZE_SMALL 256
#define BUFF_SIZE_TINY 64
#define BUFF_INCREASE BUFF_SIZE_TINY

#define CONFIG_PATH "config.xml"


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
#define strcmpi _strcmpi
#else
#define strcmpi strcasecmp
#endif


#ifdef WIN32
#ifndef alloca
	#define alloca _malloca
#endif
#else
#include <unistd.h>

#define Sleep usleep
#endif
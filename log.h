#ifndef _LOG_H
#define _LOG_H

#include <stdio.h>
#include <sys/time.h>
#include <unistd.h>

extern FILE *log_file;

#define LOG_LEVEL_DEBUG 0
#define LOG_LEVEL_INFO  1
#define LOG_LEVEL_WARN  2
#define LOG_LEVEL_ERROR 3

#define LOG_LEVEL LOG_LEVEL_INFO

#define L(l,format,...)							\
	do {								\
		struct timeval __xxts;					\
		gettimeofday(&__xxts,NULL);				\
		fprintf(log_file,					\
			"[%s] %03d.%06d %s L%d P%d " format "\n", l,	\
			(int)__xxts.tv_sec % 1000, (int) __xxts.tv_usec, \
			__FUNCTION__,__LINE__,getpid(),##__VA_ARGS__);	\
	}while(0)		

#if LOG_LEVEL <= LOG_LEVEL_DEBUG
#define D(format,...) L("DEBUG",format,##__VA_ARGS__)			
#else
#define D(format,...) 
#endif

#if LOG_LEVEL <= LOG_LEVEL_INFO
#define I(format,...) L("INFO",format,##__VA_ARGS__)			
#else 
#define I(format,...) 
#endif

#if LOG_LVEL <= LOG_LEVEL_WARN
#define W(format,...) L("WARN",format,##__VA_ARGS__)			
#else 
#define W(format,...) 
#endif

#if LOG_LEVEL <= LOG_LEVEL_ERROR
#define E(format,...) L("ERROR",format,##__VA_ARGS__)			
#else 
#define E(format,...) 
#endif


#endif

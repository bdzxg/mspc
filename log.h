#ifndef _LOG_H
#define _LOG_H

#include <stdio.h>
#include <sys/time.h>
#include <unistd.h>

extern FILE *log_file;
extern int log_level;

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
                fflush(log_file);                                       \
	}while(0)		



#define D(format, ...) if (log_level <= LOG_LEVEL_DEBUG) \
        L("DEBUG",format,##__VA_ARGS__)
                      
#define I(format,...) if (log_level <= LOG_LEVEL_INFO) \
        L("INFO",format,##__VA_ARGS__)
                                   
#define W(format,...) if (log_level <= LOG_LEVEL_WARN) \
        L("WARN",format,##__VA_ARGS__)                 
                                   
#define E(format,...) if (log_level <= LOG_LEVEL_ERROR) \
        L("ERROR",format,##__VA_ARGS__) 

#endif

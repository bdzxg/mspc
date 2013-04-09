#ifndef _LOG_H
#define _LOG_H

#include <stdio.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

extern FILE *log_file;
extern int log_level;

typedef struct log_buffer_s {
        char *buf;
        size_t size;
        size_t len;
} log_buffer_t;

#define LOG_LEVEL_DEBUG 0 
#define LOG_LEVEL_INFO  1 
#define LOG_LEVEL_WARN  2 
#define LOG_LEVEL_ERROR 3

#define TIME_NOW_BUF_SIZE 256 
#define TEMP_LOG_BUF_SIZE 2048 
#define LOG_LEVEL LOG_LEVEL_ERROR
#define NOOP(x) do{} while(0)
#define UNUSE(x) (void)(x)
#define L(l,format,...)							  \
	do {								  \
                char now_str[TIME_NOW_BUF_SIZE];                          \
                char tmp_str[TEMP_LOG_BUF_SIZE];                          \
		struct timeval tv;					  \
                struct tm lt;                                             \
                time_t now = 0;                                           \
                size_t len = 0;                                           \
		gettimeofday(&tv, NULL);				  \
                now = tv.tv_sec;                                          \
                localtime_r(&now, &lt);                                   \
                len = strftime(now_str, TIME_NOW_BUF_SIZE,                \
                                "%Y-%m-%d %H:%M:%S", &lt);                \
                len += snprintf(now_str + len, TIME_NOW_BUF_SIZE - len,   \
                                ",%03d", (int)(tv.tv_usec/1000));         \
                sprintf(tmp_str,  "[%s] %s %s %s  L%d P%d "               \
                                "" format " \n", l, now_str, __FUNCTION__,\
                               __FILE__, __LINE__,getpid(),##__VA_ARGS__);\
                write_log(tmp_str);\
	}while(0)		

#define D(format, ...) if (log_level <= LOG_LEVEL_DEBUG) \
        L("DEBUG",format,##__VA_ARGS__) 
                      
#define I(format,...) if (log_level <= LOG_LEVEL_INFO) \
        L("INFO",format,##__VA_ARGS__)
                                   
#define W(format,...) if (log_level <= LOG_LEVEL_WARN) \
        L("WARN",format,##__VA_ARGS__)                 
                                   
#define E(format,...) if (log_level <= LOG_LEVEL_ERROR) \
        L("ERROR",format,##__VA_ARGS__) 

void init_log();
void write_log(char *log_info);

#endif

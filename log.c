#include "proxy.h"

log_buffer_t *log_buf;
extern FILE* log_file;
extern pxy_settings setting;

#define LOG_BUF_SIZE 8192 

void init_log() {
        log_buf = calloc(1, sizeof(log_buffer_t));
        log_buf->buf = calloc(LOG_BUF_SIZE, sizeof(char));
        log_buf->size = 0;
        log_buf->len = LOG_BUF_SIZE;
}

void write_log(char *log_info) {
        if (log_buf->len - log_buf->size <= strlen(log_info)) {      
                if (strcmp(setting.log_file, "stdout") == 0 ||
                           strcmp(setting.log_file, "stderr") == 0) {
                        log_file = stdout;                        
                        fprintf(log_file, "%s",  log_buf->buf);                    
                } else {
                        char log_file_name[200];
                        char now_str[200];
                        strcpy(log_file_name, setting.log_file);

                        struct timeval tv;
                        struct tm lt;
                        time_t now = 0;
                        gettimeofday(&tv, NULL);
                        now = tv.tv_sec;
                        localtime_r(&now, &lt);
                        strftime(now_str, 200, "%Y%m%d_%H.log", &lt);
                        strcat(log_file_name, now_str);
                        log_file = fopen(log_file_name, "a");

                        if (NULL == log_file) {
                                log_file = stdout;
                                E("LOG FILE open failed!");
                                goto failed;
                        }

                        fprintf(log_file, "%s",  log_buf->buf);                    
                        fclose(log_file);
                }

               log_buf->size = 0;                                  
        }                                        

        sprintf(log_buf->buf + log_buf->size, "%s", log_info);             
        log_buf->size += strlen(log_info);     
failed:
       return; 
}

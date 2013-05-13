#include "log.h"
#include <stdio.h>
#include <stdlib.h>
extern log_buffer_t *log_buf;
extern FILE* log_file;

void write_log(char *log_info) {
        if (log_buf->len - log_buf->size <= strlen(log_info)) {      
                fprintf(log_file, log_buf->buf);                    
                fflush(log_file);                                  
                log_buf->size = 0;                                  
        }                                                         
        sprintf(log_buf->buf + log_buf->size, log_info);             
        log_buf->size += strlen(log_info);     
}

int main(int argc, char **argv) {
        
}

#ifndef DATALOG_H
#define DATALOG_H

#include <mysql.h>

#define QUERY_BUFFER_SIZE 1024

typedef struct
{
    char host[32]; 
    char user[32];
    char passwd[32];
    char db[32];
    unsigned int port;
    MYSQL mysql;
} db_database;


int db_init(char *host, char *user, char *passwd, char *db, unsigned int port);
int db_open_connection();
int db_execute_nonquery(char *query, unsigned int length);
void db_close_connection();

int db_gettimestr(char *buffer, int size);
int db_gettablename(char *tablename, int size, char *time);

int db_insert_log(double level,
                  double threadId,
                  double pid,
                  char *time,
                  char *loggerName,
                  char *message,
                  char *error,
                  char *marker,
                  char *threadName,
                  char *serviceName,
                  char *computer);

#endif

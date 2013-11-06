#ifndef DATALOG_H
#define DATALOG_H

#include <mysql.h>

#define QUERY_INSERT_SIZE       256
#define QUERY_TIME_SIZE         32
#define QUERY_LOGGERNAME_SIZE   256
#define QUERY_LEVEL_SIZE        16
#define QUERY_MESSAGE_SIZE      512
#define QUERY_ERROR_SIZE        512
#define QUERY_MARKER_SIZE       256
#define QUERY_THREADID_SIZE     16
#define QUERY_THREADNAME_SIZE   128
#define QUERY_PID_SIZE          16
#define QUERY_SERVICENAME_SIZE  128
#define QUERY_COMPUTER_SIZE     128

#define QUERY_BUFFER_SIZE       (QUERY_INSERT_SIZE          \
                                + QUERY_TIME_SIZE           \
                                + QUERY_LOGGERNAME_SIZE     \
                                + QUERY_LEVEL_SIZE          \
                                + QUERY_MESSAGE_SIZE        \
                                + QUERY_ERROR_SIZE          \
                                + QUERY_MARKER_SIZE         \
                                + QUERY_THREADID_SIZE       \
                                + QUERY_THREADNAME_SIZE     \
                                + QUERY_PID_SIZE            \
                                + QUERY_SERVICENAME_SIZE    \
                                + QUERY_COMPUTER_SIZE)

typedef struct
{
    char host[32]; 
    char user[32];
    char passwd[32];
    char db[32];
    char tb[32];
    unsigned int port;
	MYSQL mysql;
} db_database;


int db_init(char *host, char *user, char *passwd, char *db, unsigned int port);
int db_open_connection();
int db_execute_nonquery(char *query, unsigned int length);
void db_close_connection();

int db_gettimestr(char *buffer, int size);
int db_gettablename(char *tablename, int size, char *time);

int db_create_logdb();
int db_use_logdb();
int db_create_logtb(char *tablename);
int db_insert_log(int level,
                  int threadId,
                  int pid,
                  char *time,
                  char *loggerName,
                  char *message,
                  char *error,
                  char *marker,
                  char *threadName,
                  char *serviceName,
                  char *computer);


#endif

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <mysql.h>
#include "datalog.h"

db_database database;

int db_init(char *host, char *user, char *passwd, char *db, unsigned int port)
{
    if (host == NULL 
        || user == NULL 
        || passwd == NULL 
        || db == NULL 
        || port < 0) {        
        return 1;
    }
    
    memset(&database, 0, sizeof(db_database));
    strcpy(database.host, host);
    strcpy(database.user, user);
    strcpy(database.passwd, passwd);
    strcpy(database.db, db);    
    database.port = port;  
    
    return (mysql_init(&database.mysql) == NULL);
}

int db_open_connection()
{
    return (mysql_real_connect(&(database.mysql), 
                               database.host, 
                               database.user, 
                               database.passwd, 
                               database.db, 
                               database.port, 
                               NULL, 0) == NULL);
}

int db_execute_nonquery(char *query, unsigned int length)
{
    return mysql_real_query(&(database.mysql), query, length);
}

void db_close_connection()
{
    mysql_close(&(database.mysql));
}

int db_gettimestr(char *buffer, int size)
{
    if (buffer == NULL || size < 0) {
        return 1;
    }
    
    time_t t = time(NULL);
    struct tm *ptm = localtime(&t);
    strftime(buffer, size, "%Y-%m-%d %H:%M:%S", ptm);
    return 0;
}

int db_gettablename(char *tablename, int size, char *time)
{
    if (tablename == NULL || time == NULL || size < 0) {
        return 1;
    }

    char year[8];
    char month[8];
    char day[8];

    memset(year, 0, sizeof(year));
    memset(month, 0, sizeof(month));
    memset(day, 0, sizeof(day));

    char *current = time;
    char *index = strchr(current, '-');
    strncpy(year, current, index - current);
    current = index + 1;

    index = strchr(current, '-');
    strncpy(month, current, index - current);
    current = index + 1;

    index = strchr(current, ' ');
    strncpy(day, current, index - current);
    current = index + 1;

    memset(tablename, 0, size);
    sprintf(tablename, "HA_Logging_%s%s%s", year, month, day);
    return 0;
}

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
                  char *computer)
{
    if (time == NULL) {
        return 1;
    }
    
	char tablename[32];
	db_gettablename(tablename, sizeof(tablename), time);
		
	char query[QUERY_BUFFER_SIZE];
	memset(query, 0, sizeof(query));
	
	char *end = query;
	end += sprintf(query, "INSERT INTO %s (Time,LoggerName,Level,Message,Error,Marker,ThreadId,ThreadName,Pid,ServiceName,Computer) VALUES (", tablename);
		
	/* Time */
    *end++ = '\'';
    end += mysql_real_escape_string(&(database.mysql), end, time, strlen(time));
    *end++ = '\'';
    *end++ = ',';
    /* LoggerName */
    *end++ = '\'';
    if (loggerName != NULL)
        end += mysql_real_escape_string(&(database.mysql), end, loggerName, strlen(loggerName));
    *end++ = '\'';
    *end++ = ',';
    /* Level */
    *end++ = '\'';
    end += sprintf(end, "%f", level);
    *end++ = '\'';
    *end++ = ',';
    /* Message */
    *end++ = '\'';
    if (message != NULL)
        end += mysql_real_escape_string(&(database.mysql), end, message, strlen(message));
    *end++ = '\'';
    *end++ = ',';
    /* Error */
    *end++ = '\'';
    if (error != NULL)
        end += mysql_real_escape_string(&(database.mysql), end, error, strlen(error));
    *end++ = '\'';
    *end++ = ',';
    /* Marker */
    *end++ = '\'';
    if (marker != NULL)
        end += mysql_real_escape_string(&(database.mysql), end, marker, strlen(marker));
    *end++ = '\'';
    *end++ = ',';
    /* ThreadId */
    *end++ = '\'';
    end += sprintf(end, "%f", threadId);
    *end++ = '\'';
    *end++ = ',';
    /* ThreadName */
    *end++ = '\'';
    if (threadName != NULL)
        end += mysql_real_escape_string(&(database.mysql), end, threadName, strlen(threadName));
    *end++ = '\'';
    *end++ = ',';
    /* Pid */
    *end++ = '\'';
    end += sprintf(end, "%f", pid);
    *end++ = '\'';
    *end++ = ',';
    /* ServiceName */
    *end++ = '\'';
    if (serviceName != NULL)
        end += mysql_real_escape_string(&(database.mysql), end, serviceName, strlen(serviceName));
    *end++ = '\'';
    *end++ = ',';
    /* Computer */
    *end++ = '\'';
    if (computer != NULL)
        end += mysql_real_escape_string(&(database.mysql), end, computer, strlen(computer));
    *end++ = '\'';
    *end++ = ')';
    *end++ = ';';
    
    int result = db_execute_nonquery(query, strlen(query));
    if (result == 2013) {
        result = db_open_connection();
        if (!result) {
            result = db_execute_nonquery(query, strlen(query));
        }
    }

	return result;
}

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <mysql.h>
#include <errmsg.h>
#include "datalog.h"

db_database database;

int db_init(char *host, char *user, char *passwd, char *db, unsigned int port)
{
    if (host == NULL 
        || user == NULL 
        || passwd == NULL 
        || db == NULL 
        || port < 0) {
        return -1;
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
    int result = (mysql_real_connect(&(database.mysql), 
                                     database.host, 
                                     database.user, 
                                     database.passwd, 
                                     NULL, 
                                     database.port, 
                                     NULL, CLIENT_MULTI_STATEMENTS) == NULL);
    if (result) {
        db_close_connection();
    }
    
    return result;
}

int db_execute_nonquery(char *query, unsigned int length)
{
    // printf("db_execute_nonquery:%s\n", query);
    int result = mysql_real_query(&(database.mysql), query, length);
    if (result == CR_SERVER_LOST) {
        result = db_open_connection();
        if (!result) {
            result = mysql_real_query(&(database.mysql), query, length);
			if (!result) {
				result = db_use_logdb();
			}
        }
    }
	
	return result;
}

void db_close_connection()
{
    mysql_close(&(database.mysql));
}

int db_gettimestr(char *buffer, int size)
{
    if (buffer == NULL || size < 0) {
        return -1;
    }
    
    time_t t = time(NULL);
    struct tm *ptm = localtime(&t);
    strftime(buffer, size, "%Y-%m-%d %H:%M:%S", ptm);
    return 0;
}

int db_gettablename(char *tablename, int size, char *time)
{
    if (tablename == NULL || time == NULL || size < 0) {
        return -1;
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

int db_create_logdb()
{
    if (database.db == NULL) {
        return -1;
    }
    
	char query[QUERY_BUFFER_SIZE];
	memset(query, 0, sizeof(query));
	sprintf(query, "CREATE DATABASE IF NOT EXISTS `%s` DEFAULT CHARACTER SET utf8;", 
	        database.db);

    return db_execute_nonquery(query, strlen(query));	
}

int db_use_logdb()
{
    if (database.db == NULL) {
        return -1;
    }
    
	char query[QUERY_BUFFER_SIZE];
	memset(query, 0, sizeof(query));
	sprintf(query, "USE `%s`;", database.db);
    return db_execute_nonquery(query, strlen(query));	
}

int db_create_logtb(char *tablename)
{
    if (tablename == NULL) {
        return -1;
    }

    char query[QUERY_BUFFER_SIZE];
	memset(query, 0, sizeof(query));
	sprintf(query, 
            "CREATE TABLE IF NOT EXISTS `%s` ("
            "`Time` DATETIME NOT NULL,"
            "`LoggerName` VARCHAR(256) NOT NULL,"
            "`Level` INT(11) NOT NULL,"
            "`Message` TEXT,"
            "`Error` TEXT,"
            "`Marker` VARCHAR(256) DEFAULT NULL,"
            "`ThreadId` INT(11) NOT NULL,"
            "`ThreadName` VARCHAR(128) NOT NULL,"
            "`Pid` INT(11) NOT NULL,"
            "`ServiceName` VARCHAR(128) DEFAULT NULL,"
            "`Computer` VARCHAR(128) DEFAULT NULL,"
            "KEY `IX_Time` (`Time`) USING BTREE,"
            "KEY `IX_LoggerName` (`LoggerName`) USING BTREE,"
            "KEY `IX_Marker` (`Marker`) USING BTREE"
            ") ENGINE=MYISAM DEFAULT CHARSET=utf8;", 
            tablename);
	
    return db_execute_nonquery(query, strlen(query));	
}

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
                  char *computer)
{
    if (time == NULL || strlen(time) > QUERY_TIME_SIZE) 
        return -1;
    if (loggerName != NULL && strlen(loggerName) > QUERY_LOGGERNAME_SIZE) 
        return -2;
    if (message != NULL && strlen(message) > QUERY_MESSAGE_SIZE) 
        return -3;
    if (error != NULL && strlen(error) > QUERY_ERROR_SIZE) 
        return -4;
    if (marker != NULL && strlen(marker) > QUERY_MARKER_SIZE) 
        return -5; 
    if (threadName != NULL && strlen(threadName) > QUERY_THREADNAME_SIZE) 
        return -6; 
    if (serviceName != NULL && strlen(serviceName) > QUERY_SERVICENAME_SIZE) 
        return -7;
    if (computer != NULL && strlen(computer) > QUERY_COMPUTER_SIZE) 
        return -8;
    
	char tablename[32];
	db_gettablename(tablename, sizeof(tablename), time);
	if (strcmp(database.tb, tablename)) {
        int result = db_create_logtb(tablename);
	    if (result) {
	        return result;
	    }
	    
	    memset(database.tb, 0, sizeof(database.tb));
	    strcpy(database.tb, tablename);
	}
	
	char query[QUERY_BUFFER_SIZE];
	memset(query, 0, sizeof(query));
	
	char *end = query;
	end += sprintf(query, 
                   "INSERT INTO %s (Time,LoggerName,Level,Message,Error,Marker,"
                   "ThreadId,ThreadName,Pid,ServiceName,Computer) VALUES (", 
                   database.tb);
		
	/* Time */
    *end++ = '\'';
    end += mysql_real_escape_string(&(database.mysql), end, 
                                    time, strlen(time));
    *end++ = '\'';
    *end++ = ',';
    /* LoggerName */
    *end++ = '\'';
    if (loggerName != NULL)
        end += mysql_real_escape_string(&(database.mysql), end, 
                                        loggerName, strlen(loggerName));
    *end++ = '\'';
    *end++ = ',';
    /* Level */
    *end++ = '\'';
    end += sprintf(end, "%d", level);
    *end++ = '\'';
    *end++ = ',';
    /* Message */
    *end++ = '\'';
    if (message != NULL)
        end += mysql_real_escape_string(&(database.mysql), end, 
                                        message, strlen(message));
    *end++ = '\'';
    *end++ = ',';
    /* Error */
    *end++ = '\'';
    if (error != NULL)
        end += mysql_real_escape_string(&(database.mysql), end, 
                                        error, strlen(error));
    *end++ = '\'';
    *end++ = ',';
    /* Marker */
    *end++ = '\'';
    if (marker != NULL)
        end += mysql_real_escape_string(&(database.mysql), end, 
                                        marker, strlen(marker));
    *end++ = '\'';
    *end++ = ',';
    /* ThreadId */
    *end++ = '\'';
    end += sprintf(end, "%d", threadId);
    *end++ = '\'';
    *end++ = ',';
    /* ThreadName */
    *end++ = '\'';
    if (threadName != NULL)
        end += mysql_real_escape_string(&(database.mysql), end, 
                                        threadName, strlen(threadName));
    *end++ = '\'';
    *end++ = ',';
    /* Pid */
    *end++ = '\'';
    end += sprintf(end, "%d", pid);
    *end++ = '\'';
    *end++ = ',';
    /* ServiceName */
    *end++ = '\'';
    if (serviceName != NULL)
        end += mysql_real_escape_string(&(database.mysql), end, 
                                        serviceName, strlen(serviceName));
    *end++ = '\'';
    *end++ = ',';
    /* Computer */
    *end++ = '\'';
    if (computer != NULL)
        end += mysql_real_escape_string(&(database.mysql), end, 
                                        computer, strlen(computer));
    *end++ = '\'';
    *end++ = ')';
    *end++ = ';';
    
    return db_execute_nonquery(query, strlen(query));
}

// -----------------------------------------------------------------------------

int main(int argc, char *argv[])
{                  
    printf("QUERY_BUFFER_SIZE:%d\n", QUERY_BUFFER_SIZE);
    // printf("strlen(NULL):%d\n", strlen(NULL));
    
	int result = 0;
	printf("datalog test...\n");

	result = db_init("192.168.199.8", "root", "amigo123", "mydb66666", 3306);
	if (result) {
		printf("db_init() failed...\n");
		return result;
	}
	
	printf("db_init() ok...\n");

	result = db_open_connection();
	if (result) {
		printf("db_open_connection() failed...\n");
		return result;
	}
	
	printf("db_open_connection() ok...\n");	
	

	result = db_create_logdb();
	if (result) {
		printf("db_create_logdb() failed:%d\n", result);
		return result;
	}
		
	printf("db_create_logdb() ok...\n");
	
	
	result = db_use_logdb();
	if (result) {
		printf("db_use_logdb() failed:%d\n", result);
		return result;
	}
		
	printf("db_use_logdb() ok...\n");
	
	
	
	char time[32];	
	db_gettimestr(time, 32);
	printf("the time is : %s\n", time);	
	
    char tablename[32];	
	db_gettablename(tablename, 32, time);
	printf("the tablename is : %s\n", tablename);
	
	
    
    result = db_insert_log(80000,
                           12345,
                           67890,
                           time,
                           "com.feinno.mcore.reg2",
                           "Reg2 M'essag\ne.",
                           "Reg2 E'rro\nr.",
                           "333333333",
                           "mcp-threadname",
                           "mcp-servicename",
                           "mcp-computername");
    if (result) {
        printf("db_insert_log() failed:%d\n", result);
		return result;
    }

    printf("db_insert_log() ok...\n");
   

	db_close_connection();
	printf("db_close_connection()...\n");	
	return 0;

}

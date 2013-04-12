/**************************************************
 * Created: 2013/4/6
 * Author: Leo_xy
 * Email: xy198781@sina.com
 * Last modified: 2013/4/12 12：00
 * Version: 0.1
 * File: src/common/log.h
 * Breif: 日志信息头文件。
 **************************************************/

#ifndef LOG_HEADER
#define LOG_HEADER

#include <stdio.h>
#include <time.h>
#include <pthread.h>

#include "constants.h"

extern FILE *log_file[MAX_THREAD_NUM + 1];
extern pthread_key_t log_id;

/* 目前先用标准输出实现，考虑到效率以后可以用其他方法 */
#define LOG_MSG(format,...) do { \
    time_t t;\
    time(&t);\
    FILE *log;\
    if (log = log_file[(int)pthread_getspecific(log_id)]) { \
    fprintf(log,"##%sFILE: "__FILE__", LINE: %d, "format"\n", \
            ctime(&t), __LINE__, ##__VA_ARGS__); \
    fflush(log); \
    } else { \
        fprintf(stdout,"##%sFILE: "__FILE__", LINE: %d, "format"\n", \
                ctime(&t), __LINE__, ##__VA_ARGS__); \
        fflush(stdout); \
        } \
    } while (0)


int
init_logger(char *, int);

int
close_logger(int);



#endif

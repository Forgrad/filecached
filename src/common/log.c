/**************************************************
 * Created: 2013/4/12
 * Author: Leo_xy
 * Email: xy198781@sina.com
 * Last modified: 2013/4/15 20：00
 * Version: 0.1
 * File: src/common/log.c
 * Breif: 日志信息代码。
 **************************************************/

#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>

#include "constants.h"
#include "log.h"

FILE *log_file[MAX_THREAD_NUM + 1];
pthread_key_t log_id;

int
init_logger(char *path, int thread_num)
{
    int i;
    char path_buf[MAX_PATH_LENGTH];
    strcpy(path_buf, path);
    char *ptr = path_buf + strlen(path_buf);
    if (ptr[-1] != '/') {
        *ptr++ = '/';               /* 添加/ */
        *ptr = 0;
    }
    strcat(path_buf, "thread");
    
    ptr = path_buf + strlen(path_buf);
    for (i = 0; i < thread_num; i++) {
        int pow = 1;
        int j = i / 10;
        while(j) {
            pow++;
            j /= 10;
        }
        j = i;
        ptr[pow--] = '\0';
        while (pow >= 0) {
            ptr[pow--] = '0' + (j % 10);
            j /= 10;
        }
        if (!(log_file[i] = fopen(strcat(path_buf, ".log"), "a"))) {
            return -1;
        }
    }
    log_file[MAX_THREAD_NUM] = stdout;
    return pthread_key_create(&log_id, NULL);
}

int
close_logger(int thread_num)
{
    int i, ret;
    for (i = 0; i < thread_num; i++)
    {
        if (log_file[i]) {
            ret = fclose(log_file[i]);
        }
    }
    pthread_key_delete(log_id);
    return ret;
}











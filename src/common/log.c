/**************************************************
 * Created: 2013/4/12
 * Author: Leo_xy
 * Email: xy198781@sina.com
 * Last modified: 2013/4/19 20：00
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
#include "common.h"

FILE *log_file[MAX_THREAD_NUM + 1]; /* log文件 */
pthread_key_t log_id;               /* 每线程log id */

/* 初始化log文件 */
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

    /* 构建相应的文件名 */
    if (get_process_id() == 0) {
        strcat(path_buf, "master");
    } else {
        strcat(path_buf, "slave");
        strcatn(path_buf, get_process_id());
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
        /* 打开文件，如不存在，则创建，目录之前应存在 */
        if (!(log_file[i] = fopen(strcat(path_buf, ".log"), "a"))) {
            return -1;
        }
    }
    /* 将最后一个文件指向stdout留给init函数 */
    log_file[MAX_THREAD_NUM] = stdout;

    /* 初始化每线程变量log id */
    return pthread_key_create(&log_id, NULL);
}

/* 关闭log文件 */
int
close_logger(int thread_num)
{
    int i, ret;
    /* 关闭log文件 */
    for (i = 0; i < thread_num; i++)
    {
        if (log_file[i]) {
            ret = fclose(log_file[i]);
            log_file[i] = NULL;
        }
    }

    /* 删除log id变量 */
    pthread_key_delete(log_id);
    return ret;
}

/* 关闭指定的log文件 */
int
close_log_file(int num)
{
    int ret = fclose(log_file[num]);
    log_file[num] = NULL;
    return ret;
}

/* 设置log id */
int
set_log_id(ssize_t id)
{
    return pthread_setspecific(log_id, (void *)id);
}









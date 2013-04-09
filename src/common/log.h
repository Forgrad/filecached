/**************************************************
 * Created: 2013/4/6
 * Author: Leo_xy
 * Email: xy198781@sina.com
 * Last modified: 2013/4/6 10：00
 * Version: 0.1
 * File: src/common/log.h
 * Breif: 日志信息所需要的一些定义。
 **************************************************/

#ifndef LOG_HEADER
#define LOG_HEADER

#include <stdio.h>
#include <time.h>

#define LOG_MSG(format,...) do { \
    time_t t;\
    time(&t);\
    fprintf(stdout,"##%sFILE: "__FILE__", LINE: %d, "format"\n",\
            ctime(&t), __LINE__, ##__VA_ARGS__);                \
        } while (0)

#endif

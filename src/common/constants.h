/**************************************************
 * Created: 2013/3/31
 * Author: Leo_xy
 * Email: xy198781@sina.com
 * Last modified: 2013/4/20 15:00
 * Version: 0.1
 * File: src/common/constants.h
 * Breif: 系统常量头文件。
 **************************************************/

#ifndef CONSTANTS_HEADER
#define CONSTANTS_HEADER

/* Misc */
#define HASH_SLOTS 200
#define MAX_PATH_LENGTH 128
#define MAX_SLAVES 20
#define MAX_BLOCKS 1
#define PAGEALIGN (8ul<<10)

/* 线程数量常量 */
#define MASTER_THREAD_NUM 5
#define SLAVE_THREAD_NUM 10
#define MAX_THREAD_NUM 10

/* 请求处理常量 */
#define MAX_REQUEST_TYPES 20
#define REQUEST_TAG 32767
#define SHARE_FILE_DIS_TAG 32766
#define REQUEST_BASE_TAG 32750
#define REQUEST_TAG_NUM 10

#endif


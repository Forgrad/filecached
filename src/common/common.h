/**************************************************
 * Created: 2013/4/2
 * Author: Leo_xy
 * Email: xy198781@sina.com
 * Last modified: 2013/4/11 22：00
 * Version: 0.1
 * File: src/common/common.h
 * Breif: 一些公用数据结构头文件。
 **************************************************/
#ifndef COMMON_HEADER
#define COMMON_HEADER

#include "hashtable.h"
#include "constants.h"

/* 数据块结构体 */
struct block
{
    int slave_id;              /* 存储该块的节点id */
    int block_id;              /* 该块为文件第几数据块 */
    unsigned long size;                /* 块大小 */
    unsigned long offset;              /* 块第一字节数据所在文件内偏移 */
};

/* 共享文件信息结构体 */
struct share_file
{
    struct hash_node hnode;     /* 哈希表节点，包含文件路径字符串 */
    unsigned long size;                /* 文件大小 */
    struct block blocks[MAX_BLOCKS]; /* 文件块信息数组 */
    int block_num;                  /* 文件块数量 */
};

/* 私有文件信息结构体 */
struct private_file
{
    struct hash_node hnode;     /* 哈希表节点，包含文件路径字符串 */
    unsigned long size;                /* 文件大小 */
    struct block blocks[MAX_BLOCKS]; /* 文件块信息数组 */
    int block_num;                  /* 文件块数量 */
};

/* 数据节点信息数据结构 */
struct slave_info
{
    int id;            /* 数据节点id */
    int alive;              /* 数据节点活跃标志 */
    unsigned long free;      /* 节点剩余存储空间数量 */
    unsigned long last_update;  /* 信息上次更新时间 */
    unsigned long connections;  /* 数据传输连接情况 */
 };
/* MPI包装share_files时传递的参数 */
struct pack_param
{
    MPI_Datatype mpi_share_file_type;
    char *buff;
    int position;
    int size;
};

#endif





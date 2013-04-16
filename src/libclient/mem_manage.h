/******************************
 * Created: 2013/4/2
 * Author: star_liu
 * Email: 327177726@qq.com
 * Last modified:2013/4/15
 * Version: 0.1
 * File: src/slavenode/mem_manage.h
 * Breif: Header for memory manager mechanism
 * ****************************/
#ifndef MEM_MANAGE_H
#define MEM_MANAGE_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include "../common/hashtable.h"

typedef struct managememory 
{
     size_t totalsize;/*预取内存空间总大小*/                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                 
     size_t freesize;/*剩余内存空间大小*/
     void *start;/*初始化内存开始指针*/
     void *mem_pos;/*当前移动到的指针位置*/
     pthread_mutex_t mutex;/*互斥量，避免多线程同时对freesize进行修改访问*/
}managememory;

typedef struct mem_node/*本内存管理中有一个hash表，mem_node是该表节点结构*/
{
     struct hash_node hnode;
     void *startpos;
     int iswritting;
     size_t size;
}mem_node;
  
/*在slave上初始化一块内存资源池，成功初始化返回0，失败则返回-1*/
int
mem_init(managememory *manager, size_t size);

/*动态共享数据载入前先向该内存资源池申请空间，如果有剩余空间则返回0，文件重命名则返回-1，空间不够返回-2*/
int 
mem_malloc(managememory *manager, char filename[], size_t size);

/*读取共享数据，成功读取返回读取的字节大小，没有该数据文件则返回-1*/
ssize_t 
mem_read(char filename[], size_t pos, size_t size, void *buf);

/*一次性写入共享数据，成功写入则返回写入的数据大小，写入的数据与之前分配的空间不匹配则写入失败返回-1*/
ssize_t 
mem_write(char filename[], size_t size, void *buf);

#endif


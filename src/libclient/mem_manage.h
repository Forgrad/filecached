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
     void *start;
     void *mem_pos;
     pthread_mutex_t mutex;/*互斥量，避免多线程同时对freesize进行修改访问*/
}managememory;


typedef struct mem_node
{
     struct hash_node hnode;
     void *startpos;
     int iswritting;
     size_t size;
}mem_node;
  
int
mem_init(managememory *manager, size_t size);

int 
mem_malloc(managememory *manager, char filename[], size_t size);


ssize_t 
mem_read(char filename[], size_t pos, size_t size, void *buf);

ssize_t 
mem_write(char filename[], size_t size, void *buf);

#endif

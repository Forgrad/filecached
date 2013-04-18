/******************************
 * Created: 2013/4/2
 * Author: star_liu
 * Email: 327177726@qq.com
 * Last modified:
 * Version: 0.1
 * File:  src/slavenode/mem_manage.c
 * Breif:  内存管理接口api
 * ****************************/

#include "mem_manage.h"
#include <sys/mman.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>


/*用于记录单机内存数据信息位置的哈希表，静态全局变量*/
static  HASH_TABLE(mem_hash);


/*内存管理初始化api,成功初始化内存空间则返回0，失败则返回-1*/
int mem_init(managememory *manager, size_t size)
{
     manager->start=malloc(size);
     if (NULL==manager->start) return -1;
     if (-1==mlock(manager->start,size)) {
          free(manager->start);
          return -1;
     }
     manager->mem_pos=manager->start;
     manager->freesize=size;
     manager->totalsize=size;
     pthread_mutex_init(&(manager->mutex),NULL);
     return 0;
}


/* 内存分配接口，内存不足返回-2，文件重名返回-1，成功分配返回0 */
int mem_malloc(managememory *manager, char filename[], size_t size)
{
     mem_node *node=malloc(sizeof(mem_node));
     INIT_HASH_NODE(&node->hnode,filename);
     if (hash_insert(&node->hnode,mem_hash) != &node->hnode) {
          free(node);
       return -1;
     }
     
     int isfull=0;
     pthread_mutex_lock(&(manager->mutex));
     /*对freesize的操作都利用互斥锁来让同一时间只能有一个线程对她进行访问或操作*/
     if (size>manager->freesize)isfull=1;
     else {
     manager->freesize-=size;
     node->startpos=manager->mem_pos;
     manager->mem_pos+=size;
     }
     pthread_mutex_unlock(&(manager->mutex));
     
     if (isfull) {
       hash_del(filename,mem_hash);
       free(node);
       return -2;
     }
     
     
     node->iswritting=1;
     node->size=size;
     
     return 0;
     
}


/*单机读取内部数据，远程请求内部数据的读取接口，有该文件数据块则返回读取的字节数，读取数据不存在或则超出了已有的范围则返回-1*/
ssize_t mem_read(char filename[], size_t pos, size_t size, void* buf)
{ 
     struct hash_node *node=hash_get(filename,mem_hash);
     if (NULL==node) return -1;/*没有该文件数据块*/
     mem_node *m_node=hash_entry(node,mem_node,hnode);
     size_t blocksize=m_node->size;
     if (pos>=blocksize || (pos+size)>blocksize)return -1;/*超过了读取的范围*/ 
     while (m_node->iswritting);/*对于共享文件而已不存在这个*/
     memcpy(buf,m_node->startpos+pos,size);
     return size;
     
}


/*向目标内存地址写入数据，写入数据量大于了原有分配的数据大小则返回-1，成功写入则返回写入的数据量大小
 这里是一次性的载入，不会有后续的追加操作*/
ssize_t mem_write(char filename[], size_t size, void* buf)
{
     struct hash_node *node=hash_get(filename,mem_hash);
     mem_node *m_node=hash_entry(node,mem_node,hnode);
     size_t mem_size=m_node->size;
     if (size>mem_size) return -1;
     memcpy(m_node->startpos,buf,size);
     m_node->iswritting=0;
     return size;

}

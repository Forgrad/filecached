/******************************
 * Created: 2013/4/2
 * Author: star_liu
 * Email: 327177726@qq.com
 * Last modified:
 * Version: 0.1
 * File: src/libclient/dfsio.h
 * Breif: 上层渲染应用数据访问接口头文件
 * ****************************/

#ifndef DFSIO_H
#define DFSIO_H

/*暂定单机上共享文件元数据哈系表是share_hash,临时文件元数据哈系表是temp_hash.
 *对应的hash节点是share_node,temp_node */


#include "../slavenode/mem_manage.h"


typedef struct dmf_file
{
     struct hash_node *hnode;
     size_t file_pos;
}dmf_file;


typedef struct share_node
{
     struct hash_node hnode;
     size_t size;
     char slave_id;
}share_node;



extern struct hlist_head share_hash[];/*共享文件元数据hash表初始化*/




char getprocessid(void);/*获得本地进程编号*/

/*由徐越提供的远程数据传送接口，先暂定此结构*/
ssize_t remote_read(char filenameblkid[], char slave_id, size_t pos, size_t size, void *buf)
{
     return 1;
}



/*以下都是上层应用所需要的接口api原型*/
dmf_file *
dmf_open(char filename[]);


ssize_t
dmf_read(void *buf, size_t size, size_t count, dmf_file *file);

char *
dmf_gets(char *buf, size_t size, dmf_file *file);

dmf_file *
dmf_create(char filename[]);

ssize_t
dmf_write(void *buf, size_t size, size_t count, dmf_file *file);

int
dmf_flush(dmf_file *file);


int
dmf_eof(dmf_file *file);

int 
dmf_close(dmf_file *file);

int
dmf_seek(dmf_file* file, size_t offset,int whence);

size_t
dmf_tell(dmf_file *file);

int
dmf_remove(char filename[]);

#endif

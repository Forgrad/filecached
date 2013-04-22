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


#include "../common/hashtable.h"

/*文件描述结构*/
typedef struct dmf_file
{
    struct hash_node *hnode;/*共享或临时文件元数据挂载的hash节点指针*/
    size_t file_pos;/*文件访问操作偏移*/
}dmf_file;

/*以下都是上层应用所需要的接口api原型*/

/*打开文件，如果成功打开则返回一个描述该文件的dmf_file指针，否则没有该文件则返回NULL*/
dmf_file *
dmf_open(char filename[]);

/*读取file对应的文件数据，成功读取则返回读取的字节数，读取失败则返回-1*/
ssize_t
dmf_read(void *buf, size_t size, size_t count, dmf_file *file);

/*读取指定文件数据的一行数据，遇到\n或文件结束或读取了size-1字节时结束，补上\0结束符，读取错误返回NULL*/
char *
dmf_gets(char *buf, size_t size, dmf_file *file);

dmf_file *
dmf_create(char filename[]);

ssize_t
dmf_write(void *buf, size_t size, size_t count, dmf_file *file);

int
dmf_flush(dmf_file *file);

/*判断文件是否到了文件数据的尾部*/
int
dmf_eof(dmf_file *file);

/*关闭一个文件描述符指针*/
int 
dmf_close(dmf_file *file);

/*改变file的文件操作位置，whence取值SEEK_CUR,SEEK_SET,SEEK_END等，相对位置的偏移位置*/
int
dmf_seek(dmf_file* file, size_t offset,int whence);

/*获得文件操作的位置偏移*/
size_t
dmf_tell(dmf_file *file);

int
dmf_remove(char filename[]);

#endif


/******************************
 * dmf_open返回NULL的原因：没有该文件存在
 * dmf_read返回-1可能原因：1）文件描述符或缓存区buff为NULL，2）读取的文件大小超出了已有的数据范围。
 * dmf_gets返回NULL原因：文件描述符或缓存区buff为NULL
 * dmf_seek返回-1可能原因：所要定位的位置超出了文件已有的范围
 * ****************************/



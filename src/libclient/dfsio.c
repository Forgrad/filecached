/******************************
 * Created: 2013/4/2
 * Author: star_liu
 * Email: 327177726@qq.com
 * Last modified:
 * Version: 0.1
 * File: src/libclient/dfsio.c
 * Breif: 上层渲染应用数据访问接口函数实现
 * ****************************/


#include "dfsio.h"

HASH_TABLE(share_hash);



/*打开文件描述符，dmf_file,有该文件信息则返回对应的dmf_file,没有则返回NULL*/
dmf_file* dmf_open(char filename[])
{ 
    struct hash_node *node;
    node=hash_get(filename,share_hash);
    if (NULL==node) return NULL;
      dmf_file *file=malloc(sizeof(dmf_file));
	    file->hnode=node;
	    file->file_pos=0;
    return file;
    
}


static ssize_t share_read(void *buf, size_t size, size_t count, dmf_file *file)
{
      share_node *node=hash_entry(file->hnode,share_node,hnode);
      if (file->file_pos>=node->size ||file->file_pos+size*count>node->size)
	  return -1;/*读取溢出的情况*/
      int is_here=node->slave_id==getprocessid()?1:0;
      ssize_t result;
      
      if (is_here) {
	    result=mem_read(node->hnode.str,file->file_pos,size*count,buf);
      } else {
	    result=remote_read(node->hnode.str,node->slave_id,file->file_pos,size*count,buf);
      }
      
      if (-2==result ||-1==result) return -1;
      file->file_pos+=result;
      return result;
      
}





/*返回真实读取的数据量大小，数据缓冲区或文件描述符为NULL或则读取的数据量大小超出了已经有的就则返回-1*/
ssize_t dmf_read(void* buf, size_t size, size_t count, dmf_file* file)
{
     if (NULL==buf || NULL==file) return -1;
     return share_read(buf,size,count,file);
}


/*读取文件数据中一行的数据，成功则返回buf指针，失败则返回NULL*/
char* dmf_gets(char* buf, size_t size, dmf_file* file)
{
     if (NULL==file ||NULL==buf)return NULL;
     share_node *node=hash_entry(file->hnode,share_node,hnode);
     size_t file_have=node->size-file->file_pos;
     size_t realsize=size-1<=file_have?size-1:file_have;
     char temp[realsize];
     int result;
     if (node->slave_id==getprocessid()) 
         result=mem_read(node->hnode.str,file->file_pos,realsize,temp);
     else 
         result=remote_read(node->hnode.str,node->slave_id,file->file_pos,realsize,temp);
     if (-1==result) return NULL;
     size_t cpy_size;
     for (cpy_size=0;cpy_size<realsize;cpy_size++)
         if ('\n'==temp[cpy_size]) break;
     memcpy(buf,temp,cpy_size);
     if (cpy_size<realsize) buf[cpy_size++]='\n';
     buf[cpy_size]='\0';
     file->file_pos+=cpy_size;
     return buf;
     
}



/*关闭一个文件流，释放文件指针*/
int dmf_close(dmf_file* file)
{
     if (NULL!=file) free(file);
     return 1;
}

/*判断是否到文件的末尾了，到则返回1，否则返回0*/
int dmf_eof(dmf_file* file)
{
     share_node *s_node=hash_entry(file->hnode,share_node,hnode);
     size_t size=s_node->size;
     return (file->file_pos>=size)?1:0;
}


/*给文件流重新进行定位，定位到给定的偏移位置，定位失败返回-1，定位成功返回0，其中whence是相对于这个位置，取三个值*/
int dmf_seek(dmf_file* file, size_t offset, int whence)
{
    int result=0;
    ssize_t where;
    share_node *s_node=hash_entry(file->hnode,share_node,hnode);
    size_t size=s_node->size;
    switch (whence) {
      case SEEK_CUR:
	 where=file->file_pos+offset;
	if (where<0 || where>size) {
	  result=-1;
	  break;
	}
	file->file_pos+=offset;
        break;
      case SEEK_SET:
	if (offset<0 || offset>size) {
	  result=-1;
	  break;
	}
	file->file_pos=offset;
	break;
      case SEEK_END:
	 where=size+offset;
	if (where<0 || where>size) {
	  result=-1;
	  break;
	}
	file->file_pos=where;
	break;
    }
    return result;

}

/*返回此时文件流操作的偏移位置*/
size_t dmf_tell(dmf_file* file)
{
     return file->file_pos;
}


char getprocessid(void )
{
     return 'h';
}

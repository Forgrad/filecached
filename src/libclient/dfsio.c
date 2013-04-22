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




dmf_file* dmf_open(char filename[])
{ 
    struct hash_node *node;
    node=hash_get(filename,share_files);/*在哈希表中获得filename对应的哈希节点*/
    if (NULL==node) return NULL;
    dmf_file *file=malloc(sizeof(dmf_file));
    file->hnode=node;
    file->file_pos=0;
    return file;
    
}




static ssize_t share_read(void *buf, size_t size, size_t count, dmf_file *file)
{
      struct share_file *node=hash_entry(file->hnode,struct share_file,hnode);/*获取相应的存放共享数据信息的结构*/
      if (file->file_pos>=node->size ||file->file_pos+size*count>node->size)
      return -1;/*读取溢出的情况*/
      int is_here=node->blocks[0].slave_id==getprocessid()?1:0;/*判断共享文件是否在本地*/
      ssize_t result;
      
      if (is_here) {
        result=mem_read(node->hnode.str,file->file_pos,size*count,buf);/*在本地就本地接口调用读取*/
      } else {
        result=remote_read(node->hnode.str,node->blocks[0].slave_id,file->file_pos,size*count,buf);/*不在本地则远程接口调用读取*/
      }
      
      if (-2==result ||-1==result) return -1;/*读取失败，可能是读取的数据量超出了已有文件的范围*/
      file->file_pos+=result;
      return result;
      
}






ssize_t dmf_read(void* buf, size_t size, size_t count, dmf_file* file)
{
     if (NULL==buf || NULL==file) return -1;
     return share_read(buf,size,count,file);
}





char* dmf_gets(char* buf, size_t size, dmf_file* file)
{
     if (NULL==file ||NULL==buf)return NULL;
     struct share_file *node=hash_entry(file->hnode,struct share_file,hnode);
     size_t file_have=node->size-file->file_pos;
     size_t realsize=size-1<=file_have?size-1:file_have;
     char temp[realsize];
     int result;
     if (node->blocks[0].slave_id==getprocessid()) 
         result=mem_read(node->hnode.str,file->file_pos,realsize,temp);
     else 
         result=remote_read(node->hnode.str,node->blocks[0].slave_id,file->file_pos,realsize,temp);
     if (-1==result) return NULL;
     size_t cpy_size;
     for (cpy_size=0;cpy_size<realsize;cpy_size++)
         if ('\n'==temp[cpy_size]) break;/*判断字符串中是否有\n*/
     memcpy(buf,temp,cpy_size);
     if (cpy_size<realsize) buf[cpy_size++]='\n';/*添加\n到buf末尾*/
     buf[cpy_size]='\0';
     file->file_pos+=cpy_size;
     return buf;
     
}




int dmf_close(dmf_file* file)
{
     if (NULL!=file) free(file);
     return 1;
}


int dmf_eof(dmf_file* file)
{
     struct share_file *s_node=hash_entry(file->hnode,struct share_file,hnode);
     size_t size=s_node->size;
     return (file->file_pos>=size)?1:0;
}


/*给文件流重新进行定位，定位到给定的偏移位置，定位失败返回-1，定位成功返回0，其中whence是相对于这个位置，取三个值*/
int dmf_seek(dmf_file* file, size_t offset, int whence)
{
    int result=0;
    ssize_t where;
    struct share_file *s_node=hash_entry(file->hnode,struct share_file,hnode);
    size_t size=s_node->size;
    switch (whence) {
      case SEEK_CUR:/*文件指针当前位置*/
     where=file->file_pos+offset;
    if (where<0 || where>size) {
      result=-1;
      break;
    }
    file->file_pos+=offset;
        break;
      case SEEK_SET:/*文件开头*/
    if (offset<0 || offset>size) {
      result=-1;
      break;
    }
    file->file_pos=offset;
    break;
      case SEEK_END:/*文件结尾，可读字符的下一位*/
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

ssize_t remote_read(char filenameblkid[], int slave_id, size_t pos, size_t size, void* buf)
{
     return 1;
}

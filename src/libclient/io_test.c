#include "dfsio.h"


/*dfsio的各个接口测试程序如下*/

HASH_TABLE(share_files);/*共享数据元数据哈希表初始化*/

#define MAX_MEMORY 20*1024*1024
#define FILE_SIZE 1024*1024
char map[28]="abcdefghijklm\nnopqrstuvwxyz";/*存储文件数据为26个字母*/

int main(void)
{
     managememory manager;
     mem_init(&manager,MAX_MEMORY);
     
     /*模拟共享数据的载入过程*/
     char filename[7]="file";/*各个文件名前缀*/
     int file_i=1;
     while (file_i<=20) {
        char i[3]="";
  sprintf(i,"%d",file_i);
    memcpy(filename+4,i,3);
    struct share_file *file_node=malloc(sizeof(struct share_file));
    INIT_HASH_NODE(&file_node->hnode,filename);
    hash_insert(&file_node->hnode,share_files);
    mem_malloc(&manager,filename,FILE_SIZE);
    mem_write(filename,27,map);
    file_node->size=FILE_SIZE;
    file_node->block_num=1;
    file_node->blocks[0].slave_id=1;
    file_node->blocks[0].size=FILE_SIZE;
    file_i++;
     }
     /*读取相应的文件数据信息并显示*/
     file_i=1;
     char read_buf[27];
     while (file_i<=40) {
        char i[3]="";
    sprintf(i,"%d",file_i);
    memcpy(filename+4,i,3);
    dmf_file *file=dmf_open(filename);
    if (NULL==file) {
      printf("file%d is not exist\n",file_i);
      file_i++;
      continue;
    }
    dmf_read(read_buf,sizeof(char),27,file);
    printf("file%d is: %s\n",file_i,read_buf);
    dmf_close(file);
    file_i++;
     }
     
     /*使用seek定位，依次读取文件‘1’中的各个字符*/
     memcpy(filename+4,"1",2);
     dmf_file *file=dmf_open(filename);
     dmf_seek(file,4,SEEK_CUR);
     while (!dmf_eof(file)) {
       char e;
       dmf_read(&e,sizeof(char),1,file);
       printf("%c",e);
     }
     printf("\n");
     
     /*读取一行文件数据接口测试*/
     dmf_seek(file,0,SEEK_SET);
     char buf[30];
     dmf_gets(buf,30,file);
     printf("%s\n",buf);
     
     dmf_close(file);
     
       
     
     return 0;
}

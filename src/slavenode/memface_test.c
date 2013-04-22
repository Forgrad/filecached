#include "mem_manage.h"
#include <string.h>
#define MAX_MEMORY 20*1024*1024
#define FILE_SIZE 1024*1024
char map[27]="abcdefghijklmnopqrstuvwxyz";/*存储文件数据为26个字母*/

/*mem_manage接口的测试用例如下*/
int main(void)
{
     managememory man;/*申明一个内存管理对象*/
     
     mem_init(&man,MAX_MEMORY);/*对内存池进行初始化MAX_MEMORY大小的内存空间*/
     
     /*静态载入20个文件到本地slave中*/
     char filename[7]="file";/*各个文件名前缀*/
     int file_i=1;
     while (file_i<=30) {
        char i[3]="";
  sprintf(i,"%d",file_i);
    memcpy(filename+4,i,3);
    int result=mem_malloc(&man,filename,FILE_SIZE);/*为filename在内存池分配FILE_SIZE大小的空间*/
    if (-1==result) {
      printf("rename the file%d\n",file_i);
      file_i++;
      continue;/*申请的文件在内存池的hash表项中重名了*/
    } else if (-2==result) {
      printf("memory is full for file%d\n",file_i);
      file_i++;
      continue;/*申请的文件空间大小超出了内存池子已有的剩余空间*/
    }
    
    result=mem_write(filename,27,map);
    if (-1==result)
      printf("file%d error write size\n",file_i);
    else 
      printf("file%d write success\n",file_i);
    file_i++;
     }
     
     
     
     /*读取存储在内存池中对应文件的数据，并显示*/
     char read_buf[100]="";/*存放读取数据的缓存区*/
     file_i=26;
     while (file_i>0) {
        char i[3]="";
    sprintf(i,"%d",file_i);
        memcpy(filename+4,i,3);
    int result=mem_read(filename,file_i,1,read_buf);/*读取指定位置的指定文件的数据*/
    if (-1==result) {/*不存在该数据返回-1*/
      printf("file%d is not exist\n",file_i);
      file_i--;
      continue;
    } else {
      printf("file%d is :%s\n",file_i,read_buf);
    }
    file_i--;
     }
     
     return 0;
}

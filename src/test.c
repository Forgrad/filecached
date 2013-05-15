#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/time.h>
#include <unistd.h>

#include "common/common.h"
#include "libclient/system.h"
#include "libclient/dfsio.h"

//#define ONE_TEST 1
#define MAX_FILE_PATH 256
#define TEST_FILE_PREFIX "/home/xuyue/test_files/"
#define MAX_ACCESS_TREHAD 20


struct parm
{
    int id;
    int count;
    char *file_type;
};

void *
test_fun(void *parm)
{
    int process_id = get_process_id();
    int thread_id = ((struct parm *)parm)->id;
    int count = ((struct parm *)parm)->count;
    char *file_type = ((struct parm *)parm)->file_type;
    char file_path[MAX_FILE_PATH] = TEST_FILE_PREFIX;
    strcat(file_path, "test");
    strcat(file_path, file_type);
    strcat(file_path, "/");
    strcat(file_path, file_type);
    strcat(file_path, "_");
    
    dmf_file *dmf_files[count];
    #ifdef ONE_TEST
    int fds[count];
    #endif
    char *buff[count];
    size_t sizes[count];

    printf("slave %d thread %d start to open file!\n", process_id, thread_id);
    int i;
    for(i = 0; i < count; i++) {
        char file[MAX_FILE_PATH];
        strcpy(file, file_path);
        strcatn(file, i);
        if((dmf_files[i] = dmf_open(file)) == NULL) {
            printf("slave %d thread %d  dmf_open failed!\n", process_id, thread_id);
            return (void *)-1;
        }
        #ifdef ONE_TEST
        /* 需要在#include <fcntl.h>之前#define _GNU_SOURCE，NFS无对齐限制 */
        if((fds[i] = open(file, O_RDONLY | O_DIRECT | O_SYNC)) == -1) {
            printf("slave %d thread %d open failed!\n", process_id, thread_id);
            return (void *)-2;
        }
        #endif
        struct share_file *share_file = (struct share_file *)dmf_files[i]->file_info;
        sizes[i] = share_file->size;
        buff[i] = (char *)malloc(sizes[i]);
        memset(buff[i], 0, sizes[i]);
	mlock(buff[i], sizes[i]);
    }

    struct timeval start, end;
    double timeuse;
    
    #ifdef ONE_TEST
    printf("slave %d thread %d start to read file from disk!\n", process_id, thread_id);
    gettimeofday(&start, NULL);
    for(i = 0; i < count; i++) {
        if (sizes[i] != read(fds[i], buff[i], sizes[i])) {
            printf("slave %d thread %d disk read error!\n", process_id, thread_id);
            return (void *)-3;
        }
    }
    gettimeofday(&end, NULL);
    timeuse = 1000000 * ( end.tv_sec - start.tv_sec ) + end.tv_usec - start.tv_usec;
    timeuse /= 1000;
    printf("use %lf microseconds to read %d %s files from disk\n",timeuse, count, file_type);
    for(i = 0; i < count; i++) {
        close(fds[i]);
    }
    #endif

    printf("slave %d thread %d start to read file from dmf!\n", process_id, thread_id);
    gettimeofday(&start, NULL);
    for(i = 0; i < count; i++) {
        if(sizes[i] != dmf_read(buff[i], sizes[i], 1, dmf_files[i])) {
            printf("slave %d thread %d dmf read error!\n", process_id, thread_id);
            return (void *)-4;
            }
    }
    gettimeofday(&end, NULL);
    timeuse = 1000000 * ( end.tv_sec - start.tv_sec ) + end.tv_usec - start.tv_usec;
    timeuse /= 1000;
    printf("use %lf microseconds to read %d %s files from dmf\n",timeuse, count, file_type);
    for(i = 0; i <count; i++) {
        dmf_close(dmf_files[i]);
    }

    printf("slave %d thread %d start to write output!\n", process_id, thread_id);
    for(i = 0; i < count; i++) {
        char output_name[MAX_FILE_PATH];
        sprintf(output_name,"output/%s_%d.%d-%d", file_type, i, process_id, thread_id);
        FILE *fp;
        if((fp = fopen(output_name, "w+")) == NULL) {
            printf("slave %d thread %d open output file error!\n", process_id, thread_id);
            return (void *)-5;
        }
        size_t size = fwrite(buff[i], 1, sizes[i], fp);
        if(sizes[i] != size) {
            printf("slave %d thread %d output write error size need %d get %d!\n", process_id, thread_id, sizes[i], size);
            return (void *)-6;
        }
        fclose(fp);
    }
    printf("slave %d thread %d output writed!\n", process_id, thread_id);
    return (void *)0;
}

int
main(int argc, char **argv)
{
    if(argc != 5) {
        printf("usage: test pathname filetype count parallelnum\n");
        return -1;
    }
    int provided;
    int myid, numprocs;
    MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);
    MPI_Comm_rank(MPI_COMM_WORLD, &myid);
    MPI_Comm_size(MPI_COMM_WORLD, &numprocs);

    if(myid == 0)
    {
        int load_ret;
        load_ret = init_dmf_master(numprocs - 1, argv[1], 0, 0);
        if(load_ret != 0)
        {
            printf("init_dmf_master failed!\n");
            return -2;
        }
    }else{
        int load_ret;
        load_ret = init_dmf_slave(1999999999);
        if(load_ret != 0)
        {
            printf("init_dmf_slave failed!\n");
            return -3;
        }
        pthread_t tid;

        int parallelnum = atoi(argv[4]);
        int thread_num = parallelnum / (numprocs - 1);
        int res = parallelnum % (numprocs - 1);
        if(myid <= res) {
            thread_num += 1;
        }
        static struct parm parm[MAX_ACCESS_TREHAD];
        
        int i;
        for(i = 1; i <= thread_num; i++)
        {
            parm[i - 1].file_type = argv[2];
            parm[i - 1].id = i;
            parm[i - 1].count = atoi(argv[3]);
            load_ret = pthread_create(&tid, NULL, test_fun, (void *)(&parm[i-1]));
            if (load_ret != 0)
            {
                printf("Thread initial fail!\n");
                return -4;
            }
        }
    }
    sleep(10000);
    MPI_Finalize();
}


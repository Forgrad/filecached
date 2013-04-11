/**************************************************
 * Created: 2013/4/4
 * Author: Leo_xy
 * Email: xy198781@sina.com
 * Last modified: 2013/4/11 22：00
 * Version: 0.1
 * File: src/master/master.c
 * Breif: master节点主进程及相关函数代码。
 **************************************************/

#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>
#include <mpi.h>

#include "../common/hashtable.h"
#include "../common/common.h"
#include "../common/constants.h"
#include "../common/log.h"
#include "../libclient/system.h"
#include "../common/request.h"


static int slave_num = 0;       /* 初始slave数量 */
static struct slave_info slaves[MAX_SLAVES] = {}; /* slave信息数组 */
static pthread_mutex_t lock_slaves = PTHREAD_MUTEX_INITIALIZER; /* slave数组的互斥锁 */
static HASH_TABLE(share_files); /* 共享文件哈希表 */
static pthread_mutex_t lock_share_files = PTHREAD_MUTEX_INITIALIZER; /* 共享文件哈希表的互斥锁 */
static pthread_cond_t slaves_complete_cond = PTHREAD_COND_INITIALIZER;
static pthread_t tid[MASTER_THREAD_NUM] = {};

/* 系统初始化时调用此函数载入共享文件 */
static int
load_files(const char *fullpath, int (*func)(char *))
{
    struct stat statbuf;
    char *ptr;
    struct dirent *dirp;
    DIR *dp;
    static char path[MAX_PATH_LENGTH];
    int ret;
    int outer = 0;
    static int through = 0;
    static int file_loaded = 0;

    if (through == 0) {
        through = 1;
        outer = 1;
        strcpy(path, fullpath);
    }
    
    if (stat(path, &statbuf) < 0) { /* stat失败 */
        LOG_MSG("IN FUNC loadfiles: ERROR: stat error for %s\n", path);
        return -1;
    }
    
    if (S_ISDIR(statbuf.st_mode) == 0) { /* 如果不是目录 */
        int func_ret = func(path);
        if(func_ret == 0) {
            file_loaded++;
        }
            return func_ret;
    }
    
    ptr = path + strlen(path);  /* 指向路径末端‘\0’ */
    if (ptr[-1] != '/') {
        *ptr++ = '/';               /* 添加/ */
        *ptr = 0;
    }

    if ((dp = opendir(path)) == NULL) { /* 打开目录失败 */
        LOG_MSG("IN FUNC loadfiles： ERROR: can't read directory %s\n", path);
        return -2;
    }

    while ((dirp = readdir(dp)) != NULL) {
        /* 忽略本身及父目录 */
        if (strcmp(dirp->d_name, ".") == 0 ||   \
            strcmp(dirp->d_name, "..") == 0) {
            continue;
        }
        strcpy(ptr, dirp->d_name);

        if ((ret = load_files(path, func)) != 0) {
            break;
        }
    }

    ptr[-1] = 0;

    if (closedir(dp) < 0) {
        LOG_MSG("IN FUNC loadfiles：ERROR: can't close directory %s\n", path);
        return -3;
    }
    if (outer) {
        printf("file loaded: %d\n", file_loaded);
    }
    return ret;
}

static inline struct share_file*
init_share_file_node(char* file, struct stat *statbuf)
{
    struct share_file *file_node = (struct share_file *)malloc(sizeof(struct share_file));
    if (!file_node) {
        return NULL;
    }
    INIT_HASH_NODE(&file_node->hnode, file);
    file_node->size = statbuf->st_size;
    file_node->block_num = 0;
    return file_node;
}

/* 选择一个合适的节点，目前仅根据剩余空间大小选择 */
/* ！！需要信号量保护，待实现 */
static inline int
choose_slave(size_t size)
{
    int slave = -1;
    unsigned long free_space = 0;
    int i;
    for (i = 0; i < slave_num; i++) {
        if (slaves[i].alive && slaves[i].free > free_space && slaves[i].free >= size) {
            slave = i;
            free_space = slaves[i].free;
        }
    }
    return slave;
}

static inline struct block *
fill_block(struct block *block, size_t offset, size_t size, int block_id, int slave_id)
{
    block->size = size;
    block->offset = offset;
    block->slave_id = slave_id;
    block->block_id = block_id;
    return block;
}

/* 请求slave载入数据块，待实现 */
static int
block_load_req(char *file, struct block *block)
{
    return 0;
}

/* 分配文件的存储位置 */
static int
distribute_file(char *file, struct stat *statbuf)
{
    if (!file || !statbuf) {
        LOG_MSG("IN FUNC distribute_file： ERROR: NULL file name or statbuf!\n");
        return -1;
    }

    struct share_file *file_node = init_share_file_node(file, statbuf);
    if (!file_node) {
        LOG_MSG("IN FUNC distribute_file: ERROR: share_file node init error!\n");
        return -2;
    }
    file_node->size = statbuf->st_size;
    int slave;
    slave = choose_slave(file_node->size);
    if (slave < 0) {
        LOG_MSG("IN FUNC distribute_file: ERROR: no propriate slave\n");
        return -3;
    }
    /* 载入文件不分块 */
    int req_ret;
    req_ret = block_load_req(file_node->hnode.str, fill_block(&file_node->blocks[0], 0, file_node->size, 1, slave));
    if (req_ret) {
        LOG_MSG("IN FUNC distribute_file: ERROR: block load request failed\n");
        return -4;
    }
    file_node->block_num = 1;
    slaves[slave].free -= file_node->size; /* ！！需要信号量保护，待实现 */
    slaves[slave].last_update = time(NULL);
    if (&file_node->hnode != hash_insert(&file_node->hnode, share_files)) { /* !!!信号量保护设置位置，在插入函数中还是在外 */
        LOG_MSG("IN FUNC distribute: ERROR: file hash failed\n");
        return -5;
    }
    LOG_MSG("IN FUNC distribute: file %s load success!\n", file_node->hnode.str);
    return 0;
}

/* 载入文件信息，并分发文件 */
static inline int
load_file(char *file)
{
    struct stat statbuf;
    LOG_MSG("IN FUNC load_file: load file %s\n", file);
    if (stat(file, &statbuf) < 0) {
        LOG_MSG("IN FUNC load_file: stat error for %s\n", file);
    }
    LOG_MSG("IN FUNC load_file: file %s size %ld bytes\n", file, statbuf.st_size);
    return distribute_file(file, &statbuf);
}

/* 打印共享文件信息 */
static inline int
list_share_file(struct hash_node *node, void *count)
{
    struct share_file *file = hash_entry(node, struct share_file, hnode);
    printf("file name: %s\n", file->hnode.str);
    printf("file size: %lu\n", file->size);
    printf("file block num: %d\n", file->block_num);
    printf("block size: %lu\n", file->blocks[0].size);
    printf("block offset: %lu\n", file->blocks[0].offset);
    printf("block loc: slave %d\n", file->blocks[0].slave_id);
    (*((int *)count))++;
    return 0;
}

/* 列出所有共享文件 */
static inline int
list_share_files(void)
{
    unsigned int file_num = 0;
    int ret;
    ret = for_each_hash_entry(share_files, list_share_file, &file_num);
    printf("--------total files: %d-------\n", ret ? -1 : file_num);
    return 0;
}

/* 列出slave节点信息 */
static inline int
list_slave_status(void)
{
    printf("\n--------slave num: %d--------", slave_num);
    int i;
    for (i = 0; i < slave_num; i++) {
        printf("\nslave: %d :\n", i);
        printf("is alive: %d\n", slaves[i].alive);
        printf("free: %lu bytes\n", slaves[i].free);
        printf("last update: %s\n", ctime(&slaves[i].last_update));
    }
    return 0;
}

/* 向slave节点分发共享文件元数据表 */
static int
distribute_share_files_map(void)
{
    return 0;
}

static int
update_slave_info(struct slave_info_req *slave)
{
    slaves[slave->id].id = slave->id;
    slaves[slave->id].alive = slave->alive;
    slaves[slave->id].free = slave->free;
    slaves[slave->id].last_update = time(NULL);
    slaves[slave->id].connections = slave->connections;
    return 0;
}

static int
handle_slave_report(struct request *request, MPI_Status *status)
{
    struct slave_info_req slave;
    MPI_Datatype mpi_slave_info_type;
    MPI_Status stat;
    build_mpi_type_slave_info(&slave, &mpi_slave_info_type);
    MPI_Recv(&slave, 1, mpi_slave_info_type, status->MPI_SOURCE, request->tag, MPI_COMM_WORLD, &stat);
    update_slave_info(&slave);
    return 0;
}

static int
handle_file_open(struct request *request, MPI_Status *status)
{
    char file_name[MAX_PATH_LENGTH];
    MPI_Status stat;
    MPI_Recv(&file_name, MAX_PATH_LENGTH, MPI_CHAR, status->MPI_SOURCE, request->tag, MPI_COMM_WORLD, &stat);
    MPI_Datatype mpi_share_file_type;
    build_mpi_type_share_file(0, &mpi_share_file_type);
    struct hash_node *hnode = hash_get(file_name, share_files);
    struct share_file_req file_req = {
        .block_num = -1
    };
    if (hnode) {
        struct share_file *file = hash_entry(hnode, struct share_file, hnode);
        strcpy(file_req.name, file_name);
        file_req.size = file->size;
        file_req.block_num = file->block_num;
        memcpy(file_req.blocks, file->blocks, sizeof(file_req.blocks));
    }
    MPI_Send(&file_req, 1, mpi_share_file_type, status->MPI_SOURCE, request->tag, MPI_COMM_WORLD);
}

static request_handler request_handlers[MAX_REQUEST_TYPES] = {
    handle_slave_report
};


/* master节点请求监听线程 */
static void *
master_listen_thread(void *param)
{
    struct request request;
    MPI_Datatype mpi_requst_type;
    MPI_Status status;
    build_mpi_type_request(&request, &mpi_requst_type);
    while (1) {
    MPI_Recv(&request, 1, mpi_requst_type, MPI_ANY_SOURCE, REQUEST_TAG, MPI_COMM_WORLD, &status);
    MPI_Send(&request, 1, mpi_requst_type, status.MPI_SOURCE, request.tag, MPI_COMM_WORLD);
    request_handlers[request.request](&request, &status);
    }
    return (void *)0;
}

/* master主服务线程 */
static void *
master_main_thread(void *param)
{
    int i, ret;
    for (i = 1; i < MASTER_THREAD_NUM; i++) {
        ret = pthread_create(&tid[i], NULL, master_listen_thread, (void *)i);
     }
    if (ret != 0) {
        return (void *)ret;
    }
    pthread_exit((void *)0);
}


/* 启动master主线程，完成相关初始化。 */
int
init_dmf_master(int slaves, char *files, \
                unsigned long mem_size, \
                unsigned long file_size)
{
    LOG_MSG("IN FUNC init_dmf_master: INFO: master initializeing!\n");
    int ret;
    ret = pthread_create(&tid[0], NULL, master_main_thread, NULL);
    if (ret != 0) {
        LOG_MSG("IN FUNC init_dmf_master: ERROR: failed to init master threads!\n");
        return ret;
    }
    pthread_mutex_lock(&lock_slaves);
    while (slave_num != slaves) {
        pthread_cond_wait(&slaves_complete_cond, &lock_slaves);
    }
    pthread_mutex_unlock(&lock_slaves);
    ret = load_files(files, load_file);
    if (ret != 0) {
        LOG_MSG("IN FUNC init_dmf_master: ERROR: failed to load files!\n");
        return ret;
    }
    ret = distribute_share_files_map();
    if (ret != 0) {
        LOG_MSG("IN FUNC init_dmf_master: ERROR: failed to distribute file maps!\n");
    }
    list_share_files();
    list_slave_status();
    return ret;
}


/* 用作测试 */
int
main(int argc, char **argv)
{
    if (argc != 2 ) {
        printf("usage: %s pathname\n", argv[0]);
        return -1;
    }
    slaves[0].alive = 1;
    slaves[0].free = 1000000000ul;
    slaves[1].alive = 1;
    slaves[1].free = 1000000000ul;
    slave_num = 2;
    int load_ret;
    load_ret = load_files(argv[1], load_file);

    if (load_ret < 0) {
        printf("ERROR: Load failed!\n");
        return load_ret;
    }
    list_share_files();
    list_slave_status();
}


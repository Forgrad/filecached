/**************************************************
 * Created: 2013/4/4
 * Author: Leo_xy
 * Email: xy198781@sina.com
 * Last modified: 2013/4/21 23：00
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
#include "../common/tag.h"


static int slave_num = 0;       /* 初始slave数量 */
static struct slave_info slaves[MAX_SLAVES + 1] = {}; /* slave信息数组， slave进程从1开始编号 */
static pthread_mutex_t lock_slaves = PTHREAD_MUTEX_INITIALIZER; /* slave数组的互斥锁 */
static HASH_TABLE(share_files) = {}; /* 共享文件哈希表 */
static unsigned long share_file_num = 0; /* 哈希表项数 */
static pthread_mutex_t lock_share_files = PTHREAD_MUTEX_INITIALIZER; /* 共享文件哈希表的互斥锁 */
static pthread_cond_t slaves_complete_cond = PTHREAD_COND_INITIALIZER; /* 节点注册完成条件变量 */
static pthread_t tid[MASTER_THREAD_NUM] = {}; /* master线程tid */
static tag_pool tags = {};                    /* 请求tag池 */
static pthread_mutex_t lock_tags = PTHREAD_MUTEX_INITIALIZER; /* 保护请求tag池的互斥量 */

/* 系统初始化时调用此函数载入共享文件 */
static int
load_files(const char *fullpath, int (*func)(char *))
{
    struct stat statbuf;
    char *ptr;
    struct dirent *dirp;
    DIR *dp;
    static char path[MAX_PATH_LENGTH]; /* 路径名的本地缓冲 */
    int ret;
    int outer = 0;              /* 指示本次执行是否递归中最外层调用 */
    static int through = 0;     /* 指示函数是否被调用过 */
    static int file_loaded = 0; /* 已经载入的文件数量 */

    if (through == 0) {         /* 最外层调用时将路径复制到本地缓冲 */
        through = 1;
        outer = 1;
        strcpy(path, fullpath);
    }
    
    if (stat(path, &statbuf) < 0) { /* 获取文件信息 */
        LOG_MSG("IN FUNC load_files: ERROR: stat error for %s\n", path);
        return -1;
    }
    
    if (S_ISDIR(statbuf.st_mode) == 0) { /* 如果不是目录 */
        int func_ret = func(path);       /* 调用函数处理文件 */
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
        LOG_MSG("IN FUNC load_files： ERROR: can't read directory %s\n", path);
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
        LOG_MSG("IN FUNC load_files：ERROR: can't close directory %s\n", path);
        return -3;
    }

    if (outer == 1) {
        LOG_MSG("IN FUNC load_files：INFO: file loaded: %d\n", file_loaded);
        through = 0;
    }

    return ret;
}

/* 为文件分配share_file节点 */
static inline struct share_file*
init_share_file_node(char* file, struct stat *statbuf)
{
    /* 申请存储空间 */
    struct share_file *file_node = (struct share_file *)malloc(sizeof(struct share_file));
    if (!file_node) {
        return NULL;
    }

    /* 初始化 */
    INIT_HASH_NODE(&file_node->hnode, file);
    file_node->size = statbuf->st_size;
    file_node->block_num = 0;

    return file_node;
}

/* 选择一个合适的节点，目前仅根据剩余空间大小选择 */
static inline int
choose_slave(size_t size)
{
    int slave = -1;
    unsigned long free_space = 0;
    int i;

    pthread_mutex_lock(&lock_slaves);
    for (i = 1; i <= slave_num; i++) {
        if (slaves[i].alive && slaves[i].free > free_space && slaves[i].free >= size) {
            slave = i;
            free_space = slaves[i].free;
        }
    }
    pthread_mutex_unlock(&lock_slaves);

    return slave;
}

/* 更新块结构信息 */
static inline struct block *
fill_block(struct block *block, size_t offset, size_t size, int block_id, int slave_id)
{
    block->size = size;
    block->offset = offset;
    block->slave_id = slave_id;
    block->block_id = block_id;

    return block;
}

/* 请求slave载入数据块 */
static int
block_load_req(char *file, struct block *block)
{
    /* 准备相应mpi类型 */
    struct request request;
    MPI_Datatype mpi_request_type;
    build_mpi_type_request(&request, &mpi_request_type);
    MPI_Datatype mpi_block_type;
    build_mpi_type_block(block, &mpi_block_type);

    /* 获得请求tag */
    int ret = get_tag(tags, &lock_tags);
    if (ret < 0) {
        LOG_MSG("IN FUNC block_load_request: ERROR: tag can't be assigned!\n");
        return ret;
    }
    request.tag = ret;
    request.request = BLOCK_LOAD_REQ;

    /* 包装数据 */
    int size = 0, sum_size = 0;
    MPI_Pack_size(1, mpi_request_type, MPI_COMM_WORLD, &size);
    sum_size += size;
    MPI_Pack_size(MAX_PATH_LENGTH, MPI_CHAR, MPI_COMM_WORLD, &size);
    sum_size += size;
    MPI_Pack_size(1, mpi_block_type, MPI_COMM_WORLD, &size);
    sum_size += size;
    char buff[sum_size];
    int position = 0;
    MPI_Pack(&request, 1, mpi_request_type, buff, sum_size, &position, MPI_COMM_WORLD);
    MPI_Pack(file, MAX_PATH_LENGTH, MPI_CHAR, buff, sum_size, &position, MPI_COMM_WORLD);
    MPI_Pack(block, 1, mpi_block_type, buff, sum_size, &position, MPI_COMM_WORLD);

    /* 发送请求 */
    LOG_MSG("IN FUNC block_load_req: INFO: sending block %d for %s load request to slave %d!\n", block->block_id, file, block->slave_id);
    MPI_Send(buff, position, MPI_PACKED, block->slave_id, REQUEST_TAG, MPI_COMM_WORLD);
    LOG_MSG("IN FUNC block_load_req: INFO: block %d for %s load request sent to slave %d!\n", block->block_id, file, block->slave_id);

    /* 接收ack */
    ret = 0;
    struct request ack;
    MPI_Status status;
    MPI_Recv(&ack, 1, mpi_request_type, block->slave_id, request.tag, MPI_COMM_WORLD, &status);
    if (ack.request == request.request && ack.tag == request.tag) {
        LOG_MSG("IN FUNC block_load_req: INFO: block %d for %s loaded at slave %d!\n", block->block_id, file, block->slave_id);
    } else {
        LOG_MSG("IN FUNC block_load_req: INFO: block %d for %s loading at slave %d returned wrong ack!\n", block->block_id, file, block->slave_id);
        ret = -1;
    }
    
    /* 释放tag */
    if (release_tag(tags, request.tag, &lock_tags) != 0) {
        LOG_MSG("IN FUNC block_load_req: ERROR: tag %d release error!\n", request.tag);
        ret = -2;
    }
    
    return ret;
}

/* 分配文件的存储位置 */
static int
distribute_file(char *file, struct stat *statbuf)
{
    if ((file == NULL) || (statbuf == NULL)) {
        LOG_MSG("IN FUNC distribute_file： ERROR: NULL file name or statbuf!\n");
        return -1;
    }

    /* 为文件分配share_file节点 */
    struct share_file *file_node = init_share_file_node(file, statbuf);
    if (file_node == NULL) {
        LOG_MSG("IN FUNC distribute_file: ERROR: share_file node init error!\n");
        return -2;
    }
    file_node->size = statbuf->st_size;

    /* 选取合适的slave存储 */
    int slave;
    slave = choose_slave(file_node->size);
    if (slave < 0) {
        LOG_MSG("IN FUNC distribute_file: ERROR: no propriate slave!\n");
        return -3;
    }

    /* 载入文件不分块，请求slave载入 */
    int ret;
    LOG_MSG("IN FUNC distribute_file: INFO: request slave %d to load file %s!\n", slave, file_node->hnode.str);
    ret = block_load_req(file_node->hnode.str, fill_block(&file_node->blocks[0], 0, file_node->size, 0, slave));
    if (ret) {
        LOG_MSG("IN FUNC distribute_file: ERROR: block load request failed!\n");
        return -4;
    }
    LOG_MSG("IN FUNC distribute_file: INFO: slave %d loaded file %s\n", slave, file_node->hnode.str);
    file_node->block_num = 1;

    /* 更新节点信息 */
    pthread_mutex_lock(&lock_slaves);
    slaves[slave].free -= file_node->size;
    slaves[slave].last_update = time(NULL);
    pthread_mutex_unlock(&lock_slaves);
    LOG_MSG("IN FUNC distribute_file: INFO: slave %d status updated!\n", slave);

    /* 将share_file节点插入哈希表 */
    pthread_mutex_lock(&lock_share_files);
    if (&file_node->hnode != hash_insert(&file_node->hnode, share_files)) {
        LOG_MSG("IN FUNC distribute: ERROR: file hash failed\n");
        pthread_mutex_unlock(&lock_share_files);
        return -5;
    }
    share_file_num++;
    pthread_mutex_unlock(&lock_share_files);
    
    LOG_MSG("IN FUNC distribute: INFO: file %s load success!\n", file_node->hnode.str);

    return 0;
}

/* 载入文件信息，并分发文件 */
static inline int
load_file(char *file)
{
    struct stat statbuf;

    LOG_MSG("IN FUNC load_file: INFO: load file %s\n", file);

    if (stat(file, &statbuf) < 0) {
        LOG_MSG("IN FUNC load_file: ERROR: stat error for %s\n", file);
    }

    LOG_MSG("IN FUNC load_file: INFO: file %s size %ld bytes\n", file, statbuf.st_size);

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

    /* 遍历哈希表，打印共享文件信息 */
    pthread_mutex_lock(&lock_share_files);
    ret = for_each_hash_entry(share_files, list_share_file, &file_num);
    pthread_mutex_unlock(&lock_share_files);

    printf("--------total files: %d-------\n", ret ? -1 : file_num);

    return 0;
}

/* 列出slave节点信息 */
static inline int
list_slave_status(void)
{
    printf("\n--------slave num: %d--------", slave_num);
    int i;

    /* 打印数据节点信息 */
    pthread_mutex_lock(&lock_slaves);
    for (i = 1; i <= slave_num; i++) {
        printf("\nslave: %d :\n", i);
        printf("is alive: %d\n", slaves[i].alive);
        printf("free: %lu bytes\n", slaves[i].free);
        printf("last update: %s\n", ctime(&slaves[i].last_update));
    }
    pthread_mutex_unlock(&lock_slaves);
    
    return 0;
}

/* 将文件节点包装入传输缓冲区 */
static inline int
pack_share_file(struct hash_node *node, void *input)
{
    struct pack_param *param = input;
    struct share_file *file = hash_entry(node, struct share_file, hnode);

    MPI_Pack(file, 1, param->mpi_share_file_type, param->buff, param->size, &param->position, MPI_COMM_WORLD);

    return 0;
}

/* 向slave节点分发共享文件元数据表 */
static int
distribute_share_files_map(void)
{
    /* 准备相应mpi数据类型 */
    MPI_Status stat;
    struct pack_param param;
    struct share_file file_req;
    build_mpi_type_share_file(&file_req, &param.mpi_share_file_type);
    int file_num = share_file_num;

    /* 计算缓冲区大小 */
    LOG_MSG("IN FUNC distribute_share_files_map: INFO: packing share files info!");
    MPI_Pack_size(share_file_num, param.mpi_share_file_type, MPI_COMM_WORLD, &param.size);
    param.buff = (char *)malloc(param.size);
    param.position = 0;

    /* 填充缓冲区 */
    pthread_mutex_lock(&lock_share_files);
    for_each_hash_entry(share_files, pack_share_file, &param);
    pthread_mutex_unlock(&lock_share_files);

    /* Broadcast不能用Probe接收，因此依次发送。。。 */
    LOG_MSG("IN FUNC distribute_share_files_map: INFO: sending share files info!");
    int i;
    for (i = 1; i <= slave_num;) {
        if (MPI_Send(param.buff, param.position, MPI_PACKED, i, SHARE_FILE_DIS_TAG, MPI_COMM_WORLD) == 0) {
            i++;
        }
    }
    /* 释放缓冲区 */
    free(param.buff);

    LOG_MSG("IN FUNC distribute_share_files_map: INFO: share files info sended!");
    
    return 0;
}


/* 更新节点信息 */
static int
update_slave_info(struct slave_info *slave)
{
    pthread_mutex_lock(&lock_slaves);
    if (slaves[slave->id].alive == 0 && slave->alive == 1) {
        slave_num++;
        /* 通知等待的init函数 */
        pthread_cond_signal(&slaves_complete_cond);
    }
    if (slaves[slaves->id].alive == 1 && slave->alive == 0) {
        slave_num--;
    }
    slaves[slave->id].id = slave->id;
    slaves[slave->id].alive = slave->alive;
    slaves[slave->id].free = slave->free;
    slaves[slave->id].last_update = time(NULL);
    slaves[slave->id].connections = slave->connections;
    pthread_mutex_unlock(&lock_slaves);

    return 0;
}

/* 处理节点信息汇报 */
static int
handle_slave_report(struct handler_param *param)
{
    /* 准备相应mpi类型 */
    struct slave_info slave;
    MPI_Datatype mpi_slave_info_type;
    build_mpi_type_slave_info(&slave, &mpi_slave_info_type);

    /* 解包slave_info信息 */
    MPI_Unpack(param->buff, param->buff_size, &param->position, &slave, 1, mpi_slave_info_type, MPI_COMM_WORLD);
    LOG_MSG("IN FUNC handle_slave_report: INFO: slave info unpacked!\n");

    /* 更新slave信息 */
    update_slave_info(&slave);
    LOG_MSG("IN FUNC handle_slave_report: INFO: slave info updated!\n");

    /* 发送回执 */
    MPI_Send(&param->request, 1, param->mpi_request_type, param->status.MPI_SOURCE, param->request.tag, MPI_COMM_WORLD);

    return 0;
}

/* 请求处理函数数组 */
static request_handler request_handlers[MAX_REQUEST_TYPES] = {
    handle_slave_report
};

/* master消息处理线程 */
static void *
master_listen_thread(void *param)
{
    /* 构造request类型 */
    struct handler_param handler_param;
    build_mpi_type_request(&handler_param.request, &handler_param.mpi_request_type);

    /* 设置log id */
    ssize_t ret = 0;
    ret = set_log_id((ssize_t)param);
    if (ret != 0) {
        LOG_MSG("IN MASTER THREAD %d: INFO: slave failed to set log id!\n", (ssize_t)param);
        return (void *)ret;
    }

    /* 试探消息长度 */
    MPI_Message message;
    MPI_Mprobe(MPI_ANY_SOURCE, REQUEST_TAG, MPI_COMM_WORLD, &message, &handler_param.status);
    MPI_Get_count(&handler_param.status, MPI_PACKED, &handler_param.buff_size);
    LOG_MSG("IN MASTER THREAD %d: INFO: request message probed!\n", (ssize_t)param);
    
    /* 请求处理循环 */
    while (1) {
        char buff[handler_param.buff_size];
        handler_param.position = 0;
        handler_param.buff = buff;

        /* 根据试探长度，接收消息 */
        MPI_Mrecv(handler_param.buff, handler_param.buff_size, MPI_PACKED, &message, &handler_param.status);
        LOG_MSG("IN MASTER THREAD %d: INFO: request message received!\n", (ssize_t)param);

        /* 解包struct request结构体信息 */
        MPI_Unpack(handler_param.buff, handler_param.buff_size, &handler_param.position, &handler_param.request,
                   1, handler_param.mpi_request_type, MPI_COMM_WORLD);
        LOG_MSG("IN MASTER THREAD %d: INFO: requst %d tag %d received!", (ssize_t)param, handler_param.request.request,
                handler_param.request.tag);

        /* 调用请求处理函数执行处理过程 */
        ret = request_handlers[handler_param.request.request](&handler_param);
        if (ret != 0) {
            LOG_MSG("IN MASTER THREAD %d: ERROR: requst %d tag %d failed handling!", (ssize_t)param,
                    handler_param.request.request, handler_param.request.tag);
        } else {
            LOG_MSG("IN MASTER THREAD %d: INFO: requst %d tag %d handled!", (ssize_t)param,
                    handler_param.request.request, handler_param.request.tag);
        }

        /* 试探消息长度 */
        MPI_Mprobe(MPI_ANY_SOURCE, REQUEST_TAG, MPI_COMM_WORLD, &message, &handler_param.status);
        MPI_Get_count(&handler_param.status, MPI_PACKED, &handler_param.buff_size);
        LOG_MSG("IN MASTER THREAD %d: INFO: request message probed!\n", (ssize_t)param);
    }
    return (void *)0;
}

/* master主服务线程 */
static void *
master_main_thread(void *param)
{
    ssize_t ret = 0;

    /* 设置log id */
    ret = set_log_id((ssize_t) param);
    if (ret != 0) {
        LOG_MSG("IN MASTER THREAD %d: INFO: failed to set log id!\n", (ssize_t)param);
        return (void *)ret;
    }

    /* 创建监听线程 */
    LOG_MSG("IN MASTER THREAD %d: INFO: creating listenning threads!\n", (ssize_t)param);
    ssize_t i;
    for (i = 1; i < MASTER_THREAD_NUM; i++) {
        ret = pthread_create(&tid[i], NULL, master_listen_thread, (void *)i);
     }
    if (ret != 0) {
        LOG_MSG("IN MASTER THREAD %d: INFO: listenning thread failed to be created!\n", (ssize_t)param);
        return (void *)ret;
    }
    LOG_MSG("IN MASTER THREAD %d: INFO: listenning threads created!\n", (ssize_t)param);

    /* 等待监听线程结束 */
    for (i = 1; i < MASTER_THREAD_NUM; i++) {
        ret = pthread_join(tid[i], NULL);
        if (ret != 0) {
            LOG_MSG("IN MASTER THREAD %d: INFO: listenning thread failed to be stopped!\n", (ssize_t)param);
            return (void *)ret;
        }
    }
    LOG_MSG("IN MASTER THREAD %d: INFO: listenning threads stopped!\n", (ssize_t)param);

    /* 关闭日志文件 */
    ret = close_logger(MASTER_THREAD_NUM);
    if (ret != 0) {
        LOG_MSG("IN MASTER THREAD %d: INFO: logger failed to be closed!\n", (ssize_t)param);
    }
    return (void *)ret;
}


/* 初始化master， 并创建相关线程 */
int
init_dmf_master(int slaves, char *files, \
                unsigned long mem_size, \
                unsigned long file_size)
{
    ssize_t ret = 0;

    /* 初始化日志文件及线程日志编号 */
    ret = init_logger("./dmf_log/master/", MASTER_THREAD_NUM);
    if (ret != 0 || set_log_id((void *)MAX_THREAD_NUM)) {
        printf("IN FUN init_dmf_master: ERROR: logger initialization failed!\n NOTICE: LOG DIRCTORY MUST EXISTS!\n");
        return ret;
    }
    LOG_MSG("IN FUNC init_dmf_master: INFO: master initializing!\n");

    /* 创建master主线程 */
    ret = pthread_create(&tid[0], NULL, master_main_thread, 0);
    if (ret != 0) {
        LOG_MSG("IN FUNC init_dmf_master: ERROR: failed to init master threads!\n");
        return ret;
    }
    LOG_MSG("IN FUNC init_dmf_master: INFO: master thread created!\n");

    /* 等待slave节点汇报 */
    pthread_mutex_lock(&lock_slaves);
    while (slave_num != slaves) {
        pthread_cond_wait(&slaves_complete_cond, &lock_slaves);
    }
    pthread_mutex_unlock(&lock_slaves);
    LOG_MSG("IN FUNC init_dmf_master: INFO: slaves ready!\n");

    /* 载入文件 */
    ret = load_files(files, load_file);
    if (ret != 0) {
        LOG_MSG("IN FUNC init_dmf_master: ERROR: failed to load files!\n");
        return ret;
    }
    LOG_MSG("IN FUNC init_dmf_master: INFO: filed loaded!\n");

    /* 分发共享文件哈希表 */
    ret = distribute_share_files_map();
    if (ret != 0) {
        LOG_MSG("IN FUNC init_dmf_master: ERROR: failed to distribute file maps!\n");
        return ret;
    }
    LOG_MSG("IN FUNC init_dmf_master: INFO: share file map distributed!\n");

    /* 打印初始化后相关信息 */
    list_share_files();
    list_slave_status();

    return 0;
}


/* 用作测试 */
/* int */
/* main(int argc, char **argv) */
/* { */
/*     if (argc != 2 ) { */
/*         printf("usage: %s pathname\n", argv[0]); */
/*         return -1; */
/*     } */
/*     int provided; */
/*     MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided); */
/*     slaves[0].alive = 1; */
/*     slaves[0].free = 1000000000ul; */
/*     slaves[1].alive = 1; */
/*     slaves[1].free = 1000000000ul; */
/*     slave_num = 2; */
/*     int load_ret; */
/*     load_ret = load_files(argv[1], load_file); */

/*     if (load_ret < 0) { */
/*         printf("ERROR: Load failed!\n"); */
/*         return load_ret; */
/*     } */
/*     list_share_files(); */
/*     list_slave_status(); */
/*     int size; */
/*     MPI_Datatype mpi_share_file_type; */
/*     struct share_file share_file; */
/*     build_mpi_type_share_file(&share_file, &mpi_share_file_type); */
/*     MPI_Pack_size(1, mpi_share_file_type, MPI_COMM_WORLD, &size); */
/*     printf("size of share_file packed: %d\n", size); */
/*     printf("size of share_file: %d\n", sizeof(struct share_file)); */
/*     printf("size of unsigned long: %d\n", sizeof(unsigned long)); */
/*     int rank; */
/*     int position = 0; */
/*     MPI_Comm_rank(MPI_COMM_WORLD, &rank); */
/*     if (rank == 0) */
/*     { */
/*         char buf[1000]; */
/*         MPI_Pack(&rank, 1, MPI_INT, buf, 1000, &position, MPI_COMM_WORLD); */
/*         MPI_Pack(&size, 1, MPI_INT, buf, 1000, &position, MPI_COMM_WORLD); */
/*         MPI_Send(buf, position, MPI_PACKED, 1, 0, MPI_COMM_WORLD); */
/*         int i = 1; */
/*         while (i < 100) { */
/*             char b[i]; */
/*             printf("size of b[i]: %d\n", sizeof(b)); */
/*             i++; */
/*         } */
/*         /\* MPI_Bcast(buf, position, MPI_PACKED, 0, MPI_COMM_WORLD); *\/ */
/*     } else { */
/*         MPI_Status status; */
/*         MPI_Probe(0, 0, MPI_COMM_WORLD, &status); */
/*         int count; */
/*         MPI_Get_count(&status, MPI_INT, &count); */
/*         printf("MPI_INT received: %d\n", count); */
/*         MPI_Get_count(&status, MPI_PACKED, &count); */
/*         printf("MPI_PACKED received: %d\n", count); */
/*     } */
/*     MPI_Finalize(); */
/* } */


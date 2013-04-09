/**************************************************
 * Created: 2013/4/4
 * Author: Leo_xy
 * Email: xy198781@sina.com
 * Last modified: 2013/4/7 10：00
 * Version: 0.1
 * File: src/master/master_main.c
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

#include "../common/hashtable.h"
#include "../common/common.h"
#include "../common/constants.h"
#include "../common/log.h"

static int slave_num = 0;
static struct slave_info slaves[MAX_SLAVES] = {};
static HASH_TABLE(share_files);


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
int
block_load_req(char *file, struct block *block)
{
    return 0;
}

/* 分配文件的存储位置 */
int
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
    LOG_MSG("IN FUNC distribure: file %s load success!\n", file_node->hnode.str);
    return 0;
}


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

static inline int
list_share_file(struct hash_node *node)
{
    struct share_file *file = hash_entry(node, struct share_file, hnode);
    printf("file name: %s\n", file->hnode.str);
    printf("file size: %u\n", file->size);
    printf("file block num: %d\n", file->block_num);
    printf("block size: %u\n", file->blocks[0].size);
    printf("block offset: %u\n", file->blocks[0].offset);
    printf("block loc: slave %d\n", file->blocks[0].slave_id);
    return 0;
}

static inline int
list_slave_status(void)
{
    printf("\nslave num: %d\n", slave_num);
    int i;
    for (i = 0; i < slave_num; i++) {
        printf("slave: %d :\n", i);
        printf("is alive: %d\n", slaves[i].alive);
        printf("free: %lu bytes\n", slaves[i].free);
        printf("last update: %lu\n", slaves[i].last_update);
    }
    return 0;
}

int
main(int argc, char **argv)
{
    if (argc != 2 ) {
        printf("usage: %s pathname\n", argv[0]);
        return -1;
    }
    slaves[0].alive = 1;
    slaves[0].free = 900000ul;
    slaves[1].alive = 1;
    slaves[1].free = 1000000ul;
    slave_num = 2;
    load_files(argv[1], load_file);
    printf("travel ret: %d\n", for_each_hash_entry(share_files, list_share_file));
    list_slave_status();
}


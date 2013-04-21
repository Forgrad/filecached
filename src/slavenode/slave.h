/**************************************************
 * Created: 2013/4/11
 * Author: Leo_xy
 * Email: xy198781@sina.com
 * Last modified: 2013/4/20 17：00
 * Version: 0.1
 * File: src/slave.h
 * Breif: slave节点程序头文件。
 **************************************************/

#ifndef SLAVE_HEADER
#define SLAVE_HEADER

#include <pthread.h>

#include "../common/common.h"
#include "../common/hashtable.h"
#include "../common/tag.h"
#include "mem_manage.h"

extern pthread_mutex_t lock_share_files_slave;
extern HASH_TABLE(share_files_slave);
extern pthread_mutex_t lock_private_files;
extern HASH_TABLE(private_files);
extern tag_pool slave_tags;
extern pthread_mutex_t lock_slave_tags;
extern managememory manager;
extern struct slave_info loc_slave;

/*slave远端数据的读取*/
int
slave_remote_read(const char *file, int slave_id, unsigned long pos,
                  unsigned long size, char *buf);

#endif

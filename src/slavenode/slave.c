/**************************************************
 * Created: 2013/4/11
 * Author: Leo_xy
 * Email: xy198781@sina.com
 * Last modified: 2013/4/11 24：00
 * Version: 0.1
 * File: src/slave.c
 * Breif: slave节点程序代码。
 **************************************************/

#include <pthread.h>
#include <mpi.h>

#include "../common/common.h"
#include "../common/hashtable.h"
#include "../common/constants.h"
#include "../common/log.h"

static pthread_mutex_t lock_share_files = PTHREAD_MUTEX_INITIALIZER;
static HASH_TABLE(share_files);
static pthread_mutex_t lock_private_files = PTHREAD_MUTEX_INITIALIZER;
static HASH_TABLE(private_files);
static pthread_t tid[SLAVE_THREAD_NUM];

static void *
slave_main_thread(void *param)
{
    
}

int
init_dmf_slave(unsigned long mem)
{
    LOG_MSG("IN FUNC init_dmf_slave: INFO: slave initializing!\n");
    int ret;
    
    ret = report_slave_info();
    if (ret != 0) {
        LOG_MSG()
    }
    ret =  pthread_create(&tid[0], NULL, slave_main_thread, NULL);
    if (ret != 0) {
        LOG_MSG("IN FUNC init_dmf_slave: ERROR: failed to init slave threads!\n");
        return ret;
    }
    
}










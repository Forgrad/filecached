/**************************************************
 * Created: 2013/4/11
 * Author: Leo_xy
 * Email: xy198781@sina.com
 * Last modified: 2013/4/19 10：00
 * Version: 0.1
 * File: src/slave.h
 * Breif: slave节点程序代码。
 **************************************************/

#ifndef SLAVE_HEADER
#define SLAVE_HEADER

#include "../common/common.h"
#include "../common/hashtable.h"
#include "../common/constants.h"
#include "../common/log.h"
#include "../common/tag.h"
#include "../common/dfsio.h"
#include "../common/mem_manage.h"

static pthread_mutex_t lock_share_files = PTHREAD_MUTEX_INITIALIZER;
static HASH_TABLE(share_files);
static pthread_mutex_t lock_private_files = PTHREAD_MUTEX_INITIALIZER;
static HASH_TABLE(private_files);
static pthread_t tid[SLAVE_THREAD_NUM];

static tag_pool slave_tags = {};
static pthread_mutex_t lock_slave_tags = PTHREAD_MUTEX_INITIALIZER;
const size_t memory_size = 65535;
managememory *manager;
struct slave_info *loc_slave;
static request_handler request_handlers[MAX_REQUEST_TYPES] = {
	handle_load_request, handle_remote_read
};

/*调用mpi的接口来获取本地的slave_id*/
int getprocessid(void);

/*初始化slave*/
int init_slave_info(void);

/*slave报告自身信息*/
int report_slave_info();

/*slave载入数据*/
static int handle_load_request(struct request *request, MPI_Status *status, void *buff,
					int buff_size, int *position, MPI_Datatype *request_type);

/*slave存储共享数据映射表*/
static int handle_share_file_maps(void);

/*slave远端数据的读取*/
static int slave_remote_read(char file[], size_t s, size_t c, size_t p, int pid, void *buf);

/*slave处理远端数据请求*/
static int handle_remote_read(struct request *request, MPI_Status *status, void *buff,
				   int buff_size, int *position, MPI_Datatype *request_type);

/*slave消息处理函数*/
static void * slave_msg_passing_thread(void *param);

/*slave主线程控制线程的创建*/
static void * slave_main_thread(void *param);

/*初始化slave，并创建主线程*/
int init_dmf_slave(unsigned long mem);

#endif

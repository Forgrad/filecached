/**************************************************
 * Created: 2013/4/11
 * Author: Leo_xy
 * Email: xy198781@sina.com
 * Last modified: 2013/4/19 10：00
 * Version: 0.1
 * File: src/slave.c
 * Breif: slave节点程序代码。
 **************************************************/

#include <pthread.h>
#include <mpi.h>
#include "slave.h"


/*调用mpi的接口来获取本地的slave_id*/
int getprocessid(void)
{
	int id;
	MPI_Comm_rank(MPI_COMM_WORLD, &id);
	return id;
}

/*初始化slave*/
int 
init_slave_info(void)
{
	loc_slave->alive = 1;
	loc_slave->id = getprocessid();
	loc_slave->free  = manager->freesize;
	loc_slave->last_update = time;
	loc_slave->connections = 1;

	return 0;
}


/*slave报告自身信息*/
int 
report_slave_info()
{
	MPI_Datatype mpi_slave_info_type;
	MPI_Datatype mpi_request_type;
	MPI_Status stat;
	MPI_Message message;
	struct request req;
	int size = 0, sum_size = 0;
	

	req.request = request_type.SLAVE_REPORT;
	req.tag = get_tag(slave_tags, &lock_slave_tags);
	build_mpi_type_slave_info(loc_slave, &mpi_slave_info_type);
	build_mpi_type_request(&req, &mpi_request_type);

	MPI_Pack_size(1, mpi_slave_info_type, MPI_COMM_WORLD, &size);
	sum_size += size;
	MPI_Pack_size(1, mpi_request_type, MPI_COMM_WORLD, &size);
	sum_size += size;

	char buff[sum_size];
	int position  = 0;
	MPI_Pack(&req, 1, mpi_request_type, buff, sum_size, &position, MPI_COMM_WORLD);
	MPI_Pack(loc_slave, 1, mpi_slave_info_type, buff, sum_size, &position, MPI_COMM_WORLD);

	MPI_Send(buff, position, MPI_PACKED, 0, REQUEST_TAG, MPI_COMM_WORLD);
	LOG_MSG("IN FUNC report_slave_info INFO: sending slave info report!\n");

	MPI_Recv(&req, 1, mpi_request_type, 0, req.tag, MPI_COMM_WORLD,&stat);
	LOG_MSG("IN FUNC report_slave_info INFO: slave info  received!\n");

	release_tag(loc_slave, req.tag, lock_slave_tags);
	return 0;
}


/*slave载入数据*/
static int
handle_load_request(struct request *request, MPI_Status *status, void *buff,
					int buff_size, int *position, MPI_Datatype *request_type)
{
	struct request req;
	MPI_Datatype mpi_request_type;
	MPI_Datatype mpi_block_type;
	char file[MAX_PATH_LENGTH];
	struct block *block;
	int ret;

	build_mpi_type_request(&req, &mpi_request_type);
	build_mpi_type_block(block, &mpi_block_type);

	MPI_Unpack(buff, buff_size, position, file, MAX_PATH_LENGTH, MPI_CHAR, MPI_COMM_WORLD);
	MPI_Unpack(buff, buff_size, position, block, 1, mpi_block_type, MPI_COMM_WORLD);

	ret = mem_malloc(manager, file, block->size);
	if(ret != 0)
	{
		printf("IN FUN handle_load_request ERROR: file load failed!\n");
		return -1;
	}
	ret = mem_write_block(file,block);
	if(ret != 0)
	{
		printf("IN FUN handle_load_request ERROR: file load failed!\n");
		return -1;
	}

	MPI_Send(&req, 1, mpi_request_type, 0, req.tag, MPI_COMM_WORLD);
	return 0;
}

/*slave存储共享数据映射表*/
static int
handle_share_file_maps(void)
{
	MPI_Datatype mpi_share_file_type;
	struct share_file file_req;
	int buff_size;
	MPI_Status status;
	MPI_Message message;

	build_mpi_type_share_file(&file_req, &mpi_share_file_type);

	MPI_Mprobe(0, SHARE_FILE_DIS_TAG, MPI_COMM_WORLD, &message, &status);
	MPI_Get_count(&status, MPI_PACKED, &buff_size);
    char buff[buff_size];
	MPI_Mrecv(buff, buff_size, MPI_PACKED, &message, &status);
	int position = 0;
	
	while(position < buff_size - 1)
	{
		MPI_Unpack(buff, buff_size, position, &file_req, 1, mpi_share_file_type, MPI_COMM_WORLD);
		if(hash_insert(share_files, file_req->hnode) == NULL)
		{
			LOG_MSG("IN FUN handle_share_file_maps: ERROR: share_files table insert failed!\n");
			return -1;
		}
	}

	return 0;
}

/*slave远端数据的读取*/
static int
slave_remote_read(char file[], size_t s, size_t c, size_t p, int pid, void *buf)
{
	MPI_Datatype mpi_request_type;
	MPI_Status stat;
	MPI_Message message;
	struct request req;
	int size = 0, sum_size = 0;

	req.request = REMOTE_READ;
	req.tag = get_tag(slave_tags, &lock_slave_tags);
	build_mpi_type_request(&req, &mpi_request_type);

	MPI_Pack_size(1, mpi_request_type, MPI_COMM_WORLD, &size);
	sum_size += size;
	MPI_Pack_size(MAX_PATH_LENGTH, MPI_CHAR, MPI_COMM_WORLD, &size);
	sum_size += size;
	MPI_Pack_size(3, MPI_UNSIGNED_INT, MPI_COMM_WORLD, &size);
	sum_size += size;

	char buff[sum_size];
	int position  = 0;  unsigned int size_count[3] = {s, c, p};
	MPI_Pack(&req, 1, mpi_request_type, buff, sum_size, &position, MPI_COMM_WORLD);
	MPI_Pack(file, MAX_PATH_LENGTH, MPI_CHAR, buff, sum_size, &position, MPI_COMM_WORLD);
	MPI_Pack(size_count, 3, MPI_UNSIGNED_INT, buff, sum_size, &position, MPI_COMM_WORLD);

	MPI_Send(buff, position, MPI_PACKED, pid, REQUEST_TAG, MPI_COMM_WORLD);

	int num_buf = size_count[0] * size_count[1];
	MPI_Recv(&buf, num_buf, MPI_CHAR, pid, req.tag, MPI_COMM_WORLD,&stat);
	release_tag(loc_slave, req.tag, lock_slave_tags);
}

/*slave处理远端数据请求*/
static int
handle_remote_read(struct request *request, MPI_Status *status, void *buff,
				   int buff_size, int *position, MPI_Datatype *request_type)
{
	char buff[buff_size]; 
	unsigned int size_count[3] = {};

	char file[MAX_PATH_LENGTH];

	MPI_Unpack(buff, buff_size, position, file, MAX_PATH_LENGTH, MPI_CHAR, MPI_COMM_WORLD);
	MPI_Unpack(buff, buff_size, position, size_count, 3, MPI_UNSIGNED_INT, MPI_COMM_WORLD);


	int buf_num = size_count[0] * size_count[1];
	char buf[buf_num];
	dmf_file *dmf_file = dmf_open(file);

	struct share_file *node=hash_entry(file->hnode,struct share_file,hnode);/*获取相应的存放共享数据信息的结构*/

    mem_read(node->hnode.str,size_count[2], buf_num, buf);

	MPI_Send(buf, buf_num, MPI_CHAR, status->MPI_SOURCE, request->tag, MPI_COMM_WORLD);

	return 0;
}

/*slave消息处理函数*/
static void *
slave_msg_passing_thread(void *param)
{
	/* 构造request类型 */
	struct request request;
	MPI_Datatype mpi_request_type;    
	build_mpi_type_request(&request, &mpi_request_type);

	/* 试探消息长度 */
	int buff_size;
	MPI_Status status;
	MPI_Message message;
	MPI_Mprobe(MPI_ANY_SOURCE, REQUEST_TAG, MPI_COMM_WORLD, &message, &status);
	MPI_Get_count(&status, MPI_PACKED, &buff_size);

	/* 请求处理循环 */
	while (1) {
		char buff[buff_size];
		int position = 0;
		MPI_Mrecv(buff, buff_size, MPI_PACKED, &message, &status);
		MPI_Unpack(buff, buff_size, &position, &request, 1, mpi_request_type, MPI_COMM_WORLD);
		LOG_MSG("IN SLAVE THREAD %d: INFO: requst %d tag %d received!", loc_slave->id, request.request, request.tag);
		request_handlers[request.request](&request, &status, buff, buff_size, &position, &mpi_request_type);
		LOG_MSG("IN SLAVE THREAD %d: INFO: requst %d tag %d handled!", loc_slave->id, request.request, request.tag);
		MPI_Mprobe(MPI_ANY_SOURCE, REQUEST_TAG, MPI_COMM_WORLD, &message, &status);
		MPI_Get_count(&status, MPI_PACKED, &buff_size);
	}
	return (void *)0;
}

/*slave主线程控制线程的创建*/
static void *
slave_main_thread(void *param)
{
	int i, ret;

    /* 发送slave信息 */
	ret = report_slave_info();
	if (ret != 0) {
		return (void *)ret;
	}

	/* 创建消息处理线程 */

	LOG_MSG("IN SLAVE THREAD: INFO: creating message passing thread!\n");
	for (i = 1; i < SLAVE_THREAD_NUM; i++) {
		ret = pthread_create(&tid[i], NULL, slave_msg_passing_thread, NULL);
	}
	if (ret != 0) {
		return (void *)ret;
	}
	
	LOG_MSG("IN SLAVE THREAD: INFO: message passing thread created!\n");

	/* 等待监听线程结束 */
	for (i = 1; i < SLAVE_THREAD_NUM; i++) {
		ret = pthread_join(tid[i], NULL);
	}
	LOG_MSG("IN SLAVE THREAD: INFO: message passing thread stopped!\n");

	return (void *)ret;
}

/*初始化slave，并创建主线程*/
int
init_dmf_slave(unsigned long mem)
{
    LOG_MSG("IN FUNC init_dmf_slave: INFO: slave initializing!\n");
    int ret;

	if(mem_init(managememory,memory_size) == -1)
	{
		printf("IN FUN init_dmf_slave: ERROR: managememory initialization failed!\n");
		return 0;
	}
	init_dmf_slave();

	ret = pthread_create(&tid[0], NULL, slave_main_thread, NULL);
	if (ret != 0) {
		LOG_MSG("IN FUNC init_dmf_slave: ERROR: failed to init slave threads!\n");
		return ret;
	}
}

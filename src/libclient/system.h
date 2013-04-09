/**************************************************
 * Created: 2013/4/4
 * Author: Leo_xy
 * Email: xy198781@sina.com
 * Last modified: 2013/4/4 10：00
 * Version: 0.1
 * File: src/libclient/system.h
 * Breif: 存储系统控制接口头文件。
 **************************************************/

#ifndef SYSTEM_HEADER
#define SYSTEM_HEADER

/* master节点主线程启动函数 */
int init_dmf_master(int slave_num, char *files, \
                    unsigned long mem_size,\
                    unsigned long file_size);

/* master节点信息重置 */
int clean_dmf_master(int flag);

/* 关闭master节点相关线程，释放资源 */
int shutdown_dmf_master(void);

/* slave节点主线程启动函数 */
int init_dmf_slave(void);

/* slave节点信息重置 */
int clean_dmf_slave(int flag);

/* 关闭slave节点，释放资源 */
int shutdown_dmf_slave(void);

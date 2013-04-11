/**************************************************
 * Created: 2013/3/24
 * Author: Leo_xy
 * Email: xy198781@sina.com
 * Last modified: 2013/4/11 22：00
 * Version: 0.1
 * File: src/common/hashtable.h
 * Breif: 哈希表数据结构头文件，线程安全由外部调用者考虑。
 **************************************************/

#ifndef HASHTABLE_HEADER
#define HASHTABLE_HEADER

#include <stddef.h>
#include <string.h>

#include "constants.h"
#include "hlist.h"

/* 哈希链表节点数据结构 */
struct hash_node {
    char str[MAX_PATH_LENGTH];  /* 字符串哈希key值 */
    struct hlist_node list;     /* 链表节点 */
};

/* 通过哈希节点指针得到包装结构指针 */
#define hash_entry(ptr, type, member) container_of(ptr, type, member)


/* 哈希表节点初始化宏 */
#define HASH_NODE_INIT(string) \
    { .str = (string), .list = HLIST_NODE_INIT }
#define HASH_NODE(name, string) \
    struct hash_node name = HASH_NODE_INIT(string)
#define INIT_HASH_NODE(ptr, string) do { strcpy((ptr)->str, string); INIT_HLIST_NODE(&(ptr)->list);} \
    while (0)
#define HASH_TABLE(name) struct hlist_head name[HASH_SLOTS] = {};

/* 哈希表指针为指向hlist_head的指针 */
typedef struct hlist_head *hashtable_ptr;

/* 从字符串得到哈希值 */
static inline int
hash_string(const char *str)
{
    unsigned long sum = 0;
    while (*str) {
        sum += *str++;
    }
    return sum % HASH_SLOTS;
}

/* 获取哈希表项 */
static inline struct hash_node*
hash_get(const char *path, const struct hlist_head *head)
{
    struct hash_node *node;
    struct hlist_node *hlist;
    hlist_for_each_entry(node, hlist, &head[hash_string(path)], list) {
        if (!strcmp(node->str, path)) {
            return node;
        }
    }
    return NULL;
}

/* 哈希表遍历 */
static inline int
for_each_hash_entry(hashtable_ptr head, int (* func)(struct hash_node *, void *param), void *param)
{
    struct hash_node *tpos;
    struct hlist_node *pos;
    int ret;
    int i;
    for (i = 0; i < HASH_SLOTS; i++) {
        hlist_for_each_entry(tpos, pos, &head[i], list) {
            ret = func(tpos, param);
            if (ret != 0) return ret;
        }
    }
    return ret;
}


/* 摘除哈希表项 */
static inline struct hash_node*
hash_del(const char *path, struct hlist_head *head)
{
    struct hash_node *node;
    node = hash_get(path, head);
    if (!node) {
        return NULL;
    }
    hlist_del(&node->list);
    return node;
}

/* 插入哈希表项 */
/* 如果插入成功，返回该节点地址 */
/* 如果因为key存在而插入失败，则返回存在的key的地址 */
/* 其他原因返回NULL */
static inline struct hash_node*
hash_insert(struct hash_node *node, struct hlist_head *head)
{
    struct hash_node *find;
    struct hlist_node *hlist;
    int index = hash_string(node->str);
    hlist_for_each_entry(find, hlist, &head[index], list) {
        if (!strcmp(find->str, node->str)) {
            return find;
        }
    }
    hlist_add_head(&node->list, (struct hlist_head *)&head[index]);
    return node;
}

#endif




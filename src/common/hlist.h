/**************************************************
 * Created: 2013/4/1
 * Author: Leo_xy
 * Email: xy198781@sina.com
 * Last modified: 2013/4/1 9:42
 * Version: 0.1
 * File: src/common/hlist.h
 * Breif: 双向非循环链表数据结构的头文件。
 **************************************************/

#ifndef HLIST_HEADER
#define HLIST_HEADER

#include <stddef.h>

/* 链表节点数据结构 */
struct hlist_node
{
    struct hlist_node *next,**pprev;
};

/* 链表头数据结构 */
struct hlist_head
{
    struct hlist_node* first;
};

/* 链表头、节点初始化宏 */
#define HLIST_HEAD_INIT { .first = NULL }
#define HLIST_HEAD(name) struct hlist_head name = HLIST_HEAD_INIT
#define HLIST_NODE_INIT { .next = NULL, .pprev = NULL }
#define HLIST_NODE(name) struct hlist_node name = HLIST_NODE_INIT
#define INIT_HLIST_HEAD(ptr) ((ptr)->first = NULL)
#define INIT_HLIST_NODE(ptr) ((ptr)->next = NULL, (ptr)->pprev = NULL)

/* 判断一个节点是否在链表中 */
static inline int
hlist_unhashed(const struct hlist_node *h)
{
	return !h->pprev;
}

/* 判断一个链表是否为空 */
static inline int
hlist_empty(const struct hlist_head *h)
{
	return !h->first;
}

/* 从链表中删除节点 */
static inline void
hlist_del(struct hlist_node *n)
{
	struct hlist_node *next = n->next;
	struct hlist_node **pprev = n->pprev;
	*pprev = next;
	if (next)
		next->pprev = pprev;
    n->next = NULL;
    n->pprev = NULL;
}

/* 将节点加入到链表 */
static inline void
hlist_add_head(struct hlist_node *n, struct hlist_head *h)
{
	struct hlist_node *first = h->first;
	n->next = first;
	if (first)
		first->pprev = &n->next;
	h->first = n;
	n->pprev = &h->first;
}

/* 将节点加入到next所指链表节点之前，next不能为空 */
static inline void
hlist_add_before(struct hlist_node *n,
					struct hlist_node *next)
{
	n->pprev = next->pprev;
	n->next = next;
	next->pprev = &n->next;
	*(n->pprev) = n;
}

/* 将节点加入到next所指节点之后，next不能为空 */
static inline void
hlist_add_after(struct hlist_node *n,
					struct hlist_node *next)
{
	next->next = n->next;
	n->next = next;
	next->pprev = &n->next;

	if(next->next)
		next->next->pprev  = &next->next;
}

/* 得到包含链表元素的数据结构 */
#define container_of(ptr, type, member) ({			\
        const typeof( ((type *)0)->member ) *__mptr = (ptr);	\
        (type *)( (char *)__mptr - offsetof(type,member) );})

#define hlist_entry(ptr, type, member) container_of(ptr,type,member)

/* 依次查找链表 */
#define hlist_for_each_entry(tpos, pos, head, member)  \
	for (pos = (head)->first; \
	     pos && ({ tpos = hlist_entry(pos, typeof(*tpos), member); 1;}); \
	     pos = pos->next)

#endif

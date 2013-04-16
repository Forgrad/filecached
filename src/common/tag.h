/**************************************************
 * Created: 2013/4/16
 * Author: Leo_xy
 * Email: xy198781@sina.com
 * Last modified: 2013/4/16 10：00
 * Version: 0.1
 * File: src/common/tag.h
 * Breif: 发出请求的tag池头文件。
 **************************************************/

#ifndef TAG_HEADER
#define TAG_HEADER

#include <pthread.h>

typedef char tag_pool[REQUEST_TAG_NUM];

static inline int
get_tag(tag_pool tags, pthread_mutex_t *lock)
{
    pthread_mutex_lock(lock);
    int i;
    for (i = 0; i < REQUEST_TAG_NUM; i++) {
        if (tags[i] == 0) {
            tags[i] = 1;
            break;
        }
    }
    pthread_mutex_unlock(lock);
    return (i == REQUEST_TAG_NUM) ? -1 : (i + REQUEST_BASE_TAG);
}

static inline int
release_tag(tag_pool tags, int tag, pthread_mutex_t *lock)
{
    pthread_mutex_lock(lock);
    int ret;
    if (tags[tag - REQUEST_BASE_TAG] == 1) {
        tags[tag - REQUEST_BASE_TAG] = 0;
        ret = 0;
    } else {
        ret = -1;
    }
    pthread_mutex_unlock(lock);
    return ret;
}

#endif

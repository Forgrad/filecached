/**************************************************
 * Created: 2013/4/2
 * Author: Leo_xy
 * Email: xy198781@sina.com
 * Last modified: 2013/4/2 16：00
 * Version: 0.1
 * File: src/common/hashtable_test.c
 * Breif: 简单哈希表数据结构测试程序。
 **************************************************/

#include <stdio.h>
#include <stdlib.h>
#include "hashtable.h"

struct string_num {
    struct hash_node hnode;
    int num;
};

int
main(void)
{
    HASH_TABLE(hashtable);
    int i = 0;
    while (i < 10000)
    {
        struct string_num *node = (struct string_num *)malloc(sizeof (struct string_num));
        node->num = i;
        int pow = 1;
        int j = i / 10;
        while(j) {
            pow++;
            j /= 10;
        }
        j = i;
        char string[pow + 1];
        string[pow--] = '\0';
        while (pow >= 0) {
            string[pow--] = '0' + (j % 10);
            j /= 10;
        }
        INIT_HASH_NODE(&node->hnode, string);
        if (hash_insert(&node->hnode, hashtable) == &node->hnode) {
            printf("%s inserted \n", node->hnode.str);
        } else {
            printf("%s insert failed \n", node->hnode.str);
            exit(1);
        }
        i++;
    }
    getchar();
    i = 0;
    while (i < 10000) {
        int pow = 1;
        int j = i / 10;
        while(j) {
            pow++;
            j /= 10;
        }
        j = i;
        char string[pow + 1];
        string[pow--] = '\0';
        while (pow >= 0) {
            string[pow--] = '0' + (j % 10);
            j /= 10;
        }
        struct string_num *node = hash_entry(hash_get(string, hashtable), struct string_num, hnode);
        if ( node->num == atoi(string) && !strcmp(string, node->hnode.str)) {
            printf("get \"%s\" success \n", string);
        } else {
            printf("get failed\n");
            exit(1);
        }
        i++;
    }
    
    /* struct hash_node node; */
    /* struct hash_node node2; */
    /* INIT_HASH_NODE(&node, "abcs"); */
    /* INIT_HASH_NODE(&node2, "abcs"); */
    /* if (hash_insert(&node, hashtable) != &node) { */
    /*     printf("already exist: \"%s\"\n", node.str); */
    /* } else { */
    /*     printf("insert success: \"%s\"\n", node.str); */
    /* } */
    /* if (hash_insert(&node2, hashtable) != &node2) { */
    /*     printf("already exist: \"%s\"\n", node2.str); */
    /* } else { */
    /*     printf("insert success: \"%s\"\n", node2.str); */
    /* } */
    /* struct hash_node *get; */
    /* get = hash_get("abcs", hashtable); */
    /* if (get != &node) { */
    /*     printf("get failed\n"); */
    /* } else { */
    /*     printf("get success: \"%s\"\n", get->str); */
    /* } */
}

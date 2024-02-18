#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "map.h"
#include "memPool.h"

#define BLOCKSIZE 4096
mempool *pool = NULL;
mempool *pool_bk = NULL;
mempool *cur_pool = NULL;

map_t *map_first(root_t *tree) {
    rb_node_t *node = rb_first(tree);
    return (rb_entry(node, map_t, node));
}

map_t *map_next(rb_node_t *node) {
    rb_node_t *next =  rb_next(node);
    return rb_entry(next, map_t, node);
}

void memory_init()
{
    if(cur_pool == NULL) {
        pool = (mempool *)malloc(sizeof(mempool));
        memPoolInit(pool, BLOCKSIZE);
    }
    cur_pool = pool;
}

static void realloc_value_memory(root_t *tree)
{
    mempool *tmp_pool = NULL;
    if(pool != NULL && pool_bk == NULL)
        tmp_pool = pool_bk;
    else if(pool == NULL && pool_bk != NULL)
        tmp_pool = pool;

    tmp_pool = (mempool *)malloc(sizeof(mempool));
    memPoolInit(tmp_pool, BLOCKSIZE);
    map_t *data = NULL;
    for(data = map_first(tree); data; data = map_next(&data->node)) {
        char value_tmp[92] = {0};
        strcpy(value_tmp, data->val);
        data->val = (char*)mp_malloc(tmp_pool, strlen(value_tmp)+1);
        strcpy(data->val, value_tmp);
    }
    mp_free_all(cur_pool);
    cur_pool = tmp_pool;
    printf("Complete memory remap! Please retry property set!\n");
}

map_t *get(root_t *root, const char *str) {
   rb_node_t *node = root->rb_node;
   while (node) {
        map_t *data = container_of(node, map_t, node);

        //compare between the key with the keys in map
        int cmp = strcmp(str, data->key);
        if (cmp < 0) {
            node = node->rb_left;
        }else if (cmp > 0) {
            node = node->rb_right;
        }else {
            return data;
        }
   }
   return NULL;
}

int put(root_t *root, const char* key, const char* val) {
    rb_node_t **new_node = &(root->rb_node), *parent = NULL;
    while (*new_node) {
        map_t *this_node = container_of(*new_node, map_t, node);
        int result = strcmp(key, this_node->key);
        parent = *new_node;

        if (result < 0) {
            new_node = &((*new_node)->rb_left);
        }else if (result > 0) {
            new_node = &((*new_node)->rb_right);
        }else {
            if(strlen(val) > strlen(this_node->val)) {
                char *new_value = mp_malloc(cur_pool, strlen(val)+1);
                if(new_value == NULL) {
                    realloc_value_memory(root);
                    return 0;
                }
                sprintf(new_value, "%s", val);
                this_node->val = new_value;
            }
            else
                strcpy(this_node->val, val);
            return 0;
        }
    }

    map_t *data = (map_t*)malloc(sizeof(map_t));
    data->key = (char*)malloc((strlen(key)+1)*sizeof(char));
    strcpy(data->key, key);
    data->val = (char*)mp_malloc(cur_pool, (strlen(val)+1)*sizeof(char));
    if(data->val == NULL) {
        realloc_value_memory(root);
        return 0;
    }
    strcpy(data->val, val);

    rb_link_node(&data->node, parent, new_node);
    rb_insert_color(&data->node, root);

    return 1;
}

void map_free(map_t *node){
    if (node != NULL) {
        if (node->key != NULL) {
            free(node->key);
            node->key = NULL;
    }
        free(node);
        node = NULL;
    }
}

void memory_destroy()
{
    mp_free_all(cur_pool);
    cur_pool = NULL;
    pool_bk = NULL;
    pool = NULL;
}



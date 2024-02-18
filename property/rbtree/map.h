/***************************************************************************
 *
 * Copyright (c) 2016 zkdnfcf, Inc. All Rights Reserved
 * $Id$
 *
 **************************************************************************/
 
 /**
 * @file hash.h
 * @author zk(zkdnfc@163.com)
 * @date 2016/05/31 18:26:01
 * @version $Revision$
 * @brief
 *
 **/
#ifndef _MAP_H
#define _MAP_H

#include "rbtree.h"

struct map {
    struct rb_node node;
    char *key;
    char *val;
};

typedef struct map map_t;
typedef struct rb_root root_t;
typedef struct rb_node rb_node_t;

void memory_init(void);
void memory_destroy(void);
map_t *get(root_t *root, const char *str);
int put(root_t *root, const char* key, const char* val);
map_t *map_first(root_t *tree);
map_t *map_next(rb_node_t *node);
void map_free(map_t *node);

#endif  //_MAP_H

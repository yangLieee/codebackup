#ifndef __MEMORY_POOL_H
#define __MEMORY_POOL_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

typedef struct MEMPOOL {
    int block_size;
    int remain_size;
    char *soblock;
    char *pvalue;
}mempool;

void memPoolInit(mempool *mp, int blocksize) {
    mp->block_size = blocksize;
    mp->remain_size = blocksize;
    mp->soblock = malloc( mp->block_size );
    memset(mp->soblock, 0, blocksize);
    mp->pvalue = mp->soblock;
}

void *mp_malloc(mempool *mp, int size) {
    if(mp == NULL || mp->soblock == NULL) {
        printf("Memory Pool need Init\n");
        return NULL;
    }
    if(mp->remain_size >= size) {
        mp->pvalue += size;
        mp->remain_size -= size;
//        printf("remain_size %d\n",mp->remain_size);
        return mp->pvalue - size;
    }
    printf("Over malloc! remain_size %d\n",mp->remain_size);
    return NULL;
}

void mp_free_all(mempool *mp) {
    free(mp->soblock);
    mp->soblock = NULL;
    free(mp);
    mp = NULL;
}


#endif /* __MEMORY_POOL_H */


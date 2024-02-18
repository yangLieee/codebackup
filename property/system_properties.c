#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include "system_properties.h"
#include "rbtree/map.h"
#include "rbtree/rbtree.h"

root_t tree = RB_ROOT;

static int op_prop_file(const char *file, const char *key, const char *value, int old_valen, bool have_key)
{
    int size = 0;
    char *mem=NULL, *sof=NULL;
    FILE *fp = fopen(file,"rb");
    if(fp != NULL) {
        fseek(fp, 0, SEEK_END);
        size = ftell(fp);
        fseek(fp, 0, SEEK_SET);

        mem = (char *)malloc(size * sizeof(char));
        if(mem == NULL) {
            printf("malloc error!\n");
            return -1;
        }
        fread(mem, size, sizeof(char), fp);
        sof = mem;
    }
    fclose(fp);

    fp = fopen(file, "wb");
    if(have_key) {
        for(int i=0; i<=size; i++) {
            if(!memcmp(key, mem+i, strlen(key))) {
                int move_len = i + strlen(key) + 1;
                int ret_len = size - move_len - old_valen;
                fwrite(sof, 1, move_len, fp);
                fwrite(value, 1, strlen(value), fp);
                fwrite(sof + move_len + old_valen, 1, ret_len, fp);
                break;
            }
        }
    } else {
        fwrite(sof, 1, size, fp);
        char str[PROP_KEY_MAX + PROP_VALUE_MAX +2] = {0};
        sprintf(str, "%s=%s\n", key, value);
        fwrite(str, 1, strlen(key)+strlen(value)+2, fp);
    }
    fclose(fp);
    free(mem);
    return 0;
}

void *read_file(const char *fn, unsigned *_sz)
{
	char *data = NULL;
	int sz = 0;
	int fd = 0;

	fd = open(fn, O_RDONLY);
	if(fd < 0) return 0;

	sz = lseek(fd, 0, SEEK_END);
	if(sz < 0) goto oops;

	if(lseek(fd, 0, SEEK_SET) != 0) goto oops;

	data = (char*) malloc(sz + 2);
	if(data == 0) goto oops;

	if(read(fd, data, sz) != sz) goto oops;
	close(fd);
	data[sz] = '\n';
	data[sz+1] = 0;
	if(_sz) *_sz = sz;
	return data;

oops:
	close(fd);
	if(data != 0) free(data);
	return 0;
}

int __system_property_add(const char *key, const char *value)
{
	if (strlen(key) >= PROP_KEY_MAX)
        return -1;
	if (strlen(value) >= PROP_VALUE_MAX)
        return -1;
	if (strlen(key) < 1)
        return -1;
    put(&tree, key, value);
    return 0;
}

int __system_property_list()
{
    int sum = 0;
    map_t *data = NULL;
    for(data = map_first(&tree); data; data = map_next(&data->node)) {
        printf("[%s] : [%s]\n ", data->key, data->val);
        sum++;
    }
    printf("\ttotal %d properties!\n", sum);
    return 0;
}

void __system_property_release()
{
    map_t *nodeFree = NULL;
    for (nodeFree = map_first(&tree); nodeFree; nodeFree = map_first(&tree)) {
        if (nodeFree) {
            rb_erase(&nodeFree->node, &tree);
            map_free(nodeFree);
        }
    }
}

int __system_property_get(const char *key, char *value)
{
    map_t *data = NULL;
    data = get(&tree, key);
    if(data == NULL) {
        memset(value, 0, strlen(value));
        if(!strncmp(key, "persist.", 8)) {
            char re_key[PROP_KEY_MAX] = {0};
            memcpy(re_key, "ro.", 3);
            memcpy(re_key + 3, key + 8, PROP_KEY_MAX-8);
            data = get(&tree, re_key);
            if(data != NULL)
                strncpy(value, data->val, strlen(data->val));
        }
    }
    else
        strncpy(value, data->val, strlen(data->val));
    return 0;
}

int __system_property_set(const char *key, const char *value, const char *filepath)
{
	if (strlen(key) >= PROP_KEY_MAX)
        return -1;
	if (strlen(value) >= PROP_VALUE_MAX)
        return -1;
	if (strlen(key) < 1)
        return -1;

    map_t *data = NULL;
    data = get(&tree, key);
    if(data == NULL){
        if(!strncmp(key, "persist.", 8))
            op_prop_file(filepath, key, value, 0, 0);
        __system_property_add(key, value);
    }
    else {
        if(!strncmp(key, "ro.", 3)) {
            printf("%s permission is read only!\n", key);
            return -1;
        }
        if(!strncmp(key, "persist.", 8))
            op_prop_file(filepath, key, value, strlen(data->val), 1);
        put(&tree, key, value);
    }
    return 0;
}


void __prop_memory_init() {
    memory_init();
}

void __prop_memory_destroy() {
    map_t *nodeFree = NULL;
    for (nodeFree = map_first(&tree); nodeFree; nodeFree = map_first(&tree)) {
        if (nodeFree) {
            rb_erase(&nodeFree->node, &tree);
            map_free(nodeFree);
        }
    }
    memory_destroy();
}

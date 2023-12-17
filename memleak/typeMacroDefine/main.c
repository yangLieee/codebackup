#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define STORE_PATH "memleakStore"
void* my_malloc(size_t size, const char* filename, const char* func, int line)
{
    char buffer[128] = {0};
    const char* storepath = STORE_PATH;
    if(access(storepath, F_OK) != 0) {
        mkdir(storepath,0755);
    }

    void* ptr = malloc(size);
    if(ptr != NULL) {
        snprintf(buffer, sizeof(buffer), "%s/%p.mem", storepath, ptr);
        FILE* fp = fopen(buffer, "w");
        memset(buffer, 0, sizeof(buffer));
        snprintf(buffer, sizeof(buffer), "[+] (%s %s %d) (%p + %ld)\n", filename, func, line, ptr, size);
        fwrite(buffer, sizeof(char), sizeof(buffer), fp);
        fclose(fp);
    }
    return ptr;
}

void my_free(void* ptr, const char* filename, const char* func, int line)
{
    char buffer[128] = {0};
    const char* storepath = STORE_PATH;
    if(access(storepath, F_OK) != 0) {
        printf("%s not exist. Please Check \n", storepath);
        return;
    }

    snprintf(buffer, sizeof(buffer), "%s/%p.mem", storepath, ptr);
    if(unlink(buffer) != 0) {
        printf("Double Free Error\n");
        FILE* fp = fopen(buffer, "w");
        memset(buffer, 0, sizeof(buffer));
        snprintf(buffer, sizeof(buffer), "[-] (%s %s %d) (%p)\n", filename, func, line, ptr);
        fwrite(buffer, sizeof(char), sizeof(buffer), fp);
        fclose(fp);
        return ;
    }
    free(ptr);
}

#define malloc(size)    my_malloc(size, __FILE__,__func__,__LINE__)
#define free(ptr)       my_free(ptr, __FILE__,__func__,__LINE__)

void main(int argc, char* argv[])
{
    void* ptr1 = malloc(10);
    void* ptr2 = malloc(20);
    void* ptr3 = malloc(30);
    void* ptr4 = malloc(40);
    void* ptr5 = ptr1;

    free(ptr1);
    free(ptr2);
    free(ptr4);
    free(ptr5);
}

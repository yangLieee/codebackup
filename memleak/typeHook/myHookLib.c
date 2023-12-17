 #define _GNU_SOURCE        
 #include <stdio.h>         
 #include <stdlib.h>        
 #include <dlfcn.h>

typedef void  (*glibc_free)(void* ptr);
typedef void* (*glibc_malloc)(size_t size);

static glibc_malloc gmalloc = NULL;
static glibc_free   gfree   = NULL;

static void hook_init(void)
{
    if(gmalloc == NULL) {
        gmalloc = dlsym(RTLD_NEXT, "malloc");
        if (!gmalloc) {
            fprintf(stderr, "unable to get malloc symbol!\n");
            exit(1);
        }
    }

    if(gfree == NULL) {
        gfree = dlsym(RTLD_NEXT, "free");
        if (!free) {
            fprintf(stderr, "unable to get free symbol!\n");
            exit(1);
        }
    }
    fprintf(stderr, "malloc and free successfully wrapped\n");
}

void* malloc(size_t size)
{
    void* caller = __builtin_return_address(1);
    if(gmalloc == NULL) {
        hook_init();
    }

    void* ptr = gmalloc(size);
    fprintf(stderr, "[+] caller %p malloc from %p size %ld\n", caller, ptr, size);
    // 如果打印，一定要用fprintf(stderr)，否则会产生无限循环，因为fprintf(stdout)也会使用malloc！

    return ptr;
}

void free(void* ptr)
{
    void* caller = __builtin_return_address(1);
    if(gfree == NULL) {
        hook_init();
    }

    gfree(ptr);
    fprintf(stderr, "[-] caller %p free from %p\n", caller, ptr);
}


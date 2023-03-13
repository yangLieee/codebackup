#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/*
 * 数组不连续初始化
 */
static void array_point_init(void)
{
    int array_a[100] = { [29] = 29, [13] = 2 };
    int array_b[100] = { [10 ... 23] = 1, [45 ... 89] = 3 };
    // switch ... case 也可以用...这个特性
    printf("a: %d %d\nb: %d %d\n", array_a[29], array_a[13], array_b[11], array_b[49]);
}


/*
 *  比较两个数据的最大值
 *  (void) (&_x == &_y) 作用是对于不同类型数据比较，指令类型不同会报警告
 */
#define my_max(x,y) ({              \
            typeof(x) _x = (x);     \
            typeof(y) _y = (y);     \
            (void) (&_x == &_y);    \
            _x > _y ? _x : _y;      \
        })

/*
 * 根据结构体中某一成员的地址得到结构体首地址
 */
#define offset( TYPE , MEMBER ) ((size_t) & (((TYPE*)0)->MEMBER))
#define container_of(ptr, type, member) ({                      \
            const typeof(((type*)0)->member)* _ptr = (ptr);   \
            (type*) ((char*)_ptr - offset(type, member));        \
        })



/*
 * 变长结构体
 */
struct student {
    int height;
    int weight;
    int age;
    
    int len;
    int ptr[0];
};

static void change_len_demo(void)
{
    struct student* yli;
    yli = (struct student*)malloc(sizeof(struct student) + 100);
    yli->len = 100;
    strcpy(yli->ptr, "I am yli\n");
    puts(yli->ptr);
    free(yli);
}

/*
 * builtin 函数
 */

void func_0(void)
{
    int *p = NULL;
    int *pr = NULL;
    p = __builtin_frame_address(0);
    pr = __builtin_return_address(0);
    printf("func_0 frame address: %p\n",p);
    printf("func_0 address : %p\n", pr);
    p = __builtin_frame_address(1);
    pr = __builtin_return_address(1);
    printf("call func_0 frame address: %p\n",p);
    printf("call func_0 address : %p\n", pr);
}
void func_1(void)
{
    int *p = NULL;
    p = __builtin_frame_address(0);
    printf("func_1 frame address: %p\n",p);
    func_0();
}

int main(int argc, char* argv[])
{
#if 0
    array_point_init();
    printf("%d\n", my_max(2, 9));
#endif
    change_len_demo();
    func_1();
    return 0;
}

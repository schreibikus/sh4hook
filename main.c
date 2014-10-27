#include <stdio.h>
#include <time.h>
#include "apihook.h"

void my_hook(int val)
{
    printf("%s\n", __FUNCTION__);
    printf("%s: Hooked with %d\n", __FUNCTION__, val);
}

void my_post_hook(int val)
{
    printf("%s: Post Hooked with %d\n", __FUNCTION__, val);
}

void api_hook_test(int val)
{
    #if 0
    int cal = val * val;
    int mas[10];
    int i;
    
    srand(time(0));
    
    for(i = 0; i < sizeof(mas)/sizeof(mas[0]); i++)
    {
        mas[i] = rand() + cal;
    }
    
    printf("val * val = %d and first elem %d\n", cal, mas[0]);
    
    for(i = 0; i < sizeof(mas)/sizeof(mas[0]); i++)
        cal += mas[i];
    
    printf("my val %d\n", val);
    val += cal;
    sleep(1);
    #else
    #if 0
    __asm__ ( "nop\n"
    "nop\n"
    "nop\n"
    "nop\n"
    "nop\n"
    "nop\n"
    "nop\n"
    );
    #endif
    int destval;
    __asm__ ( "mov r5,%0\n"
            : "=r" (destval)
    );
    printf("%s: Val %d and testval %d\n", __FUNCTION__, val, destval);
    sleep(1);
    #endif
}

int main()
{
    printf("Address of test function is 0x%X\n", (unsigned int)api_hook_test);
    api_hook_set(api_hook_test, my_hook, my_post_hook);
    api_hook_test(4);
    api_hook_test(7);
    api_hook_test(10);
    return 0;
}

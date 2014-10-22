#include <stdio.h>
#include "apihook.h"

void my_hook(int val)
{
    printf("%s\n", __FUNCTION__);
    printf("%s: Hooked with %d\n", __FUNCTION__, val);
}

int main()
{
    printf("Address of test function is 0x%X\n", (unsigned int)api_hook_test);
    api_hook_set(api_hook_test, my_hook);
    api_hook_test(4);
    api_hook_test(7);
    api_hook_test(10);
    return 0;
}

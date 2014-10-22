#include "apihook.h"
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <sys/mman.h>
#include <unistd.h>

void api_hook_test(int val)
{
#if 0
    int cal;
    int mas[10];
    int i;
    
    __asm__ ( "nop\n"
              "nop\n"
              "nop\n"
              "nop\n"
              "nop\n"
              "nop\n"
              "nop\n"
    );
    
    cal = val * val;

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
    __asm__ ( "nop\n"
    "nop\n"
    "nop\n"
    "nop\n"
    "nop\n"
    "nop\n"
    "nop\n"
    );
    printf("%s: Val %d\n", __FUNCTION__, val);
    //sleep(1);
#endif
}

void my_mega_code()
{
    int val = 234;

    printf("My mega function %d\n", val);
}

void jump_code(void (*func)())
{
    int value = 0x12345678;
#if 0
    __asm__ ( /*"mov.l %0,r0\n"*/
              "braf %0\n"
              "nop\n"
              : /* no output registers */
              : "r"(value) );
#endif
    __asm__ ( /*"mov.l %0,r0\n"*/
              "jsr @%0\n"
              "nop\n"
              : /* no output registers */
              : "r"(value) );

//    my_mega_code();
              
//    (*func)();
//    printf("Hello\n");
}

int api_hook_set(void *desire_func, void *hook)
{
    int i = 0;
    int pagesize = sysconf(_SC_PAGE_SIZE);
    unsigned int *address = desire_func;
    unsigned short *programm = desire_func;
    unsigned char *buffer;

    printf("Set branch from 0x%X address to 0x%X address\n", (unsigned int)desire_func, (unsigned int)hook);
    for(i = 0; i < 10; i++)
    {
        printf("address 0x%08X: data 0x%08X\n", address, *address);
        address++;
    }

    if (mprotect((void*)((unsigned int)programm & ~(pagesize - 1)), pagesize, PROT_READ | PROT_WRITE | PROT_EXEC) < 0)
    {
        printf("%s failed mprotect!!\n", __FUNCTION__);
    }
    else
    {
        buffer = memalign(pagesize, pagesize);
        if(!buffer)
        {
            printf("%s Failed allocate buffer\n", __FUNCTION__, __LINE__);
        }
        else
        {
            if (mprotect((void*)((unsigned int)buffer & ~(pagesize - 1)), pagesize, PROT_READ | PROT_WRITE | PROT_EXEC) < 0)
            {
                printf("%s failed mprotect!!\n", __FUNCTION__);
            }
            else
            {
                memset(buffer, 0, pagesize);

                printf("OK!\n");
                programm = buffer;
#if 1
                //programm = desire_func;
                *programm++ = 0xd00A; // mov.l #hook,r0
                *programm++ = 0x4f22; // sts.l pr, @-r15
                *programm++ = 0x400b; // JSR R0
                *programm++ = 0x09; // nop - executed after JSR
                *programm++ = 0x4f26; // lds.l @r15+,pr // restore return address
                *programm++ = 0x09;
                *programm++ = 0x09; // start copiied from desire_func
                *programm++ = 0x09;
                *programm++ = 0x09;
                *programm++ = 0x09;
                *programm++ = 0x09;
                *programm++ = 0x09;
                *programm++ = 0xd001; // mov.l (buffer - desire_func - 6),r0
                *programm++ = 0x402b; // jmp r0
                *programm++ = 0x09; // nop - executed before jmp
                *programm++ = 0x09; // nop - for align 4, can be other value
                *programm++ = ((unsigned int)desire_func + 12) & 0xFFFF;
                *programm++ = (((unsigned int)desire_func + 12) >> 16) & 0xFFFF;
                *programm++ = 0x09;
                *programm++ = 0x09;
                *programm++ = 0x09;
                *programm++ = 0x09;
                //*programm++ = 0x09;
                //*programm++ = 0x09;
                *programm++ = ((unsigned int)hook) & 0xFFFF;
                *programm++ = (((unsigned int)hook) >> 16) & 0xFFFF;

    //        programm = buffer;
    //        *programm = 0xd101;
    //    programm++;
    //    *programm = 0x0123;
    //    programm++;
    //    *programm = 0x09;
    //    programm++;
    //    *programm = 0x09;
    //    programm++;
            
#endif
                // copy code
                for(i = 0; i < 6; i++)
                {
                    *((unsigned short*)(buffer + 10 + 2 * i)) = *((unsigned short*)(desire_func + 2 * i));
                }

        //memcpy(desire_func, jump_code, 16);
#if 0
        programm = desire_func;
        *programm++ = 0xd001; // mov.l (buffer - desire_func - 6),r0
        *programm++ = 0x0023; // braf r0
        *programm++ = 0x09; // nop - executed after braf
        *programm++ = 0x09; // nop - for align 4, can be other value
        *programm++ = (unsigned int)((unsigned int)buffer - (unsigned int)desire_func - 6) & 0xFFFF;
        *programm++ = ((unsigned int)((unsigned int)buffer - (unsigned int)desire_func - 6) >> 16) & 0xFFFF;
#endif
        programm = desire_func;
        *programm++ = 0xd001; // mov.l (buffer),r0
        *programm++ = 0x402b; // jmp r0
        *programm++ = 0x09; // nop - executed after jmp
        *programm++ = 0x09; // nop - for align 4, can be other value
        *programm++ = ((unsigned int)buffer) & 0xFFFF;
        *programm++ = (((unsigned int)buffer) >> 16) & 0xFFFF;

        address = desire_func;
        for(i = 0; i < 10; i++)
        {
            printf("address 0x%08X: data 0x%08X\n", address, *address);
            address++;
        }
                printf("%s: Hook code\n", __FUNCTION__);
                address = buffer;
                for(i = 0; (i < 50) && (*address); i++)
                {
                    printf("address 0x%08X: data 0x%08X\n", address, *address);
                    address++;
                }
            }
        }
    }
}

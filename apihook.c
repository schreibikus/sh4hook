#include "apihook.h"
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <sys/mman.h>
#include <unistd.h>
#include <string.h>

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
    sleep(1);
#endif
}

#if 0
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
#endif
int api_hook_set(void *desire_func, void *hook)
{
    unsigned int desired_cpfunc_offset = 0;
    unsigned int additional_data_offset = 0;
    int pagesize = sysconf(_SC_PAGE_SIZE);
    unsigned short *programm;
    unsigned short *buffer;
    unsigned short *tptr;
    int i;

    printf("Set branch from 0x%X address to 0x%X address\n", (unsigned int)desire_func, (unsigned int)hook);
    for(i = 0, tptr = desire_func; i < 10; i++, tptr++)
        printf("desired function 0x%08X: data 0x%04X\n", tptr, *tptr);

    programm = desire_func;
    if (mprotect((void*)((unsigned int)programm & ~(pagesize - 1)), pagesize, PROT_READ | PROT_WRITE | PROT_EXEC) < 0)
    {
        printf("%s failed mprotect!!\n", __FUNCTION__);
    }
    else
    {
        buffer = (unsigned short *)memalign(pagesize, pagesize);
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
                memset((void*)buffer, 0, pagesize);

                printf("OK!\n");
                programm = buffer;

                *programm++ = 0xd015; // mov.l #hook,r0
                *programm++ = 0x2f46; // mov.l r4, @-r15
                *programm++ = 0x2f56; // mov.l r5, @-r15
                *programm++ = 0x2f66; // mov.l r6, @-r15
                *programm++ = 0x2f76; // mov.l r7, @-r15
                *programm++ = 0x4f22; // sts.l pr, @-r15
                *programm++ = 0x09; // for future usage
                *programm++ = 0x09; // for future usage
                *programm++ = 0x400b; // JSR R0 - must be aligned
                *programm++ = 0x09; // nop - executed after JSR
                *programm++ = 0x4f26; // lds.l @r15+,pr // restore return address
                *programm++ = 0x67f6; // mov.l @r15+,r7
                *programm++ = 0x66f6; // mov.l @r15+,r6
                *programm++ = 0x65f6; // mov.l @r15+,r5
                *programm++ = 0x64f6; // mov.l @r15+,r4
                *programm++ = 0x09; // for future usage
                *programm++ = 0x09; // for future usage
                *programm++ = 0x09; // for future usage
                desired_cpfunc_offset = (unsigned int)programm - (unsigned int)buffer; // offset for copy code
                *programm++ = 0x09; // start copiied from desire_func
                *programm++ = 0x09;
                *programm++ = 0x09;
                *programm++ = 0x09;
                *programm++ = 0x09;
                *programm++ = 0x09; // end copiied from desire_func
                *programm++ = 0x09; // not function for avoid some issues
                *programm++ = 0x09; // not function for avoid some issues
                *programm++ = 0xd001; // mov.l (buffer - desire_func - 6),r0
                *programm++ = 0x402b; // jmp r0
                *programm++ = 0x09; // nop - executed before jmp
                *programm++ = 0x09; // nop - for align 4, can be other value
                *programm++ = ((unsigned int)desire_func + 12) & 0xFFFF;
                *programm++ = (((unsigned int)desire_func + 12) >> 16) & 0xFFFF;
                additional_data_offset = (unsigned int)programm - (unsigned int)buffer; // offset for copied data
                *programm++ = 0x09;
                *programm++ = 0x09;
                *programm++ = 0x09;
                *programm++ = 0x09;
                *programm++ = 0x09;
                *programm++ = 0x09;
                *programm++ = 0x09;
                *programm++ = 0x09;
                *programm++ = 0x09;
                *programm++ = 0x09;
                *programm++ = 0x09;
                *programm++ = 0x09;
                *programm++ = ((unsigned int)hook) & 0xFFFF;
                *programm++ = (((unsigned int)hook) >> 16) & 0xFFFF;

                // copy code
                for(i = 0; i < 6; i++)
                {
                    unsigned short instruction = *((unsigned short*)(desire_func + 2 * i));
                    
                    if((instruction == 0x09) // nop
                       || ((instruction & 0xF0FF) == 0x4022) // sts.l pr,@-Rn
                       || ((instruction & 0xF00F) == 0x6003) // mov Rm,Rn
                       || ((instruction & 0xF00F) == 0x2000) // mov.b Rm,@Rn
                       || ((instruction & 0xF00F) == 0x2001) // mov.w Rm,@Rn
                       || ((instruction & 0xF00F) == 0x2002) // mov.l Rm,@Rn
                       
                       || ((instruction & 0xF00F) == 0x6000) // mov.b @Rm,Rn
                       || ((instruction & 0xF00F) == 0x6001) // mov.w @Rm,Rn
                       || ((instruction & 0xF00F) == 0x6002) // mov.l @Rm,Rn
                       
                       || ((instruction & 0xF00F) == 0x2004) // mov.b Rm,@-Rn
                       || ((instruction & 0xF00F) == 0x2005) // mov.w Rm,@-Rn
                       || ((instruction & 0xF00F) == 0x2006) // mov.l Rm,@-Rn


                            )
                    {
                        // just copy opcode
                        *((unsigned short*)((void*)buffer + desired_cpfunc_offset + 2 * i)) = instruction;
                    }
                    else
                    {
                        printf("%s %d: Have some interesting instruction 0x%04X\n", __FUNCTION__, __LINE__, instruction);
                        *((unsigned short*)((void*)buffer + desired_cpfunc_offset + 2 * i)) = instruction;
                    }
                }

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

                tptr = desire_func;
                for(i = 0; i < 10; i++)
                {
                    printf("Modified desired function 0x%08X: data 0x%04X\n", tptr, *tptr);
                    tptr++;
                }

                printf("%s: Our hook function code\n", __FUNCTION__);
                tptr = buffer;
                for(i = 0; (i < 50) && (*tptr); i++)
                {
                    printf("tptr 0x%08X: data 0x%04X\n", tptr, *tptr);
                    tptr++;
                }
            }
        }
    }
}

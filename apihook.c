#include "apihook.h"
#ifndef __KERNEL__
#include <stdlib.h>
#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>
#include <string.h>
#else
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/mm.h>
#include <asm/page.h>
#endif

#ifdef __KERNEL__
#define printf(x...) printk(x)
#endif

int api_hook_set(void *desire_func, void *hook, void *post_hook)
{
    unsigned int desired_cpfunc_offset = 0;
    unsigned int additional_data_offset = 0;
    unsigned int post_hook_init_code_offset = 0;
    unsigned int hook_code_offset = 0;
#ifndef __KERNEL__
    int pagesize = sysconf(_SC_PAGE_SIZE);
#else
    int pagesize = PAGE_SIZE;
#endif
    unsigned short *programm;
    unsigned short *buffer;
    unsigned short regnum;
    unsigned short *tptr;
    int i;

    printf("Set branch from 0x%X address to 0x%X address\n", (unsigned int)desire_func, (unsigned int)hook);
    for(i = 0, tptr = desire_func; i < 10; i++, tptr++)
        printf("desired function 0x%08X: data 0x%04X\n", (unsigned int)tptr, *tptr);
#ifndef __KERNEL__
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
#else
    {
        {
            buffer = kmalloc(pagesize, GFP_KERNEL);
            if(!buffer)
            {
                printk("%s failed alloc memory!\n", __FUNCTION__);
            }
#endif
            else
            {
                memset((void*)buffer, 0, pagesize);

                printf("OK!\n");
                programm = buffer;

                *programm++ = 0x2f06; // mov.l r0, @-r15
                *programm++ = 0x2f16; // mov.l r1, @-r15
                *programm++ = 0x2f26; // mov.l r2, @-r15
                *programm++ = 0x2f36; // mov.l r3, @-r15
                *programm++ = 0x2f46; // mov.l r4, @-r15
                *programm++ = 0x2f56; // mov.l r5, @-r15
                *programm++ = 0x2f66; // mov.l r6, @-r15
                *programm++ = 0x2f76; // mov.l r7, @-r15
                // Save T-flag
                *programm++ = 0x0029; // movt r0
                *programm++ = 0x2f06; // mov.l r0, @-r15
                // Save PR register
                *programm++ = 0x4f22; // sts.l pr, @-r15
                hook_code_offset = (unsigned int)programm - (unsigned int)buffer; // offset for modify next command
                *programm++ = 0xd000; // mov.l #hook,r0 - calculated later

                *programm++ = 0x400b; // JSR R0 - must be aligned
                *programm++ = 0x09; // nop - executed after JSR
                *programm++ = 0x4f26; // lds.l @r15+,pr // restore return address
                *programm++ = 0x60f6; // mov.l @r15+,r0 // restore T flag
                *programm++ = 0x4001; // shlr r0        // restore T flag
                *programm++ = 0x67f6; // mov.l @r15+,r7
                *programm++ = 0x66f6; // mov.l @r15+,r6
                *programm++ = 0x65f6; // mov.l @r15+,r5
                *programm++ = 0x64f6; // mov.l @r15+,r4
                *programm++ = 0x63f6; // mov.l @r15+,r3
                *programm++ = 0x62f6; // mov.l @r15+,r2
                *programm++ = 0x61f6; // mov.l @r15+,r1
                *programm++ = 0x60f6; // mov.l @r15+,r0
                *programm++ = 0x09; // for future usage

                if(post_hook)
                {
                    *programm++ = 0x2f46; // mov.l r4, @-r15
                    *programm++ = 0x2f56; // mov.l r5, @-r15
                    *programm++ = 0x2f66; // mov.l r6, @-r15
                    *programm++ = 0x2f76; // mov.l r7, @-r15
                    *programm++ = 0x4f22; // sts.l pr, @-r15
                    // modify PR register
                    *programm++ = 0x2f06; // mov.l r0, @-r15
                    post_hook_init_code_offset = (unsigned int)programm - (unsigned int)buffer; // offset for modify next command
                    *programm++ = 0xd000; // mov.l #post_hook_init,r0 - calculated later
                    *programm++ = 0x402a; // lds r0,pr // store return address
                    *programm++ = 0x60f6; // mov.l @r15+,r0
                    *programm++ = 0x09; // for future usage
                }

                desired_cpfunc_offset = (unsigned int)programm - (unsigned int)buffer; // offset for copy code
                *programm++ = 0x09; // start copiied from desire_func
                *programm++ = 0x09;
                *programm++ = 0x09;
                *programm++ = 0x09;
                *programm++ = 0x09;
                *programm++ = 0x09; // end copiied from desire_func
                *programm++ = 0x09; // not function for avoid some issues
                *programm++ = 0x09; // not function for avoid some issues
                 // return to function
                *programm++ = 0x2f06; // mov.l r0, @-r15
                *programm++ = 0xd001; // mov.l (buffer - desire_func - 6),r0
                *programm++ = 0x402b; // jmp @r0
                *programm++ = 0x60f6; // mov.l @r15+,r0 - executed after jmp and for align 4
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

                // calculate mov.l #hook,r0 command
                *((unsigned short*)((void*)buffer + hook_code_offset)) = 0xd000 | (((((unsigned int)programm - (unsigned int)buffer - hook_code_offset + 3) & ~0x3) - 4) >> 2);
                *programm++ = ((unsigned int)hook) & 0xFFFF;
                *programm++ = (((unsigned int)hook) >> 16) & 0xFFFF;

                if(post_hook)
                {
                    // calculate mov.l #post_hook_init,r0 command
                    *((unsigned short*)((void*)buffer + post_hook_init_code_offset)) = 0xd000 | (((((unsigned int)programm - (unsigned int)buffer - post_hook_init_code_offset + 3) & ~0x3) - 4) >> 2);
                    post_hook_init_code_offset = (unsigned int)programm + 4;
                    printf("Jump address 0x%X\n", post_hook_init_code_offset);
                    *programm++ = ((unsigned int)post_hook_init_code_offset) & 0xFFFF;
                    *programm++ = (((unsigned int)post_hook_init_code_offset) >> 16) & 0xFFFF;

                    *programm++ = 0x09;
                    *programm++ = 0x4f26; // lds.l @r15+,pr // restore return address
                    *programm++ = 0x67f6; // mov.l @r15+,r7
                    *programm++ = 0x66f6; // mov.l @r15+,r6
                    *programm++ = 0x65f6; // mov.l @r15+,r5
                    *programm++ = 0x64f6; // mov.l @r15+,r4

                    *programm++ = 0x2f06; // mov.l r0, @-r15
                    *programm++ = 0xd001; // mov.l (buffer),r0
                    *programm++ = 0x402b; // jmp @r0
                    *programm++ = 0x60f6; // mov.l @r15+,r0 - executed after jmp and for align 4
                    *programm++ = ((unsigned int)post_hook) & 0xFFFF;
                    *programm++ = (((unsigned int)post_hook) >> 16) & 0xFFFF;
                }

                // copy code
                for(i = 0; i < 6; i++)
                {
                    unsigned short instruction = *((unsigned short*)(desire_func + 2 * i));

                    if((instruction == 0x09) // nop
                        || (instruction == 0x000B) // rts
                        )
                    {
                        // just copy opcode
                    }
                    else if(((instruction & 0xF000) == 0xe000) // mov #imm,Rn
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

                        || ((instruction & 0xF00F) == 0x6004) // mov.b @Rm+,Rn
                        || ((instruction & 0xF00F) == 0x6005) // mov.w @Rm+,Rn
                        || ((instruction & 0xF00F) == 0x6006) // mov.l @Rm+,Rn

                        || ((instruction & 0xF00F) == 0x300C) // add Rm,Rn
                        || ((instruction & 0xF000) == 0x7000) // add #imm,Rn
                        || ((instruction & 0xF00F) == 0x300E) // addc Rm,Rn
                        || ((instruction & 0xF00F) == 0x300F) // addv Rm,Rn

                        || ((instruction & 0xF00F) == 0x0007) // mul.l Rm,Rn
                        || ((instruction & 0xF0FF) == 0x400B) // jsr @Rn

                        || ((instruction & 0xF0FF) == 0x4022) // sts.l pr,@-Rn

                         )
                    {
                        // just copy opcode
                    }
                    else if((instruction & 0xF000) == 0xD000) // mov.l @(disp,PC),Rn - MOVe constant value
                    {
                        int disp = (instruction & 0xFF) << 2; // offset from (desire_func + 2 * i)

                        //printf("%s %d: ORIGINAL INSTRUCTION 0x%X\n", __FUNCTION__, __LINE__, instruction);

                        // modify instruction
                        instruction = (instruction & 0xFF00) |  ((additional_data_offset - (desired_cpfunc_offset + ((2 * i) & ~0x3)) - 4) >> 2);
                        //printf("%s %d: NEW INSTRUCTION 0x%X\n", __FUNCTION__, __LINE__, instruction);

                        // store data
                        additional_data_offset = (additional_data_offset + 3) & ~0x3;
                        *((unsigned short*)((void*)buffer + additional_data_offset)) = *((unsigned short*)(desire_func + ((2 * i) & ~0x3) + 4 + disp));
                        //printf("%s %d: COPIED DATA 0x%X\n", __FUNCTION__, __LINE__, *((unsigned short*)(desire_func + ((2 * i) & ~0x3) + 4 + disp)));
                        additional_data_offset += 2;
                        disp += 2;
                        *((unsigned short*)((void*)buffer + additional_data_offset)) = *((unsigned short*)(desire_func + ((2 * i) & ~0x3) + 4 + disp));
                        //printf("%s %d: COPIED DATA 0x%X\n", __FUNCTION__, __LINE__, *((unsigned short*)(desire_func + ((2 * i) & ~0x3) + 4 + disp)));
                        additional_data_offset += 2;
                    }
                    else if((instruction & 0xF0FF) == 0x402B) // jmp @Rn
                    {
                        printf("%s %d: Need make visual control of function with address 0x%08X\n", __FUNCTION__, __LINE__, (unsigned int)desire_func);
                    }
                    else
                    {
                        printf("%s %d: Have some interesting instruction 0x%04X\n", __FUNCTION__, __LINE__, instruction);
                    }

                    // just save opcode
                    *((unsigned short*)((void*)buffer + desired_cpfunc_offset + 2 * i)) = instruction;
                }

                programm = desire_func;
                *programm++ = 0x2f06; // mov.l r0, @-r15
                *programm++ = 0xd001; // mov.l (buffer),r0
                *programm++ = 0x402b; // jmp @r0
                *programm++ = 0x60f6; // mov.l @r15+,r0 - executed after jmp and for align 4
                *programm++ = ((unsigned int)buffer) & 0xFFFF;
                *programm++ = (((unsigned int)buffer) >> 16) & 0xFFFF;

                tptr = desire_func;
                for(i = 0; i < 10; i++)
                {
                    printf("Modified desired function 0x%08X: data 0x%04X\n", (unsigned int)tptr, *tptr);
                    tptr++;
                }

                printf("%s: Our hook function code\n", __FUNCTION__);
                tptr = buffer;
                for(i = 0; (i < 100) && (*tptr); i++)
                {
                    printf("tptr 0x%08X: data 0x%04X\n", (unsigned int)tptr, *tptr);
                    tptr++;
                }
            }
        }
    }
    return 0;
}

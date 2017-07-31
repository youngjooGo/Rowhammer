/*
 * Example of using hugepage memory in a user application using the mmap
 * system call with MAP_HUGETLB flag.  Before running this program make
 * sure the administrator has allocated enough default sized huge pages
 * to cover the 256 MB allocation.
 *
 * For ia64 architecture, Linux kernel reserves Region number 4 for hugepages.
 * That means the addresses starting with 0x800000... will need to be
 * specified.  Specifying a fixed address is not required on ppc64, i386
 * or x86_64.
 */
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <time.h>
#include <sys/time.h>
#include <math.h>
#include "rowhammer.h"
#include "mapping.h"

// #define NDEBUG
#include <assert.h>

#define PRECISE_CHECK 0

#define PATTERN_ALL_ONE 1
#define PATTERN_ALL_ZERO 2
#define PATTERN_ROW_STRIPE_ODD 3
#define PATTERN_ROW_STRIPE_EVEN 4
#define PATTERN_COLUMN_STRIPE_ODD 5
#define PATTERN_COLUMN_STRIPE_EVEN 6
#define PATTERN_LATTICE_ODD 7
#define PATTERN_LATTICE_EVEN 8
#define PATTERN_TARGET_ONE 9
#define PATTERN_TARGET_ZERO 10

#define DATA_ONE ((char) 0xff)
#define DATA_ROW_STRIPE (DATA_ONE) 
#define DATA_COLUMN_STRIPE ((char) 0xaa)
#define DATA_LATTICE (DATA_COLUMN_STRIPE)

#define LENGTH (256UL*1024*1024)
#define PROTECTION (PROT_READ | PROT_WRITE)

#ifndef MAP_HUGETLB
#define MAP_HUGETLB 0x40000 /* arch specific */
#endif

/* Only ia64 requires this */
#ifdef __ia64__
#define ADDR (void *)(0x8000000000000000UL)
#define FLAGS (MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB | MAP_FIXED)
#else
#define ADDR (void *)(0x0UL)
#define FLAGS (MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB)
#endif

#define ON_RANGE(x) __on_range((unsigned long) x)
#define __on_range(x) ((x >= start_addr)? \
                    ((x < end_addr)? 1 : 0) : 0)

#define HUGEPAGE_SIZE 21
#define ROW_START_BIT 16
#define BYTE_ON_ROW ((int) pow(2, BANK_END))

#define BANK_BIT_NUM 4
#define BANK_SIZE ((int) pow(2, BANK_BIT_NUM))
#define ROW_ON_PAGE ((int) pow(2, 4))
#define PAGE_NUM (LENGTH / ((int) pow(2, HUGEPAGE_SIZE)))

// Global variables
static unsigned long start_addr, end_addr;
FILE *ofp;

static void check_bytes(char *addr)
{
	printf("First hex is %x\n", *((unsigned int *)addr));
}

static void write_bytes(char *addr, int pattern)
{
	unsigned long i, j, k;
    char odd_data, even_data, data;
    char *current = addr;

    switch(pattern)
    {
        case PATTERN_ALL_ONE:
            odd_data = even_data = DATA_ONE;
            break;
        case PATTERN_ALL_ZERO:
            odd_data = even_data = ~DATA_ONE;
            break;
        case PATTERN_ROW_STRIPE_ODD:
            odd_data = DATA_ONE;
            even_data = ~DATA_ONE;
            break;
        case PATTERN_ROW_STRIPE_EVEN:
            odd_data = ~DATA_ONE;
            even_data = DATA_ONE;
            break;
        case PATTERN_COLUMN_STRIPE_ODD:
            odd_data = even_data = DATA_COLUMN_STRIPE;
            break;
        case PATTERN_COLUMN_STRIPE_EVEN:
            odd_data = even_data = ~DATA_COLUMN_STRIPE;
            break;
        case PATTERN_LATTICE_ODD:
            odd_data = DATA_COLUMN_STRIPE;
            even_data = ~DATA_COLUMN_STRIPE;
            break;
        case PATTERN_LATTICE_EVEN:
            odd_data = ~DATA_COLUMN_STRIPE;
            even_data = DATA_COLUMN_STRIPE;
            break;
        case PATTERN_TARGET_ONE:
        case PATTERN_TARGET_ZERO:
        default:
            printf("[Info] Pattern misinput error!\n");
            assert(0);
    }

    for(i = 0; i < LENGTH; i++)
    {
        if( ( (char)current >> 16 ) % 2 == 0 )
            data = even_data;
        else
            data = odd_data;

        *current = data;
        current++;
    }

    printf("%p - %p\n", addr, current);
}

static int read_bytes(char *addr, char *target, int pattern)
{
	unsigned long i, j, k;
    int result = 0;
    char odd_data, even_data, data;
    char *current = addr;
    int count = 0;

    switch(pattern)
    {
        case PATTERN_ALL_ONE:
            odd_data = even_data = DATA_ONE;
            break;
        case PATTERN_ALL_ZERO:
            odd_data = even_data = ~DATA_ONE;
            break;
        case PATTERN_ROW_STRIPE_ODD:
            odd_data = DATA_ONE;
            even_data = ~DATA_ONE;
            break;
        case PATTERN_ROW_STRIPE_EVEN:
            odd_data = ~DATA_ONE;
            even_data = DATA_ONE;
            break;
        case PATTERN_COLUMN_STRIPE_ODD:
            odd_data = even_data = DATA_COLUMN_STRIPE;
            break;
        case PATTERN_COLUMN_STRIPE_EVEN:
            odd_data = even_data = ~DATA_COLUMN_STRIPE;
            break;
        case PATTERN_LATTICE_ODD:
            odd_data = DATA_COLUMN_STRIPE;
            even_data = ~DATA_COLUMN_STRIPE;
            break;
        case PATTERN_LATTICE_EVEN:
            odd_data = ~DATA_COLUMN_STRIPE;
            even_data = DATA_COLUMN_STRIPE;
            break;
        case PATTERN_TARGET_ONE:
        case PATTERN_TARGET_ZERO:
        default:
            printf("[Info] Pattern misinput error!\n");
            assert(0);
    }

    for(i = 0; i < LENGTH; i++)
    {
       
        if( ( (char)cureent >> 16 ) % 2 == 0 )
            data = even_data;
        else
            data = odd_data;

		if (*current != data) 
        {
			printf("Target %p - Mismatch at %p: %x\n", target, current, (*(current) & 0xff));
			fprintf(ofp, "Target %p - Mismatch at %p: %x\n", target, current, (*(current) & 0xff));
            *current = data;
            result++;
		}
        current++;
       
    }

	return result;
}

static void load_sequence(int *sequence)
{
    FILE *ifp;
    int i;

    ifp = fopen("random_sequence.txt", "r");

    for(i = 0; i < ROW_ON_PAGE; i++)
    {
        fscanf(ifp, "%d ", &sequence[i]);
    }
}

static void spend_msec(int msec)
{
    int count = 0;
    unsigned long temp_ms;
    struct timeval temp_time;
    
    gettimeofday(&temp_time, NULL);
    temp_ms = temp_time.tv_usec / 1000;

    for(;count < msec - 1;)
    {
        gettimeofday(&temp_time, NULL);

        if(temp_time.tv_usec / 1000 != temp_ms)
        {
            temp_ms = temp_time.tv_usec / 1000;
            count++;
        }
    }
}
unsigned long inc_bank_address(char ptr, int inc)
{
    unsigned long bank,row,column,bank_1,bank_2;

    bank_1 = (ptr & 0x7000) >> 12;
    bank_2 = (ptr & 0x300000) >> 17;
    bank = (bank_1 | bank_2) + inc;
    bank_1 = bank & (0x7);
    bank_2 = bank * (0x18);
    bank = (bank_1 << 17) | (bank_2 << 12);
    row = ptr & 0x1ffcf0000;
    column = ptr & 0x8fff;

    return (bank | row | column);
}

unsigned long inc_row_address(unsigned long ptr, int inc)
{
    char bank,row,column,row_1,row_2;

    bank = ptr & 0x307000;
    row_1 = ptr & 0x1ffc00000;
    row_2 = ptr & 0xf0000;
    row_1 = row_1 >> 18;
    row_2 = row_2 >> 16;
    row = (row_1 | row_2) + inc;
    row_1 = (row & 0x7ff0) >> 4;
    row_2 = (row & 0xf);
    row = (row_1 << 22) | (row_2 << 16);
    column = ptr & 0x8fff;

    return (bank | row | column);
}
// Start time is ranging from 0 to 64 (ms)
static int hugepage_hammering_starttime(
        char *addr, 
        int *sequence, 
        int pattern,
        struct timeval basetime,
        int run_offset)
{
    struct timeval start_time, end_time;
    unsigned long i, upper, lower, timediff;
    struct timeval temp_time;
    long prev_row = -1, temp_ms;
    char prev_bank[8] = {0}, prev_page = -1;
    unsigned long target_addr = (unsigned long) addr;
    int j, k, l, offset;

    check_bytes(addr);

    for(i = 0; i < PAGE_NUM; i++)
    {
        for(j = 0; j < ROW_ON_PAGE; j++)
        {
            target_addr = inc_row_address(target_addr, j);

            for(k = 0; k < BANK_SIZE; k++)
            {
                target_addr = inc_bank_address( target_addr,k);

                // Get the adjacent upper row and the adjacent lower row.
                upper = get_upper_row(target_addr);
                lower = get_lower_row(target_addr);

                // Remove out of range cases.
                if(!ON_RANGE(upper) || !ON_RANGE(lower))
                    continue;

                // Set start time of hammering.
                for(offset = 0; offset < 64; offset++)
                {
                    gettimeofday(&temp_time, NULL);
                    temp_ms = temp_time.tv_usec / 1000;
                    while(1)
                    {
                        gettimeofday(&temp_time, NULL);

                        if(temp_time.tv_usec / 1000 != temp_ms)
                        {
                            temp_ms = temp_time.tv_usec / 1000;

                            timediff = (temp_time.tv_sec * 1000000 + temp_time.tv_usec) / 1000;
                            if((timediff % 64) == offset)
                                break;
                        }
                    }
    
                    double_sided_hammer((long *) upper, (long *) lower);
                }
               
            }

            if(PRECISE_CHECK)
                read_bytes(addr, addr + i*(1<<HUGEPAGE_SIZE) + 
                             sequence[j] * (1<<BANK_END), pattern);
        }
    }

    return 0;
}

static int hugepage_hammering(
        char *addr, 
        int *sequence, 
        int pattern)
{
    struct timeval dummy;
    hugepage_hammering_starttime(addr, sequence, pattern, dummy, -1);
}

int main(int argc, char *argv[])
{
	void *addr;
	int ret;
    time_t now;
    int sequence[ROW_ON_PAGE];
    int pattern = PATTERN_ROW_STRIPE_ODD, i;
    struct timeval basetime;
    int count = 0;
    
    if (argc>1)
        ofp = fopen(argv[1], "w");
    else
        assert(0);
    

	addr = mmap(ADDR, LENGTH, PROTECTION, FLAGS, 0, 0);
	if (addr == MAP_FAILED) {
		perror("mmap");
		exit(1);
	}

    load_sequence(sequence);

    start_addr = (long) addr;
    end_addr = (long) ((char *) addr + LENGTH);
	printf("Returned address is %lx %lx\n", start_addr, end_addr);
	fprintf(ofp, "Returned address is %lx %lx\n", start_addr, end_addr);

    gettimeofday(&basetime, NULL);

    for (pattern = 7; pattern < 8; pattern++)
    {
        // for(i = 0; i < 64; i++)
        {
            printf("Input pattern: %d / Offset: %d\n", pattern, 0);
            fprintf(ofp, "Input pattern: %d / Offset: %d\n", pattern, 0);
            check_bytes(addr);
            write_bytes(addr, pattern);
            read_bytes(addr, NULL, pattern);
            now = time(NULL);
            printf("Timestamp: %ld\n", now);
            // hugepage_hammering(addr, sequence, pattern);
            hugepage_hammering_starttime(addr, sequence, pattern, basetime, 0);
            printf("Timestamp: %ld \t Spent time: %ld\n", time(NULL), time(NULL) - now);
            read_bytes(addr, NULL, pattern);
        }
    }

	/* munmap() length of MAP_HUGETLB memory must be hugepage aligned */
	if (munmap(addr, LENGTH)) {
		perror("munmap");
		exit(1);
	}

	return ret;
}

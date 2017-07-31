#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <errno.h>
#include <string.h>

#define PAGEMAP_ENTRY 8
#define GET_BIT(X,Y) (X & ((uint64_t)1<<Y)) >> Y
#define GET_PFN(X) X & 0x7FFFFFFFFFFFFF

const int __endian_bit = 1;
#define is_bigendian() ( (*(char*)&__endian_bit) == 0 )

uintptr_t virt_to_phys_huge(uintptr_t vaddr)
{
    FILE *kpagecgroup;
    uintptr_t paddr = 0;
    int pagesize = 4*1024;
    int offset = (vaddr / pagesize) * sizeof(uint64_t);
    uint64_t flags;

    if((kpagecgroup = fopen("/proc/kpageflags", "rb")))
    {
        if(lseek(fileno(kpagecgroup), offset, SEEK_SET) == offset)
        {
            if(fread(&flags, sizeof(uint64_t), 1, kpagecgroup))
            {
            } else
                return 3;
        } else
            return 2;
    } else
        return 1;

    return paddr;
}

uintptr_t virt_to_phys(uintptr_t vaddr) {
    FILE *pagemap;
    intptr_t paddr = 0;
    long pagesize = 1024*1024*2;
    int offset = (vaddr / pagesize) * sizeof(uint64_t);
    uint64_t e;

    printf("Page size: %ld\n", pagesize);
    // https://www.kernel.org/doc/Documentation/vm/pagemap.txt
    if ((pagemap = fopen("/proc/self/pagemap", "rb"))) {
        if (lseek(fileno(pagemap), offset, SEEK_SET) == offset) {
            if (fread(&e, sizeof(uint64_t), 1, pagemap)) {
                if (e & (1ULL << 63)) 
                { // page present ?
                    paddr = e & ((1ULL << 54) - 1); // pfn mask
                    paddr = paddr << 21;
                    // add offset within page
                    paddr = paddr | (vaddr & (pagesize - 1));
                } else
                    return 4;   
            } else
                return 3;
        } else
            return 2;
        fclose(pagemap);
    } else
        return 1;

    return paddr;
}

/*
long virt_to_phys(unsigned long virt_addr)
{
    int i, c, pid, status;
    uint64_t read_val, file_offset;
    char path_buf [0x100] = {};
    FILE * f;
    unsigned long phys_addr;


    pid = getpid();
    // printf("pid : %d\n",pid);
    sprintf(path_buf, "/proc/%u/pagemap", pid);
    //printf("Big endian? %d\n", is_bigendian());
    f = fopen(path_buf, "rb");
    if(!f){
        printf("Error! Cannot open %s\n", path_buf);
        sleep(1);
        return -1;
    }

    //Shifting by virt-addr-offset number of bytes
    //and multiplying by the size of an address (the size of an entry in pagemap file)
    file_offset = virt_addr / getpagesize() * PAGEMAP_ENTRY;
    status = fseek(f, file_offset, SEEK_SET);
    if(status){
        perror("Failed to do fseek!");
        return -1;
    }
    errno = 0;
    read_val = 0;
    unsigned char c_buf[PAGEMAP_ENTRY];
    for(i=0; i < PAGEMAP_ENTRY; i++){
        c = getc(f);
        if(c==EOF){
            printf("\nReached end of the file\n");
            return 0;
        }
        if(is_bigendian())
            c_buf[i] = c;
        else
            c_buf[PAGEMAP_ENTRY - i - 1] = c;
        //printf("[%d]0x%x ", i, c);
    }
    for(i=0; i < PAGEMAP_ENTRY; i++){
        //printf("%d ",c_buf[i]);
        read_val = (read_val << 8) + c_buf[i];
    }
    if(GET_BIT(read_val, 63))
    {
        phys_addr = read_val & ((1ULL << 54) - 1); // pfn mask
        phys_addr = phys_addr * sysconf(_SC_PAGESIZE);
        // add offset within page
        phys_addr = phys_addr | (virt_addr & (sysconf(_SC_PAGESIZE) - 1));
        
        PT[PTN].PA = (GET_PFN(read_val)<<12)|(0xFFF&virt_addr);
        //printf("virtual : %p   /   physical : %p\n",PT[PTN].PA,virt_addr);
        PT[PTN++].VA = (unsigned long) virt_addr;
        
    }
        //return GET_PFN(read_val);
    else
        printf("Page not present\n");
    if(GET_BIT(read_val, 62))
        printf("Page swapped\n");
    fclose(f);
    return phys_addr;
}
*/

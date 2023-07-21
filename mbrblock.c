#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include "mbrblock.h"
#include "lfs_port.h"

#define min(a,b) (a>b?b:a)
#define max(a,b) (a>b?a:b)


typedef uint32_t bd_addr_t;
typedef uint32_t bd_size_t;

int mbr_sync(void)
{
    return 0;
}

int mbr_read(void *b, bd_addr_t addr, bd_size_t size)
{
    user_flash_read(addr, b, size);
    return 0;
}

int mbr_program(const void *b, bd_addr_t addr, bd_size_t size)
{
    user_flash_prog(addr, b, size);
    return 0;
}

int mbr_erase(bd_addr_t addr, bd_size_t size)
{
    user_flash_erase(addr, (size+SECTOR_SIZE-1)/SECTOR_SIZE);
    return 0;
}

int mbr_zero(bd_addr_t addr, bd_size_t size)
{
    char buf[SECTOR_SIZE] = {0};
    for(int i=0; i<(size+SECTOR_SIZE-1)/SECTOR_SIZE; i++)
    {
    	user_flash_prog(addr+i*sizeof(buf), buf, sizeof(buf));
    }
    return 0;
}

bd_size_t mbr_get_program_size(void)
{
    return 1;
}

bd_size_t mbr_get_erase_size()
{
    return SECTOR_SIZE;
}






// Little-endian conversion, should compile to noop
// if system is little-endian
static inline uint32_t tole32(uint32_t a)
{
    union {
        uint32_t u32;
        uint8_t u8[4];
    } w;

    w.u8[0] = a >>  0;
    w.u8[1] = a >>  8;
    w.u8[2] = a >> 16;
    w.u8[3] = a >> 24;

    return w.u32;
}

static inline uint32_t fromle32(uint32_t a)
{
    return tole32(a);
}

static void tochs(uint32_t lba, uint8_t chs[3])
{
    uint32_t sector = min(lba, 0xfffffd) + 1;
    chs[0] = (sector >> 6) & 0xff;
    chs[1] = ((sector >> 0) & 0x3f) | ((sector >> 16) & 0xc0);
    chs[2] = (sector >> 14) & 0xff;
}


void *partition_mbr_read(void);
void partition_mbr_free(void *mbr);
int partition_check(void *mbr, uint8_t part, MBR_PART_ATTR *attr);
int partition(struct mbr_table *table, int part, uint8_t type, bd_addr_t start, bd_addr_t stop);
void partition_format_buffer(void *mbr_buffer, int mbr_buffer_size, uint32_t table_start_offset);
void partition_write_setup(struct mbr_table *table);
void partition_mbr_buffer_program(void *mbr_buffer, int mbr_buffer_size);
void *partition_mbr_buffer_read(uint32_t *mbr_buffer_size);


void *partition_mbr_buffer_read(uint32_t *mbr_buffer_size)
{
    // Allocate smallest buffer necessary to write MBR
    uint32_t buffer_size = sizeof(struct mbr_table);

    // Prevent alignment issues
    if (buffer_size % mbr_get_program_size() != 0) {
        buffer_size += mbr_get_program_size() - (buffer_size % mbr_get_program_size());
    }

    uint8_t *buffer = (uint8_t *)malloc(buffer_size);

    // Check for existing MBR
    int err = mbr_read(buffer, 512 - buffer_size, buffer_size);
    if (err) {
        free(buffer);
        return NULL;
    }
    
    *mbr_buffer_size = buffer_size;
    
    return buffer;
}

void partition_mbr_buffer_program(void *mbr_buffer, int mbr_buffer_size)
{
    // Write out MBR
    int err = mbr_erase(0, mbr_get_erase_size());
    if (err) {
        free(mbr_buffer);
        return;
    }

    err = mbr_program(mbr_buffer, 512 - mbr_buffer_size, mbr_buffer_size);
    free(mbr_buffer);
}

void partition_write_setup(struct mbr_table *table)
{
    // Setup default values for MBR
    table->signature[0] = 0x55;
    table->signature[1] = 0xaa;
    
    memset(table->entries, 0, sizeof(table->entries));
}

void partition_format_buffer(void *mbr_buffer, int mbr_buffer_size, uint32_t table_start_offset)
{
    
    // As the erase operation may do nothing, erase remainder of the buffer, to eradicate
    // any remaining programmed data (such as previously programmed file systems).
    if (table_start_offset > 0) {
        memset(mbr_buffer, 0xFF, table_start_offset);
    }
    if (table_start_offset + sizeof(struct mbr_table) < mbr_buffer_size) {
        memset(mbr_buffer + table_start_offset + sizeof(struct mbr_table), 0xFF,
               mbr_buffer_size - (table_start_offset + sizeof(struct mbr_table)));
    }
}

// Partition after address are turned into absolute
// addresses, assumes bd is initialized
static int partition_absolute(struct mbr_table *table, int part, uint8_t type,
    bd_size_t offset, bd_size_t size)
{
    // For Windows-formatted SD card, it is not partitioned (no MBR), but its PBR has the
    // same boot signature (0xaa55) as MBR. We would easily mis-recognize this SD card has valid
    // partitions if we only check partition type. We add check by only accepting 0x00 (inactive)
    // /0x80 (active) for valid partition status.
    for (int i = 1; i <= 4; i++) {
        if (table->entries[i - 1].status != 0x00 &&
                table->entries[i - 1].status != 0x80) {
            memset(table->entries, 0, sizeof(table->entries));
            break;
        }
    }

    // Setup new partition
    //MBED_ASSERT(part >= 1 && part <= 4);
    table->entries[part - 1].status = 0x00; // inactive (not bootable)
    table->entries[part - 1].type = type;

    // lba dimensions
    //MBED_ASSERT(bd->is_valid_erase(offset, size));
    uint32_t sector = max(mbr_get_erase_size(), 512);
    uint32_t lba_offset = offset / sector;
    uint32_t lba_size = size / sector;
    table->entries[part - 1].lba_offset = tole32(lba_offset);
    table->entries[part - 1].lba_size = tole32(lba_size);

    // chs dimensions
    tochs(lba_offset,            table->entries[part - 1].chs_start);
    tochs(lba_offset + lba_size - 1, table->entries[part - 1].chs_stop);

    // Check that we don't overlap other entries
    for (int i = 1; i <= 4; i++) {
        if (i != part && table->entries[i - 1].type != 0x00) {
            uint32_t neighbor_lba_offset = fromle32(table->entries[i - 1].lba_offset);
            uint32_t neighbor_lba_size = fromle32(table->entries[i - 1].lba_size);
            //MBED_ASSERT(
            //    (lba_offset >= neighbor_lba_offset + neighbor_lba_size) ||
            //    (lba_offset + lba_size <= neighbor_lba_offset));
            (void)neighbor_lba_offset;
            (void)neighbor_lba_size;
        }
    }

    return 0;
}


int partition(struct mbr_table *table, int part, uint8_t type, bd_addr_t start, bd_addr_t stop)
{
    // Calculate dimensions
    bd_size_t offset = start;
    bd_size_t size;

    if (offset < 512) {
        offset += max(mbr_get_erase_size(), 512);
    }

    size = stop - offset;

    int err = partition_absolute(table, part, type, offset, size);
    if (err) {
        return err;
    }

    return 0;
}

enum {
	PART_TYPE_FAT12      = 0x01, //FAT12
	PART_TYPE_XENIX_ROOT = 0x02, //XENIX root
	PART_TYPE_XENIX      = 0x03, //XENIX /usr
	PART_TYPE_FAT16      = 0x04, //FAT16 (16位)
	PART_TYPE_EXT        = 0x05, //扩展分区
	PART_TYPE_FAT16_32BIT = 0x06, //FAT16 (32位)
	PART_TYPE_NTFS       = 0x07, //NTFS（Windows NT/2000/XP/Vista/7/8/10）
	PART_TYPE_FAT32      = 0x0B, //FAT32
	PART_TYPE_FAT32_LBA  = 0x0C, //FAT32 (LBA)
	PART_TYPE_LINUX_SWAP = 0x82, //Linux Swap 分区
	PART_TYPE_LINUX      = 0x83, //Linux 文件系统
	PART_TYPE_FREEBSD    = 0xA5, //FreeBSD
	PART_TYPE_OPENBSD    = 0xA6, //OpenBSD
	PART_TYPE_HFS        = 0xA8, //macOS HFS+
	PART_TYPE_EFI        = 0xEB, //EFI System 分区
};

const MBR_PART_ATTR mbr_attr[] = {
    {   
        .part = 1, 
        .offset = 0x200, 
        .size = 0x80000,
        .type = PART_TYPE_LINUX,
    },
    {
        .part = 2, 
        .offset = 0x80200, 
        .size = 0x10000,
        .type = PART_TYPE_LINUX,
    },
    {
        .part = 3, 
        .offset = 0x90200, 
        .size = 0x6fe00,
        .type = PART_TYPE_LINUX,
    },
};

void partition_init(void)
{
    void *mbr = partition_mbr_read();
    uint32_t buffer_size = 0;
    uint32_t table_start_offset;
    void *buffer;

    for(int i=0; i<sizeof(mbr_attr)/sizeof(mbr_attr[0]); i++)
    {
        MBR_PART_ATTR attr = {0};
        int err = partition_check(mbr, mbr_attr[i].part, &attr);
        
        printf("read partition %d type %d, offset %d, size %d.\n", 
	    		attr.part, attr.type, attr.offset, attr.size);
	    		
        if(err || i != mbr_attr[i].part-1 || attr.type != mbr_attr[i].type)
        {
            //repartition
            printf("partition_init error, repartition. %d %d %d\n", err, i != mbr_attr[i].part-1,
            		attr.type != mbr_attr[i].type);
            partition_mbr_free(mbr);
            goto repart;
        }
    }
    
    if(sizeof(mbr_attr)/sizeof(mbr_attr[0]) == 0)
    {
    	struct mbr_table *table = (struct mbr_table *)mbr;
    	if (table->signature[0] == 0x55 && table->signature[1] == 0xaa) {
	    mbr_erase(0, mbr_get_erase_size());
	}
    }
    
    partition_mbr_free(mbr);
    
    printf("partition_init ok\n");
    return;
    
repart:
    buffer = partition_mbr_buffer_read(&buffer_size);
    memset(buffer, 0xFF, buffer_size);
    
    printf("buffer_size %d\n", buffer_size);
    
    //assert(buffer);
    
    table_start_offset = buffer_size - sizeof(struct mbr_table);
    struct mbr_table *table = (struct mbr_table *)(buffer+table_start_offset);
    
    partition_write_setup(table);
    
    for(int i=0; i<sizeof(mbr_attr)/sizeof(mbr_attr[0]); i++)
    {
    	printf("create entry table: \n\t%d, type %d, offset %d, size %d\n", 
    		mbr_attr[i].part, mbr_attr[i].type, mbr_attr[i].offset, mbr_attr[i].size);
        partition(table, 
			mbr_attr[i].part, 
			mbr_attr[i].type, 
			mbr_attr[i].offset, 
			mbr_attr[i].offset + mbr_attr[i].size);
    }
    
    uint8_t *p_data = (uint8_t *)table;
    for(int i=0; i<5; i++)
    {
    	for(int j=0; j<16; j++)
    	{
    		printf("%02hhx ", p_data[i*16+j]);
    	}
    	printf("\n");
    }
    
    partition_format_buffer(buffer, buffer_size, table_start_offset);
    
    partition_mbr_buffer_program(buffer, buffer_size);
}


void *partition_mbr_read(void)
{
    uint32_t buffer_size;
    void *buffer = 0;
    
    // Allocate smallest buffer necessary to write MBR
    buffer_size = sizeof(struct mbr_table);
    buffer = malloc(buffer_size);

    int err = mbr_read(buffer, 512 - buffer_size, buffer_size);
    if (err) {
        return NULL;
    }
    
    return buffer;
}

void partition_mbr_free(void *mbr)
{
    free(mbr);
}


int partition_check(void *mbr, uint8_t part, MBR_PART_ATTR *attr)
{
    struct mbr_table *table;
    bd_size_t sector;

    // Check for valid table
    table = (struct mbr_table *)mbr;
    if (table->signature[0] != 0x55 || table->signature[1] != 0xaa) {
        return BD_ERROR_INVALID_MBR;
    }

    // Check for valid partition status
    // Same reason as in partition_absolute regarding Windows-formatted SD card
    if (table->entries[part - 1].status != 0x00 &&
            table->entries[part - 1].status != 0x80) {
        return BD_ERROR_INVALID_PARTITION;
    }

    // Check for valid entry
    // 0x00 = no entry
    // 0x05, 0x0f = extended partitions, currently not supported
    if ((table->entries[part - 1].type == 0x00 ||
            table->entries[part - 1].type == 0x05 ||
            table->entries[part - 1].type == 0x0f)) {
        return BD_ERROR_INVALID_PARTITION;
    }

    // Get partition attributes
    if(attr)
    {
        sector = (uint32_t)max(mbr_get_erase_size(), 512);
        attr->type = table->entries[part - 1].type;
        attr->offset = fromle32(table->entries[part - 1].lba_offset) * sector;
        attr->size   = fromle32(table->entries[part - 1].lba_size)   * sector;
        
        printf("attr type %d, offset %d, size %d.\n", attr->type, attr->offset, attr->size);
    }    

    return BD_ERROR_OK;
}


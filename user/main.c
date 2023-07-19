
#include "lfs_port.h"
#include "lfs.h"
#include "mbrblock.h"

// variables used by the filesystem
lfs_t lfs;
lfs_file_t file;

#define PART_1_OFFSET SECTOR_SIZE

enum lfs_error user_provided_block_device_read(const struct lfs_config *c, lfs_block_t block,
            lfs_off_t off, void *buffer, lfs_size_t size)
{
	uint32_t offset = block * c->block_size + off + PART_1_OFFSET;

	user_flash_read(offset, buffer, size);

	return LFS_ERR_OK;
}

enum lfs_error user_provided_block_device_prog(const struct lfs_config *c, lfs_block_t block,
            lfs_off_t off, const void *buffer, lfs_size_t size)
{
	uint32_t offset = block * c->block_size + off + PART_1_OFFSET;

	user_flash_prog(offset, buffer, size);

	return LFS_ERR_OK;
}

enum lfs_error user_provided_block_device_erase(const struct lfs_config *c, lfs_block_t block)
{
	uint32_t offset = block * c->block_size + PART_1_OFFSET;

	user_flash_erase(offset, 1);
	
	return LFS_ERR_OK;
}

enum lfs_error user_provided_block_device_sync(const struct lfs_config *c)
{
	(void)c;
	user_flash_sync();
	return LFS_ERR_OK;
}

// configuration of the filesystem is provided by this struct
const struct lfs_config cfg = {
    // block device operations
    .read  = user_provided_block_device_read,
    .prog  = user_provided_block_device_prog,
    .erase = user_provided_block_device_erase,
    .sync  = user_provided_block_device_sync,

    // block device configuration
    .read_size = 16,
    .prog_size = 16,
    .block_size = SECTOR_SIZE,
    .block_count = 2047,
    .cache_size = 16,
    .lookahead_size = 16,
    .block_cycles = 500,
};




// entry point
int main(void) {

    partition_init();
	//user_flash_erase(0x1000, 6);
    //return 0;

    // mount the filesystem
    int err = lfs_mount(&lfs, &cfg);

    printf("lfs _mount err: %d\n", err);
    // reformat if we can't mount the filesystem
    // this should only happen on the first boot
    if (err) {
        lfs_format(&lfs, &cfg);
        lfs_mount(&lfs, &cfg);
    }

    // read current count
    uint32_t boot_count = 0;
    lfs_file_open(&lfs, &file, "boot_count", LFS_O_RDWR | LFS_O_CREAT);
    lfs_file_read(&lfs, &file, &boot_count, sizeof(boot_count));

    // update boot count
    boot_count += 1;
    lfs_file_rewind(&lfs, &file);
    lfs_file_write(&lfs, &file, &boot_count, sizeof(boot_count));

    // remember the storage is not updated until the file is closed successfully
    lfs_file_close(&lfs, &file);


    // print the boot count
    printf("boot_count: %d\n", boot_count);
    lfs_file_t file_a;
    lfs_file_open(&lfs, &file_a, "a", LFS_O_RDWR | LFS_O_CREAT);
    
    char buf[1024] = {0};
    lfs_file_read(&lfs, &file_a, buf, sizeof(buf));
    printf("a: %s\n", buf);
    lfs_file_close(&lfs, &file_a);
    
    
    // release any resources we were using
    lfs_unmount(&lfs);
    
}

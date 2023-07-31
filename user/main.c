#include "lfs_port.h"
#include "lfs.h"
#include "mbrblock.h"

#include <pthread.h>

// variables used by the filesystem
lfs_t lfs[4];
lfs_file_t file[4];


uint32_t get_offset_part(const struct lfs_config *c);


enum lfs_error user_provided_block_device_read(const struct lfs_config *c, lfs_block_t block,
            lfs_off_t off, void *buffer, lfs_size_t size)
{
	uint32_t offset_part = get_offset_part(c);
	if(offset_part == 0)
	{
		printf("error get_offset_part %p\n", c);
		return LFS_ERR_OK;
	}
	uint32_t offset = block * c->block_size + off + offset_part;

	user_flash_read(offset, buffer, size);

	return LFS_ERR_OK;
}

enum lfs_error user_provided_block_device_prog(const struct lfs_config *c, lfs_block_t block,
            lfs_off_t off, const void *buffer, lfs_size_t size)
{
	uint32_t offset_part = get_offset_part(c);
	if(offset_part == 0)
	{
		printf("error get_offset_part %p\n", c);
		return LFS_ERR_OK;
	}
	uint32_t offset = block * c->block_size + off + offset_part;

	user_flash_prog(offset, buffer, size);

	return LFS_ERR_OK;
}

enum lfs_error user_provided_block_device_erase(const struct lfs_config *c, lfs_block_t block)
{
	uint32_t offset_part = get_offset_part(c);
	if(offset_part == 0)
	{
		printf("error get_offset_part %p\n", c);
		return LFS_ERR_OK;
	}
	uint32_t offset = block * c->block_size + offset_part;

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
const struct lfs_config cfg[4] = {
	{
		// block device operations
		.read  = user_provided_block_device_read,
		.prog  = user_provided_block_device_prog,
		.erase = user_provided_block_device_erase,
		.sync  = user_provided_block_device_sync,

		// block device configuration
		.read_size = 16,
		.prog_size = 16,
		.block_size = SECTOR_SIZE,
		.block_count = 1024,
		.cache_size = 16,
		.lookahead_size = 16,
		.block_cycles = 500,
	},
	{
		// block device operations
		.read  = user_provided_block_device_read,
		.prog  = user_provided_block_device_prog,
		.erase = user_provided_block_device_erase,
		.sync  = user_provided_block_device_sync,

		// block device configuration
		.read_size = 16,
		.prog_size = 16,
		.block_size = SECTOR_SIZE,
		.block_count = 128,
		.cache_size = 16,
		.lookahead_size = 16,
		.block_cycles = 500,
	},
	{
		// block device operations
		.read  = user_provided_block_device_read,
		.prog  = user_provided_block_device_prog,
		.erase = user_provided_block_device_erase,
		.sync  = user_provided_block_device_sync,

		// block device configuration
		.read_size = 16,
		.prog_size = 16,
		.block_size = SECTOR_SIZE,
		.block_count = 895,
		.cache_size = 16,
		.lookahead_size = 16,
		.block_cycles = 500,
	},
	{},
};


extern const MBR_PART_ATTR mbr_attr[3];

uint32_t get_offset_part(const struct lfs_config *c)
{
	for(int i=0; i<4; i++)
	{
		if(c == &cfg[i])
		{
			//printf(" %p -- %p \n", c, &cfg[i]);
			return mbr_attr[i].offset;
		}
	}
	return 0;
}

void *pthread_mount(void *arg)
{
	int i = (int)arg;
	
	printf(" pthread_mount %d %d\n", sizeof(mbr_attr)/sizeof(mbr_attr[0]), i);
	if(sizeof(mbr_attr)/sizeof(mbr_attr[0]) <= i)
	{
		printf("exit\n");
		pthread_exit((void *)0);
		printf("exit\n");
	}

	// mount the filesystem
	int err = lfs_mount(&lfs[i], &cfg[i]);

	printf("[pthread %d]lfs _mount err: %d\n", i, err);
	// reformat if we can't mount the filesystem
	// this should only happen on the first boot
	if (err) {
		lfs_format(&lfs[i], &cfg[i]);
		lfs_mount(&lfs[i], &cfg[i]);
	}

	// read current count
	uint32_t boot_count = 0;
	lfs_file_open(&lfs[i], &file[i], "boot_count", LFS_O_RDWR | LFS_O_CREAT);
	lfs_file_read(&lfs[i], &file[i], &boot_count, sizeof(boot_count));

	// update boot count
	boot_count += i;
	lfs_file_rewind(&lfs[i], &file[i]);
	lfs_file_write(&lfs[i], &file[i], &boot_count, sizeof(boot_count));

	// remember the storage is not updated until the file is closed successfully
	lfs_file_close(&lfs[i], &file[i]);


	// print the boot count
	printf("[pthread %d]boot_count: %d\n", i, boot_count);
	lfs_file_t file_a;
	lfs_file_open(&lfs[i], &file_a, "a", LFS_O_RDWR | LFS_O_CREAT);

	char buf[1024] = {0};
	lfs_file_read(&lfs[i], &file_a, buf, sizeof(buf));
	printf("[pthread %d]a: %s\n", i, buf);
	lfs_file_close(&lfs[i], &file_a);


	printf("lfs_unmount\n");

	// release any resources we were using
	lfs_unmount(&lfs[i]);

	pthread_exit((void *)0);
}

// entry point
int main(void) {

	partition_init();
	//user_flash_erase(0x1000, 6);
	//return 0;
	pthread_t pid[4];
	void *res;
	
	for(int i=0; i<4; i++)
	{
		printf("check %p\n", &cfg[i]);
	}

	for(int i=0; i<4; i++)
	{
		pthread_create(&pid[i], NULL, pthread_mount, (void *)i);
	}
	
	for(int i=0; i<4; i++)
	{
		pthread_join(pid[i], &res);
	}
}

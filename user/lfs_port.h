#ifndef __LFS_PORT_H_
#define __LFS_PORT_H_

#include <stdint.h>


#define SECTOR_SIZE 512

int user_flash_read(uint32_t address, void *buffer, uint32_t size);
int user_flash_prog(uint32_t address, const void *buffer, uint32_t size);
int user_flash_erase(uint32_t address, uint32_t block_num);
int user_flash_sync(void);


#endif //__LFS_PORT_H_


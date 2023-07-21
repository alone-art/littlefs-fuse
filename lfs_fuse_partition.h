#ifndef LFS_FUSE_PARTITION_H
#define LFS_FUSE_PARTITION_H

#include "mbrblock.h"

typedef struct {
	//struct mbr_table table;
	MBR_PART_ATTR attr[16];
	int cnt;
} PART_t;

PART_t *lfs_fuse_partition_read(const char *path);




#endif //LFS_FUSE_PARTITION_H


#ifndef LFS_FUSE_PARTITION_H
#define LFS_FUSE_PARTITION_H

#include "mbrblock.h"

typedef struct {
	struct mbr_table table;
	int partition_cnt;
} PART_t;

struct mbr_table *lfs_fuse_partition_read(const char *path);




#endif //LFS_FUSE_PARTITION_H


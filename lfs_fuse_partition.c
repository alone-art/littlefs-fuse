#include "lfs_fuse_partition.h"
#include <string.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#if !defined(__FreeBSD__)
#include <sys/ioctl.h>
#include <linux/fs.h>
#elif defined(__FreeBSD__)
#define BLKSSZGET DIOCGSECTORSIZE
#define BLKGETSIZE DIOCGMEDIASIZE
#include <sys/disk.h>
#endif

#include <stdio.h>
#include "mbrblock.h"

PART_t *lfs_fuse_partition_read(const char *path)
{
    printf("check device %s partition\n", path);
    int fd = open(path, O_RDWR);
    if (fd < 0) {
        return NULL;
    }
    
    PART_t *part = NULL;
    
    // Allocate smallest buffer necessary to write MBR
    part = (PART_t *)malloc(sizeof(PART_t));
    memset(part, 0, sizeof(PART_t));
    
    struct mbr_table table = {0};

    lseek(fd,  512 - sizeof(struct mbr_table), SEEK_SET);
    int err = read(fd, &table, sizeof(struct mbr_table));
    if (err < 0) {
	printf("read faild: %s.\n", strerror(errno));
        return NULL;
    }
    close(fd);
    
    for(int i=0; i<4; i++)
    {
    	printf("check partition %d\n", i);
        MBR_ERR err = partition_check(&table, i+1, &part->attr[i]);
        
        if(err == BD_ERROR_INVALID_MBR)
        {
            free(part);
	    printf("INVALID_MBR %d\n", i);
            return NULL;
        }
        else if(err == BD_ERROR_OK)
        {
            part->cnt++;
            printf("vaild partition count %d\n", part->cnt);
        }
    }
    
    //free(buffer);
    
    return part;

}








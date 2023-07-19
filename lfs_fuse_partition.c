#include "lfs_fuse_partition.h"


PART_t *lfs_fuse_partition_read(const char *path)
{
    int fd = open(path, O_RDWR);
    if (fd < 0) {
        return NULL;
    }
    
    PART_t *part = NULL;
    
    // Allocate smallest buffer necessary to write MBR
    part = malloc(sizeof(PART_t));
    memset(part, 0, sizeof(PART_t));

    lseek(fd,  512 - sizeof(struct mbr_table), SEEK_SET);
    int err = read(fd, &part->table, sizeof(struct mbr_table));
    if (err) {
        return NULL;
    }
    close(fd);
    
    for(int i=0; i<4; i++)
    {
        MBR_ERR err = partition_check(&part->table, i, NULL);
        
        if(err == BD_ERROR_INVALID_MBR)
        {
            free(part);
            return NULL;
        }
        else if(err = BD_ERROR_OK)
        {
            part->partition_cnt++;
        }
    }
    
    //free(buffer);
    
    return part;

}








#ifndef __MBRBLOCK_H_
#define __MBRBLOCK_H_


#pragma pack(1)
// On disk structures, all entries are little endian
struct mbr_entry {
    uint8_t status;
    uint8_t chs_start[3];
    uint8_t type;
    uint8_t chs_stop[3];
    uint32_t lba_offset;
    uint32_t lba_size;
};

struct mbr_table {
    struct mbr_entry entries[4];
    uint8_t signature[2];
};
#pragma pack()


typedef enum {
    BD_ERROR_OK,
    BD_ERROR_INVALID_MBR,
    BD_ERROR_INVALID_PARTITION,
} MBR_ERR;

void partition_init(void);


#endif //__MBRBLOCK_H_


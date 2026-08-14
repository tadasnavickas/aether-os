#ifndef AETHERFS_H
#define AETHERFS_H

#include <stdint.h>
#include <stddef.h>

#define AETHERFS_MAGIC       0xAE78F501
#define AETHERFS_BLOCK_SIZE  512
#define AETHERFS_MAX_FILES   64
#define AETHERFS_DIRECT_BLKS 8

// Superblock (Sector 0)
struct aetherfs_superblock {
    uint32_t magic;           // 0xAE78F501
    uint32_t block_size;      // 512 byte
    uint32_t total_blocks;    // Total blocks on disk
    uint32_t free_blocks;     // Free blocks
    uint32_t inode_count;     // Total inode (64)
    uint32_t free_inodes;     // Free inode
    uint8_t  reserved[488];   // Padding to 512 bytes
} __attribute__((packed));

// Structure Inode (64 bytes: 8 units per sector)
struct aetherfs_inode {
    char     name[32];        // File name
    uint32_t size;            // Size in bytes
    uint32_t is_used;         // 1 = occupied, 0 = free
    uint32_t blocks[AETHERFS_DIRECT_BLKS]; // Data block numbers
    uint8_t  reserved[8];
} __attribute__((packed));

void aetherfs_init(void);
void aetherfs_format(void);
void aetherfs_list(void);
int  aetherfs_create_file(const char *name);
int  aetherfs_write_file(const char *name, const char *data, size_t len);
int  aetherfs_read_file(const char *name, char *buf, size_t max_len);
int  aetherfs_delete_file(const char *name);

#endif
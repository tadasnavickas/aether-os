#include "aetherfs.h"
#include <stdbool.h>

#define TOTAL_DISK_BLOCKS 1024

static uint8_t disk_storage[TOTAL_DISK_BLOCKS * AETHERFS_BLOCK_SIZE];
static uint8_t *disk = disk_storage;

void kprint(const char *str);
void kputchar(char c);
void kprint_dec(uint64_t val);
void kprint_hex(uint64_t val);

static int strcmp(const char *a, const char *b) {
    while (*a && (*a == *b)) {
        a++;
        b++;
    }
    return *(const unsigned char *)a - *(const unsigned char *)b;
}

static void strncpy(char *dest, const char *src, size_t n) {
    size_t i;
    for (i = 0; i < n && src[i] != '\0'; i++) {
        dest[i] = src[i];
    }
    for (; i < n; i++) {
        dest[i] = '\0';
    }
}

static void disk_read_block(uint32_t block, void *buf) {
    if (block >= TOTAL_DISK_BLOCKS) return;
    uint8_t *src = disk + (block * AETHERFS_BLOCK_SIZE);
    uint8_t *dst = (uint8_t *)buf;
    for (size_t i = 0; i < AETHERFS_BLOCK_SIZE; i++) dst[i] = src[i];
}

static void disk_write_block(uint32_t block, const void *buf) {
    if (block >= TOTAL_DISK_BLOCKS) return;
    uint8_t *dst = disk + (block * AETHERFS_BLOCK_SIZE);
    const uint8_t *src = (const uint8_t *)buf;
    for (size_t i = 0; i < AETHERFS_BLOCK_SIZE; i++) dst[i] = src[i];
}

void aetherfs_format(void) {
    for (size_t i = 0; i < TOTAL_DISK_BLOCKS * AETHERFS_BLOCK_SIZE; i++) {
        disk[i] = 0;
    }

    struct aetherfs_superblock sb;
    sb.magic = AETHERFS_MAGIC;
    sb.block_size = AETHERFS_BLOCK_SIZE;
    sb.total_blocks = TOTAL_DISK_BLOCKS;
    sb.free_blocks = TOTAL_DISK_BLOCKS - 10;
    sb.inode_count = AETHERFS_MAX_FILES;
    sb.free_inodes = AETHERFS_MAX_FILES;

    disk_write_block(0, &sb);

    uint8_t bitmap[AETHERFS_BLOCK_SIZE] = {0};
    bitmap[0] = 0xFF;
    bitmap[1] = 0x03;
    disk_write_block(1, bitmap);
}

void aetherfs_init(void) {
    aetherfs_format();

    aetherfs_create_file("readme.txt");
    const char *msg = "Welcome to native AetherFS inode-based filesystem!";
    size_t len = 0;
    while (msg[len]) len++;
    aetherfs_write_file("readme.txt", msg, len);
}

void aetherfs_list(void) {
    int count = 0;

    for (uint32_t i = 0; i < AETHERFS_MAX_FILES; i++) {
        uint32_t block = 2 + (i / 8);
        uint32_t offset = (i % 8) * sizeof(struct aetherfs_inode);
        uint8_t blk_buf[AETHERFS_BLOCK_SIZE];
        disk_read_block(block, blk_buf);

        struct aetherfs_inode *node_ptr = (struct aetherfs_inode *)(blk_buf + offset);
        if (node_ptr->is_used) {
            kprint("  ");
            kprint(node_ptr->name);
            kprint("  [");
            kprint_dec(node_ptr->size);
            kprint(" bytes]\n");
            count++;
        }
    }

    if (count == 0) {
        kprint("  (No files found)\n");
    }
}

int aetherfs_create_file(const char *name) {
    uint8_t blk_buf[AETHERFS_BLOCK_SIZE];

    for (uint32_t i = 0; i < AETHERFS_MAX_FILES; i++) {
        uint32_t block = 2 + (i / 8);
        uint32_t offset = (i % 8) * sizeof(struct aetherfs_inode);
        disk_read_block(block, blk_buf);

        struct aetherfs_inode *node = (struct aetherfs_inode *)(blk_buf + offset);
        if (!node->is_used) {
            node->is_used = 1;
            node->size = 0;
            strncpy(node->name, name, 31);
            for (int b = 0; b < AETHERFS_DIRECT_BLKS; b++) node->blocks[b] = 0;

            disk_write_block(block, blk_buf);
            return 0;
        }
    }
    return -1;
}

int aetherfs_write_file(const char *name, const char *data, size_t len) {
    uint8_t blk_buf[AETHERFS_BLOCK_SIZE];
    uint8_t bitmap[AETHERFS_BLOCK_SIZE];
    disk_read_block(1, bitmap);

    for (uint32_t i = 0; i < AETHERFS_MAX_FILES; i++) {
        uint32_t block = 2 + (i / 8);
        uint32_t offset = (i % 8) * sizeof(struct aetherfs_inode);
        disk_read_block(block, blk_buf);

        struct aetherfs_inode *node = (struct aetherfs_inode *)(blk_buf + offset);
        if (node->is_used && strcmp(node->name, name) == 0) {
            uint32_t target_block = node->blocks[0];

            if (target_block == 0) {
                for (uint32_t b = 10; b < TOTAL_DISK_BLOCKS; b++) {
                    if (!(bitmap[b / 8] & (1 << (b % 8)))) {
                        bitmap[b / 8] |= (1 << (b % 8));
                        target_block = b;
                        break;
                    }
                }
                if (target_block == 0) return -1;
                disk_write_block(1, bitmap);
            }

            node->blocks[0] = target_block;
            node->size = len;
            disk_write_block(block, blk_buf);

            uint8_t data_buf[AETHERFS_BLOCK_SIZE] = {0};
            size_t copy_len = (len < AETHERFS_BLOCK_SIZE) ? len : AETHERFS_BLOCK_SIZE;
            for (size_t d = 0; d < copy_len; d++) data_buf[d] = data[d];
            disk_write_block(target_block, data_buf);

            return 0;
        }
    }
    return -1;
}

int aetherfs_read_file(const char *name, char *buf, size_t max_len) {
    uint8_t blk_buf[AETHERFS_BLOCK_SIZE];

    for (uint32_t i = 0; i < AETHERFS_MAX_FILES; i++) {
        uint32_t block = 2 + (i / 8);
        uint32_t offset = (i % 8) * sizeof(struct aetherfs_inode);
        disk_read_block(block, blk_buf);

        struct aetherfs_inode *node = (struct aetherfs_inode *)(blk_buf + offset);
        if (node->is_used && strcmp(node->name, name) == 0) {
            if (node->size == 0) {
                buf[0] = '\0';
                return 0;
            }

            uint8_t data_buf[AETHERFS_BLOCK_SIZE];
            disk_read_block(node->blocks[0], data_buf);

            size_t copy_len = (node->size < max_len - 1) ? node->size : max_len - 1;
            for (size_t d = 0; d < copy_len; d++) buf[d] = data_buf[d];
            buf[copy_len] = '\0';
            return 0;
        }
    }
    return -1;
}

int aetherfs_delete_file(const char *name) {
    uint8_t blk_buf[AETHERFS_BLOCK_SIZE];
    uint8_t bitmap[AETHERFS_BLOCK_SIZE];
    disk_read_block(1, bitmap);

    for (uint32_t i = 0; i < AETHERFS_MAX_FILES; i++) {
        uint32_t block = 2 + (i / 8);
        uint32_t offset = (i % 8) * sizeof(struct aetherfs_inode);
        disk_read_block(block, blk_buf);

        struct aetherfs_inode *node = (struct aetherfs_inode *)(blk_buf + offset);
        if (node->is_used && strcmp(node->name, name) == 0) {
            node->is_used = 0;
            if (node->blocks[0] >= 10) {
                bitmap[node->blocks[0] / 8] &= ~(1 << (node->blocks[0] % 8));
                disk_write_block(1, bitmap);
            }
            node->blocks[0] = 0;
            node->size = 0;
            disk_write_block(block, blk_buf);
            return 0;
        }
    }
    return -1;
}
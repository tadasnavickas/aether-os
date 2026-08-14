#include "vfs.h"
#include "../limine.h"
#include "../mm/pmm.h"
#include <stdbool.h>

void kprint(const char *str);
void kprint_dec(uint64_t val);
void kprint_hex(uint64_t val);

__attribute__((used, section(".requests")))
static volatile struct limine_module_request module_request = {
    .id = LIMINE_MODULE_REQUEST,
    .revision = 0
};

struct tar_header {
    char name[100];
    char mode[8];
    char uid[8];
    char gid[8];
    char size[12];
    char mtime[12];
    char chksum[8];
    char typeflag;
    char linkname[100];
    char magic[6];
    char version[2];
    char uname[32];
    char gname[32];
    char devmajor[8];
    char devminor[8];
    char prefix[155];
};

static uint8_t *tar_archive = NULL;
static size_t tar_size = 0;

static size_t oct2bin(const char *str, int size) {
    size_t n = 0;
    for (int i = 0; i < size; i++) {
        if (str[i] >= '0' && str[i] <= '7') {
            n = (n << 3) + (str[i] - '0');
        } else if (str[i] == '\0' || str[i] == ' ') {
            break;
        }
    }
    return n;
}

static int strcmp(const char *a, const char *b) {
    while (*a && (*a == *b)) {
        a++;
        b++;
    }
    return *(const unsigned char *)a - *(const unsigned char *)b;
}

void vfs_init(void) {
    if (!module_request.response || module_request.response->module_count < 1) {
        kprint("[WARN] No initrd module loaded!\n");
        return;
    }

    struct limine_file *initrd = module_request.response->modules[0];
    tar_archive = (uint8_t *)initrd->address;
    tar_size = initrd->size;
}

void vfs_list_files(void) {
    if (!tar_archive) {
        kprint("No filesystem mounted.\n");
        return;
    }

    uint8_t *ptr = tar_archive;
    while (ptr < tar_archive + tar_size) {
        struct tar_header *header = (struct tar_header *)ptr;
        if (header->name[0] == '\0') break;

        size_t size = oct2bin(header->size, 11);
        kprint("  ");
        kprint(header->name);
        kprint("  [");
        kprint_dec(size);
        kprint(" bytes]\n");

        ptr += 512 + ((size + 511) / 512) * 512;
    }
}

int vfs_read_file(const char *name, char *buffer, size_t max_size) {
    if (!tar_archive) return -1;

    uint8_t *ptr = tar_archive;
    while (ptr < tar_archive + tar_size) {
        struct tar_header *header = (struct tar_header *)ptr;
        if (header->name[0] == '\0') break;

        size_t size = oct2bin(header->size, 11);
        if (strcmp(header->name, name) == 0) {
            char *file_data = (char *)(ptr + 512);
            size_t copy_size = (size < max_size - 1) ? size : max_size - 1;
            for (size_t i = 0; i < copy_size; i++) {
                buffer[i] = file_data[i];
            }
            buffer[copy_size] = '\0';
            return 0;
        }

        ptr += 512 + ((size + 511) / 512) * 512;
    }
    return -1;
}
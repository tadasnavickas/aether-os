#ifndef VFS_H
#define VFS_H

#include <stdint.h>
#include <stddef.h>

void vfs_init(void);
void vfs_list_files(void);
int vfs_read_file(const char *name, char *buffer, size_t max_size);

#endif
#ifndef COMMON_MMAP_H
#define COMMON_MMAP_H

#include <common/types.h>

/**
 * Opens and memory-maps a file for reading only.
 * @param path      Path to the file to open
 * @param out_data  Output parameter for file data
 * @return 0 on success, 1 on empty file, -1 if cannot open, -2 if cannot stat, -3 if mmap fails
 */
int mmap_open(const char* path, data_buf* out_data);

/**
 * Unmaps and closes the file.
 * @param data  Input parameter for file data
 * @return 0 on success, -1 on error.
 */
int mmap_close(data_buf* data);

#endif // COMMON_MMAP_H
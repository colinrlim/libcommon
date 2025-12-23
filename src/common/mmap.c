#include <common/mmap.h>

#ifdef _WIN32 // WINDOWS
#include <windows.h>

int mmap_open(const char* path, data_buf* out_data) {
    HANDLE hFile, hMapping;
    LARGE_INTEGER fileSize;
    
    hFile = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
        return -1;
    
    if (!GetFileSizeEx(hFile, &fileSize)) {
        CloseHandle(hFile);
        return -2;
    }
    
    if (fileSize.QuadPart == 0) {
        out_data->size = 0;
        out_data->data = NULL;
        CloseHandle(hFile);
        return 1;
    }
    
    hMapping = CreateFileMappingA(hFile, NULL, PAGE_READONLY, 0, 0, NULL);
    if (hMapping == NULL) {
        CloseHandle(hFile);
        return -3;
    }
    
    out_data->data = MapViewOfFile(hMapping, FILE_MAP_READ, 0, 0, 0);
    if (out_data->data == NULL) {
        CloseHandle(hMapping);
        CloseHandle(hFile);
        return -3;
    }
    
    out_data->size = (size_t)fileSize.QuadPart;
    
    CloseHandle(hMapping);
    CloseHandle(hFile);
    return 0;
}

int mmap_close(data_buf* data) {
    if (data->data == NULL)
        return 0;
    return UnmapViewOfFile(data->data) ? 0 : -1;
}

#else // POSIX

#include <sys/mman.h>
#include <fcntl.h>    // for open, O_RDONLY
#include <unistd.h>   // for close
#include <sys/stat.h> // for fstat, struct stat

int mmap_open(const char* path, data_buf* out_data) {
    int         fd;
    struct stat sb;

    fd = open(path, O_RDONLY);
    if (fd == -1)
        return -1; // Could not open file

    if (fstat(fd, &sb) == -1) {
        close(fd);
        return -2; // Could not get file size
    }

    if (sb.st_size == 0) { // Empty file
        out_data->size = 0;
        out_data->data = NULL;
        close(fd);
        return 1;
    }

    out_data->size = sb.st_size;
    out_data->data = mmap(NULL, sb.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (out_data->data == MAP_FAILED) {
        close(fd);
        return -3; // Could not map file
    }
    close(fd);

    return 0;
}

int mmap_close(data_buf* data) {
    if (data->data == NULL)
        return 0;
    return munmap(data->data, data->size);
}

#endif
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>

#define MAX_FILE_SIZE 1024 * 1024 // 1MB

typedef struct {
    char *init; // keep original ptr if need to go back to beginning
    char *cur; // current position in file
} Proxy_File;

Proxy_File proxy_file = {0};

#ifndef _WIN32
#include <sys/mman.h>
bool import_proxy_file(char *filepath) {

    int fd = open(filepath,O_RDONLY);
    
    proxy_file.init = (char*)mmap(NULL, MAX_FILE_SIZE, PROT_READ, MAP_PRIVATE , fd, 0);
    
    if (fd < 0) {
        fprintf(stderr,"[!] could not open proxy file \"%s\"\n",filepath);
        return false;
    }

    if (strlen(proxy_file.init) == 0) {
        fprintf(stderr,"[!] proxy file \"%s\" is empty\n",filepath);
        return false;
    }
    
    proxy_file.cur = proxy_file.init;
    close(fd);
    return true;
}

#else

bool import_proxy_file(char *filepath) {

    path_to_windows_path(filepath);

    HANDLE f = CreateFileA(filepath, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    
    if (f == INVALID_HANDLE_VALUE) {
        fprintf(stderr,"[!] could not open proxy file \"%s\"\n",filepath);
        return false;
    }

    HANDLE fmap = CreateFileMappingA(f, NULL, PAGE_READONLY, 0, 0, NULL);
    if (!fmap) {
        fprintf(stderr,"[!] could not map proxy file to memory \"%s\"\n",filepath);
        CloseHandle(f);
        return false;
    }

    // if a file was already opened, close it
    if (proxy_file.init != 0) UnmapViewOfFile(proxy_file.init);

    proxy_file.init = (char*)MapViewOfFile(fmap, FILE_MAP_READ, 0, 0, 0);

    if (strlen(proxy_file.init) == 0) {
        fprintf(stderr,"[!] proxy file \"%s\" is empty\n",filepath);
        return false;
    }
    
    proxy_file.cur = proxy_file.init;
    
    CloseHandle(fmap);
    CloseHandle(f); 
    return true;
}
#endif

void get_next_proxy(char *dst) {
    // populate the provided char* with next proxy in file

    if (proxy_file.init == 0) {
        fprintf(stderr,"[!] no proxy file is opened");
        return;
    }

    while (is_whitespace(*proxy_file.cur)) *proxy_file.cur++;

    if (*proxy_file.cur == '\0') {
        proxy_file.cur = proxy_file.init;
    }

    int length = 0;

    while (*proxy_file.cur != '\0') {
        while (is_whitespace(*proxy_file.cur)) *proxy_file.cur++;
        while (*proxy_file.cur != '\0' && !is_whitespace(*proxy_file.cur)) {
            *proxy_file.cur++;
            length++;
        }
        break;
    }

    if (length == 0) {
        fprintf(stderr,"[!] no proxy could be parsed\n");
    }
    
    strncpy(dst,proxy_file.cur - length,length);
    
    dst[length] = '\0';
    
}

// int main() {

//     char filename[128] = "http.txt";

//     if (!read_proxy_file(filename)) return 1;

//     char p[128];

//     for (int i=0;i<=2807;i++) {
//         get_next_proxy(p);
//         printf("%s ", p);
//     }
    
//     return 0;
// }

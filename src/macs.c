#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>

char *MAC_FILE = {0};

#ifndef _WIN32
#include <sys/mman.h>
bool import_mac_file(char *filepath) {

    int fd = open(filepath,O_RDONLY);
    
    if (fd < 0) {
        fprintf(stderr,"[!] could not open mac file \"%s\"\n",filepath);
        return false;
    }


    // Assert file size > 0 else mmap will segfault
    FILE *stream = fdopen(fd,"r");
    fseek(stream, 0L, SEEK_END);
    long size = ftell(stream);
    if (size <= 0) {
        fprintf(stderr,"[!] mac file \"%s\" has size 0\n",filepath);
        fclose(stream);
        return false;
    }
    rewind(stream);

    MAC_FILE = (char*)mmap(NULL, MAX_FILE_SIZE, PROT_READ, MAP_PRIVATE , fd, 0);

    if (MAC_FILE == NULL) {
        fprintf(stderr,"[!] mac file \"%s\" is empty\n",filepath);
        return false;
    }

    close(fd);
    return true;
}

#else

bool import_mac_file(char *filepath) {

    path_to_windows_path(filepath);

    HANDLE f = CreateFileA(filepath, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    
    if (f == INVALID_HANDLE_VALUE) {
        fprintf(stderr,"[!] could not open mac file \"%s\"\n",filepath);
        return false;
    }

    HANDLE fmap = CreateFileMappingA(f, NULL, PAGE_READONLY, 0, 0, NULL);
    if (!fmap) {
        fprintf(stderr,"[!] could not map mac file to memory \"%s\"\n",filepath);
        CloseHandle(f);
        return false;
    }

    // if a file was already opened, close it
    if (MAC_FILE != 0) UnmapViewOfFile(MAC_FILE);

    MAC_FILE = (char*)MapViewOfFile(fmap, FILE_MAP_READ, 0, 0, 0);

    if (strlen(MAC_FILE) == 0) {
        fprintf(stderr,"[!] mac file \"%s\" is empty\n",filepath);
        return false;
    }
    
    CloseHandle(fmap);
    CloseHandle(f); 
    return true;
}
#endif


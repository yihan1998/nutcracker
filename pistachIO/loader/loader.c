
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <dirent.h>
#include <fcntl.h>
#include <libtcc.h>

#include "opt.h"
#include "printk.h"
#include "syscall.h"
#include "loader/loader.h"

#define LOADER_PATH "/tmp/loader.socket"

int loader_fd;

void search_files(const char * dirpath, char ** files, int * nr_file) {
    char path[512];
    char * file_path;
    struct dirent * dp;
    DIR * dir;

    dir = opendir(dirpath);
    if (!dir) {
        return;
    }

    while ((dp = readdir(dir)) != NULL) {
        if (strcmp(dp->d_name, ".") != 0 && strcmp(dp->d_name, "..") != 0) {
            // Construct new path from our base path
            snprintf(path, sizeof(path), "%s/%s", dirpath, dp->d_name);

            struct stat path_stat;
            stat(path, &path_stat);

            // Check if it's a directory
            if (S_ISDIR(path_stat.st_mode)) {
                // Recursively search this directory
                search_files(path, files, nr_file);
            } else {
                // Check if the file is a C source file
                if (strstr(dp->d_name, ".c") != NULL) {
                    file_path = (char *)calloc(strlen(path), sizeof(char));
                    strcpy(file_path, path);
                    files[(*nr_file)++] = file_path;
                }
            }
        }
    }

    closedir(dir);
}

int compile_and_run(const char * filepath) {
    FILE * file;
    char * buffer;
    long len;
    size_t bytes_read;
    int nr_file = 0;
    char * input_file[16] = {0};

    TCCState * s = tcc_new();
    if (!s) {
        pr_err("Could not create tcc state\n");
        return -1;
    }

#if defined(__x86_64__) || defined(__i386__)
    tcc_set_lib_path(s, "/usr/lib/x86_64-linux-gnu/tcc");
#elif defined(__arm__) || defined(__aarch64__)
    tcc_set_lib_path(s, "/usr/local/lib");
#else
    pr_err("Unknown architecture!\n");
#endif
    tcc_add_include_path(s, "/usr/include");

    search_files(filepath, input_file, &nr_file);

    tcc_set_output_type(s, TCC_OUTPUT_MEMORY);
    for (int i = 0; i < nr_file; i++) {
        // file = fopen("/local/yihan/Nutcracker-dev/tests/socket/socket.c", "rb");
        file = fopen(input_file[i], "rb");
        if (file == NULL) {
            pr_err("Error opening input file\n");
            return -1;
        }

        // Seek to the end of the file to determine its size
        fseek(file, 0, SEEK_END);
        len = ftell(file);
        rewind(file); // Go back to the beginning of the file

        // Allocate memory to store the entire file
        buffer = (char *)malloc(len + 1);
        if (buffer == NULL) {
            pr_err("Memory allocation failed\n");
            fclose(file);
            return -1;
        }

        // Step 2: Read the file into the buffer
        bytes_read = fread(buffer, 1, len, file);
        if (bytes_read < len) {
            if (feof(file)) {
                pr_err("Premature end of file\n");
            } else if (ferror(file)) {
                pr_err("Error reading file\n");
            }
            free(buffer);
            fclose(file);
            return -1;
        }
        buffer[bytes_read] = '\0'; // For text files, null-terminate the string

        tcc_compile_string(s, buffer);

        free(buffer);
    }

    // Relocate the compiled program into memory
    if (tcc_relocate(s, TCC_RELOCATE_AUTO) < 0) {
        pr_err("Could not relocate tcc code\n");
        return 1;
    }

    // Retrieve a pointer to the compiled function
    int (*func)(void) = tcc_get_symbol(s, "main");
    if (!func) {
        pr_err("Could not find main function\n");
        return 1;
    }

    tcc_delete(s);

    // Call the function
    func();

    return 0;
}

int __init loader_init(void) {
    struct sockaddr_un address;
	address.sun_family = AF_UNIX;
	strcpy(address.sun_path, LOADER_PATH);

    if (remove(address.sun_path) == -1 && errno != ENOENT) {
        pr_crit("Failed remove existing socket file!\n");
        return -1;
    }

    if ((loader_fd = libc_socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
        pr_crit("Failed to create socket for IPC!\n");
        return -1;
    }

    int flags = fcntl(loader_fd, F_GETFL);
    if (fcntl(loader_fd, F_SETFL, flags | O_NONBLOCK) < 0) {
        pr_crit("Failed to set IPC socket to non-blocking mode!\n");
        return -1;
    }

    if (libc_bind(loader_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        pr_crit("Failed to bind socket!\n");
        return -1;
    }

    if (chmod(LOADER_PATH, 0666) == -1) {
        pr_crit("Failed to change socket privilege!");
        return -1;
    }

    if (libc_listen(loader_fd, 20) < 0) {
        pr_crit("Failed to listen socket!\n");
        return -1;
    }

    return 0;
}
#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <dlfcn.h>
#include <dirent.h>
#include <fcntl.h>
#include <pcap.h>
#include <libgen.h>

#include "init.h"
#include "utils/printk.h"
#include "ipc/ipc.h"

static const char * get_file_extension(const char * filename) {
    const char *dot = strrchr(filename, '.');
    if (!dot || dot == filename) return NULL;
    return dot + 1;
}

static int attach_c_function(const char * filepath) {
    void (*entrypoint)(void);
    char *error;

    void* preload = dlopen(filepath, RTLD_NOW | RTLD_GLOBAL);
    if (!preload) {
        fprintf(stderr, "Failed to load preload library: %s\n", dlerror());
        exit(EXIT_FAILURE);
    }

    if ((error = dlerror()) != NULL)  {
        fprintf(stderr, "%s\n", error);
        exit(EXIT_FAILURE);
    }

    *(void **) (&entrypoint) = dlsym(preload, "entrypoint");
    if(entrypoint) {
        entrypoint();
        return 0;
    }
    return 0;
}

int cacao_attach_and_run(const char * filepath) {
    char path[strlen(filepath) + 1];  // +1 for the null terminator
    strcpy(path, filepath);

    char * name = basename(path);
    const char *extension = get_file_extension(name);

    if (extension) {
        if (strcmp(extension, "so") == 0) {
            attach_c_function(filepath);
        } else {
            printf("Unsupported file type: %s\n", extension);
        }
    } else {
        printf("No file extension found.\n");
    }

    return 0;
}

int __init cacao_init(void) {
    // char *error;
    // pr_info("Preloading netfilter...\n");

    // /* Preload libnftnl */
    // void* libnftnl_preload = dlopen("./build/libnftnl.so", RTLD_NOW | RTLD_GLOBAL);
    // if (!libnftnl_preload) {
    //     fprintf(stderr, "Failed to load preload library: %s\n", dlerror());
    //     exit(EXIT_FAILURE);
    // }

    // if ((error = dlerror()) != NULL)  {
    //     fprintf(stderr, "%s\n", error);
    //     exit(EXIT_FAILURE);
    // }

    // void (*preload_lnftnl_init)(void);
    // *(void **) (&preload_lnftnl_init) = dlsym(libnftnl_preload, "nftnl_init");
    // assert(preload_lnftnl_init != NULL);
    // preload_lnftnl_init();

    return 0;
}
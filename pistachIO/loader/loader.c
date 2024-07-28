#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <dirent.h>
#include <fcntl.h>
#include <libtcc.h>
#include <libgen.h>
#include <dlfcn.h>
#include <Python.h>

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

const char *get_file_extension(const char *filename) {
    const char *dot = strrchr(filename, '.');
    if (!dot || dot == filename) return NULL;
    return dot + 1;
}

const void strip_py_extension(char *filename) {
    char *dot = strrchr(filename, '.');
    if (dot != NULL && strcmp(dot, ".py") == 0) {
        *dot = '\0';
    }
}

// const char * extract_directory(const char *filepath) {
//     // Copy the filepath because dirname modifies the input string
//     char filepath_copy[1024];
//     strncpy(filepath_copy, filepath, sizeof(filepath_copy));

//     // Ensure null termination
//     filepath_copy[sizeof(filepath_copy) - 1] = '\0';

//     // Extract the directory path
//     return dirname(filepath_copy);
// }

void call_python_function(const char *script_path, const char *module_name, const char *function_name) {
    printf("Initializing Python interpreter...\n");
    
    // Initialize the Python interpreter
    Py_Initialize();
    if (!Py_IsInitialized()) {
        fprintf(stderr, "Failed to initialize Python interpreter\n");
        return;
    }

    printf("Setting Python path...\n");
    // Set the Python path to include the directory of the script
    PyObject *sys_path = PySys_GetObject("path");
    PyObject *path = PyUnicode_DecodeFSDefault(script_path);
    if (sys_path && path) {
        PyList_Append(sys_path, path);
        Py_DECREF(path);
    } else {
        PyErr_Print();
        fprintf(stderr, "Failed to set Python path\n");
        Py_Finalize();
        return;
    }

    printf("-> Script path: %s\n", script_path);

    printf("Importing module...\n");
    // Import the module
    PyObject *pName = PyUnicode_DecodeFSDefault(module_name);
    PyObject *pModule = PyImport_Import(pName);
    Py_DECREF(pName);

    if (pModule != NULL) {
        printf("Getting function from module...\n");
        // Get the function from the module
        PyObject *pFunc = PyObject_GetAttrString(pModule, function_name);
        if (pFunc && PyCallable_Check(pFunc)) {
            // Call the function
            printf("Calling function...\n");
            PyObject *pValue = PyObject_CallObject(pFunc, NULL);
            if (pValue != NULL) {
                printf("Result of call: %ld\n", PyLong_AsLong(pValue));
                Py_DECREF(pValue);
            } else {
                PyErr_Print();
                fprintf(stderr, "Call failed\n");
            }
            Py_XDECREF(pFunc);
        } else {
            if (PyErr_Occurred())
                PyErr_Print();
            fprintf(stderr, "Cannot find function \"%s\"\n", function_name);
        }
        Py_DECREF(pModule);
    } else {
        PyErr_Print();
        fprintf(stderr, "Failed to load \"%s\"\n", module_name);
    }

    // Finalize the Python interpreter
    Py_Finalize();
}

int attach_python_function(const char * filepath) {
    char path[strlen(filepath) + 1];  // +1 for the null terminator
    strcpy(path, filepath);

    char * name = basename(path);
    const char * dir = dirname(path);
    strip_py_extension(name);

    printf("Path: %s, name: %s\n", dir, name);

    // call_python_function(dir, name, "entrypoint");

    const char *script_path = "/home/ubuntu/Nutcracker/apps/recommendation";

    printf("Dir: %s, script path: %s(compare: %s)\n", dir, script_path, (strcmp(dir, script_path) == 0)? "SAME" : "NOT SAME");
    // const char *module_name = "server"; // Module name without the .py extension

    // call_python_function(script_path, name, "entrypoint");
    call_python_function(dir, name, "entrypoint");

    return 0;
}

int attach_c_function(const char * filepath) {
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
    assert(entrypoint != NULL);
    entrypoint();

    return 0;
}

int attach_and_run(const char * filepath) {
    char path[strlen(filepath) + 1];  // +1 for the null terminator
    strcpy(path, filepath);

    char * name = basename(path);
    const char *extension = get_file_extension(name);

    if (extension) {
        if (strcmp(extension, "so") == 0) {
            attach_c_function(filepath);
        } else if (strcmp(extension, "py") == 0) {
            attach_python_function(filepath);
        } else {
            printf("Unsupported file type: %s\n", extension);
        }
    } else {
        printf("No file extension found.\n");
    }

    return 0;
}

#if 0
int compile_and_run(const char * filepath) {
    FILE * file;
    char * buffer;
    long len;
    size_t bytes_read;
    int nr_file = 0;
    char * input_file[16] = {0};

    // Make a copy of the original path since basename might modify it
    char path[strlen(filepath) + 1];  // +1 for the null terminator
    strcpy(path, filepath);

    char * name = basename(path);

    TCCState * s = tcc_new();
    if (!s) {
        pr_err("Could not create tcc state\n");
        return -1;
    }

#if defined(__x86_64__) || defined(__i386__)
    tcc_set_lib_path(s, "/usr/lib/x86_64-linux-gnu/tcc");
#elif defined(__arm__) || defined(__aarch64__)
    // tcc_set_lib_path(s, "/usr/local/lib");
#ifdef CONFIG_BLUEFIELD2
    tcc_set_lib_path(s, "/usr/lib/aarch64-linux-gnu/tcc");
#endif   /* CONFIG_BLUEFIELD2 */
#ifdef CONFIG_OCTEON
    tcc_set_lib_path(s, "/usr/local/lib/tcc");
#endif  /* CONFIG_OCTEON */
#else
    pr_err("Unknown architecture!\n");
#endif
    // tcc_add_library_path(s, "/usr/lib/aarch64-linux-gnu/");
#ifdef CONFIG_BLUEFIELD2
    tcc_add_include_path(s, "/home/ubuntu/Nutcracker/pistachIO/include/");
#endif   /* CONFIG_BLUEFIELD2 */
#ifdef CONFIG_OCTEON
    tcc_add_include_path(s, "/root/Nutcracker/pistachIO/include/");
#endif  /* CONFIG_OCTEON */
    tcc_add_include_path(s, "/usr/include");

    search_files(filepath, input_file, &nr_file);

    tcc_set_output_type(s, TCC_OUTPUT_MEMORY);
    for (int i = 0; i < nr_file; i++) {
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
        buffer = (char *)calloc(len + 1, sizeof(char));
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

        if (tcc_compile_string(s, buffer) < 0) {
            pr_err("Could not compile tcc code\n");
        }

        free(buffer);
    }

    // Relocate the compiled program into memory
#if defined(CONFIG_BLUEFIELD2)
    if (tcc_relocate(s) < 0) {
#elif defined(CONFIG_OCTEON)
    if (tcc_relocate(s, TCC_RELOCATE_AUTO) < 0) {
#endif
        pr_err("Could not relocate tcc code\n");
        return 1;
    }

    // Retrieve a pointer to the compiled function
    int (*func)(void) = tcc_get_symbol(s, strcat(name, "_init"));
    if (!func) {
        pr_err("Could not find main function\n");
        return 1;
    }

    // tcc_delete(s);

    // Call the function
    func();

    return 0;
}
#endif
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
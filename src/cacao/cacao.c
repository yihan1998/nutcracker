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
#if 0
#include <Python.h>
#endif
#include <libgen.h>
#include <linux/netfilter.h>
#include <zlib.h>

#include "init.h"
#include "printk.h"
#include "ipc/ipc.h"

static const char * get_file_extension(const char * filename) {
    const char *dot = strrchr(filename, '.');
    if (!dot || dot == filename) return NULL;
    return dot + 1;
}
#if 0
static int include_python_function(const char * dirpath, const char * module, struct nf_hook_python_ops * ops) {
    printf("Dir: %s, module: %s\n", dirpath, module);
    // Set the Python path to include the directory of the script
    PyObject *sys_path = PySys_GetObject("path");
    PyObject *path = PyUnicode_DecodeFSDefault(dirpath);
    if (sys_path && path) {
        PyList_Append(sys_path, path);
        Py_DECREF(path);
    } else {
        PyErr_Print();
        fprintf(stderr, "Failed to set Python path\n");
        Py_Finalize();
        return -1;
    }

    // Import the module
    PyObject *pName = PyUnicode_DecodeFSDefault(module);
    PyObject *pModule = PyImport_Import(pName);
    Py_DECREF(pName);

    if (pModule != NULL) {
        // Get the function from the module
        PyObject *pHook = PyObject_GetAttrString(pModule, ops->hook);
        PyObject *pCond = PyObject_GetAttrString(pModule, ops->cond);
        nf_register_net_python_hook(NULL, ops, pHook, pCond);
    } else {
        PyErr_Print();
        fprintf(stderr, "Failed to load \"%s\"\n", module);
    }
    return 0;
}
#endif
static int attach_c_function(const char * filepath) {
    void (*entrypoint)(void);
    // void (*new_thread_entry)(void);
    // struct nf_hook_python_ops * (*nf_register)(void);
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
#if 0
    *(void **) (&nf_register) = dlsym(preload, "nf_register");
    if (nf_register) {
        struct nf_hook_python_ops * ops = nf_register();
        printf("hook: %s, cond: %s\n", ops->hook, ops->cond);
        char directory[strlen(filepath) + 1];  // +1 for the null terminator
        char module[strlen(filepath) + 1];

        // Find the last occurrence of '/'
        const char * last_slash = strrchr(filepath, '/');
        if (last_slash == NULL) {
            pr_err("Invalid project path!\n");
            return -1;
        }

        // Copy the directory part
        size_t dir_len = last_slash - filepath;
        strncpy(directory, filepath, dir_len);
        directory[dir_len] = '\0';  // Null-terminate the string

        // Find the last occurrence of '.' after the last '/'
        const char *last_dot = strrchr(last_slash, '.');
        if (last_dot == NULL) {
            // If no '.' found, the module name is everything after the last '/'
            strcpy(module, last_slash + 1);
        } else {
            // Copy the module name part
            size_t mod_len = last_dot - last_slash - 1;
            strncpy(module, last_slash + 1, mod_len);
            strncpy(module + mod_len, "_py", 4);
            mod_len += 4;
            module[mod_len] = '\0';  // Null-terminate the string
        }
        include_python_function(directory, module, ops);
        return 0;
    }
#endif
    return 0;
}

#if 0
static int attach_python_function(const char * filepath) {
    char directory[strlen(filepath) + 1];  // +1 for the null terminator
    char module[strlen(filepath) + 1];
    const char *function_name = "entrypoint";

    // Find the last occurrence of '/'
    const char * last_slash = strrchr(filepath, '/');
    if (last_slash == NULL) {
        pr_err("Invalid project path!\n");
        return -1;
    }

    // Copy the directory part
    size_t dir_len = last_slash - filepath;
    strncpy(directory, filepath, dir_len);
    directory[dir_len] = '\0';  // Null-terminate the string

    // Find the last occurrence of '.' after the last '/'
    const char *last_dot = strrchr(last_slash, '.');
    if (last_dot == NULL) {
        // If no '.' found, the module name is everything after the last '/'
        strcpy(module, last_slash + 1);
    } else {
        // Copy the module name part
        size_t mod_len = last_dot - last_slash - 1;
        strncpy(module, last_slash + 1, mod_len);
        module[mod_len] = '\0';  // Null-terminate the string
    }

    // Set the Python path to include the directory of the script
    PyObject *sys_path = PySys_GetObject("path");
    PyObject *path = PyUnicode_DecodeFSDefault(directory);
    if (sys_path && path) {
        PyList_Append(sys_path, path);
        Py_DECREF(path);
    } else {
        PyErr_Print();
        fprintf(stderr, "Failed to set Python path\n");
        Py_Finalize();
        return -1;
    }

    // Import the module
    PyObject *pName = PyUnicode_DecodeFSDefault(module);
    PyObject *pModule = PyImport_Import(pName);
    Py_DECREF(pName);

    if (pModule != NULL) {
        // Get the function from the module
        PyObject *pFunc = PyObject_GetAttrString(pModule, function_name);
        if (pFunc && PyCallable_Check(pFunc)) {
            // Call the function
            PyObject *pValue = PyObject_CallObject(pFunc, NULL);
            if (pValue != NULL) {
                // printf("Result of call: %ld\n", PyLong_AsLong(pValue));
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
        fprintf(stderr, "Failed to load \"%s\"\n", module);
    }

    return 0;
}
#endif

int cacao_attach_and_run(const char * filepath) {
    char path[strlen(filepath) + 1];  // +1 for the null terminator
    strcpy(path, filepath);

    char * name = basename(path);
    const char *extension = get_file_extension(name);

    if (extension) {
        if (strcmp(extension, "so") == 0) {
            attach_c_function(filepath);
#if 0
        } else if (strcmp(extension, "py") == 0) {
            attach_python_function(filepath);
#endif
        } else {
            printf("Unsupported file type: %s\n", extension);
        }
    } else {
        printf("No file extension found.\n");
    }

    return 0;
}

#if 0
static int __init python_env_init(void) {
    Py_Initialize();
    if (!Py_IsInitialized()) {
        pr_err("Failed to initialized python interpreter\n");
        return -1;
    }
    return 0;
}
#endif

int __init cacao_init(void) {
    char *error;

#if 0
    /* Init python env */
    python_env_init();
#endif

    pr_info("Preloading netfilter...\n");
    /* Preload libnftnl */
    void* libnftnl_preload = dlopen("./build/lib/libnftnl.so", RTLD_NOW | RTLD_GLOBAL);
    if (!libnftnl_preload) {
        fprintf(stderr, "Failed to load preload library: %s\n", dlerror());
        exit(EXIT_FAILURE);
    }

    if ((error = dlerror()) != NULL)  {
        fprintf(stderr, "%s\n", error);
        exit(EXIT_FAILURE);
    }

    void (*preload_lnftnl_init)(void);
    *(void **) (&preload_lnftnl_init) = dlsym(libnftnl_preload, "nftnl_init");
    assert(preload_lnftnl_init != NULL);
    preload_lnftnl_init();

    return 0;
}
#include <stddef.h>

#include "libbpf.h"

struct bpf_object * bpf_object__open_file(const char *path, const struct bpf_object_open_opts *opts) {
    printf("Hello from %s\n", __func__);
    return NULL;
}

int bpf_object__load(struct bpf_object *obj) {
    printf("Hello from %s\n", __func__);
    return 0;
}

struct bpf_program * bpf_object__find_program_by_name(const struct bpf_object *obj, const char *name) {
    printf("Hello from %s\n", __func__);
    return NULL;
}

struct bpf_program *bpf_program__next(struct bpf_program *prog, const struct bpf_object *obj) {
    printf("Hello from %s\n", __func__);
    return NULL;
}

long libbpf_get_error(const void *ptr) {
    printf("Hello from %s\n", __func__);
    return 0;
}

struct bpf_link * bpf_program__attach(struct bpf_program *prog) {
    printf("Hello from %s\n", __func__);
    return NULL;
}
#include <bpf/libbpf.h>
#include <stdio.h>

int main() {
    struct bpf_object *obj;
    char filename[] = "libdns_filter.bpf.so";

    // Load the BPF program
    obj = bpf_object__open_file(filename, NULL);
    if (libbpf_get_error(obj)) {
        fprintf(stderr, "Error opening BPF object file.\n");
        return 1;
    }

    // Load and verify the BPF program
    if (bpf_object__load(obj)) {
        fprintf(stderr, "Error loading BPF object.\n");
        return 1;
    }

    // Automatically pin and attach all programs in the object
    struct bpf_link *link = NULL;
    struct bpf_program *prog = bpf_program__next(NULL, obj);
    while (prog) {
        link = bpf_program__attach(prog);
        if (libbpf_get_error(link)) {
            fprintf(stderr, "Error attaching BPF program.\n");
            return 1;
        }
        prog = bpf_program__next(prog, obj);
    }

    // Cleanup is handled automatically by libbpf on exit
    return 0;
}
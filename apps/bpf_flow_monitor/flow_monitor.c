#include <stdio.h>
#include <libbpf.h>

int main(void) {
    struct bpf_object * obj;
    char filename[] = "dns_filter.bpf.so";

    // Load the BPF program
    obj = bpf_object__open_file(filename, NULL);

    return 0;
}
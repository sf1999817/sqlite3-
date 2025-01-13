#include "vmlinux.h"
#include <bpf/bpf_helpers.h>
#include <bpf/bpf_tracing.h>

char LICENSE[] SEC("license") = "Dual BSD/GPL";

SEC("uprobe/sqlite3BtreeInsert")
int uprobe_sqlite3BtreeInsert(struct pt_regs *ctx) {
    bpf_printk("sqlite3BtreeInsert called!\n");
    return 0;
}
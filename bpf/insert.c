#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <sys/resource.h>
#include <bpf/libbpf.h>
#include "insert.skel.h"

static int libbpf_print_fn(enum libbpf_print_level level, const char *format, va_list args)
{
    return vfprintf(stderr, format, args);
}

static volatile sig_atomic_t stop;

static void sig_int(int signo)
{
    stop = 1;
}

int main(int argc, char **argv)
{
    struct insert_bpf *skel;
    int err;
    LIBBPF_OPTS(bpf_uprobe_opts, insert_opts);

    /* Set up libbpf errors and debug info callback */
    libbpf_set_print(libbpf_print_fn);

    /* Load and verify BPF application */
    skel = insert_bpf__open_and_load();
    if (!skel) {
        fprintf(stderr, "Failed to open and load BPF skeleton\n");
        return 1;
    }

    /* Attach uprobe handler for sqlite3BtreeInsert */
    insert_opts.func_name = "sqlite3BtreeInsert";
    skel->links.uprobe_sqlite3BtreeInsert = bpf_program__attach_uprobe_opts(
        skel->progs.uprobe_sqlite3BtreeInsert,
        -1 /* self pid */, "/usr/local/bin/sqlite3",  0  /* offset for function */, &insert_opts /* opts */);

    if (!skel->links.uprobe_sqlite3BtreeInsert) {
        err = -errno;
        fprintf(stderr, "Failed to attach uprobe: %d\n", err);
        goto cleanup;
    }

    /* Attach additional probes (if needed) */
    err = insert_bpf__attach(skel); // If you need additional auto attachments, use this
    if (err) {
        fprintf(stderr, "Failed to attach BPF skeleton: %d\n", err);
        goto cleanup;
    }

    /* Set signal handler for SIGINT */
    if (signal(SIGINT, sig_int) == SIG_ERR) {
        fprintf(stderr, "Can't set signal handler: %s\n", strerror(errno));
        goto cleanup;
    }

    printf("Successfully started! Please run `sudo cat /sys/kernel/debug/tracing/trace_pipe` "
           "to see output of the BPF programs.\n");

    /* Main loop to handle events */
    while (!stop) {
        fprintf(stderr, ".");
        sleep(1);
    }

cleanup:
    /* Cleanup and resource deallocation */
    if (skel) {
        insert_bpf__destroy(skel);
    }
    return err;
}
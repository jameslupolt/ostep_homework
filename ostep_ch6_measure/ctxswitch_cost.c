\
#define _GNU_SOURCE
#include "common.h"
#include <sys/wait.h>

static void xwrite(int fd, const void *buf, size_t n) {
    const uint8_t *p = (const uint8_t *)buf;
    while (n > 0) {
        ssize_t r = write(fd, p, n);
        if (r < 0) {
            if (errno == EINTR) continue;
            perror("write");
            exit(1);
        }
        p += (size_t)r;
        n -= (size_t)r;
    }
}

static void xread(int fd, void *buf, size_t n) {
    uint8_t *p = (uint8_t *)buf;
    while (n > 0) {
        ssize_t r = read(fd, p, n);
        if (r < 0) {
            if (errno == EINTR) continue;
            perror("read");
            exit(1);
        }
        if (r == 0) {
            fprintf(stderr, "read: EOF\n");
            exit(1);
        }
        p += (size_t)r;
        n -= (size_t)r;
    }
}

static double measure_pipe_pair_ns_per_iter(long iters) {
    int p[2];
    if (pipe(p) != 0) {
        perror("pipe");
        exit(1);
    }

    uint8_t byte = 0xAB;

    // Warm-up a bit
    for (int i = 0; i < 10000; i++) {
        xwrite(p[1], &byte, 1);
        xread(p[0], &byte, 1);
    }

    uint64_t t0 = now_ns();
    for (long i = 0; i < iters; i++) {
        xwrite(p[1], &byte, 1);
        xread(p[0], &byte, 1);
    }
    uint64_t t1 = now_ns();
    if (t0 == 0 || t1 == 0) {
        fprintf(stderr, "clock_gettime failed during pipe baseline\n");
        exit(1);
    }

    close(p[0]);
    close(p[1]);

    return (double)(t1 - t0) / (double)iters;
}

int main(int argc, char **argv) {
    long iters = parse_long_arg(argc, argv, "--iters", 2000000L);
    if (iters <= 0) {
        fprintf(stderr, "--iters must be > 0\n");
        return 2;
    }

    // Pin parent now; child will pin after fork.
    bool pinned_parent = pin_to_cpu0();
    if (!pinned_parent) {
        fprintf(stderr, "Warning: parent sched_setaffinity() failed (%s). Continuing.\n", strerror(errno));
    }

    // Baseline: a write+read pair on a pipe in a single process (no context switch).
    double pipe_pair_ns = measure_pipe_pair_ns_per_iter(iters);

    int p2c[2], c2p[2];
    if (pipe(p2c) != 0) { perror("pipe p2c"); return 1; }
    if (pipe(c2p) != 0) { perror("pipe c2p"); return 1; }

    pid_t pid = fork();
    if (pid < 0) { perror("fork"); return 1; }

    uint8_t byte = 0xCD;

    if (pid == 0) {
        // Child
        bool pinned_child = pin_to_cpu0();
        if (!pinned_child) {
            fprintf(stderr, "Warning: child sched_setaffinity() failed (%s). Continuing.\n", strerror(errno));
        }

        // Close unused ends
        close(p2c[1]); // child reads from p2c[0]
        close(c2p[0]); // child writes to c2p[1]

        // Warm-up: respond to a few pings
        for (int i = 0; i < 10000; i++) {
            xread(p2c[0], &byte, 1);
            xwrite(c2p[1], &byte, 1);
        }

        for (long i = 0; i < iters; i++) {
            xread(p2c[0], &byte, 1);
            xwrite(c2p[1], &byte, 1);
        }

        close(p2c[0]);
        close(c2p[1]);
        _exit(0);
    }

    // Parent
    close(p2c[0]); // parent writes to p2c[1]
    close(c2p[1]); // parent reads from c2p[0]
    int status = 0;

    // Warm-up handshake
    for (int i = 0; i < 10000; i++) {
        xwrite(p2c[1], &byte, 1);
        xread(c2p[0], &byte, 1);
    }

    uint64_t t0 = now_ns();
    for (long i = 0; i < iters; i++) {
        xwrite(p2c[1], &byte, 1);
        xread(c2p[0], &byte, 1);
    }
    uint64_t t1 = now_ns();
    if (t0 == 0 || t1 == 0) {
        fprintf(stderr, "clock_gettime failed during ping-pong timing\n");
        close(p2c[1]);
        close(c2p[0]);
        waitpid(pid, &status, 0);
        return 1;
    }

    close(p2c[1]);
    close(c2p[0]);

    waitpid(pid, &status, 0);

    double pingpong_ns_per_iter = (double)(t1 - t0) / (double)iters;

    // Each ping-pong iteration forces two context switches (parent->child and child->parent).
    // Each iteration also includes 2 pipe "pairs" worth of syscalls (one per process).
    // Estimate: pingpong_iter ~= 2*ctxsw + 2*pipe_pair
    double ctxsw_ns_est = (pingpong_ns_per_iter - 2.0 * pipe_pair_ns) / 2.0;

    printf("Context switch cost estimate (two-process pipe ping-pong)\n");
    printf("iters: %ld\n", iters);
    printf("pipe baseline:     %.2f ns per (write+read) pair (single process)\n", pipe_pair_ns);
    printf("ping-pong:         %.2f ns per iteration (includes 2 context switches)\n", pingpong_ns_per_iter);
    printf("ctxsw estimate:    %.2f ns per context switch (after baseline subtraction)\n", ctxsw_ns_est);
    printf("               ~=  %.3f us per context switch\n", ctxsw_ns_est / 1000.0);
    printf("\nNotes:\n");
    printf("  - This is an *estimate*; the ping-pong path includes scheduling and pipe overhead.\n");
    printf("  - If the estimate is negative or tiny, increase iterations or reduce system noise.\n");
    printf("  - Under WSL 2, virtualization and host scheduling can add jitter.\n");
    printf("  - Pinning to one CPU reduces noise; if affinity failed, results may be less stable.\n");

    return 0;
}

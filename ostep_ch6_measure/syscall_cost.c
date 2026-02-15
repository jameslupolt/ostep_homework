\
#define _GNU_SOURCE
#include "common.h"
#include <sys/syscall.h>

// We use syscall(SYS_getpid) to ensure we actually enter the kernel.
// Beware: some libc wrappers (and some "syscalls" like gettimeofday) can be served via vDSO.

static inline long do_syscall(void) {
    return syscall(SYS_getpid);
}

int main(int argc, char **argv) {
    long iters = parse_long_arg(argc, argv, "--iters", 20000000L);

    // Pinning reduces noise; not required, but helpful.
    if (!pin_to_cpu0()) {
        fprintf(stderr, "Warning: sched_setaffinity() failed (%s). Continuing unpinned.\n", strerror(errno));
    }

    volatile long sink = 0;

    // Warm-up
    for (int i = 0; i < 100000; i++) {
        sink ^= do_syscall();
    }

    // Measure loop+syscall
    uint64_t t0 = now_ns();
    for (long i = 0; i < iters; i++) {
        sink ^= do_syscall();
        compiler_barrier();
    }
    uint64_t t1 = now_ns();

    // Measure loop overhead (same structure, no syscall)
    uint64_t t2 = now_ns();
    for (long i = 0; i < iters; i++) {
        sink ^= (long)i;   // do something cheap so the loop isn't removed
        compiler_barrier();
    }
    uint64_t t3 = now_ns();

    uint64_t with_ns = t1 - t0;
    uint64_t base_ns = t3 - t2;

    // Avoid negative if weirdness happens.
    int64_t net_ns = (int64_t)with_ns - (int64_t)base_ns;
    if (net_ns <= 0) {
        fprintf(stderr, "Net time <= 0 (with=%" PRIu64 " ns, base=%" PRIu64 " ns). Try more iterations.\n", with_ns, base_ns);
        return 1;
    }

    double per_call_ns = (double)net_ns / (double)iters;

    printf("System call cost estimate using syscall(SYS_getpid)\n");
    printf("iters: %ld\n", iters);
    printf("total with syscall: %" PRIu64 " ns\n", with_ns);
    printf("total base loop:   %" PRIu64 " ns\n", base_ns);
    printf("net (with-base):   %" PRIi64 " ns\n", net_ns);
    printf("estimated cost:    %.2f ns per syscall\n", per_call_ns);
    printf("                 = %.3f us per syscall\n", per_call_ns / 1000.0);
    printf("\nNotes:\n");
    printf("  - This subtracts a baseline loop to reduce loop overhead.\n");
    printf("  - Results will vary; run multiple times and report min/median.\n");
    printf("  - Under WSL 2, extra virtualization overhead and scheduling jitter may inflate results.\n");

    // prevent unused warning
    if (sink == 0xdeadbeef) printf("sink=%ld\n", sink);
    return 0;
}

\
#define _GNU_SOURCE
#include <errno.h>
#include <inttypes.h>
#include <sched.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

static uint64_t now_ns(void) {
    // Prefer a monotonic clock for measurement (not affected by wall-clock adjustments).
    // CLOCK_MONOTONIC_RAW may not exist on all systems; fall back if needed.
#if defined(CLOCK_MONOTONIC_RAW)
    const clockid_t cid = CLOCK_MONOTONIC_RAW;
#else
    const clockid_t cid = CLOCK_MONOTONIC;
#endif
    struct timespec ts;
    if (clock_gettime(cid, &ts) != 0) {
        // Last resort: return 0; caller should treat as fatal.
        return 0;
    }
    return (uint64_t)ts.tv_sec * 1000000000ull + (uint64_t)ts.tv_nsec;
}

static void compiler_barrier(void) {
    // Prevent the compiler from optimizing away loops too aggressively.
    __asm__ __volatile__("" ::: "memory");
}

static bool pin_to_cpu0(void) {
    cpu_set_t set;
    CPU_ZERO(&set);
    CPU_SET(0, &set);
    if (sched_setaffinity(0 /* self */, sizeof(set), &set) != 0) {
        return false;
    }
    return true;
}

static long parse_long_arg(int argc, char **argv, const char *flag, long defval) {
    for (int i = 1; i < argc - 1; i++) {
        if (strcmp(argv[i], flag) == 0) {
            char *end = NULL;
            errno = 0;
            long v = strtol(argv[i + 1], &end, 10);
            if (end == argv[i + 1] || !end || *end != '\0' || errno == ERANGE) {
                fprintf(stderr, "Bad value for %s: %s\n", flag, argv[i + 1]);
                exit(2);
            }
            return v;
        }
    }
    return defval;
}

\
#include "common.h"

int main(int argc, char **argv) {
    long iters = parse_long_arg(argc, argv, "--iters", 1000000L);

    uint64_t min_ns = UINT64_MAX;
    uint64_t max_ns = 0;
    long zeros = 0;

    // Warm-up
    for (int i = 0; i < 10000; i++) {
        (void)now_ns();
    }

    for (long i = 0; i < iters; i++) {
        uint64_t a = now_ns();
        uint64_t b = now_ns();
        if (a == 0 || b == 0) {
            fprintf(stderr, "clock_gettime failed\n");
            return 1;
        }
        uint64_t d = b - a;
        if (d == 0) zeros++;
        if (d < min_ns) min_ns = d;
        if (d > max_ns) max_ns = d;
    }

    printf("Timer back-to-back call deltas over %ld iterations:\n", iters);
    printf("  min:  %" PRIu64 " ns\n", min_ns);
    printf("  max:  %" PRIu64 " ns\n", max_ns);
    printf("  zeros: %ld (i.e., measured delta=0)\n", zeros);
    printf("\nInterpretation:\n");
    printf("  The minimum non-zero delta is a practical lower bound on useful timer resolution.\n");
    printf("  If you see lots of zeros, you need more iterations per measurement in other tests.\n");
    return 0;
}

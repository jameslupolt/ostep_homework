\
#include "common.h"

static int cmp_u64(const void *a, const void *b) {
    const uint64_t ua = *(const uint64_t *)a;
    const uint64_t ub = *(const uint64_t *)b;
    return (ua > ub) - (ua < ub);
}

// Percentile helper for a sorted array of uint64 values.
//
// Notes:
// - "Nearest index" rule (no interpolation).
// - Avoids libm functions (so no extra -lm link step needed).
static uint64_t pct_u64_nearest(const uint64_t *sorted, size_t n, double pct) {
    if (n == 0) return 0;
    if (pct <= 0.0) return sorted[0];
    if (pct >= 100.0) return sorted[n - 1];

    // Map pct in [0,100] to index in [0, n-1]
    const double pos = (pct / 100.0) * (double)(n - 1);
    size_t idx = (size_t)(pos + 0.5); // round-to-nearest
    if (idx >= n) idx = n - 1;
    return sorted[idx];
}

int main(int argc, char **argv) {
    long iters = parse_long_arg(argc, argv, "--iters", 1000000L);

    bool pin = false;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--pin") == 0) {
            pin = true;
            break;
        }
    }
    if (pin && !pin_to_cpu0()) {
        fprintf(stderr, "warning: could not pin to CPU0 (continuing anyway)\n");
    }

    if (iters <= 0) {
        fprintf(stderr, "--iters must be > 0\n");
        return 2;
    }

    const size_t n = (size_t)iters;
    uint64_t *deltas = (uint64_t *)malloc(n * sizeof(uint64_t));
    if (!deltas) {
        fprintf(stderr, "malloc failed for %zu samples (try a smaller --iters)\n", n);
        return 1;
    }

    uint64_t min_ns = UINT64_MAX;
    uint64_t max_ns = 0;
    uint64_t min_nonzero_ns = UINT64_MAX;
    long zeros = 0;
    long backwards = 0;
    unsigned __int128 sum = 0;

    // Warm-up
    for (int i = 0; i < 10000; i++) {
        (void)now_ns();
    }

    for (size_t i = 0; i < n; i++) {
        uint64_t a = now_ns();
        uint64_t b = now_ns();
        if (a == 0 || b == 0) {
            fprintf(stderr, "clock_gettime failed\n");
            free(deltas);
            return 1;
        }
        if (b < a) {
            // Should not happen with a monotonic clock, but WSL/VM setups can be odd.
            // Record as 0 and count it.
            backwards++;
            deltas[i] = 0;
            zeros++;
            continue;
        }
        uint64_t d = b - a;
        deltas[i] = d;
        sum += (unsigned __int128)d;
        if (d == 0) zeros++;
        if (d < min_ns) min_ns = d;
        if (d > max_ns) max_ns = d;
        if (d != 0 && d < min_nonzero_ns) min_nonzero_ns = d;
    }

    const long double mean_ns = (long double)sum / (long double)n;

    // Sort so we can compute percentiles, find unique values, mode, etc.
    qsort(deltas, n, sizeof(uint64_t), cmp_u64);

    // Find first non-zero delta (if any)
    size_t nz_start = 0;
    while (nz_start < n && deltas[nz_start] == 0) nz_start++;
    const size_t n_nonzero = n - nz_start;

    // Unique-count + mode (most frequent delta)
    size_t unique = 0;
    uint64_t mode_val = 0;
    size_t mode_cnt = 0;
    {
        size_t run = 0;
        for (size_t i = 0; i < n; i++) {
            if (i == 0 || deltas[i] != deltas[i - 1]) {
                unique++;
                run = 1;
            } else {
                run++;
            }
            if (run > mode_cnt) {
                mode_cnt = run;
                mode_val = deltas[i];
            }
        }
    }

    // Percentiles (all deltas, including 0)
    const uint64_t p50 = pct_u64_nearest(deltas, n, 50.0);
    const uint64_t p90 = pct_u64_nearest(deltas, n, 90.0);
    const uint64_t p95 = pct_u64_nearest(deltas, n, 95.0);
    const uint64_t p99 = pct_u64_nearest(deltas, n, 99.0);
    const uint64_t p999 = pct_u64_nearest(deltas, n, 99.9);

    // Percentiles (non-zero deltas only)
    uint64_t nz_p50 = 0, nz_p90 = 0, nz_p95 = 0, nz_p99 = 0, nz_p999 = 0;
    if (n_nonzero > 0) {
        const uint64_t *nz = deltas + nz_start;
        nz_p50 = pct_u64_nearest(nz, n_nonzero, 50.0);
        nz_p90 = pct_u64_nearest(nz, n_nonzero, 90.0);
        nz_p95 = pct_u64_nearest(nz, n_nonzero, 95.0);
        nz_p99 = pct_u64_nearest(nz, n_nonzero, 99.0);
        nz_p999 = pct_u64_nearest(nz, n_nonzero, 99.9);
    }

    printf("Timer back-to-back call deltas over %ld iterations:\n", iters);
    if (pin) {
        printf("  affinity: attempted to pin to CPU0\n");
    }
    printf("  min:        %" PRIu64 " ns\n", min_ns);
    if (min_nonzero_ns != UINT64_MAX) {
        printf("  min_nonzero: %" PRIu64 " ns\n", min_nonzero_ns);
    } else {
        printf("  min_nonzero: (none; all deltas were 0)\n");
    }
    printf("  p50:        %" PRIu64 " ns\n", p50);
    printf("  p90:        %" PRIu64 " ns\n", p90);
    printf("  p95:        %" PRIu64 " ns\n", p95);
    printf("  p99:        %" PRIu64 " ns\n", p99);
    printf("  p99.9:      %" PRIu64 " ns\n", p999);
    printf("  max:        %" PRIu64 " ns\n", max_ns);
    printf("  mean:       %.2Lf ns\n", mean_ns);
    printf("  zeros:      %ld (%.2Lf%% of samples)\n", zeros,
           100.0L * (long double)zeros / (long double)n);
    if (backwards > 0) {
        printf("  backwards:  %ld (b < a; recorded as 0)\n", backwards);
    }
    printf("  unique:     %zu distinct delta values\n", unique);
    printf("  mode:       %" PRIu64 " ns (%zu samples; %.2Lf%%)\n", mode_val, mode_cnt,
           100.0L * (long double)mode_cnt / (long double)n);

    if (n_nonzero > 0) {
        printf("\nNon-zero deltas only (n=%zu):\n", n_nonzero);
        printf("  min_nonzero: %" PRIu64 " ns\n", deltas[nz_start]);
        printf("  p50:         %" PRIu64 " ns\n", nz_p50);
        printf("  p90:         %" PRIu64 " ns\n", nz_p90);
        printf("  p95:         %" PRIu64 " ns\n", nz_p95);
        printf("  p99:         %" PRIu64 " ns\n", nz_p99);
        printf("  p99.9:       %" PRIu64 " ns\n", nz_p999);
    }
    printf("\nInterpretation:\n");
    printf("  • If p50 is 0, your timer often doesn't tick between back-to-back reads;\n");
    printf("    you'll need to time longer-running loops (more iterations per measurement).\n");
    printf("  • min_nonzero is a rough lower bound on observable resolution; percentiles\n");
    printf("    tell you how often you actually observe that resolution in practice.\n");
    printf("  • Large max values usually mean you were preempted between the two calls\n");
    printf("    (scheduler / VM noise), not that the clock is that coarse.\n");

    free(deltas);
    return 0;
}

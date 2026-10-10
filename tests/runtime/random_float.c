/* Pin private PRNG state without adding a public seeding API. */
#include "../../runtime/walk_runtime.c"
#include <float.h>

static int failures = 0;

static void check_range(double min, double max) {
    walk_rt_random_state = 1;
    for (int i = 0; i < 64; i++) {
        double sample = walk_rt_random_float(min, max);
        if (!isfinite(sample) || sample < min || sample >= max) {
            fprintf(stderr, "random.float(%a, %a) returned %a outside its finite range\n", min, max, sample);
            failures++;
            return;
        }
    }
}

int main(void) {
    check_range(-DBL_MAX, DBL_MAX);
    check_range(-DBL_MAX, 1.0);
    check_range(-1.0, DBL_MAX);
    check_range(DBL_MAX / 2, DBL_MAX);
    check_range(-DBL_MAX, -DBL_MAX / 2);
    check_range(1.0, nextafter(1.0, 2.0));
    check_range(nextafter(-1.0, -2.0), -1.0);
    check_range(0.0, nextafter(0.0, 1.0));
    check_range(nextafter(0.0, -1.0), 0.0);
    check_range(-DBL_MIN, DBL_MIN);
    check_range(-2.0, 3.0);

    /* Seed 1's second draw has unit > 0.5 and previously rounded up to max. */
    walk_rt_random_state = 33554433ULL;
    if (walk_rt_random_float(1.0, nextafter(1.0, 2.0)) != 1.0) {
        fprintf(stderr, "random.float included the adjacent upper endpoint\n");
        failures++;
    }
    if (walk_rt_random_float(DBL_MAX, DBL_MAX) != DBL_MAX ||
        walk_rt_random_float(-DBL_MAX, -DBL_MAX) != -DBL_MAX ||
        walk_rt_random_float(0.0, 0.0) != 0.0) {
        fprintf(stderr, "random.float singleton result changed\n");
        failures++;
    }
    walk_rt_random_state = 1;
    if (walk_rt_random_float(DBL_MAX, -DBL_MAX) != DBL_MAX || walk_rt_random_state != 1) {
        fprintf(stderr, "random.float reversed bounds changed\n");
        failures++;
    }
    /* Keep existing nonfinite-bound behavior outside the finite-range fix. */
    if (!isnan(walk_rt_random_float(-INFINITY, 1.0)) ||
        !isnan(walk_rt_random_float(INFINITY, INFINITY)) ||
        !isnan(walk_rt_random_float(NAN, 1.0)) ||
        walk_rt_random_float(1.0, INFINITY) != INFINITY) {
        fprintf(stderr, "random.float nonfinite-bound behavior changed\n");
        failures++;
    }
    if (failures != 0) { return 1; }
    puts("random.float deterministic runtime tests passed");
    return 0;
}

/* Include the implementation to pin private PRNG state without a public seed API. */
#include "../../runtime/walk_runtime.c"

int main(void) {
    /* Seed 1 produces 5180492295206395165, then 12380297144915551517.
       For [LLONG_MIN, 0], the threshold is 9223372036854775807: reject the
       first draw and map the second. A plain modulo implementation fails. */
    walk_rt_random_state = 1;
    if (walk_rt_random_int(LLONG_MIN, 0) != -6066446928794000100LL ||
        walk_rt_random_state != 1126174793148417ULL) {
        fprintf(stderr, "random.int did not reject the incomplete bucket\n");
        return 1;
    }
    /* The full range uses the first draw directly, with defined conversion. */
    walk_rt_random_state = 1;
    if (walk_rt_random_int(LLONG_MIN, LLONG_MAX) != -4042879741648380643LL ||
        walk_rt_random_state != 33554433ULL) {
        fprintf(stderr, "random.int full-range mapping failed\n");
        return 1;
    }
    if (walk_rt_random_int(LLONG_MIN, LLONG_MIN) != LLONG_MIN ||
        walk_rt_random_int(LLONG_MAX, LLONG_MAX) != LLONG_MAX ||
        walk_rt_random_int(LLONG_MAX, LLONG_MIN) != LLONG_MAX) {
        fprintf(stderr, "random.int endpoint bounds failed\n");
        return 1;
    }
    puts("random.int deterministic runtime tests passed");
    return 0;
}

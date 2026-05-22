#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <time.h>

typedef struct { long long *items; long long len; } WalkArrayInt;
typedef struct { double *items; long long len; } WalkArrayFloat;
typedef struct { bool *items; long long len; } WalkArrayBool;
typedef struct { const char **items; long long len; } WalkArrayString;

static long long __walk_random_int(long long min, long long max) {
    if (max < min) { return min; }
    return min + (rand() % (max - min + 1));
}

int main(void) {
    printf("%s\n", "hello" == NULL ? "null" : "hello");
    printf("%lld\n", (long long)((1 + 2)));
    printf("%s\n", (true) ? "true" : "false");
    return 0;
}

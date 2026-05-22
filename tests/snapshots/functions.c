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

long long add(long long a, long long b);
long long fact(long long n);

long long add(long long a, long long b) {
    return (a + b);
}

long long fact(long long n) {
    if ((n <= 1)) {
        return 1;
    }
    return (n * fact((n - 1)));
}

int main(void) {
    printf("%lld\n", (long long)(add(2, 3)));
    printf("%lld\n", (long long)(fact(5)));
    return 0;
}

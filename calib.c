#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <x86intrin.h>

#define N	1000

uint32_t cache_miss(void* ptr) {
    uint32_t junk;
    register uint64_t t1, t2;

    _mm_clflush(ptr);

    t1 = __rdtscp(&junk);
    junk = *(uint64_t*)ptr;
    t2 = __rdtscp(&junk);

    return (uint32_t) (t2 - t1);
}

uint32_t cache_hit(void* ptr) {
    uint32_t junk;
    register uint64_t t1, t2;

    t1 = __rdtscp(&junk);
    junk = *(uint64_t*)ptr;
    t2 = __rdtscp(&junk);

    return  (uint32_t) (t2 - t1);
}

int main(int argc, char* argv[argc+1]) {
    uint32_t hit_average;
    uint32_t miss_average;

    uint64_t dummy;

    dummy = 0xdeadbeefdeadbeef;

    (void*) &dummy;

    for (size_t i=0; i < N; ++i)
        hit_average = (cache_hit(&dummy) + hit_average * i) / (i+1);

    for (size_t i=0; i < N; ++i)
        miss_average = (cache_miss(&dummy) + miss_average * i) / (i+1);

    printf("hit average:  %d\n", hit_average);
    printf("miss average: %d\n", miss_average);

    uint32_t threshold = (hit_average * 2 + miss_average) / 3;
    printf("cache hit threshold: %d\n", threshold);

    return EXIT_SUCCESS;
}
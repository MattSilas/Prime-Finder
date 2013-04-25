#include <limits.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#define CHAR_OFFSET(b) ((b) / 8)
#define BIT_OFFSET(b)  ((b) % 8)
#define BIT_TEST(array,x)       (array[CHAR_OFFSET(x)] & (1 << BIT_OFFSET(x)))
#define BIT_SET(array,x)       (array[CHAR_OFFSET(x)] |= (1 << BIT_OFFSET(x)))

unsigned long max;
int num_threads;
unsigned long factor = 1;
unsigned char *bitmap = NULL;
pthread_mutex_t mutex_bitmap = PTHREAD_MUTEX_INITIALIZER;

/*
 * Sieve using only odd numbers. Two is handled as an exception.
 * Param: thread number
 */
void *isPrime(void *arg);

int main(int argc, char *argv[]) {
    int i;
    num_threads = 50;
    max = 200000;
    
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
    
    bitmap = (unsigned char*) calloc(max, 4);
    if (bitmap == NULL) {
        perror("Bit map creation failed");
        exit(EXIT_FAILURE);
    }
    
    pthread_t threads[num_threads];
    for (i = 0; i < num_threads; ++i)
    {
        if(pthread_create(&threads[i], &attr, isPrime, (void *) i))
        {
            perror("Error creating thread");
            exit(EXIT_FAILURE);
        }
    }
    
    for (i = 0; i < num_threads; i++)
        pthread_join(threads[i], NULL);
    free(bitmap);
    pthread_attr_destroy(&attr);
    
    pthread_exit(NULL);
}

void *isPrime(void *arg) {
    pthread_mutex_lock(&mutex_bitmap);
    unsigned long val;
    while (factor * factor <= max)
    {
        factor += 2;
        if (!BIT_TEST(bitmap, factor))
            for (val = 3 * factor; val < max; val += factor << 1)
                BIT_SET(bitmap, val);
    }
    pthread_mutex_unlock(&mutex_bitmap);
    pthread_exit(NULL);
}
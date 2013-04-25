#define _BSD_SOURCE

#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <semaphore.h>
#include <sys/time.h>
#include <limits.h>
#include <sys/wait.h>

#define CHAR_OFFSET(b) ((b) / 8)
#define BIT_OFFSET(b)  ((b) % 8)
#define BIT_TEST(array,x)       (array[CHAR_OFFSET(x)] & (1 << BIT_OFFSET(x)))
#define BIT_SET(array,x)       (array[CHAR_OFFSET(x)] |= (1 << BIT_OFFSET(x)))

unsigned long MAX = UINT_MAX;
unsigned long factor = 1;
unsigned char *bitmap;
unsigned long val;
unsigned long start=1;

/*
 * Sieve of Eratosthenes - only searches for odds to improve performance
 * Param: process_max is the top value the current process will search for
 * Global: factor is the current number isPrime is searching for and factors of  
 */
void isPrime(int process_max) {
  while (factor * factor <= process_max)
    {
      factor += 2;
      if (!BIT_TEST(bitmap, factor))
	for (val = 3 * factor; val < process_max; val += factor << 1)
	  BIT_SET(bitmap, val);
    }
  
}

/*
 * Mount_shmem written verbatim from lecture 
 */
void *mount_shmem(char *path, int size)
{
  int shmem_fd;
  void *addr;
    
  shmem_fd = shm_open(path, O_CREAT|O_RDWR,S_IRUSR|S_IWUSR);
  if(shmem_fd==-1)
    {
      perror("failed to open shmem object");
      exit(EXIT_FAILURE);
    }
    
  if(ftruncate(shmem_fd, size)==-1)
    {
      perror("failed to resize shmem object");
      exit(EXIT_FAILURE);
    }
    
  addr = mmap(NULL, size, PROT_READ|PROT_WRITE,MAP_SHARED,shmem_fd,0);
  if(addr == MAP_FAILED)
    {
      perror("failed to map shmem object");
      exit(EXIT_FAILURE);
    }
    
  return addr;
}

/*
 * Prints all the primes that were found
 */
void print_primes() {
  printf("primes: \n%d\t", 2);
  while ((start += 2) < MAX)
    if (!BIT_TEST(bitmap, start))
      printf("%li\t",start);
}

int main(int argc, const char * argv[])
{
  int process_max;
  int num_children=1;//change this to add proceses
  int spawn_count=1;
  pid_t child;
  int status;

  sem_t *sem = sem_open("/mjs_prime_sem",O_CREAT,S_IRUSR|S_IWUSR,1);
  if(!sem)
    {
      perror("sem failed");
      exit(EXIT_FAILURE);
    }
    
  bitmap = (unsigned char*) calloc(MAX, 4);
  if (bitmap == NULL) {
    perror("Bit map creation failed");
    exit(EXIT_FAILURE);
  }

  int bitmap_size = sizeof(bitmap);
  int i = 0;  
  int size = 1024*1024*60;
    
  void *addr = mount_shmem("sieves", size);
  
  bitmap = (unsigned char *) addr;
  for(i = 0;i<num_children;++i)
    {
      child = fork();
      process_max=(MAX/num_children)*spawn_count;
      spawn_count=spawn_count+1;
      if(child==0){
	sem_wait(sem);
	isPrime(process_max);
	sem_post(sem);
	sem_close(sem);
	exit(EXIT_SUCCESS);
      }
      else{
	child = wait(NULL);
	  if(child ==-1){
	    if(errno ==ECHILD){
	      sem_destroy(sem);
	      exit(EXIT_SUCCESS);
	    }
	    else{
	      perror("Error waiting for process");
	      exit(EXIT_FAILURE);
	    }
	  }
       }
    }

  return 0;
}

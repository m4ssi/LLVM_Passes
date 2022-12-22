#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdatomic.h>


uint64_t rdtsc(void)
{
    uint32_t lo, hi;
    __asm__ __volatile__ ("rdtsc" : "=a" (lo), "=d" (hi));
    return (uint64_t)(((uint64_t)hi << (uint64_t)32) | (uint64_t)lo);
}

#define SIZE 1024;

int main ( int argc, char ** argv)
{
	int n = SIZE;
	int * x = (int *) malloc ( n * sizeof(int));

	uint64_t beg = rdtsc();
	for (int i = 0; i < n; i++)
	{
		if ( i % 2 == 0)	// Odd
		{
			if ( i % 4 == 0)
			{
				x[i] = 4;
			}
			else
			{
				x[i] = 2;
			}
			
		}
		else 		// Oven
		{
			x[i] = 1;
		}
	}

	uint64_t end = rdtsc() - beg;
	
	printf ("Elapsed : %ld cycles\n", end);

	free (x);
	return 0;
}

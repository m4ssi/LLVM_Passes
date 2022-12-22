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

	uint64_t beg0 = rdtsc();
	
	for (int i = 0; i < n; i++)
	{
		printf ("Iter %d done\n", i);
	}
	
	uint64_t end0 = rdtsc() - beg0;

	uint64_t beg1 = rdtsc();
	
	for (int i = 0; i < n; i++)
	{
		printf ("Iter %d done\n", i);
	}
	
	uint64_t end1 = rdtsc() - beg1;

	uint64_t beg2 = rdtsc();
	
	for (int i = 0; i < n; i++)
	{
		printf ("Iter %d done\n", i);
	}
	
	uint64_t end2 = rdtsc() - beg2;

	uint64_t beg3 = rdtsc();
	
	for (int i = 0; i < n; i++)
	{
		printf ("Iter %d done\n", i);
	}
	
	uint64_t end3 = rdtsc() - beg3;

	uint64_t beg4 = rdtsc();
	
	for (int i = 0; i < n; i++)
	{
		printf ("Iter %d done\n", i);
	}
	
	uint64_t end4 = rdtsc() - beg4;
	
	printf ("Loop 4 :		%ld cycles\n", end0);
	printf ("Loop 3 :		%ld cycles\n", end1);
	printf ("Loop 2 :		%ld cycles\n", end2);
	printf ("Loop 1 :		%ld cycles\n", end3);
	printf ("Loop 0 :		%ld cycles\n", end4);
	
	return 0;
}

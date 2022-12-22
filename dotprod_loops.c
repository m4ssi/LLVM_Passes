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

#define V_SIZE 64
#define M_SIZE (V_SIZE * V_SIZE)

double dotprod ( double * a, double * b, int size)
{
	double dp = 0.0;
	for (int i = 0; i < size; i++)	// Loop 0
	{
		dp += a[i] * b[i];
	}
	return dp;
}

void init_vectors ( double * x, int size, double odd, double oven)
{
	for (int i = 0; i < size; i++)
	{
		if ( i % 3 == 0)
			x[i] = odd;
		else if ( i % 3 == 1)
			x[i] = oven;
		else
			x[i] =0.0;
	}
}

int main ( void)
{			
	// Vectors
	double	x[V_SIZE],
			y[V_SIZE];
	
	// Init Vectors, expected dotprod : V_SIZE
	init_vectors ( x, V_SIZE, 1.0, 1.0);
	init_vectors ( y, V_SIZE, 1.0, 1.0);

	double dp = dotprod ( x, y, V_SIZE);

	if ( dp != (double) V_SIZE)
	{
		printf ("==== dotprod : error\n");
		printf ("==== dotprod : expected %lf - %+8e\n", (double) V_SIZE, (double) V_SIZE);
		printf ("==== dotprod : received %lf - %+8e\n", dp, dp);
		return 0;
	}

	printf ("==== All tests passed !\n");
	
	return 0;
}

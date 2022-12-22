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

void matprod_2d ( double a[V_SIZE][V_SIZE], double b[V_SIZE][V_SIZE], double r[V_SIZE][V_SIZE], int size)
{
	for (int i = 0; i < size; i++)	// Loop 3
	{
		for (int j = 0; j < size; j++) // Loop 2
		{
			r[i][j] = 0.0;
			for (int k = 0; k < size; k++)	// Loop 1
			{
				r[i][j] += a[i][k] * b[k][j];
			}
		}
	}
}

void matprod_1d ( double * a, double *b, double * r, int size)
{
	for (int i = 0; i < size; i++) 	// Loop 6
	{
		for (int j = 0; j < size; j++)	// Loop 5
		{
			r[i * size + j] = 0.0;
			for (int k = 0; k < size; k++)	// Loop 4
			{
				r[i * size + j] += a[i * size + k] * b[k * size + j];
			}
		}
		for (int j = 0; j < size; j++)	// Loop 5
		{
			r[i * size + j] = 0.0;
			for (int k = 0; k < size; k++)	// Loop 4
			{
				r[i * size + j] += 0;
			}
		}
	}	
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

void init_matrix ( double a[V_SIZE][V_SIZE], double v, int size)
{
	for (int i = 0; i < size; i++)	// Loop 11
	{
		for (int j = 0; j < size; j++)	// Loop 10
		{
			a[i][j] = v;
		}
	}	
}

int main ( void)
{
	// Matrix
	double	A[V_SIZE][V_SIZE],
			B[V_SIZE][V_SIZE],
			R[V_SIZE][V_SIZE];
			
	// Linearised matrix
	double	Al[M_SIZE],
			Bl[M_SIZE],
			Rl[M_SIZE];
			
	// Vectors
	double	x[V_SIZE],
			y[V_SIZE];
	
	// Init Vectors, expected dotprod : V_SIZE
	init_vectors ( x, V_SIZE, 1.0, 2.0);
	init_vectors ( y, V_SIZE, 2.0, 1.0);


	// Init matrix, expected result : Rij = V_SIZE
	init_vectors ( Al, M_SIZE, 1.0, 2.0);
	init_vectors ( Bl, M_SIZE, 2.0, 1.0);
	init_vectors ( Rl, M_SIZE, 0.0, 0.0);
	
	// Ini matrix
	init_matrix ( A, 1.0, V_SIZE);
	init_matrix ( B, 1.0, V_SIZE);
	init_matrix ( R, 1.0, V_SIZE);
	double dp = dotprod ( x, y, V_SIZE);
	
	matprod_1d ( Al, Bl, Rl, V_SIZE);
	matprod_2d ( A, B, R, V_SIZE);	
	if ( dp != (double) V_SIZE)
	{
		printf ("==== dotprod : error\n");
		printf ("==== dotprod : expected %lf - %+8e\n", (double) V_SIZE, (double) V_SIZE);
		printf ("==== dotprod : received %lf - %+8e\n", dp, dp);
		return 0;
	}
	for (int i = 0; i < M_SIZE; i++)	// Loop 9
	{
		if (Rl[i] != (double) V_SIZE)
		{
			printf ("==== mat1d : error\n");
			printf ("==== mat1d : expected %lf - %+8e\n", (double) V_SIZE, (double) V_SIZE);
			printf ("==== mat1d : received %lf - %+8e\n", Rl[i], Rl[i]);
			return 2;			
		}
	}
	for (int i = 0; i < V_SIZE; i++)	// Loop 8
		for (int j = 0; j < V_SIZE; j++)	// Loop 7
			if ( R[i][j] != (double) V_SIZE)
			{
				printf ("==== mat2d : error\n");
				printf ("==== mat2d : expected %lf - %+8e\n", (double) V_SIZE, (double) V_SIZE);
				printf ("==== mat2d : received %lf - %+8e\n",R[i][j], R[i][j]);
				return 3;			
			}
	printf ("==== All tests passed !\n");
	
	return 0;
}

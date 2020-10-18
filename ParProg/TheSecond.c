#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

#define L 1.0
#define C 1.0
#define T1 1.0
#define T2 2.0

int main(int argc, char **argv) {
	if (argc < 3) {
		printf("Usage: %s N T\n", argv[0]);
	}

	int N = atoi(argv[1]);
	double T = atof(argv[2]);
	double *u1, *u2;
	double h = L / (N - 1);
	double dt = 0.3 * h * h / C;
	int steps = T / dt;
	int i, j;
	int myrank, procs_num;

	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &myrank);
	MPI_Comm_size(MPI_COMM_WORLD, &procs_num);

    int count_num; 
	count_num = N / procs_num; // количество считаемых узлов
	// распределяем узлы по первым процессам если их не кратное количество
	if (myrank < N % procs_num)	count_num++;

	/* Выделение памяти. */
	int len;
	if (myrank == 0 || myrank == procs_num - 1)
		len = count_num + 1;
	else 
		len = count_num + 2;
	if (procs_num == 1)
		len = N;

	u1 = (double*) calloc (len, sizeof(double));
	u2 = (double*) calloc (len, sizeof(double));

	/* Граничные условия. */
	if (myrank == 0) 
		u1[0] = T1;
	if (myrank == procs_num - 1) 
		u1[len - 1] = T2;
    for (i = 0; i < len; i++) 
    	u2[i] = u1[i];

	/* Цикл интегрирования. */
	for (i = 0; i < steps; i++) {
		if (myrank % 2 == 0) {
			if (myrank != 0) {
				MPI_Send (&u1[1], 1, MPI_DOUBLE, myrank - 1, 0, MPI_COMM_WORLD);
				MPI_Recv (&u1[0], 1, MPI_DOUBLE, myrank - 1, MPI_ANY_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
			}

			if (myrank != procs_num - 1) {
				MPI_Recv (&u1[len-1], 1, MPI_DOUBLE, myrank + 1, MPI_ANY_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
				MPI_Send (&u1[len-2], 1, MPI_DOUBLE, myrank + 1, 0, MPI_COMM_WORLD);
			}
		} else {
			if (myrank != procs_num - 1) {
				MPI_Recv (&u1[len-1], 1, MPI_DOUBLE, myrank + 1, MPI_ANY_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
				MPI_Send (&u1[len-2], 1, MPI_DOUBLE, myrank + 1, 0, MPI_COMM_WORLD);
			}

			if (myrank != 0) {
				MPI_Send (&u1[1], 1, MPI_DOUBLE, myrank - 1, 0, MPI_COMM_WORLD);
				MPI_Recv (&u1[0], 1, MPI_DOUBLE, myrank - 1, MPI_ANY_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
			}
		}

		for (j = 1; j < len - 1; j++)
			u2[j] = u1[j] + 0.3 * (u1[j-1] - 2 * u1[j] + u1[j+1]);

		double *t = u1;
		u1 = u2; u2 = t;
	}

	if (myrank != 0) {
		MPI_Send (&len, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
		MPI_Send (u1, len, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD);
	}
	else {
		double *res = (double*) calloc(N, sizeof(double));
		for (i = 0; i < len - 1; i++)
			res[i] = u1[i];
		int res_len = len - 1;
		
		if (N == len) {
			res[len - 1] = u1[len - 1];
			res_len++;
		}

		double *buf = (double*) calloc(len + 1, sizeof(double));
		
		for (i = 1; i < procs_num; i++) {
			int ilen;
			MPI_Recv (&ilen, 1, MPI_INT, i, MPI_ANY_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
			MPI_Recv (buf, ilen, MPI_DOUBLE, i, MPI_ANY_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

			if (i != procs_num - 1) {
				for (j = 1; j < ilen - 1; j++)
					res[j + res_len - 1] = buf[j];
				res_len += ilen - 2;
			} else {
				for (j = 1; j < ilen; j++)
					res[j + res_len - 1] = buf[j];
				res_len += ilen;
			}
		}

		free (buf);

		for (i = 0; i < N; i++)
			printf("%f %f\n", h * i, res[i]);
	}
	
	free(u1);
	free(u2);
	MPI_Finalize();
	return 0;
}

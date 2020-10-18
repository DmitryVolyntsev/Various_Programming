#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <mpi.h>	// Заголовочный файл MPI

double f(double x) {		// Интегрируемая функция
	return sqrt(4.0 - x * x);
}



double one_node_int(int N, double a, double b) {	// Вычисление интеграла от a до b с разбиением на N отрезков
	int i = 0;
	double s = 0;
	double x1, x2;
	for (i = 0; i < N; i++) {
		x1 = a + ((double) i) / N * (b - a);
		x2 = a + ((double) (i+1)) / N * (b - a);
		s += (f(x1) + f(x2)) * (x2 - x1) / 2;
	}
	return s;
}



int main(int argc, char *argv[]) {
	int i, N;
	int myrank, p;
	double s;		// Значение интеграла
	MPI_Status Status;	// Тип данных MPI
	
	struct {		// Структура, передаваемая главным процессом
		int N;
		double a;
		double b;
	} parameters;

	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &p);	// Получаем число процессов
	MPI_Comm_rank(MPI_COMM_WORLD, &myrank);	// Получаем ID моего процесса
	
	

	if (myrank == 0) {			// Управляющий узел, если ID = 0
		double begin, end, Tp, T1, S, step;
		double piece[8];		// Значения кусочков интегралов
		
		if (argc < 2) {			// Из аргументов в командной строки узнаём N
			printf("Введите аргумент N -- число отрезков в разбиении");
			exit(1);
		} else {
			N = atoi(argv[1]);  //строка в число
		}

		printf("\n\nN = %d\n\nОднопоточное вычисление интеграла:\n", N);
		begin = MPI_Wtime();    //считаем текущее время
		s = one_node_int(N, 0, 1);
		end = MPI_Wtime();
		T1 = end - begin;   //время работы
		printf("Значение интеграла: %lf\nЗатраченное время: %lf секунд\n", s, T1);
		


		printf("\n\nМногопоточное вычисление интеграла:\nКоличество процессов: %d\n\n", p);
		begin = MPI_Wtime();

		// Стратегия примерно такая:
		// Если N делится нацело на p, то делим отрезки между процессами поровну
		// В противном случае первым N%p процессам достаётся на один кусочек больше

		if (N % p == 0) {
			parameters.N = N/p;
			step = 1.0/p;
			parameters.b = 0;
			for (i = 1; i < p; i++) {			// Рассылаем другим процессам задачи
				parameters.a = parameters.b;
				parameters.b = parameters.a + step;
				MPI_Send((char*)(&parameters), (int)sizeof(typeof(parameters)), MPI_CHAR, i, 1, MPI_COMM_WORLD);	
			}
		} else {
			parameters.N = N/p + 1;
			step = ((double)parameters.N) / N;
			parameters.b = 0;
			for (i = 1; i <= N%p; i++) {			// Рассылаем другим процессам задачи
				parameters.a = parameters.b;
				parameters.b = parameters.a + step;
				MPI_Send((char*)(&parameters), (int)sizeof(typeof(parameters)), MPI_CHAR, i, 1, MPI_COMM_WORLD);	
			}
			parameters.N = N/p;
			step = ((double)parameters.N) / N;
			for (i = 1; i < p; i++) {
				parameters.a = parameters.b;
				parameters.b = parameters.a + step;
				MPI_Send((char*)(&parameters), (int)sizeof(typeof(parameters)), MPI_CHAR, i, 1, MPI_COMM_WORLD);
			}
		}
		
		// Вычисляем свою часть интеграла
		piece[0] = one_node_int(parameters.N, parameters.b, 1.0);
		
		// Собираем результаты от других процессов
		for (i = 1; i < p; i++) {
			MPI_Recv(&piece[i], 1, MPI_DOUBLE, i, 1, MPI_COMM_WORLD, &Status);
		}
		s = 0;
		for (i = 0; i < p; i++) {
			s += piece[i];
		}
		end = MPI_Wtime();
		Tp = end - begin;
		for (i = 0; i < p; i++) {
			printf("Процесс %d. Значение интеграла: %lg\n", i, piece[i]);
		}
		printf("\nЗначение интеграла: %lf\nЗатраченное время: %lf секунд\n\n", s, Tp);
		S = T1 / Tp;
	}

	
	else {					// Все остальные процессы
		MPI_Recv((char*)(&parameters), (int)sizeof(typeof(parameters)), MPI_CHAR, 0, 1, MPI_COMM_WORLD, &Status);
		s = one_node_int(parameters.N, parameters.a, parameters.b);
		MPI_Send(&s, 1, MPI_DOUBLE, 0, 1, MPI_COMM_WORLD);
	}
	
	MPI_Finalize();
	return 0;
}
